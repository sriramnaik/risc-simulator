#include "rvss_vm_pipelined.h"
#include "../common/instructions.h"
#include <QDebug>

using instruction_set::Instruction;
using instruction_set::get_instr_encoding;

RVSSVMPipelined::RVSSVMPipelined(RegisterFile *sharedRegisters, QObject *parent)
    : RVSSVM(sharedRegisters, parent)
{
    registers_ = sharedRegisters;
}

RVSSVMPipelined::~RVSSVMPipelined() = default;

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

    program_counter_ = 0;
    if (program_size_ > 0)
        emit pipelineStageChanged(program_counter_, "IF");
    else
        emit pipelineStageChanged(0, "IF_CLEAR");

    branch_history_table_.resize(BHT_SIZE, true);    // default predict taken
    branch_target_buffer_.resize(BHT_SIZE, 0);
    branch_prediction_enabled_ = false;
    dynamic_branch_prediction_enabled_ = false;

}

// IF stage fetch with stall and PC update handling
// void RVSSVMPipelined::IF_stage()
// {
//     if (stall_)
//     {
//         if_id_next_ = if_id_;
//         return;
//     }

//     if (pc_update_pending_)
//     {
//         // On branch taken or jump, update PC and flush IF stage
//         program_counter_ = pc_update_value_;
//         pc_update_pending_ = false;
//         pc_update_value_ = 0;

//         if_id_next_.valid = false; // flush
//         return;
//     }

//     if (program_counter_ >= program_size_)
//     {
//         if_id_next_.valid = false;
//         return;
//     }

//     if_id_next_.pc = program_counter_;
//     if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
//     if_id_next_.valid = true;

//     program_counter_ += 4;
// }

void RVSSVMPipelined::IF_stage()
{
    if (stall_)
    {
        if_id_next_ = if_id_;
        return;
    }

    if (pc_update_pending_)
    {
        program_counter_ = pc_update_value_;
        pc_update_pending_ = false;
        pc_update_value_ = 0;
        if_id_next_.valid = false;  // flush IF
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
    if (stall_)
    {
        id_ex_next_ = ID_EX(); // insert bubble
        return;
    }

    if (!if_id_.valid)
    {
        id_ex_next_ = ID_EX(); // bubble
        return;
    }

    uint32_t instr = if_id_.instruction;
    uint8_t curr_rs1 = (instr >> 15) & 0b11111;
    uint8_t curr_rs2 = (instr >> 20) & 0b11111;

    // Check system call ecall
    uint32_t opcode = instr & 0x7f;
    uint8_t funct3 = (instr >> 12) & 0x7;
    auto ecall_encoding = get_instr_encoding(Instruction::kecall);
    id_ex_next_.is_syscall = (opcode == static_cast<uint32_t>(ecall_encoding.opcode) &&
                              funct3 == static_cast<uint8_t>(ecall_encoding.funct3));

    // **NEW: Detect floating-point instructions**
    bool is_float_instr = instruction_set::isFInstruction(instr);
    bool is_double_instr = instruction_set::isDInstruction(instr);
    id_ex_next_.is_float = is_float_instr || is_double_instr;

    if (hazard_detection_enabled_)
    {
        bool should_stall = false;

        // Load-use hazard detection
        bool load_use = hazard_unit_.DetectLoadUseHazard(id_ex_.rd, id_ex_.mem_read, curr_rs1, curr_rs2);
        if (load_use)
            should_stall = true;

        // Hazard detection if forwarding off
        if (!forwarding_enabled_)
        {
            bool ex_hazard = hazard_unit_.DetectEXHazard(id_ex_.rd, id_ex_.reg_write, curr_rs1, curr_rs2);
            bool mem_hazard = hazard_unit_.DetectMEMHazard(ex_mem_.rd, ex_mem_.reg_write, curr_rs1, curr_rs2);
            if (ex_hazard || mem_hazard)
                should_stall = true;
        }

        if (should_stall)
        {
            stall_ = true;
            stall_cycles_++;
            id_ex_next_ = ID_EX(); // bubble
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
    id_ex_next_.funct7 = (instr >> 25) & 0b1111111;
    id_ex_next_.imm = ImmGenerator(instr);

    // **NEW: Read from FPR for floating-point instructions**
    if (is_float_instr || is_double_instr)
    {
        uint8_t funct7 = id_ex_next_.funct7;
        // For FMV/FCVT from integer or FLW/FLD, read from GPR
        if (funct7 == 0b1101000 || funct7 == 0b1111000 || funct7 == 0b1101001 || funct7 == 0b1111001 ||
            opcode == 0b0000111 || opcode == 0b0100111)
        {
            id_ex_next_.reg1_value = registers_->ReadGpr(id_ex_next_.rs1);
        }
        else
        {
            id_ex_next_.reg1_value = registers_->ReadFpr(id_ex_next_.rs1);
        }
        id_ex_next_.reg2_value = registers_->ReadFpr(id_ex_next_.rs2);

        // rs3 for fused operations
        uint8_t rs3 = (instr >> 27) & 0b11111;
        id_ex_next_.reg3_value = registers_->ReadFpr(rs3);
    }
    else
    {
        id_ex_next_.reg1_value = registers_->ReadGpr(id_ex_next_.rs1);
        id_ex_next_.reg2_value = registers_->ReadGpr(id_ex_next_.rs2);
    }

    control_unit_.SetControlSignals(instr);
    id_ex_next_.reg_write = control_unit_.GetRegWrite();
    id_ex_next_.mem_read = control_unit_.GetMemRead();
    id_ex_next_.mem_write = control_unit_.GetMemWrite();
    id_ex_next_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_next_.alu_src = control_unit_.GetAluSrc();
    id_ex_next_.branch = control_unit_.GetBranch();
}

// Modified EX_stage() to handle floating-point:
void RVSSVMPipelined::EX_stage()
{
    if (!id_ex_.valid)
    {
        ex_mem_next_.valid = false;
        return;
    }

    ex_mem_next_.valid = true;
    ex_mem_next_.pc = id_ex_.pc;
    ex_mem_next_.instruction = id_ex_.instruction;
    ex_mem_next_.rd = id_ex_.rd;
    ex_mem_next_.reg_write = id_ex_.reg_write;
    ex_mem_next_.mem_read = id_ex_.mem_read;
    ex_mem_next_.mem_write = id_ex_.mem_write;
    ex_mem_next_.mem_to_reg = id_ex_.mem_to_reg;
    ex_mem_next_.is_float = id_ex_.is_float;

    // **NEW: Handle floating-point instructions**
    if (id_ex_.is_float)
    {
        uint8_t opcode = id_ex_.instruction & 0x7F;
        uint8_t funct3 = id_ex_.funct3;
        uint8_t funct7 = id_ex_.funct7;
        uint8_t rm = funct3;

        if (rm == 0b111)
            rm = registers_->ReadCsr(0x002);

        uint64_t op1 = id_ex_.reg1_value;
        uint64_t op2 = id_ex_.reg2_value;
        uint64_t op3 = id_ex_.reg3_value;
        uint64_t store_data = id_ex_.reg2_value;

        // Apply forwarding for floating-point operands
        if (forwarding_enabled_)
        {
            auto rs1_src = forwarding_unit_.GetRs1Source(
                ex_mem_.reg_write, ex_mem_.rd,
                mem_wb_.reg_write, mem_wb_.rd,
                id_ex_.rs1);

            if (rs1_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM)
                op1 = ex_mem_.alu_result;
            else if (rs1_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB)
                op1 = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;

            auto rs2_src = forwarding_unit_.GetRs2Source(
                ex_mem_.reg_write, ex_mem_.rd,
                mem_wb_.reg_write, mem_wb_.rd,
                id_ex_.rs2);

            if (rs2_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM)
            {
                op2 = ex_mem_.alu_result;
                store_data = ex_mem_.alu_result;
            }
            else if (rs2_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB)
            {
                uint64_t fwd = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
                op2 = fwd;
                store_data = fwd;
            }
        }

        if (id_ex_.alu_src)
            op2 = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm));

        alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());

        uint8_t fcsr_status = 0;
        bool is_double = instruction_set::isDInstruction(id_ex_.instruction);

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

        // Update FCSR
        registers_->WriteCsr(0x003, fcsr_status);

        ex_mem_next_.reg2_value = store_data;
        return;
    }

    // Regular integer execution path continues here...
    uint64_t op1 = id_ex_.reg1_value;
    uint64_t op2 = id_ex_.reg2_value;
    uint64_t store_data = id_ex_.reg2_value;

    if (forwarding_enabled_)
    {
        auto rs1_src = forwarding_unit_.GetRs1Source(
            ex_mem_.reg_write, ex_mem_.rd,
            mem_wb_.reg_write, mem_wb_.rd,
            id_ex_.rs1);

        if (rs1_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM)
            op1 = ex_mem_.alu_result;
        else if (rs1_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB)
            op1 = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;

        auto rs2_src = forwarding_unit_.GetRs2Source(
            ex_mem_.reg_write, ex_mem_.rd,
            mem_wb_.reg_write, mem_wb_.rd,
            id_ex_.rs2);

        if (rs2_src == ForwardingUnit::ForwardingSource::FROM_EX_MEM){
            op2 = ex_mem_.alu_result;
            store_data = ex_mem_.alu_result;}
        else if (rs2_src == ForwardingUnit::ForwardingSource::FROM_MEM_WB){
            uint64_t fwd = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
            op2 = fwd;
            store_data = fwd;
        }
    }

    uint8_t opcode = id_ex_.instruction & 0x7F;
    if (opcode == 0b0110111) { // LUI
        op2 = static_cast<uint64_t>(id_ex_.imm) << 12;
        op1 = 0;
    }
    else if (opcode == 0b0010111) { // AUIPC
        op2 = static_cast<uint64_t>(id_ex_.imm) << 12;
        op1 = static_cast<uint64_t>(id_ex_.pc);
    }
    else if (id_ex_.alu_src) {
        op2 = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm));
    }

    alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
    bool overflow = false;
    std::tie(ex_mem_next_.alu_result, overflow) = alu_.execute(aluOperation, op1, op2);

    ex_mem_next_.reg2_value = store_data;
    ex_mem_next_.branch_taken = false;

    // Branch handling code continues...
    if (id_ex_.branch)
    {
        bool take = false;
        uint8_t funct3 = id_ex_.funct3;
        switch (funct3)
        {
        case 0b000: take = (ex_mem_next_.alu_result == 0); break; // BEQ
        case 0b001: take = (ex_mem_next_.alu_result != 0); break; // BNE
        case 0b100: take = (ex_mem_next_.alu_result == 1); break; // BLT
        case 0b101: take = (ex_mem_next_.alu_result == 0); break; // BGE
        case 0b110: take = (ex_mem_next_.alu_result == 1); break; // BLTU
        case 0b111: take = (ex_mem_next_.alu_result == 0); break; // BGEU
        }

        if (take)
        {
            ex_mem_next_.branch_taken = true;
            ex_mem_next_.branch_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc)) + id_ex_.imm;
            pc_update_pending_ = true;
            pc_update_value_ = ex_mem_next_.branch_target;
            if_id_next_.valid = false;
            id_ex_next_.valid = false;
        }

        if (branch_prediction_enabled_)
        {
            size_t index = (id_ex_.pc >> 2) % BHT_SIZE;

            if (dynamic_branch_prediction_enabled_)
            {
                bool actual_taken = take;
                bool predicted_taken = branch_history_table_[index];
                branch_history_table_[index] = actual_taken;

                if (actual_taken)
                    branch_target_buffer_[index] = ex_mem_next_.branch_target;
                else
                    branch_target_buffer_[index] = 0;

                if (predicted_taken != actual_taken)
                {
                    pc_update_pending_ = true;
                    pc_update_value_ = actual_taken ? ex_mem_next_.branch_target : id_ex_.pc + 4;
                    if_id_next_.valid = false;
                    id_ex_next_.valid = false;
                }
            }
            else
            {
                if (take)
                    branch_target_buffer_[index] = ex_mem_next_.branch_target;
                else
                    branch_target_buffer_[index] = 0;

                bool predicted_taken = (branch_target_buffer_[index] != 0);

                if (predicted_taken != take)
                {
                    pc_update_pending_ = true;
                    pc_update_value_ = take ? ex_mem_next_.branch_target : id_ex_.pc + 4;
                    if_id_next_.valid = false;
                    id_ex_next_.valid = false;
                }
            }
        }
        else
        {
            if (take)
            {
                pc_update_pending_ = true;
                pc_update_value_ = ex_mem_next_.branch_target;
                if_id_next_.valid = false;
                id_ex_next_.valid = false;
            }
        }
    }
}

// Modified MEM_stage() to handle floating-point loads/stores:
void RVSSVMPipelined::MEM_stage()
{
    if (!ex_mem_.valid)
    {
        mem_wb_next_.valid = false;
        return;
    }

    mem_wb_next_.valid = true;
    mem_wb_next_.rd = ex_mem_.rd;
    mem_wb_next_.reg_write = ex_mem_.reg_write;
    mem_wb_next_.mem_to_reg = ex_mem_.mem_to_reg;
    mem_wb_next_.alu_result = ex_mem_.alu_result;
    mem_wb_next_.pc = ex_mem_.pc;
    mem_wb_next_.is_float = ex_mem_.is_float;

    // **NEW: Handle floating-point loads**
    if (ex_mem_.mem_read)
    {
        uint8_t opcode = ex_mem_.instruction & 0x7F;

        if (opcode == 0b0000111) // Floating-point load (FLW/FLD)
        {
            uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
            if (funct3 == 0b010) // FLW
            {
                mem_wb_next_.mem_data = memory_controller_.ReadWord(ex_mem_.alu_result);
            }
            else if (funct3 == 0b011) // FLD
            {
                if (registers_->GetIsa() == ISA::RV64)
                    mem_wb_next_.mem_data = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
            }
        }
        else // Integer loads
        {
            uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
            switch (funct3)
            {
            case 0b000: mem_wb_next_.mem_data = static_cast<int8_t>(memory_controller_.ReadByte(ex_mem_.alu_result)); break;
            case 0b001: mem_wb_next_.mem_data = static_cast<int16_t>(memory_controller_.ReadHalfWord(ex_mem_.alu_result)); break;
            case 0b010: mem_wb_next_.mem_data = static_cast<int32_t>(memory_controller_.ReadWord(ex_mem_.alu_result)); break;
            case 0b011: mem_wb_next_.mem_data = memory_controller_.ReadDoubleWord(ex_mem_.alu_result); break;
            case 0b100: mem_wb_next_.mem_data = memory_controller_.ReadByte(ex_mem_.alu_result); break;
            case 0b101: mem_wb_next_.mem_data = memory_controller_.ReadHalfWord(ex_mem_.alu_result); break;
            case 0b110: mem_wb_next_.mem_data = memory_controller_.ReadWord(ex_mem_.alu_result); break;
            default: mem_wb_next_.mem_data = 0; break;
            }
        }
    }

    // **NEW: Handle floating-point stores**
    // if (ex_mem_.mem_write)
    // {
    //     uint8_t opcode = ex_mem_.instruction & 0x7F;

    //     if (opcode == 0b0100111) // Floating-point store (FSW/FSD)
    //     {
    //         uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
    //         if (funct3 == 0b010) // FSW
    //         {
    //             memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF);
    //         }
    //         else if (funct3 == 0b011) // FSD
    //         {
    //             if (registers_->GetIsa() == ISA::RV64)
    //                 memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value);
    //         }
    //     }
    //     else // Integer stores
    //     {
    //         uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
    //         switch (funct3)
    //         {
    //         case 0b000: memory_controller_.WriteByte(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFF); break;
    //         case 0b001: memory_controller_.WriteHalfWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFF); break;
    //         case 0b010: memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF); break;
    //         case 0b011: memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value); break;
    //         default: break;
    //         }
    //     }
    // }

    if (ex_mem_.mem_write)
    {
        uint8_t opcode = ex_mem_.instruction & 0x7F;

        if (opcode == 0b0100111) // Floating-point store
        {
            uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;

            if (recording_enabled_) {
                MemoryChange mem_change;
                mem_change.address = ex_mem_.alu_result;

                if (funct3 == 0b010) { // FSW
                    uint32_t old_val = memory_controller_.ReadWord(ex_mem_.alu_result);
                    for (int i = 0; i < 4; ++i)
                        mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                    uint32_t new_val = ex_mem_.reg2_value & 0xFFFFFFFF;
                    for (int i = 0; i < 4; ++i)
                        mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                }
                else if (funct3 == 0b011) { // FSD
                    uint64_t old_val = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
                    for (int i = 0; i < 8; ++i)
                        mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                    uint64_t new_val = ex_mem_.reg2_value;
                    for (int i = 0; i < 8; ++i)
                        mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                }
                current_delta_.memory_changes.push_back(mem_change);
            }

            // Now do the actual write
            if (funct3 == 0b010)
                memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF);
            else if (funct3 == 0b011 && registers_->GetIsa() == ISA::RV64)
                memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value);
        }
        else // Integer stores - add similar recording
        {
            uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;

            if (recording_enabled_) {
                MemoryChange mem_change;
                mem_change.address = ex_mem_.alu_result;

                switch (funct3) {
                case 0b000: // SB
                    mem_change.old_bytes_vec.push_back(memory_controller_.ReadByte(ex_mem_.alu_result));
                    mem_change.new_bytes_vec.push_back(ex_mem_.reg2_value & 0xFF);
                    break;
                case 0b001: // SH
                {
                    uint16_t old_val = memory_controller_.ReadHalfWord(ex_mem_.alu_result);
                    mem_change.old_bytes_vec.push_back(old_val & 0xFF);
                    mem_change.old_bytes_vec.push_back((old_val >> 8) & 0xFF);
                    uint16_t new_val = ex_mem_.reg2_value & 0xFFFF;
                    mem_change.new_bytes_vec.push_back(new_val & 0xFF);
                    mem_change.new_bytes_vec.push_back((new_val >> 8) & 0xFF);
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
                    break;
                }
                case 0b011: // SD
                    if (registers_->GetIsa() == ISA::RV64) {
                        uint64_t old_val = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
                        for (int i = 0; i < 8; ++i)
                            mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                        uint64_t new_val = ex_mem_.reg2_value;
                        for (int i = 0; i < 8; ++i)
                            mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                    }
                    break;
                }
                current_delta_.memory_changes.push_back(mem_change);
            }

            // Actual writes
            switch (funct3) {
            case 0b000: memory_controller_.WriteByte(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFF); break;
            case 0b001: memory_controller_.WriteHalfWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFF); break;
            case 0b010: memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF); break;
            case 0b011: memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value); break;
            }
        }
    }
}

// Modified WB_stage() to handle floating-point writeback:
// void RVSSVMPipelined::WB_stage()
// {
//     if (!mem_wb_.valid)
//         return;

//     uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;

//     if (mem_wb_.reg_write && mem_wb_.rd != 0)
//     {
//         // **NEW: Write to FPR for floating-point instructions**
//         if (mem_wb_.is_float)
//         {
//             uint64_t value = write_val;
//             if (registers_->GetIsa() == ISA::RV32)
//                 value &= 0xFFFFFFFF;
//             registers_->WriteFpr(mem_wb_.rd, value);
//             emit fprUpdated(mem_wb_.rd, value);
//         }
//         else
//         {
//             registers_->WriteGpr(mem_wb_.rd, write_val);
//             emit gprUpdated(mem_wb_.rd, write_val);
//         }
//     }

//     instructions_retired_++;
// }

void RVSSVMPipelined::WB_stage()
{
    if (!mem_wb_.valid)
        return;

    uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;

    if (mem_wb_.reg_write && mem_wb_.rd != 0)
    {
        if (recording_enabled_) {
            RegisterChange change;
            change.reg_index = mem_wb_.rd;

            if (mem_wb_.is_float) {
                change.reg_type = 2; // FPR
                change.old_value = registers_->ReadFpr(mem_wb_.rd);
                change.new_value = (registers_->GetIsa() == ISA::RV32) ? (write_val & 0xFFFFFFFF) : write_val;
            } else {
                change.reg_type = 0; // GPR
                change.old_value = registers_->ReadGpr(mem_wb_.rd);
                change.new_value = write_val;
            }
            current_delta_.register_changes.push_back(change);
        }

        if (mem_wb_.is_float)
        {
            uint64_t value = write_val;
            if (registers_->GetIsa() == ISA::RV32)
                value &= 0xFFFFFFFF;
            registers_->WriteFpr(mem_wb_.rd, value);
            emit fprUpdated(mem_wb_.rd, value);
        }
        else
        {
            registers_->WriteGpr(mem_wb_.rd, write_val);
            emit gprUpdated(mem_wb_.rd, write_val);
        }
    }

    instructions_retired_++;
}

// Advance pipeline registers with event emission
void RVSSVMPipelined::advance_pipeline_registers()
{
    mem_wb_ = mem_wb_next_;
    ex_mem_ = ex_mem_next_;
    id_ex_ = id_ex_next_;
    if_id_ = if_id_next_;

    mem_wb_next_ = MEM_WB();
    ex_mem_next_ = EX_MEM();
    id_ex_next_ = ID_EX();
    if_id_next_ = IF_ID();
    stall_ = false;

    if (mem_wb_.valid)
        emit pipelineStageChanged(mem_wb_.pc, "WB");
    else
        emit pipelineStageChanged(0, "WB_CLEAR");

    if (ex_mem_.valid)
        emit pipelineStageChanged(ex_mem_.pc, "MEM");
    else
        emit pipelineStageChanged(0, "MEM_CLEAR");

    if (id_ex_.valid)
        emit pipelineStageChanged(id_ex_.pc, "EX");
    else
        emit pipelineStageChanged(0, "EX_CLEAR");

    if (if_id_.valid)
        emit pipelineStageChanged(if_id_.pc, "ID");
    else
        emit pipelineStageChanged(0, "ID_CLEAR");

    if (program_counter_ < program_size_)
        emit pipelineStageChanged(program_counter_, "IF");
    else
        emit pipelineStageChanged(0, "IF_CLEAR");
}

// Run loop to cycle pipeline until completion or stop requested
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

// void RVSSVMPipelined::Step()
// {
//     // IF_stage();
//     // ID_stage();
//     // EX_stage();
//     // MEM_stage();
//     // WB_stage();
//     // advance_pipeline_registers();
//     // cycle_s_++;
//     bool pipeline_has_work = (if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
//     bool fetch_remaining = (program_counter_ < program_size_);

//     // If no more work and no more instructions left to fetch, stop
//     // std::cout << "[CYCLE] Pipeline has work: " << pipeline_has_work
//     //           << " (IF:" << if_id_.valid << " ID:" << id_ex_.valid
//     //           << " EX:" << ex_mem_.valid << " MEM:" << mem_wb_.valid << ")" << std::endl;
//     // std::cout << "[CYCLE] Fetch remaining: " << fetch_remaining
//     //           << " (PC=" << program_counter_ << ", size=" << program_size_ << ")" << std::endl;

//     if (!pipeline_has_work && !fetch_remaining)
//     {
//         output_status_ = "VM_PROGRAM_END";
//         emit pipelineStageChanged(0, "IF_CLEAR");
//         emit pipelineStageChanged(0, "ID_CLEAR");
//         emit pipelineStageChanged(0, "EX_CLEAR");
//         emit pipelineStageChanged(0, "MEM_CLEAR");
//         emit pipelineStageChanged(0, "WB_CLEAR");
//         return;
//     }

//     // Normal pipeline cycle
//     WB_stage();
//     MEM_stage();
//     EX_stage();
//     ID_stage();
//     IF_stage();
//     advance_pipeline_registers();

//     cycle_s_++;
//     // DumpPipelineState();
// }

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
        emit pipelineStageChanged(0, "IF_CLEAR");
        emit pipelineStageChanged(0, "ID_CLEAR");
        emit pipelineStageChanged(0, "EX_CLEAR");
        emit pipelineStageChanged(0, "MEM_CLEAR");
        emit pipelineStageChanged(0, "WB_CLEAR");
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

    // Push to undo stack
    pipeline_undo_stack_.push(delta);

    // Clear current delta for next step
    current_delta_ = StepDelta();
}

void RVSSVMPipelined::Undo()
{
    if (pipeline_undo_stack_.empty()) {
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

    // Emit pipeline stage updates to UI
    if (if_id_.valid)
        emit pipelineStageChanged(if_id_.pc, "ID");
    else
        emit pipelineStageChanged(0, "ID_CLEAR");

    if (id_ex_.valid)
        emit pipelineStageChanged(id_ex_.pc, "EX");
    else
        emit pipelineStageChanged(0, "EX_CLEAR");

    if (ex_mem_.valid)
        emit pipelineStageChanged(ex_mem_.pc, "MEM");
    else
        emit pipelineStageChanged(0, "MEM_CLEAR");

    if (mem_wb_.valid)
        emit pipelineStageChanged(mem_wb_.pc, "WB");
    else
        emit pipelineStageChanged(0, "WB_CLEAR");

    if (program_counter_ < program_size_)
        emit pipelineStageChanged(program_counter_, "IF");
    else
        emit pipelineStageChanged(0, "IF_CLEAR");

    qDebug() << "Undo completed - PC restored to" << program_counter_;
}

void RVSSVMPipelined::SetPipelineConfig(bool hazardEnabled,
                                        bool forwardingEnabled,
                                        bool branchPredictionEnabled,
                                        bool dynamicPredictionEnabled = false)
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


