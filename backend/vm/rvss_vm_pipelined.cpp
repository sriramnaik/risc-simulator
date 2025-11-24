#include "rvss_vm_pipelined.h"
#include "../common/instructions.h"
#include <QDebug>

using instruction_set::get_instr_encoding;
using instruction_set::Instruction;

RVSSVMPipelined::RVSSVMPipelined(RegisterFile *sharedRegisters, QObject *parent)
    : RVSSVM(sharedRegisters, parent)
{
    registers_ = sharedRegisters;
}

RVSSVMPipelined::~RVSSVMPipelined() = default;

void RVSSVMPipelined::LoadProgram(const AssembledProgram &program)
{
    // Call base class implementation
    RVSSVM::LoadProgram(program);

    if (program_size_ > 0)
    {
        stage_to_pc_["IF"] = 0;
        emit pipelineStageChanged(0, "IF");
    }
}

void RVSSVMPipelined::Reset()
{
    RVSSVM::Reset();
    if_id_ = IF_ID();
    if_id_next_ = IF_ID();
    id_ex_ = ID_EX();
    id_ex_next_ = ID_EX();
    ex_mem_ = EX_MEM();
    ex_mem_next_ = EX_MEM();
    mem_wb_ = MEM_WB();
    mem_wb_next_ = MEM_WB();

    pc_update_pending_ = false;
    pc_update_value_ = 0;
    stall_ = false;
    flush_pipeline_ = false;

    emit pipelineStageChanged(0, "IF_CLEAR");
    emit pipelineStageChanged(0, "ID_CLEAR");
    emit pipelineStageChanged(0, "EX_CLEAR");
    emit pipelineStageChanged(0, "MEM_CLEAR");
    emit pipelineStageChanged(0, "WB_CLEAR");

    branch_target_buffer_.assign(BHT_SIZE, 0);
    if (dynamic_branch_prediction_enabled_)
    {
        // Dynamic prediction: start conservative (NOT_TAKEN)
        branch_history_table_.assign(BHT_SIZE, false);
    }
    else
    {
        // Static prediction: BHT not used, but initialize anyway
        branch_history_table_.assign(BHT_SIZE, true);
    }

    // Clear undo stack
    while (!pipeline_undo_stack_.empty())
        pipeline_undo_stack_.pop();

    stage_to_pc_.clear();
}

void RVSSVMPipelined::IF_stage()
{
    // Handle PC updates FIRST (highest priority)
    if (pc_update_pending_)
    {
        program_counter_ = pc_update_value_;
        pc_update_pending_ = false;
        pc_update_value_ = 0;
        if_id_next_.valid = false;
        return;
    }

    if (flush_pipeline_)
    {
        if_id_next_.valid = false;
        return;
    }

    if (stall_)
    {
        if_id_next_ = if_id_;
        return;
    }

    if (program_counter_ >= program_size_)
    {
        if_id_next_.valid = false;
        return;
    }

    // Default: fetch next sequential instruction
    uint64_t predicted_pc = program_counter_ + 4;

    // ✅ REFACTORED: Mutually exclusive branch prediction modes
    if (branch_prediction_enabled_)
    {
        size_t index = (program_counter_ >> 2) % BHT_SIZE;
        uint64_t target = branch_target_buffer_[index];
        bool predict_taken = false;

        qDebug() << "IF: PC:" << QString::number(program_counter_, 16)
                 << "BTB Index:" << index
                 << "BTB Target:" << QString::number(target, 16);

        // In IF_stage(), around line 157:
        if (dynamic_branch_prediction_enabled_)
        {
            qDebug() << "IF: Using DYNAMIC branch prediction";

            // Check if we have a BTB entry
            if (target != 0)
            {
                // Use BHT to predict
                predict_taken = branch_history_table_[index];
                qDebug() << "IF: BTB hit - BHT predicts"
                         << (predict_taken ? "TAKEN" : "NOT_TAKEN");
            }
            else
            {
                // BTB miss - first encounter, predict not taken
                predict_taken = false;
                qDebug() << "IF: BTB miss - predicting NOT_TAKEN";
            }
        }
        else
        {
            // ============================================================
            // STATIC PREDICTION: Backward taken, forward not taken
            // ============================================================
            qDebug() << "IF: Using STATIC branch prediction";

            if (target != 0)
            {
                // We have a BTB entry - check branch direction
                if (target < program_counter_)
                {
                    // Backward branch (likely a loop) - predict taken
                    predict_taken = true;
                    qDebug() << "IF: BACKWARD branch detected - predicting TAKEN";
                }
                else if (target > program_counter_)
                {
                    // Forward branch - predict not taken
                    predict_taken = false;
                    qDebug() << "IF: FORWARD branch detected - predicting NOT_TAKEN";
                }
                else
                {
                    // Self-loop (rare) - predict taken
                    predict_taken = true;
                    qDebug() << "IF: SELF-loop detected - predicting TAKEN";
                }
            }
            else
            {
                // BTB miss - predict not taken (first encounter)
                predict_taken = false;
                qDebug() << "IF: BTB miss - predicting NOT_TAKEN (first encounter)";
            }
        }

        // Apply prediction
        if (predict_taken && target != 0)
        {
            predicted_pc = target;
            qDebug() << "IF: Prediction = TAKEN, jumping to:"
                     << QString::number(predicted_pc, 16);
            if_id_next_.predicted_taken = predict_taken;
        }
        else
        {
            if_id_next_.predicted_taken = false;
            qDebug() << "IF: Prediction = NOT_TAKEN, sequential PC:"
                     << QString::number(predicted_pc, 16);
        }
    }
    else
    {
        qDebug() << "IF: Branch prediction DISABLED - always sequential";
    }

    // Fetch instruction
    if_id_next_.pc = program_counter_;
    if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
    if_id_next_.valid = true;

    qDebug() << "IF: Fetched from:" << QString::number(program_counter_, 16)
             << "Next PC:" << QString::number(predicted_pc, 16);

    // Update PC for next fetch
    program_counter_ = predicted_pc;
}

void RVSSVMPipelined::ID_stage()
{
    qDebug() << "\n=== ID STAGE START ===";

    if (flush_pipeline_)
    {
        qDebug() << "ID: Pipeline flushed - inserting bubble";
        id_ex_next_ = ID_EX();
        return;
    }

    if (stall_)
    {
        qDebug() << "ID: Stalled - inserting bubble";
        id_ex_next_ = ID_EX();
        return;
    }

    if (!if_id_.valid)
    {
        qDebug() << "ID: Invalid instruction - inserting bubble";
        id_ex_next_ = ID_EX();
        return;
    }

    uint32_t instr = if_id_.instruction;
    uint8_t curr_rs1 = (instr >> 15) & 0b11111;
    uint8_t curr_rs2 = (instr >> 20) & 0b11111;
    uint32_t opcode = instr & 0x7f;
    uint8_t funct3 = (instr >> 12) & 0x7;
    uint8_t funct7 = (instr >> 25) & 0b1111111;
    id_ex_next_.predicted_taken = if_id_.predicted_taken;

    qDebug() << "ID: PC:" << QString::number(if_id_.pc, 16);
    qDebug() << "ID: Instruction:" << QString::number(instr, 16);
    qDebug() << "ID: Opcode:" << QString::number(opcode, 2).rightJustified(7, '0');
    qDebug() << "ID: rs1:" << curr_rs1 << "rs2:" << curr_rs2;
    qDebug() << "ID: funct3:" << QString::number(funct3, 2) << "funct7:" << QString::number(funct7, 2);

    // Check system call
    auto ecall_encoding = get_instr_encoding(Instruction::kecall);
    id_ex_next_.is_syscall = (opcode == static_cast<uint32_t>(ecall_encoding.opcode) &&
                              funct3 == static_cast<uint8_t>(ecall_encoding.funct3));
    if (id_ex_next_.is_syscall) {
        qDebug() << "ID: *** ECALL DETECTED ***";
    }

    // Detect floating-point instructions
    bool is_float_instr = instruction_set::isFInstruction(instr);
    bool is_double_instr = instruction_set::isDInstruction(instr);
    id_ex_next_.is_float = is_float_instr || is_double_instr;

    if (id_ex_next_.is_float) {
        qDebug() << "ID: *** FLOATING-POINT INSTRUCTION ***";
        qDebug() << "ID: Float:" << is_float_instr << "Double:" << is_double_instr;
    }

    // ✅ FIX: Hazard detection with proper stall counting
    if (hazard_detection_enabled_)
    {
        bool should_stall = false;
        bool load_use = hazard_unit_.DetectLoadUseHazard(id_ex_.rd, id_ex_.mem_read, curr_rs1, curr_rs2);
        if (load_use) {
            qDebug() << "ID: LOAD-USE HAZARD detected! rd:" << id_ex_.rd;
            should_stall = true;
        }

        if (!forwarding_enabled_)
        {
            bool ex_hazard = hazard_unit_.DetectEXHazard(id_ex_.rd, id_ex_.reg_write, curr_rs1, curr_rs2);
            bool mem_hazard = hazard_unit_.DetectMEMHazard(ex_mem_.rd, ex_mem_.reg_write, curr_rs1, curr_rs2);
            if (ex_hazard) {
                qDebug() << "ID: EX HAZARD detected! rd:" << id_ex_.rd;
                should_stall = true;
            }
            if (mem_hazard) {
                qDebug() << "ID: MEM HAZARD detected! rd:" << ex_mem_.rd;
                should_stall = true;
            }
        }

        if (should_stall)
        {
            qDebug() << "ID: STALLING pipeline";
            stall_ = true;
            // ✅ FIX: Only increment stall counter once per stall event
            // The counter will be incremented in Step() or Run() once per cycle
            id_ex_next_ = ID_EX();
            return;
        }
    }

    // Continue with normal decode...
    id_ex_next_.valid = true;
    id_ex_next_.pc = if_id_.pc;
    id_ex_next_.instruction = instr;
    id_ex_next_.rs1 = curr_rs1;
    id_ex_next_.rs2 = curr_rs2;
    id_ex_next_.rd = (instr >> 7) & 0b11111;
    id_ex_next_.funct3 = funct3;
    id_ex_next_.funct7 = funct7;
    id_ex_next_.imm = ImmGenerator(instr);

    qDebug() << "ID: rd:" << id_ex_next_.rd << "imm:" << id_ex_next_.imm;

    // Register reading logic
    if (is_float_instr || is_double_instr)
    {
        qDebug() << "ID: Reading floating-point registers";

        // For loads (FLW/FLD), rs1 is base address from GPR
        if (opcode == 0b0000111) // FLW/FLD
        {
            id_ex_next_.reg1_value = registers_->ReadGpr(curr_rs1);
            id_ex_next_.reg2_value = 0;
            qDebug() << "ID: FLW/FLD - Base (GPR x" << curr_rs1 << "):"
                     << QString::number(id_ex_next_.reg1_value, 16);
        }
        // For stores (FSW/FSD), rs1 is base address (GPR), rs2 is data (FPR)
        else if (opcode == 0b0100111) // FSW/FSD
        {
            id_ex_next_.reg1_value = registers_->ReadGpr(curr_rs1);
            id_ex_next_.reg2_value = registers_->ReadFpr(curr_rs2);
            qDebug() << "ID: FSW/FSD - Base (GPR x" << curr_rs1 << "):"
                     << QString::number(id_ex_next_.reg1_value, 16);
            qDebug() << "ID: FSW/FSD - Data (FPR f" << curr_rs2 << "):"
                     << QString::number(id_ex_next_.reg2_value, 16);
        }
        // For FCVT/FMV from integer to float
        else if (funct7 == 0b1101000 || funct7 == 0b1111000 || // Float conversions
                 funct7 == 0b1101001 || funct7 == 0b1111001)   // Double conversions
        {
            id_ex_next_.reg1_value = registers_->ReadGpr(curr_rs1);
            id_ex_next_.reg2_value = 0;
            qDebug() << "ID: FCVT/FMV int->float - Source (GPR x" << curr_rs1 << "):"
                     << QString::number(id_ex_next_.reg1_value, 16);
        }
        // For FCVT/FMV from float to integer, or FCLASS
        else if (funct7 == 0b1100000 || funct7 == 0b1110000 || funct7 == 0b1110001 || // Float to int
                 funct7 == 0b1100001 || funct7 == 0b1110001)                          // Double to int
        {
            id_ex_next_.reg1_value = registers_->ReadFpr(curr_rs1);
            id_ex_next_.reg2_value = 0;
            qDebug() << "ID: FCVT/FMV float->int - Source (FPR f" << curr_rs1 << "):"
                     << QString::number(id_ex_next_.reg1_value, 16);
        }
        // Standard FP operations
        else
        {
            id_ex_next_.reg1_value = registers_->ReadFpr(curr_rs1);
            id_ex_next_.reg2_value = registers_->ReadFpr(curr_rs2);
            qDebug() << "ID: FP operation - rs1 (FPR f" << curr_rs1 << "):"
                     << QString::number(id_ex_next_.reg1_value, 16);
            qDebug() << "ID: FP operation - rs2 (FPR f" << curr_rs2 << "):"
                     << QString::number(id_ex_next_.reg2_value, 16);
        }

        // rs3 for fused multiply-add operations
        uint8_t rs3 = (instr >> 27) & 0b11111;
        id_ex_next_.reg3_value = registers_->ReadFpr(rs3);
        if (rs3 != 0) {
            qDebug() << "ID: rs3 (FPR f" << rs3 << "):"
                     << QString::number(id_ex_next_.reg3_value, 16);
        }
    }
    else
    {
        id_ex_next_.reg1_value = registers_->ReadGpr(curr_rs1);
        id_ex_next_.reg2_value = registers_->ReadGpr(curr_rs2);
        qDebug() << "ID: Integer operation - rs1 (GPR x" << curr_rs1 << "):"
                 << QString::number(id_ex_next_.reg1_value, 16);
        qDebug() << "ID: Integer operation - rs2 (GPR x" << curr_rs2 << "):"
                 << QString::number(id_ex_next_.reg2_value, 16);
    }

    control_unit_.SetControlSignals(instr);
    id_ex_next_.reg_write = control_unit_.GetRegWrite();
    id_ex_next_.mem_read = control_unit_.GetMemRead();
    id_ex_next_.mem_write = control_unit_.GetMemWrite();
    id_ex_next_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_next_.alu_src = control_unit_.GetAluSrc();
    id_ex_next_.branch = control_unit_.GetBranch();

    qDebug() << "ID: Control signals - RegWrite:" << id_ex_next_.reg_write
             << "MemRead:" << id_ex_next_.mem_read
             << "MemWrite:" << id_ex_next_.mem_write
             << "MemToReg:" << id_ex_next_.mem_to_reg
             << "AluSrc:" << id_ex_next_.alu_src
             << "Branch:" << id_ex_next_.branch;
    qDebug() << "=== ID STAGE END ===\n";
}

void RVSSVMPipelined::EX_stage()
{
    qDebug() << "\n=== EX STAGE START ===";

    if (!id_ex_.valid)
    {
        qDebug() << "EX: Invalid instruction - bubble";
        ex_mem_next_.valid = false;
        return;
    }

    qDebug() << "EX: PC:" << QString::number(id_ex_.pc, 16)
             << "Instruction:" << QString::number(id_ex_.instruction, 16);

    ex_mem_next_.valid = true;
    ex_mem_next_.pc = id_ex_.pc;
    ex_mem_next_.instruction = id_ex_.instruction;
    ex_mem_next_.rd = id_ex_.rd;
    ex_mem_next_.reg_write = id_ex_.reg_write;
    ex_mem_next_.mem_read = id_ex_.mem_read;
    ex_mem_next_.mem_write = id_ex_.mem_write;
    ex_mem_next_.mem_to_reg = id_ex_.mem_to_reg;
    ex_mem_next_.is_float = id_ex_.is_float;
    ex_mem_next_.is_syscall = id_ex_.is_syscall;

    uint8_t opcode = id_ex_.instruction & 0x7F;
    qDebug() << "EX: Opcode:" << QString::number(opcode, 2).rightJustified(7, '0');

    // ✅ Handle system calls
    if (id_ex_.is_syscall)
    {
        qDebug() << "EX: Processing ECALL";
        // System call handling would go here
        // For now, just pass through to MEM stage
        ex_mem_next_.alu_result = 0;
        ex_mem_next_.reg2_value = 0;
        qDebug() << "=== EX STAGE END ===\n";
        return;
    }

    // Handle floating-point instructions
    if (id_ex_.is_float)
    {
        qDebug() << "EX: >>> FLOATING-POINT EXECUTION <<<";
        uint8_t funct3 = id_ex_.funct3;
        uint8_t funct7 = id_ex_.funct7;

        uint64_t op1 = id_ex_.reg1_value;
        uint64_t op2 = id_ex_.reg2_value;
        uint64_t op3 = id_ex_.reg3_value;
        uint64_t store_data = id_ex_.reg2_value;

        qDebug() << "EX: Initial op1:" << QString::number(op1, 16);
        qDebug() << "EX: Initial op2:" << QString::number(op2, 16);
        qDebug() << "EX: Initial op3:" << QString::number(op3, 16);

        // ✅ FIX: Apply forwarding for FP instructions
        if (forwarding_enabled_)
        {
            qDebug() << "EX: Checking forwarding for FP instruction";

            // ✅ FIX: Don't forward if source is x0 (hardwired zero)
            if (id_ex_.rs1 != 0)
            {
                auto rs1_src = forwarding_unit_.GetRs1Source(
                    ex_mem_.reg_write, ex_mem_.rd,
                    mem_wb_.reg_write, mem_wb_.rd,
                    id_ex_.rs1);

                if (rs1_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM) {
                    op1 = ex_mem_.alu_result;
                    qDebug() << "EX: FORWARDING rs1 from EX/MEM:" << QString::number(op1, 16);
                } else if (rs1_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB) {
                    op1 = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
                    qDebug() << "EX: FORWARDING rs1 from MEM/WB:" << QString::number(op1, 16);
                }
            }
            else
            {
                op1 = 0;  // Explicitly set x0 to zero
            }

            // ✅ FIX: Don't forward if source is x0 (hardwired zero)
            if (id_ex_.rs2 != 0)
            {
                auto rs2_src = forwarding_unit_.GetRs2Source(
                    ex_mem_.reg_write, ex_mem_.rd,
                    mem_wb_.reg_write, mem_wb_.rd,
                    id_ex_.rs2);

                if (rs2_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM) {
                    op2 = ex_mem_.alu_result;
                    store_data = ex_mem_.alu_result;
                    qDebug() << "EX: FORWARDING rs2 from EX/MEM:" << QString::number(op2, 16);
                } else if (rs2_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB) {
                    uint64_t fwd = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
                    op2 = fwd;
                    store_data = fwd;
                    qDebug() << "EX: FORWARDING rs2 from MEM/WB:" << QString::number(op2, 16);
                }
            }
            else
            {
                op2 = 0;  // Explicitly set x0 to zero
                store_data = 0;
            }
        }

        // Handle FLW/FLD (address calculation)
        if (opcode == 0b0000111) // FLW/FLD
        {
            ex_mem_next_.alu_result = op1 + static_cast<int64_t>(id_ex_.imm);
            ex_mem_next_.reg2_value = 0;
            qDebug() << "EX: FLW/FLD address calculation";
            qDebug() << "EX: Base:" << QString::number(op1, 16)
                     << "+ Offset:" << id_ex_.imm
                     << "= Address:" << QString::number(ex_mem_next_.alu_result, 16);
            qDebug() << "=== EX STAGE END ===\n";
            return;
        }

        // Handle FSW/FSD (address calculation + data forwarding)
        if (opcode == 0b0100111) // FSW/FSD
        {
            ex_mem_next_.alu_result = op1 + static_cast<int64_t>(id_ex_.imm);
            ex_mem_next_.reg2_value = store_data;
            qDebug() << "EX: FSW/FSD address calculation";
            qDebug() << "EX: Base:" << QString::number(op1, 16)
                     << "+ Offset:" << id_ex_.imm
                     << "= Address:" << QString::number(ex_mem_next_.alu_result, 16);
            qDebug() << "EX: Store data:" << QString::number(store_data, 16);
            qDebug() << "=== EX STAGE END ===\n";
            return;
        }

        // For other FP operations, execute through FPU
        uint8_t rm = funct3;
        if (rm == 0b111) {
            rm = registers_->ReadCsr(0x002);
            qDebug() << "EX: Using dynamic rounding mode from CSR:" << rm;
        } else {
            qDebug() << "EX: Using static rounding mode:" << rm;
        }

        if (id_ex_.alu_src) {
            op2 = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm));
            qDebug() << "EX: Using immediate as op2:" << QString::number(op2, 16);
        }

        alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
        uint8_t fcsr_status = 0;
        bool is_double = instruction_set::isDInstruction(id_ex_.instruction);

        qDebug() << "EX: Executing FP operation - Double:" << is_double;
        qDebug() << "EX: op1:" << QString::number(op1, 16)
                 << "op2:" << QString::number(op2, 16)
                 << "op3:" << QString::number(op3, 16);

        if (is_double && registers_->GetIsa() == ISA::RV64)
        {
            std::tie(ex_mem_next_.alu_result, fcsr_status) =
                alu::Alu::dfpexecute(aluOperation, op1, op2, op3, rm);
        }
        else if (!is_double)
        {
            std::tie(ex_mem_next_.alu_result, fcsr_status) =
                alu::Alu::fpexecute(aluOperation, op1, op2, op3, rm);
        }

        qDebug() << "EX: FP result:" << QString::number(ex_mem_next_.alu_result, 16);
        qDebug() << "EX: FCSR status:" << QString::number(fcsr_status, 2);

        registers_->WriteCsr(0x003, fcsr_status);
        emit csrUpdated(0x003, fcsr_status);

        ex_mem_next_.reg2_value = store_data;
        qDebug() << "=== EX STAGE END ===\n";
        return;
    }

    // Regular integer execution path
    qDebug() << "EX: Integer execution path";
    uint64_t op1 = id_ex_.reg1_value;
    uint64_t op2 = id_ex_.reg2_value;
    uint64_t store_data = id_ex_.reg2_value;

    qDebug() << "EX: Initial op1:" << QString::number(op1, 16);
    qDebug() << "EX: Initial op2:" << QString::number(op2, 16);

    if (forwarding_enabled_)
    {
        qDebug() << "EX: Checking forwarding for integer instruction";

        auto rs1_src = forwarding_unit_.GetRs1Source(
            ex_mem_.reg_write, ex_mem_.rd,
            mem_wb_.reg_write, mem_wb_.rd,
            id_ex_.rs1);

        if (rs1_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM) {
            op1 = ex_mem_.alu_result;
            qDebug() << "EX: FORWARDING rs1 from EX/MEM:" << QString::number(op1, 16);
        } else if (rs1_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB) {
            uint64_t fwd = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
            op1 = fwd;
            qDebug() << "EX: FORWARDING rs1 from MEM/WB:" << QString::number(op1, 16);
        }

        // rs2 / store-data forwarding
        auto rs2_src = forwarding_unit_.GetRs2Source(
            ex_mem_.reg_write, ex_mem_.rd,
            mem_wb_.reg_write, mem_wb_.rd,
            id_ex_.rs2);

        if (rs2_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM) {
            op2 = ex_mem_.alu_result;
            store_data = ex_mem_.alu_result;
            qDebug() << "EX: FORWARDING rs2 from EX/MEM:" << QString::number(op2, 16);
        } else if (rs2_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB) {
            uint64_t fwd = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
            op2 = fwd;
            store_data = fwd;
            qDebug() << "EX: FORWARDING rs2 from MEM/WB:" << QString::number(op2, 16);
        }
    }

    // Handle special instruction types
    if (opcode == 0b0110111) // LUI
    {
        op2 = static_cast<uint64_t>(id_ex_.imm) << 12;
        op1 = 0;
        qDebug() << "EX: LUI - Loading upper immediate:" << QString::number(op2, 16);
    }
    else if (opcode == 0b0010111) // AUIPC
    {
        op2 = static_cast<uint64_t>(id_ex_.imm) << 12;
        op1 = static_cast<uint64_t>(id_ex_.pc);
        qDebug() << "EX: AUIPC - PC:" << QString::number(op1, 16)
                 << "+ Immediate:" << QString::number(op2, 16);
    }
    else if (id_ex_.alu_src)
    {
        op2 = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm));
        qDebug() << "EX: Using immediate as op2:" << QString::number(op2, 16);
    }

    alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
    bool overflow = false;
    std::tie(ex_mem_next_.alu_result, overflow) = alu_.execute(aluOperation, op1, op2);

    qDebug() << "EX: ALU result:" << QString::number(ex_mem_next_.alu_result, 16)
             << "Overflow:" << overflow;

    ex_mem_next_.reg2_value = store_data;
    ex_mem_next_.branch_taken = false;

    // ✅ FIX: Corrected branch handling logic
    if (id_ex_.branch)
    {
        qDebug() << "EX: Processing branch instruction";
        bool take = false;
        uint8_t funct3 = id_ex_.funct3;

        // Determine if branch should be taken
        switch (funct3)
        {
        case 0b000: take = (ex_mem_next_.alu_result == 0); qDebug() << "EX: BEQ"; break;
        case 0b001: take = (ex_mem_next_.alu_result != 0); qDebug() << "EX: BNE"; break;
        case 0b100: take = (ex_mem_next_.alu_result == 1); qDebug() << "EX: BLT"; break;
        case 0b101: take = (ex_mem_next_.alu_result == 0); qDebug() << "EX: BGE"; break;
        case 0b110: take = (ex_mem_next_.alu_result == 1); qDebug() << "EX: BLTU"; break;
        case 0b111: take = (ex_mem_next_.alu_result == 0); qDebug() << "EX: BGEU"; break;
        }

        uint64_t branch_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc)) + id_ex_.imm;
        uint64_t next_pc = take ? branch_target : (id_ex_.pc + 4);

        qDebug() << "EX: Branch taken:" << take << "Target:" << QString::number(branch_target, 16);

        // ✅ FIX: Unified branch handling
        // ✅ FIX: Unified branch handling
        if (branch_prediction_enabled_)
        {
            size_t index = (id_ex_.pc >> 2) % BHT_SIZE;
            bool predicted_taken = id_ex_.predicted_taken;

            if (dynamic_branch_prediction_enabled_)
            {
                // Get the prediction that was made in IF stage
                predicted_taken = branch_history_table_[index];

                qDebug() << "EX: Dynamic prediction was:" << predicted_taken
                         << "Actual:" << take;

                // ✅ Update BHT state based on actual outcome
                branch_history_table_[index] = take;

                // ✅ Update BTB with target when branch is taken
                if (take) {
                    branch_target_buffer_[index] = branch_target;
                    qDebug() << "EX: Updating BTB[" << index << "] with target:"
                             << QString::number(branch_target, 16);
                }
            }
            else  // Static prediction
            {
                // Check if we had a BTB entry (means we predicted something)
                uint64_t btb_target = branch_target_buffer_[index];

                if (btb_target != 0)
                {
                    // We had a prediction - check what it was
                    if (btb_target < id_ex_.pc)
                    {
                        // Backward branch - we predicted TAKEN
                        predicted_taken = true;
                    }
                    else
                    {
                        // Forward branch - we predicted NOT_TAKEN
                        predicted_taken = false;
                    }
                }
                else
                {
                    // First time seeing this branch - predicted NOT_TAKEN
                    predicted_taken = false;
                }

                qDebug() << "EX: Static prediction was:" << predicted_taken
                         << "Actual:" << take;

                // Always update BTB with the target
                branch_target_buffer_[index] = branch_target;
            }

            // ✅ Handle misprediction
            if (predicted_taken != take)
            {
                qDebug() << "EX: *** BRANCH MISPREDICTION - FLUSHING ***";
                qDebug() << "EX: Correcting PC to:"
                         << QString::number(take ? branch_target : (id_ex_.pc + 4), 16);

                pc_update_pending_ = true;
                pc_update_value_ = take ? branch_target : (id_ex_.pc + 4);
                flush_pipeline_ = true;
            }
            else
            {
                qDebug() << "EX: Branch prediction CORRECT!";
            }
        }
        else  // ✅ No prediction enabled - always flush when branch is taken
        {
            if (take)
            {
                qDebug() << "EX: Taking branch to:" << QString::number(branch_target, 16);
                pc_update_pending_ = true;
                pc_update_value_ = branch_target;
                flush_pipeline_ = true;
            }
            else
            {
                qDebug() << "EX: Branch not taken - continuing sequential execution";
            }
        }

        ex_mem_next_.branch_taken = take;
        ex_mem_next_.branch_target = branch_target;
    }


    // Handle JAL
    if (opcode == 0b1101111) // JAL
    {
        uint64_t jump_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc)) + id_ex_.imm;
        ex_mem_next_.alu_result = id_ex_.pc + 4;  // Return address
        qDebug() << "EX: JAL to:" << QString::number(jump_target, 16)
                 << "Return address:" << QString::number(ex_mem_next_.alu_result, 16);
        pc_update_pending_ = true;
        pc_update_value_ = jump_target;
        flush_pipeline_ = true;
    }
    // Handle JALR
    else if (opcode == 0b1100111) // JALR
    {
        uint64_t jump_target = (op1 + static_cast<int64_t>(id_ex_.imm)) & ~1ULL;
        ex_mem_next_.alu_result = id_ex_.pc + 4;  // Return address
        qDebug() << "EX: JALR to:" << QString::number(jump_target, 16)
                 << "Return address:" << QString::number(ex_mem_next_.alu_result, 16);
        pc_update_pending_ = true;
        pc_update_value_ = jump_target;
        flush_pipeline_ = true;
    }

    qDebug() << "=== EX STAGE END ===\n";
}

void RVSSVMPipelined::MEM_stage()
{
    qDebug() << "\n=== MEM STAGE START ===";

    if (!ex_mem_.valid)
    {
        qDebug() << "MEM: Invalid instruction - bubble";
        mem_wb_next_.valid = false;
        return;
    }

    qDebug() << "MEM: PC:" << QString::number(ex_mem_.pc, 16)
             << "Instruction:" << QString::number(ex_mem_.instruction, 16);

    mem_wb_next_.valid = true;
    mem_wb_next_.rd = ex_mem_.rd;
    mem_wb_next_.reg_write = ex_mem_.reg_write;
    mem_wb_next_.mem_to_reg = ex_mem_.mem_to_reg;
    mem_wb_next_.alu_result = ex_mem_.alu_result;
    mem_wb_next_.pc = ex_mem_.pc;
    mem_wb_next_.is_float = ex_mem_.is_float;
    mem_wb_next_.instruction = ex_mem_.instruction;
    mem_wb_next_.is_syscall = ex_mem_.is_syscall;

    uint8_t opcode = ex_mem_.instruction & 0x7F;
    uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;

    qDebug() << "MEM: rd:" << mem_wb_next_.rd << "is_float:" << mem_wb_next_.is_float;
    qDebug() << "MEM: ALU result:" << QString::number(ex_mem_.alu_result, 16);

    // ✅ FIX: Alignment checks BEFORE memory access
    if (ex_mem_.mem_read || ex_mem_.mem_write)
    {
        uint64_t addr = ex_mem_.alu_result;
        bool alignment_ok = true;

        switch (funct3)
        {
        case 0b001:  // LH/SH - 2-byte alignment
        case 0b101:  // LHU
            if (addr & 0x1) {
                alignment_ok = false;
                qDebug() << "MEM: 2-byte alignment violation";
            }
            break;
        case 0b010:  // LW/SW/FLW/FSW - 4-byte alignment
        case 0b110:  // LWU
            if (addr & 0x3) {
                alignment_ok = false;
                qDebug() << "MEM: 4-byte alignment violation";
            }
            break;
        case 0b011:  // LD/SD/FLD/FSD - 8-byte alignment
            if (addr & 0x7) {
                alignment_ok = false;
                qDebug() << "MEM: 8-byte alignment violation";
            }
            break;
        case 0b000:  // LB/SB - no alignment required
        case 0b100:  // LBU
            // Byte access - always aligned
            break;
        }

        if (!alignment_ok)
        {
            qDebug() << "MEM: *** ALIGNMENT FAULT at address"
                     << QString::number(addr, 16) << "***";
            output_status_ = "VM_ALIGNMENT_FAULT";
            stop_requested_ = true;
            mem_wb_next_.valid = false;
            return;
        }
    }

    // Handle floating-point loads
    if (ex_mem_.mem_read)
    {
        qDebug() << "MEM: *** MEMORY READ ***";
        qDebug() << "MEM: Address:" << QString::number(ex_mem_.alu_result, 16);

        if (opcode == 0b0000111) // FLW/FLD
        {
            qDebug() << "MEM: Floating-point load operation";
            if (funct3 == 0b010) // FLW
            {
                uint32_t raw_value = memory_controller_.ReadWord(ex_mem_.alu_result);
                // NaN-box for single precision
                mem_wb_next_.mem_data = 0xFFFFFFFF00000000ULL | raw_value;
                qDebug() << "MEM: FLW - Raw value:" << QString::number(raw_value, 16);
                qDebug() << "MEM: FLW - NaN-boxed:" << QString::number(mem_wb_next_.mem_data, 16);
            }
            else if (funct3 == 0b011) // FLD
            {
                if (registers_->GetIsa() == ISA::RV64) {
                    mem_wb_next_.mem_data = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
                    qDebug() << "MEM: FLD - Value:" << QString::number(mem_wb_next_.mem_data, 16);
                }
            }
        }
        else // Integer loads
        {
            qDebug() << "MEM: Integer load operation - funct3:" << QString::number(funct3, 2);
            switch (funct3)
            {
            case 0b000:
                mem_wb_next_.mem_data = static_cast<int8_t>(memory_controller_.ReadByte(ex_mem_.alu_result));
                qDebug() << "MEM: LB (signed byte):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            case 0b001:
                mem_wb_next_.mem_data = static_cast<int16_t>(memory_controller_.ReadHalfWord(ex_mem_.alu_result));
                qDebug() << "MEM: LH (signed halfword):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            case 0b010:
                mem_wb_next_.mem_data = static_cast<int32_t>(memory_controller_.ReadWord(ex_mem_.alu_result));
                qDebug() << "MEM: LW (signed word):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            case 0b011:
                mem_wb_next_.mem_data = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
                qDebug() << "MEM: LD (doubleword):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            case 0b100:
                mem_wb_next_.mem_data = memory_controller_.ReadByte(ex_mem_.alu_result);
                qDebug() << "MEM: LBU (unsigned byte):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            case 0b101:
                mem_wb_next_.mem_data = memory_controller_.ReadHalfWord(ex_mem_.alu_result);
                qDebug() << "MEM: LHU (unsigned halfword):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            case 0b110:
                mem_wb_next_.mem_data = memory_controller_.ReadWord(ex_mem_.alu_result);
                qDebug() << "MEM: LWU (unsigned word):" << QString::number(mem_wb_next_.mem_data, 16);
                break;
            }
        }
    }

    // Handle stores (floating-point and integer)
    if (ex_mem_.mem_write)
    {
        qDebug() << "MEM: *** MEMORY WRITE ***";
        qDebug() << "MEM: Address:" << QString::number(ex_mem_.alu_result, 16);
        qDebug() << "MEM: Data:" << QString::number(ex_mem_.reg2_value, 16);

        if (opcode == 0b0100111) // FSW/FSD
        {
            qDebug() << "MEM: Floating-point store operation";

            if (recording_enabled_)
            {
                MemoryChange mem_change;
                mem_change.address = ex_mem_.alu_result;

                if (funct3 == 0b010) // FSW
                {
                    uint32_t old_val = memory_controller_.ReadWord(ex_mem_.alu_result);
                    for (int i = 0; i < 4; ++i)
                        mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                    uint32_t new_val = ex_mem_.reg2_value & 0xFFFFFFFF;
                    for (int i = 0; i < 4; ++i)
                        mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                    qDebug() << "MEM: FSW - Old:" << QString::number(old_val, 16)
                             << "New:" << QString::number(new_val, 16);
                }
                else if (funct3 == 0b011) // FSD
                {
                    uint64_t old_val = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
                    for (int i = 0; i < 8; ++i)
                        mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                    for (int i = 0; i < 8; ++i)
                        mem_change.new_bytes_vec.push_back((ex_mem_.reg2_value >> (i * 8)) & 0xFF);
                    qDebug() << "MEM: FSD - Old:" << QString::number(old_val, 16)
                             << "New:" << QString::number(ex_mem_.reg2_value, 16);
                }
                current_delta_.memory_changes.push_back(mem_change);
            }

            // Perform the write
            if (funct3 == 0b010) // FSW
            {
                uint32_t store_val = ex_mem_.reg2_value & 0xFFFFFFFF;
                memory_controller_.WriteWord(ex_mem_.alu_result, store_val);
                qDebug() << "MEM: FSW written - Value:" << QString::number(store_val, 16);
            }
            else if (funct3 == 0b011 && registers_->GetIsa() == ISA::RV64) // FSD
            {
                memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value);
                qDebug() << "MEM: FSD written - Value:" << QString::number(ex_mem_.reg2_value, 16);
            }
        }
        else // Integer stores
        {
            qDebug() << "MEM: Integer store operation - funct3:" << QString::number(funct3, 2);

            if (recording_enabled_)
            {
                MemoryChange mem_change;
                mem_change.address = ex_mem_.alu_result;

                switch (funct3)
                {
                case 0b000: // SB
                    mem_change.old_bytes_vec.push_back(memory_controller_.ReadByte(ex_mem_.alu_result));
                    mem_change.new_bytes_vec.push_back(ex_mem_.reg2_value & 0xFF);
                    qDebug() << "MEM: SB recording";
                    break;
                case 0b001: // SH
                {
                    uint16_t old_val = memory_controller_.ReadHalfWord(ex_mem_.alu_result);
                    mem_change.old_bytes_vec.push_back(old_val & 0xFF);
                    mem_change.old_bytes_vec.push_back((old_val >> 8) & 0xFF);
                    uint16_t new_val = ex_mem_.reg2_value & 0xFFFF;
                    mem_change.new_bytes_vec.push_back(new_val & 0xFF);
                    mem_change.new_bytes_vec.push_back((new_val >> 8) & 0xFF);
                    qDebug() << "MEM: SH recording";
                    break;
                }
                case 0b010: // SW
                {
                    uint32_t old_val = memory_controller_.ReadWord(ex_mem_.alu_result);
                    for (int i = 0; i < 4; ++i)
                        mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                    uint32_t new_val = ex_mem_.reg2_value & 0xFFFFFFFF;
                    for (int i = 0; i < 4; ++i)
                        mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                    qDebug() << "MEM: SW recording";
                    break;
                }
                case 0b011: // SD
                    if (registers_->GetIsa() == ISA::RV64)
                    {
                        uint64_t old_val = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
                        for (int i = 0; i < 8; ++i)
                            mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                        for (int i = 0; i < 8; ++i)
                            mem_change.new_bytes_vec.push_back((ex_mem_.reg2_value >> (i * 8)) & 0xFF);
                        qDebug() << "MEM: SD recording";
                    }
                    break;
                }
                current_delta_.memory_changes.push_back(mem_change);
            }

            switch (funct3)
            {
            case 0b000:
                memory_controller_.WriteByte(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFF);
                qDebug() << "MEM: SB written";
                break;
            case 0b001:
                memory_controller_.WriteHalfWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFF);
                qDebug() << "MEM: SH written";
                break;
            case 0b010:
                memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF);
                qDebug() << "MEM: SW written";
                break;
            case 0b011:
                if (registers_->GetIsa() == ISA::RV64) {
                    memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value);
                    qDebug() << "MEM: SD written";
                }
                break;
            }
        }
    }

    qDebug() << "=== MEM STAGE END ===\n";
}

void RVSSVMPipelined::WB_stage()
{
    if (!mem_wb_.valid)
    {
        return;
    }

    uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
    uint8_t opcode = mem_wb_.instruction & 0x7F;
    uint8_t funct3 = (mem_wb_.instruction >> 12) & 0b111;
    uint8_t funct7 = (mem_wb_.instruction >> 25) & 0b1111111;

    // ✅ FIX: Proper GPR/FPR write detection
    if (mem_wb_.reg_write && mem_wb_.rd != 0)
    {
        bool write_to_fpr = mem_wb_.is_float;
        bool write_to_gpr = false;

        // Float/Double to integer conversions write to GPR
        if (funct7 == 0b1100000 || funct7 == 0b1100001)  // FCVT.W/L.S/D
        {
            write_to_gpr = true;
            write_to_fpr = false;
        }
        // FMV from FPR to GPR
        else if (funct7 == 0b1110000 || funct7 == 0b1110001)  // FMV.X.W/D
        {
            write_to_gpr = true;
            write_to_fpr = false;
        }
        // FCLASS writes to GPR
        else if (funct7 == 0b1110000 && funct3 == 0b001)  // FCLASS
        {
            write_to_gpr = true;
            write_to_fpr = false;
        }
        // Floating-point comparisons write to GPR
        else if (opcode == 0b1010011 &&
                 (funct7 == 0b1010000 || funct7 == 0b1010001))  // FEQ/FLT/FLE
        {
            write_to_gpr = true;
            write_to_fpr = false;
        }

        // Record changes for undo
        if (recording_enabled_)
        {
            RegisterChange reg_change;
            reg_change.reg_type = write_to_fpr ? 2 : 0;
            reg_change.reg_index = mem_wb_.rd;
            reg_change.old_value = write_to_fpr ?
                                       registers_->ReadFpr(mem_wb_.rd) : registers_->ReadGpr(mem_wb_.rd);
            reg_change.new_value = write_val;
            current_delta_.register_changes.push_back(reg_change);
        }

        // Write to correct register file
        if (write_to_fpr)
        {
            // ✅ FIX: NaN-box ALL single-precision results, not just loads
            bool is_single = !instruction_set::isDInstruction(mem_wb_.instruction);

            if (is_single)
            {
                // Extract lower 32 bits and NaN-box
                uint32_t float_bits = write_val & 0xFFFFFFFF;
                uint64_t boxed_value = 0xFFFFFFFF00000000ULL | float_bits;
                registers_->WriteFpr(mem_wb_.rd, boxed_value);
                emit fprUpdated(mem_wb_.rd, boxed_value);
            }
            else  // Double precision - write as-is
            {
                registers_->WriteFpr(mem_wb_.rd, write_val);
                emit fprUpdated(mem_wb_.rd, write_val);
            }
        }
        else  // Write to GPR
        {
            registers_->WriteGpr(mem_wb_.rd, write_val);
            emit gprUpdated(mem_wb_.rd, write_val);
        }
    }

    instructions_retired_++;
}


// ============================================================================
// CORRECTED advance_pipeline_registers() - Fix clear/update order
// ============================================================================
void RVSSVMPipelined::advance_pipeline_registers()
{
    // ✅ FIX: Clear old stage locations AFTER saving state but BEFORE updating
    std::map<std::string, uint64_t> old_stage_to_pc = stage_to_pc_;

    // Advance pipeline registers
    mem_wb_ = mem_wb_next_;
    ex_mem_ = ex_mem_next_;
    id_ex_ = id_ex_next_;
    if_id_ = if_id_next_;

    // Initialize next registers to empty
    mem_wb_next_ = MEM_WB();
    ex_mem_next_ = EX_MEM();
    id_ex_next_ = ID_EX();
    if_id_next_ = IF_ID();

    // ✅ FIX: Clear flags immediately to prevent persistent effects
    stall_ = false;
    flush_pipeline_ = false;

    // Now clear old stage locations
    for (const auto& [stage, pc] : old_stage_to_pc)
    {
        QString q = QString::fromStdString(stage + "_CLEAR");
        emit pipelineStageChanged(pc, q);
    }

    // Update stage_to_pc with new locations
    stage_to_pc_.clear();

    // Emit new stage locations
    if (mem_wb_.valid)
    {
        stage_to_pc_["WB"] = mem_wb_.pc;
        emit pipelineStageChanged(mem_wb_.pc, "WB");
    }

    if (ex_mem_.valid)
    {
        stage_to_pc_["MEM"] = ex_mem_.pc;
        emit pipelineStageChanged(ex_mem_.pc, "MEM");
    }

    if (id_ex_.valid)
    {
        stage_to_pc_["EX"] = id_ex_.pc;
        emit pipelineStageChanged(id_ex_.pc, "EX");
    }

    if (if_id_.valid)
    {
        stage_to_pc_["ID"] = if_id_.pc;
        emit pipelineStageChanged(if_id_.pc, "ID");
    }

    if (program_counter_ < program_size_)
    {
        stage_to_pc_["IF"] = program_counter_;
        emit pipelineStageChanged(program_counter_, "IF");
    }
}

void RVSSVMPipelined::Run()
{
    while (!stop_requested_)
    {
        bool pipeline_has_work = (if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
        bool fetch_remaining = (program_counter_ < program_size_);

        if (!pipeline_has_work && !fetch_remaining)
            break;

        WB_stage();
        MEM_stage();
        EX_stage();
        ID_stage();
        IF_stage();
        advance_pipeline_registers();

        cycle_s_++;
    }
    if(branch_prediction_enabled_)
        DumpBranchPredictionTables(globals::branchPredectionPath);
}

bool RVSSVMPipelined::IsPipelineEmpty() const
{
    return !(if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
}

void RVSSVMPipelined::DebugRun()
{
    Run();
}

void RVSSVMPipelined::Undo()
{
    if (pipeline_undo_stack_.empty())
    {
        qDebug() << "Undo stack is empty";
        return;
    }

    PipelineStepDelta last = pipeline_undo_stack_.top();
    pipeline_undo_stack_.pop();

    // Restore register changes
    for (const auto &change : last.register_changes)
    {
        switch (change.reg_type)
        {
        case 0: // GPR
            registers_->WriteGpr(change.reg_index, change.old_value);
            emit gprUpdated(change.reg_index, change.old_value);
            break;
        case 1: // CSR
            registers_->WriteCsr(change.reg_index, change.old_value);
            emit csrUpdated(change.reg_index, change.old_value);
            break;
        case 2: // FPR
            registers_->WriteFpr(change.reg_index, change.old_value);
            emit fprUpdated(change.reg_index, change.old_value);
            break;
        }
    }

    // Restore memory changes
    for (const auto &change : last.memory_changes)
    {
        for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
            memory_controller_.WriteByte(change.address + i, change.old_bytes_vec[i]);
    }

    if (stage_to_pc_.count("WB"))
    {
        emit pipelineStageChanged(stage_to_pc_["WB"], "WB_CLEAR");
    }
    if (stage_to_pc_.count("MEM"))
    {
        emit pipelineStageChanged(stage_to_pc_["MEM"], "MEM_CLEAR");
    }
    if (stage_to_pc_.count("EX"))
    {
        emit pipelineStageChanged(stage_to_pc_["EX"], "EX_CLEAR");
    }
    if (stage_to_pc_.count("ID"))
    {
        emit pipelineStageChanged(stage_to_pc_["ID"], "ID_CLEAR");
    }
    if (stage_to_pc_.count("IF"))
    {
        emit pipelineStageChanged(stage_to_pc_["IF"], "IF_CLEAR");
    }

    // Restore pipeline registers
    if_id_ = last.old_if_id;
    id_ex_ = last.old_id_ex;
    ex_mem_ = last.old_ex_mem;
    mem_wb_ = last.old_mem_wb;

    // Initialize next registers to empty
    if_id_next_ = IF_ID();
    id_ex_next_ = ID_EX();
    ex_mem_next_ = EX_MEM();
    mem_wb_next_ = MEM_WB();

    // Restore control state
    program_counter_ = last.old_pc;
    stall_ = last.old_stall;
    pc_update_pending_ = last.old_pc_update_pending;
    pc_update_value_ = last.old_pc_update_value;

    // Restore statistics
    cycle_s_ = last.old_cycle;
    instructions_retired_ = last.old_instructions_retired;
    stall_cycles_ = last.old_stall_cycles;

    stage_to_pc_.clear();

    if (mem_wb_.valid)
    {
        stage_to_pc_["WB"] = mem_wb_.pc;
        emit pipelineStageChanged(mem_wb_.pc, "WB");
    }

    if (ex_mem_.valid)
    {
        stage_to_pc_["MEM"] = ex_mem_.pc;
        emit pipelineStageChanged(ex_mem_.pc, "MEM");
    }

    if (id_ex_.valid)
    {
        stage_to_pc_["EX"] = id_ex_.pc;
        emit pipelineStageChanged(id_ex_.pc, "EX");
    }

    if (if_id_.valid)
    {
        stage_to_pc_["ID"] = if_id_.pc;
        emit pipelineStageChanged(if_id_.pc, "ID");
    }

    if (program_counter_ < program_size_)
    {
        stage_to_pc_["IF"] = program_counter_;
        emit pipelineStageChanged(program_counter_, "IF");
    }
}

void RVSSVMPipelined::Step()
{
    // Save current pipeline state for undo
    PipelineStepDelta delta;
    delta.old_pc = program_counter_;
    delta.old_if_id = if_id_;
    delta.old_id_ex = id_ex_;
    delta.old_ex_mem = ex_mem_;
    delta.old_mem_wb = mem_wb_;
    delta.old_stall = stall_;
    delta.old_pc_update_pending = pc_update_pending_;
    delta.old_pc_update_value = pc_update_value_;
    delta.old_cycle = cycle_s_;
    delta.old_instructions_retired = instructions_retired_;
    delta.old_stall_cycles = stall_cycles_;

    // Check if pipeline is done
    bool pipeline_has_work = (if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
    bool fetch_remaining = (program_counter_ < program_size_);

    if (!pipeline_has_work && !fetch_remaining)
    {
        output_status_ = "VM_PROGRAM_END";

        // Clear all pipeline stages
        if (stage_to_pc_.count("IF"))
            emit pipelineStageChanged(stage_to_pc_["IF"], "IF_CLEAR");
        if (stage_to_pc_.count("ID"))
            emit pipelineStageChanged(stage_to_pc_["ID"], "ID_CLEAR");
        if (stage_to_pc_.count("EX"))
            emit pipelineStageChanged(stage_to_pc_["EX"], "EX_CLEAR");
        if (stage_to_pc_.count("MEM"))
            emit pipelineStageChanged(stage_to_pc_["MEM"], "MEM_CLEAR");
        if (stage_to_pc_.count("WB"))
            emit pipelineStageChanged(stage_to_pc_["WB"], "WB_CLEAR");

        stage_to_pc_.clear();
        return;
    }

    // Enable recording for undo
    recording_enabled_ = true;

    // Execute pipeline stages
    WB_stage();
    MEM_stage();
    EX_stage();
    ID_stage();
    IF_stage();

    bool was_stalled = stall_;

    advance_pipeline_registers();

    // Count stall after advancing (flag is now cleared)
    if (was_stalled)
    {
        stall_cycles_++;
    }
    // Disable recording
    recording_enabled_ = false;

    cycle_s_++;

    // Save new pipeline state
    delta.new_pc = program_counter_;
    delta.new_if_id = if_id_;
    delta.new_id_ex = id_ex_;
    delta.new_ex_mem = ex_mem_;
    delta.new_mem_wb = mem_wb_;
    delta.new_stall = stall_;
    delta.register_changes = current_delta_.register_changes;
    delta.memory_changes = current_delta_.memory_changes;

    // ✅ Limit undo stack size
    const size_t MAX_UNDO_STACK_SIZE = 10000;
    if (pipeline_undo_stack_.size() >= MAX_UNDO_STACK_SIZE)
    {
        // Remove oldest entry (more efficient method)
        std::stack<PipelineStepDelta> temp_stack;
        temp_stack.push(pipeline_undo_stack_.top());
        pipeline_undo_stack_.pop();

        while (!pipeline_undo_stack_.empty())
        {
            temp_stack.push(pipeline_undo_stack_.top());
            pipeline_undo_stack_.pop();
        }

        while (!temp_stack.empty())
        {
            pipeline_undo_stack_.push(temp_stack.top());
            temp_stack.pop();
        }
    }

    // Push to undo stack
    pipeline_undo_stack_.push(delta);

    // Clear current delta for next step
    current_delta_ = StepDelta();
    if(branch_prediction_enabled_){
        qDebug() << "DumpBranchPrediction called";
        DumpBranchPredictionTables(globals::branchPredectionPath);
    }
}

void RVSSVMPipelined::SetPipelineConfig(bool hazardEnabled,
                                        bool forwardingEnabled,
                                        bool branchPredictionEnabled,
                                        bool dynamicPredictionEnabled)
{
    hazard_detection_enabled_ = hazardEnabled;
    forwarding_enabled_ = forwardingEnabled;

    branch_prediction_enabled_ = branchPredictionEnabled;
    dynamic_branch_prediction_enabled_ = dynamicPredictionEnabled;

    if (branch_prediction_enabled_)
    {

        branch_target_buffer_.assign(BHT_SIZE, 0);
        qDebug() << "buffer assigned";
        if (branch_prediction_enabled_)
        {
            branch_target_buffer_.assign(BHT_SIZE, 0);

            // ✅ FIX: Initialize BHT based on prediction mode
            if (dynamic_branch_prediction_enabled_)
            {
                // For dynamic prediction, start with NOT_TAKEN (false)
                // This is more conservative and learns over time
                branch_history_table_.assign(BHT_SIZE, false);
            }
            else
            {
                // For static prediction, we don't use BHT
                branch_history_table_.assign(BHT_SIZE, true);
            }
        }
    }
}

void RVSSVMPipelined::DumpBranchPredictionTables(const std::filesystem::path &filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        qDebug() << "Warning: Unable to open file for branch prediction dump:"
                 << QString::fromStdString(filepath.string());
        return;
    }

    file << "================================================================================\n";
    file << "                    BRANCH PREDICTION TABLES DUMP\n";
    file << "================================================================================\n";
    file << "Prediction Mode: ";
    if (!branch_prediction_enabled_)
        file << "DISABLED (No Prediction)\n";
    else if (dynamic_branch_prediction_enabled_)
        file << "1-BIT DYNAMIC PREDICTION\n";
    else
        file << "STATIC PREDICTION (Always Taken)\n";

    file << "Current Cycle: " << cycle_s_ << "\n";
    file << "Instructions Retired: " << instructions_retired_ << "\n";
    file << "Stall Cycles: " << stall_cycles_ << "\n";
    file << "================================================================================\n\n";

    // Count valid entries
    int btb_valid_entries = 0;
    int bht_taken_count = 0;
    int bht_not_taken_count = 0;

    for (size_t i = 0; i < BHT_SIZE; i++)
    {
        if (branch_target_buffer_[i] != 0)
            btb_valid_entries++;

        if (branch_history_table_[i])
            bht_taken_count++;
        else
            bht_not_taken_count++;
    }

    file << "Summary:\n";
    file << "  BTB Valid Entries: " << btb_valid_entries << " / " << BHT_SIZE << "\n";
    file << "  BHT Taken: " << bht_taken_count << " / " << BHT_SIZE << "\n";
    file << "  BHT Not Taken: " << bht_not_taken_count << " / " << BHT_SIZE << "\n";
    file << "\n";
    file << "================================================================================\n";
    file << "                    COMBINED BTB + BHT TABLE\n";
    file << "================================================================================\n";

    file << std::left
         << std::setw(8) << "Index"
         << std::setw(12) << "PC (Hex)"
         << std::setw(16) << "BTB Target"
         << std::setw(15) << "BHT State"
         << std::setw(20) << "Branch Type" << "\n";
    file << "--------------------------------------------------------------------------------\n";

    for (size_t i = 0; i < BHT_SIZE; i++)
    {
        uint64_t target = branch_target_buffer_[i];
        bool bht_state = branch_history_table_[i];

        // Skip empty entries unless you want to see all
        if (target == 0)
            continue;

        uint64_t pc = i * 4;  // Approximate PC

        std::stringstream pc_ss, target_ss;
        pc_ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << pc;

        std::string target_str;
        std::string branch_type;

        if (target != 0)
        {
            target_ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << target;
            target_str = target_ss.str();

            // Determine branch direction
            if (target < pc)
                branch_type = "BACKWARD (Loop)";
            else if (target > pc)
                branch_type = "FORWARD";
            else
                branch_type = "SELF";
        }
        else
        {
            target_str = "---";
            branch_type = "No Branch";
        }

        std::string bht_str = bht_state ? "TAKEN (1)" : "NOT TAKEN (0)";

        file << std::left
             << std::setw(8) << i
             << std::setw(12) << pc_ss.str()
             << std::setw(16) << target_str
             << std::setw(15) << bht_str
             << std::setw(20) << branch_type << "\n";
    }

    file << "================================================================================\n\n";

    // Detailed BTB Table
    file << "================================================================================\n";
    file << "                    BRANCH TARGET BUFFER (BTB) - DETAILED\n";
    file << "================================================================================\n";
    file << std::left
         << std::setw(8) << "Index"
         << std::setw(12) << "PC"
         << std::setw(16) << "Target (Hex)"
         << std::setw(12) << "Target (Dec)"
         << std::setw(10) << "Offset" << "\n";
    file << "--------------------------------------------------------------------------------\n";

    for (size_t i = 0; i < BHT_SIZE; i++)
    {
        uint64_t target = branch_target_buffer_[i];
        if (target == 0)
            continue;

        uint64_t pc = i * 4;
        int64_t offset = target - pc;

        std::stringstream pc_ss, target_ss;
        pc_ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << pc;
        target_ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << target;

        std::string offset_str = (offset > 0 ? "+" : "") + std::to_string(offset);

        file << std::left
             << std::setw(8) << i
             << std::setw(12) << pc_ss.str()
             << std::setw(16) << target_ss.str()
             << std::setw(12) << target
             << std::setw(10) << offset_str << "\n";
    }

    file << "================================================================================\n\n";

    // Detailed BHT Table
    file << "================================================================================\n";
    file << "                    BRANCH HISTORY TABLE (BHT) - DETAILED\n";
    file << "================================================================================\n";
    file << std::left
         << std::setw(8) << "Index"
         << std::setw(12) << "PC"
         << std::setw(15) << "Prediction"
         << std::setw(12) << "State Bit" << "\n";
    file << "--------------------------------------------------------------------------------\n";

    for (size_t i = 0; i < BHT_SIZE; i++)
    {
        // Only show entries that have corresponding BTB entries
        if (branch_target_buffer_[i] == 0)
            continue;

        uint64_t pc = i * 4;
        bool state = branch_history_table_[i];

        std::stringstream pc_ss;
        pc_ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << pc;

        file << std::left
             << std::setw(8) << i
             << std::setw(12) << pc_ss.str()
             << std::setw(15) << (state ? "TAKEN" : "NOT TAKEN")
             << std::setw(12) << (state ? 1 : 0) << "\n";
    }

    file << "================================================================================\n\n";

    // Statistics Section
    file << "================================================================================\n";
    file << "                    BRANCH PREDICTION STATISTICS\n";
    file << "================================================================================\n";

    if (branch_prediction_enabled_)
    {
        file << "Total Branches Encountered: " << btb_valid_entries << "\n";

        double btb_coverage = (btb_valid_entries * 100.0) / BHT_SIZE;
        file << "BTB Coverage: " << std::fixed << std::setprecision(2) << btb_coverage << "%\n";

        if (dynamic_branch_prediction_enabled_)
        {
            double taken_pct = (bht_taken_count * 100.0) / BHT_SIZE;
            double not_taken_pct = (bht_not_taken_count * 100.0) / BHT_SIZE;

            file << "\n1-Bit Predictor State Distribution:\n";
            file << "  Predicting TAKEN: " << bht_taken_count
                 << " (" << std::fixed << std::setprecision(2) << taken_pct << "%)\n";
            file << "  Predicting NOT TAKEN: " << bht_not_taken_count
                 << " (" << std::fixed << std::setprecision(2) << not_taken_pct << "%)\n";
        }
    }
    else
    {
        file << "Branch prediction is DISABLED\n";
    }

    file << "================================================================================\n";

    file.close();
    qDebug() << "Branch prediction tables dumped to:" << QString::fromStdString(filepath.string());
}

void RVSSVMPipelined::PrintBranchPredictionTables()
{
    qDebug() << "\n╔════════════════════════════════════════════════════════════════╗";
    qDebug() << "║          BRANCH PREDICTION TABLES (RUNTIME VIEW)              ║";
    qDebug() << "╚════════════════════════════════════════════════════════════════╝";
    qDebug() << "Prediction Mode:" << (branch_prediction_enabled_ ?
                                           (dynamic_branch_prediction_enabled_ ? "1-BIT DYNAMIC" : "STATIC") : "DISABLED");
    qDebug() << "Cycle:" << cycle_s_ << "Instructions:" << instructions_retired_;

    qDebug() << "\n--- Active BTB Entries ---";
    qDebug() << QString("Index").leftJustified(8)
             << QString("PC").leftJustified(12)
             << QString("Target").leftJustified(12)
             << QString("BHT").leftJustified(10);
    qDebug() << "----------------------------------------------------------------";

    int count = 0;
    for (size_t i = 0; i < BHT_SIZE && count < 20; i++)  // Limit to 20 entries for console
    {
        if (branch_target_buffer_[i] != 0)
        {
            uint64_t pc = i * 4;
            qDebug() << QString::number(i).leftJustified(8)
                     << QString("0x%1").arg(pc, 8, 16, QChar('0')).leftJustified(12)
                     << QString("0x%1").arg(branch_target_buffer_[i], 8, 16, QChar('0')).leftJustified(12)
                     << QString(branch_history_table_[i] ? "T" : "NT").leftJustified(10);
            count++;
        }
    }

    if (count == 0)
        qDebug() << "  (No active branches yet)";
    else if (count == 20)
        qDebug() << "  ... (showing first 20 entries, see dump file for complete table)";

    qDebug() << "================================================================\n";
}

// ✅ NEW: Add this debugging helper function to header and implementation
void RVSSVMPipelined::DumpPipelineState()
{
    qDebug() << "\n╔══════════════════════════════════════════════════════════╗";
    qDebug() << "║              PIPELINE STATE DUMP                         ║";
    qDebug() << "╚══════════════════════════════════════════════════════════╝";
    qDebug() << "Cycle:" << cycle_s_ << "| Instructions Retired:" << instructions_retired_;
    qDebug() << "Stall Cycles:" << stall_cycles_;
    qDebug() << "PC:" << QString::number(program_counter_, 16) << "/ Program Size:" << program_size_;
    qDebug() << "";
    qDebug() << "Control Flags:";
    qDebug() << "  flush_pipeline_      =" << flush_pipeline_;
    qDebug() << "  stall_               =" << stall_;
    qDebug() << "  pc_update_pending_   =" << pc_update_pending_;
    qDebug() << "  pc_update_value_     =" << QString::number(pc_update_value_, 16);
    qDebug() << "  stop_requested_      =" << stop_requested_;
    qDebug() << "";
    qDebug() << "Pipeline Registers:";
    qDebug() << "  IF/ID:  valid=" << if_id_.valid
             << " pc=" << QString::number(if_id_.pc, 16)
             << " instr=" << QString::number(if_id_.instruction, 16);
    qDebug() << "  ID/EX:  valid=" << id_ex_.valid
             << " pc=" << QString::number(id_ex_.pc, 16)
             << " rd=" << id_ex_.rd
             << " rs1=" << id_ex_.rs1
             << " rs2=" << id_ex_.rs2;
    qDebug() << "  EX/MEM: valid=" << ex_mem_.valid
             << " pc=" << QString::number(ex_mem_.pc, 16)
             << " rd=" << ex_mem_.rd
             << " alu_result=" << QString::number(ex_mem_.alu_result, 16);
    qDebug() << "  MEM/WB: valid=" << mem_wb_.valid
             << " pc=" << QString::number(mem_wb_.pc, 16)
             << " rd=" << mem_wb_.rd
             << " reg_write=" << mem_wb_.reg_write;
    qDebug() << "";
    qDebug() << "Pipeline has work:" << (if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
    qDebug() << "Fetch remaining:" << (program_counter_ < program_size_);
    qDebug() << "════════════════════════════════════════════════════════════\n";
}
