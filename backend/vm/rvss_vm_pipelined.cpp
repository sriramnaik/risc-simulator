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

    // ✅ After loading program, show IF at PC=0
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

    branch_history_table_.resize(BHT_SIZE, true); // default predict taken
    branch_target_buffer_.resize(BHT_SIZE, 0);
    branch_prediction_enabled_ = false;
    dynamic_branch_prediction_enabled_ = false;

    // Clear undo stack
    while (!pipeline_undo_stack_.empty())
        pipeline_undo_stack_.pop();

    stage_to_pc_.clear();
}

void RVSSVMPipelined::IF_stage()
{
    // qDebug() << "[IF-stage]: Instruction pc : " << program_counter_;

    // ✅ FIX 1: Handle PC updates FIRST (highest priority)
    if (pc_update_pending_)
    {
        program_counter_ = pc_update_value_;
        pc_update_pending_ = false;
        pc_update_value_ = 0;
        if_id_next_.valid = false; // flush IF
        return;
    }

    // ✅ FIX 2: Handle flush properly with return
    if (flush_pipeline_)
    {
        if_id_next_.valid = false; // Insert NOP
        return;                    // Don't fetch when flushing
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

    uint64_t predicted_pc = program_counter_ + 4;

    if (branch_prediction_enabled_)
    {
        size_t index = (program_counter_ >> 2) % BHT_SIZE;
        uint64_t target = branch_target_buffer_[index];
        bool predict_taken = true;

        if (dynamic_branch_prediction_enabled_)
        {
            predict_taken = branch_history_table_[index];
        }

        if (predict_taken && target != 0)
            predicted_pc = target;
    }

    if_id_next_.pc = program_counter_;
    if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
    if_id_next_.valid = true;

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
            stall_cycles_++;
            id_ex_next_ = ID_EX();
            return;
        }
    }

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

    // Register reading logic with debugging
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

    uint8_t opcode = id_ex_.instruction & 0x7F;
    qDebug() << "EX: Opcode:" << QString::number(opcode, 2).rightJustified(7, '0');

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

        // Apply forwarding for FP instructions
        if (forwarding_enabled_)
        {
            qDebug() << "EX: Checking forwarding for FP instruction";

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
            op1 = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
            qDebug() << "EX: FORWARDING rs1 from MEM/WB:" << QString::number(op1, 16);
        }

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

    // Branch handling
    if (id_ex_.branch)
    {
        qDebug() << "EX: Processing branch instruction";
        bool take = false;
        uint8_t funct3 = id_ex_.funct3;

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
        qDebug() << "EX: Branch taken:" << take << "Target:" << QString::number(branch_target, 16);

        if (branch_prediction_enabled_)
        {
            size_t index = (id_ex_.pc >> 2) % BHT_SIZE;
            bool predicted_taken = false;

            if (dynamic_branch_prediction_enabled_)
            {
                predicted_taken = branch_history_table_[index];
                qDebug() << "EX: Dynamic prediction - Predicted:" << predicted_taken << "Actual:" << take;

                branch_history_table_[index] = take;
                if (take)
                    branch_target_buffer_[index] = branch_target;
                else
                    branch_target_buffer_[index] = 0;

                if (predicted_taken != take)
                {
                    qDebug() << "EX: *** BRANCH MISPREDICTION - FLUSHING ***";
                    pc_update_pending_ = true;
                    pc_update_value_ = take ? branch_target : (id_ex_.pc + 4);
                    flush_pipeline_ = true;
                }
            }
            else
            {
                predicted_taken = (branch_target_buffer_[index] != 0);
                qDebug() << "EX: Static prediction - Predicted:" << predicted_taken << "Actual:" << take;

                if (take)
                    branch_target_buffer_[index] = branch_target;
                else
                    branch_target_buffer_[index] = 0;

                if (predicted_taken != take)
                {
                    qDebug() << "EX: *** BRANCH MISPREDICTION - FLUSHING ***";
                    pc_update_pending_ = true;
                    pc_update_value_ = take ? branch_target : (id_ex_.pc + 4);
                    flush_pipeline_ = true;
                }
            }
        }
        else
        {
            if (take)
            {
                qDebug() << "EX: Taking branch to:" << QString::number(branch_target, 16);
                ex_mem_next_.branch_taken = true;
                ex_mem_next_.branch_target = branch_target;
                pc_update_pending_ = true;
                pc_update_value_ = branch_target;
                flush_pipeline_ = true;
            }
        }
    }

    // Handle JAL and JALR
    if (opcode == 0b1101111) // JAL
    {
        uint64_t jump_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc)) + id_ex_.imm;
        ex_mem_next_.alu_result = id_ex_.pc + 4;
        qDebug() << "EX: JAL to:" << QString::number(jump_target, 16)
                 << "Return address:" << QString::number(ex_mem_next_.alu_result, 16);
        pc_update_pending_ = true;
        pc_update_value_ = jump_target;
        flush_pipeline_ = true;
    }
    else if (opcode == 0b1100111) // JALR
    {
        uint64_t jump_target = (op1 + static_cast<int64_t>(id_ex_.imm)) & ~1ULL;
        ex_mem_next_.alu_result = id_ex_.pc + 4;
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

    uint8_t opcode = ex_mem_.instruction & 0x7F;
    uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;

    qDebug() << "MEM: rd:" << mem_wb_next_.rd << "is_float:" << mem_wb_next_.is_float;
    qDebug() << "MEM: ALU result:" << QString::number(ex_mem_.alu_result, 16);

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

    // Handle floating-point stores
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
    qDebug() << "\n=== WB STAGE START ===";

    if (!mem_wb_.valid)
    {
        qDebug() << "WB: Invalid instruction - bubble";
        return;
    }

    qDebug() << "WB: PC:" << QString::number(mem_wb_.pc, 16)
             << "Instruction:" << QString::number(mem_wb_.instruction, 16);

    uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;

    qDebug() << "WB: rd:" << mem_wb_.rd
             << "reg_write:" << mem_wb_.reg_write
             << "mem_to_reg:" << mem_wb_.mem_to_reg
             << "is_float:" << mem_wb_.is_float;
    qDebug() << "WB: Write value:" << QString::number(write_val, 16);

    if (mem_wb_.reg_write)
    {
        uint8_t opcode = mem_wb_.instruction & 0x7F;
        uint8_t funct7 = (mem_wb_.instruction >> 25) & 0b1111111;

        qDebug() << "WB: Opcode:" << QString::number(opcode, 2).rightJustified(7, '0');
        qDebug() << "WB: funct7:" << QString::number(funct7, 2).rightJustified(7, '0');

        // Determine destination register type
        bool write_to_fpr = mem_wb_.is_float;
        bool write_to_gpr = false;

        // Float/Double to integer conversions write to GPR
        if (funct7 == 0b1100000 || funct7 == 0b1100001) // FCVT.W.S/D, FCVT.L.D
        {
            write_to_gpr = true;
            write_to_fpr = false;
            qDebug() << "WB: FCVT float->int detected - writing to GPR";
        }
        // FMV from FPR to GPR
        else if (funct7 == 0b1110000 || funct7 == 0b1110001) // FMV.X.W, FMV.X.D
        {
            write_to_gpr = true;
            write_to_fpr = false;
            qDebug() << "WB: FMV.X.* detected - writing to GPR";
        }

        // Record changes
        if (recording_enabled_)
        {
            RegisterChange reg_change;
            reg_change.reg_type = write_to_fpr ? 2 : 0; // 2=FPR, 0=GPR
            reg_change.reg_index = mem_wb_.rd;
            reg_change.old_value = write_to_fpr ? registers_->ReadFpr(mem_wb_.rd) : registers_->ReadGpr(mem_wb_.rd);
            reg_change.new_value = write_val;
            current_delta_.register_changes.push_back(reg_change);

            qDebug() << "WB: Recording change - Type:" << (write_to_fpr ? "FPR" : "GPR")
                     << "Index:" << mem_wb_.rd
                     << "Old:" << QString::number(reg_change.old_value, 16)
                     << "New:" << QString::number(write_val, 16);
        }

        // Write to correct register file
        if (write_to_fpr)
        {
            qDebug() << "WB: *** WRITING TO FPR ***";

            // For FLW/FLD loads
            if (opcode == 0b0000111)
            {
                uint8_t funct3 = (mem_wb_.instruction >> 12) & 0b111;
                if (funct3 == 0b010) // FLW specifically
                {
                    // Value is already NaN-boxed from MEM stage
                    registers_->WriteFpr(mem_wb_.rd, write_val);
                    emit fprUpdated(mem_wb_.rd, write_val);
                    qDebug() << "WB: FLW to f" << mem_wb_.rd << ":" << QString::number(write_val, 16);
                }
                else // FLD
                {
                    registers_->WriteFpr(mem_wb_.rd, write_val);
                    emit fprUpdated(mem_wb_.rd, write_val);
                    qDebug() << "WB: FLD to f" << mem_wb_.rd << ":" << QString::number(write_val, 16);
                }
            }
            else // Other FP operations
            {
                // Check if it's single-precision that needs NaN-boxing
                bool is_single = !instruction_set::isDInstruction(mem_wb_.instruction);

                if (is_single)
                {
                    uint32_t float_bits = write_val & 0xFFFFFFFF;
                    uint64_t boxed_value = 0xFFFFFFFF00000000ULL | float_bits;
                    registers_->WriteFpr(mem_wb_.rd, boxed_value);
                    emit fprUpdated(mem_wb_.rd, boxed_value);
                    qDebug() << "WB: Single-precision FP to f" << mem_wb_.rd
                             << "- Raw:" << QString::number(float_bits, 16)
                             << "Boxed:" << QString::number(boxed_value, 16);
                }
                else // Double precision
                {
                    registers_->WriteFpr(mem_wb_.rd, write_val);
                    emit fprUpdated(mem_wb_.rd, write_val);
                    qDebug() << "WB: Double-precision FP to f" << mem_wb_.rd
                             << ":" << QString::number(write_val, 16);
                }
            }
        }
        else // Write to GPR
        {
            qDebug() << "WB: *** WRITING TO GPR ***";
            registers_->WriteGpr(mem_wb_.rd, write_val);
            emit gprUpdated(mem_wb_.rd, write_val);
            qDebug() << "WB: Writing to x" << mem_wb_.rd << ":" << QString::number(write_val, 16);
        }
    }
    else if (mem_wb_.rd == 0 && mem_wb_.reg_write)
    {
        qDebug() << "WB: Attempted write to x0 (hardwired zero) - ignored";
    }
    else
    {
        qDebug() << "WB: No register write for this instruction";
    }

    instructions_retired_++;
    qDebug() << "WB: Instructions retired:" << instructions_retired_;
    qDebug() << "=== WB STAGE END ===\n";
}

void RVSSVMPipelined::advance_pipeline_registers()
{

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

    mem_wb_ = mem_wb_next_;
    ex_mem_ = ex_mem_next_;
    id_ex_ = id_ex_next_;
    if_id_ = if_id_next_;

    mem_wb_next_ = MEM_WB();
    ex_mem_next_ = EX_MEM();
    id_ex_next_ = ID_EX();
    if_id_next_ = IF_ID();
    stall_ = false;
    flush_pipeline_ = false;

    // ✅ Emit new stage locations
    if (mem_wb_.valid)
    {
        stage_to_pc_["WB"] = mem_wb_.pc;
        emit pipelineStageChanged(mem_wb_.pc, "WB");
    }
    else
    {
        stage_to_pc_.erase("WB");
    }

    if (ex_mem_.valid)
    {
        stage_to_pc_["MEM"] = ex_mem_.pc;
        emit pipelineStageChanged(ex_mem_.pc, "MEM");
    }
    else
    {
        stage_to_pc_.erase("MEM");
    }

    if (id_ex_.valid)
    {
        stage_to_pc_["EX"] = id_ex_.pc;
        emit pipelineStageChanged(id_ex_.pc, "EX");
    }
    else
    {
        stage_to_pc_.erase("EX");
    }

    if (if_id_.valid)
    {
        stage_to_pc_["ID"] = if_id_.pc;
        emit pipelineStageChanged(if_id_.pc, "ID");
    }
    else
    {
        stage_to_pc_.erase("ID");
    }

    if (program_counter_ < program_size_)
    {
        stage_to_pc_["IF"] = program_counter_;
        emit pipelineStageChanged(program_counter_, "IF");
    }
    else
    {
        stage_to_pc_.erase("IF");
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
}

bool RVSSVMPipelined::IsPipelineEmpty() const
{
    return !(if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
}

void RVSSVMPipelined::DebugRun()
{
    Run();
}

void RVSSVMPipelined::Step()
{
    // Save current pipeline state
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

        if (stage_to_pc_.count("IF"))
        {
            emit pipelineStageChanged(stage_to_pc_["ID"], "ID_CLEAR");
        }
        if (stage_to_pc_.count("ID"))
        {
            emit pipelineStageChanged(stage_to_pc_["ID"], "ID_CLEAR");
        }
        if (stage_to_pc_.count("EX"))
        {
            emit pipelineStageChanged(stage_to_pc_["EX"], "EX_CLEAR");
        }
        if (stage_to_pc_.count("MEM"))
        {
            emit pipelineStageChanged(stage_to_pc_["MEM"], "MEM_CLEAR");
        }
        if (stage_to_pc_.count("WB"))
        {
            emit pipelineStageChanged(stage_to_pc_["WB"], "WB_CLEAR");
        }

        stage_to_pc_.clear();
        return;
    }

    // Enable recording
    recording_enabled_ = true;

    // Execute pipeline stages
    WB_stage();
    MEM_stage();
    EX_stage();
    ID_stage();
    IF_stage();
    advance_pipeline_registers();

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

    // ✅ FIX 4: Limit undo stack size to prevent memory issues
    const size_t MAX_UNDO_STACK_SIZE = 10000;
    if (pipeline_undo_stack_.size() >= MAX_UNDO_STACK_SIZE)
    {
        // Remove oldest entry (bottom of stack)
        std::stack<PipelineStepDelta> temp_stack;
        while (pipeline_undo_stack_.size() > 1)
        {
            temp_stack.push(pipeline_undo_stack_.top());
            pipeline_undo_stack_.pop();
        }
        pipeline_undo_stack_.pop(); // Remove the oldest

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
        if (dynamic_branch_prediction_enabled_)
            branch_history_table_.assign(BHT_SIZE, true);
    }
}
