#include "rvss_vm.h"
#include "../../utils.h"
#include "../../globals.h"
#include "../../common/instructions.h"
// #include "../../config.h"

#include <cctype>
#include <cstdint>
#include <qdebug.h>
#include <tuple>
#include <stack>
// #include <string>
#include <atomic>
#include <QString>

// Proper Qt constructor/destructor
RVSSVM::RVSSVM(RegisterFile *sharedRegisters, QObject *parent)
    : QObject(parent), VmBase()
{
    registers_ = sharedRegisters; // Use the shared instance
}
RVSSVM::~RVSSVM() = default;

void RVSSVM::Fetch()
{
    current_instruction_ = memory_controller_.ReadWord(program_counter_);
    UpdateProgramCounter(4);
}

void RVSSVM::Decode()
{
    control_unit_.SetControlSignals(current_instruction_);
}

void RVSSVM::Execute()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

    if (opcode == 0b1110011 && funct3 == 0b000) {
        HandleSyscall();
        return;
    }

    if (instruction_set::isFInstruction(current_instruction_)) {
        ExecuteFloat();
        return;
    } else if (instruction_set::isDInstruction(current_instruction_)) {
        if (registers_->GetIsa() == ISA::RV64)
            ExecuteDouble();
        else
            emit vmError("Double-precision not supported in RV32");
        return;
    } else if (opcode == 0b1110011) {
        ExecuteCsr();
        return;
    }

    uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    int32_t imm = ImmGenerator(current_instruction_);

    uint64_t reg1_value = registers_->ReadGpr(rs1);
    uint64_t reg2_value = registers_->ReadGpr(rs2);

    bool overflow = false;
    if (control_unit_.GetAluSrc()) {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
    }

    alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
    std::tie(execution_result_, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);

    // Branch instructions
    if (opcode == 0b1100111 || opcode == 0b1101111) { // JALR/JAL
        next_pc_ = static_cast<int64_t>(program_counter_);
        UpdateProgramCounter(-4);
        return_address_ = program_counter_ + 4;
        if (opcode == 0b1100111) {
            UpdateProgramCounter(-program_counter_ + execution_result_);
        } else {
            UpdateProgramCounter(imm);
        }
    }
    else if (opcode == 0b1100011) {
        bool takeBranch = false;
        switch (funct3) {
        case 0b000: takeBranch = (execution_result_ == 0); break; // BEQ
        case 0b001: takeBranch = (execution_result_ != 0); break; // BNE
        case 0b100: takeBranch = (execution_result_ < 0); break; // BLT
        case 0b101: takeBranch = (execution_result_ >= 0); break; // BGE
        case 0b110: takeBranch = (static_cast<uint64_t>(reg1_value) < static_cast<uint64_t>(reg2_value)); break; // BLTU
        case 0b111: takeBranch = (static_cast<uint64_t>(reg1_value) >= static_cast<uint64_t>(reg2_value)); break; // BGEU
        }
        if (takeBranch) {
            UpdateProgramCounter(-4);
            UpdateProgramCounter(imm);
        }
    }

    if (opcode == 0b0010111) { // AUIPC
        execution_result_ = static_cast<int64_t>(program_counter_) - 4 + (imm << 12);
    }
}

void RVSSVM::ExecuteFloat()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rm = funct3;
    uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    uint8_t rs3 = (current_instruction_ >> 27) & 0b11111;

    uint8_t fcsr_status = 0;
    int32_t imm = ImmGenerator(current_instruction_);

    if (rm == 0b111)
        rm = registers_->ReadCsr(0x002);

    uint64_t reg1_value = registers_->ReadFpr(rs1);
    uint64_t reg2_value = registers_->ReadFpr(rs2);
    uint64_t reg3_value = registers_->ReadFpr(rs3);

    if (funct7 == 0b1101000 || funct7 == 0b1111000 || opcode == 0b0000111 || opcode == 0b0100111)
        reg1_value = registers_->ReadGpr(rs1);

    if (control_unit_.GetAluSrc())
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));

    alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
    std::tie(execution_result_, fcsr_status) = alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    registers_->WriteCsr(0x003, fcsr_status);
    emit csrUpdated(0x003, fcsr_status);
}

void RVSSVM::ExecuteDouble()
{
    if (registers_->GetIsa() != ISA::RV64) {
        emit vmError("Double-precision not supported in RV32");
        return;
    }
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rm = funct3;
    uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    uint8_t rs3 = (current_instruction_ >> 27) & 0b11111;

    uint8_t fcsr_status = 0;
    int32_t imm = ImmGenerator(current_instruction_);

    uint64_t reg1_value = registers_->ReadFpr(rs1);
    uint64_t reg2_value = registers_->ReadFpr(rs2);
    uint64_t reg3_value = registers_->ReadFpr(rs3);

    if (funct7 == 0b1101001 || funct7 == 0b1111001 || opcode == 0b0000111 || opcode == 0b0100111)
        reg1_value = registers_->ReadGpr(rs1);

    if (control_unit_.GetAluSrc())
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));

    alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
    std::tie(execution_result_, fcsr_status) = alu::Alu::dfpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);
}

void RVSSVM::ExecuteCsr()
{
    uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
    uint16_t csr = (current_instruction_ >> 20) & 0xFFF;
    uint64_t csr_val = registers_->ReadCsr(csr);

    csr_target_address_ = csr;
    csr_old_value_ = csr_val;
    csr_write_val_ = registers_->ReadGpr(rs1);
    csr_uimm_ = rs1;
}

void RVSSVM::HandleSyscall()
{
    uint64_t syscall_number = registers_->ReadGpr(17);
    switch (syscall_number)
    {
    case SYSCALL_PRINT_INT:
        emit syscallOutput(QString("[Syscall output: %1]").arg((qint64)registers_->ReadGpr(10)));
        break;
    case SYSCALL_PRINT_FLOAT:
    {
        float float_value;
        uint64_t raw = registers_->ReadGpr(10);
        std::memcpy(&float_value, &raw, sizeof(float_value));
        emit syscallOutput(QString("[Syscall output: %1]").arg(float_value));
        break;
    }
    case SYSCALL_PRINT_DOUBLE:
    {
        double double_value;
        uint64_t raw = registers_->ReadGpr(10);
        std::memcpy(&double_value, &raw, sizeof(double_value));
        emit syscallOutput(QString("[Syscall output: %1]").arg(double_value));
        break;
    }
    case SYSCALL_PRINT_STRING:
        emit syscallOutput("[Syscall output: ...string... (not implemented yet)]");
        break;
    case SYSCALL_EXIT:
        stop_requested_ = true;
        emit vmError(QString("VM exited with code: %1").arg(registers_->ReadGpr(10)));
        break;
    default:
        emit vmError(QString("Unknown syscall: %1").arg(syscall_number));
        break;
    }
}

void RVSSVM::WriteMemory()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

    if (opcode == 0b1110011 && funct3 == 0b000)
        return;

    if (instruction_set::isFInstruction(current_instruction_)) {
        WriteMemoryFloat();
        return;
    }
    else if (instruction_set::isDInstruction(current_instruction_)) {
        if (registers_->GetIsa() == ISA::RV64)
            WriteMemoryDouble();
        else
            emit vmError("Double-precision stores not supported in RV32");
        return;
    }

    if (control_unit_.GetMemRead()) {
        switch (funct3) {
        case 0b000: memory_result_ = static_cast<int8_t>(memory_controller_.ReadByte(execution_result_)); break;
        case 0b001: memory_result_ = static_cast<int16_t>(memory_controller_.ReadHalfWord(execution_result_)); break;
        case 0b010: memory_result_ = static_cast<int32_t>(memory_controller_.ReadWord(execution_result_)); break;
        case 0b011:
            if (registers_->GetIsa() == ISA::RV64)
                memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
            break;
        case 0b100: memory_result_ = static_cast<uint8_t>(memory_controller_.ReadByte(execution_result_)); break;
        case 0b101: memory_result_ = static_cast<uint16_t>(memory_controller_.ReadHalfWord(execution_result_)); break;
        case 0b110: memory_result_ = static_cast<uint32_t>(memory_controller_.ReadWord(execution_result_)); break;
        }
    }



    if (control_unit_.GetMemWrite()) {
        if (recording_enabled_) {
            MemoryChange mem_change;
            mem_change.address = execution_result_;

            // Read old bytes before overwriting
            switch (funct3) {
            case 0b000: // SB
                mem_change.old_bytes_vec.push_back(memory_controller_.ReadByte(execution_result_));
                mem_change.new_bytes_vec.push_back(registers_->ReadGpr(rs2) & 0xFF);
                break;
            case 0b001: // SH
            {
                uint16_t old_val = memory_controller_.ReadHalfWord(execution_result_);
                mem_change.old_bytes_vec.push_back(old_val & 0xFF);
                mem_change.old_bytes_vec.push_back((old_val >> 8) & 0xFF);
                uint16_t new_val = registers_->ReadGpr(rs2) & 0xFFFF;
                mem_change.new_bytes_vec.push_back(new_val & 0xFF);
                mem_change.new_bytes_vec.push_back((new_val >> 8) & 0xFF);
                break;
            }
            case 0b010: // SW
            {
                uint32_t old_val = memory_controller_.ReadWord(execution_result_);
                for (int i = 0; i < 4; ++i)
                    mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                uint32_t new_val = registers_->ReadGpr(rs2) & 0xFFFFFFFF;
                for (int i = 0; i < 4; ++i)
                    mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                break;
            }
            case 0b011: // SD
                if (registers_->GetIsa() == ISA::RV64) {
                    uint64_t old_val = memory_controller_.ReadDoubleWord(execution_result_);
                    for (int i = 0; i < 8; ++i)
                        mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                    uint64_t new_val = registers_->ReadGpr(rs2);
                    for (int i = 0; i < 8; ++i)
                        mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                }
                break;
            }
            current_delta_.memory_changes.push_back(mem_change);
        }
        switch (funct3) {
        case 0b000: memory_controller_.WriteByte(execution_result_, registers_->ReadGpr(rs2) & 0xFF); break;
        case 0b001: memory_controller_.WriteHalfWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFF); break;
        case 0b010: memory_controller_.WriteWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFFFFFF); break;
        case 0b011:
            if (registers_->GetIsa() == ISA::RV64)
                memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadGpr(rs2));
            break;
        }

        // Optionally update undo/redo stacks (not abbreviated for brevity).
    }
}

// void RVSSVM::WriteMemoryFloat()
// {
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     if (control_unit_.GetMemRead())
//         memory_result_ = memory_controller_.ReadWord(execution_result_);

//     if (control_unit_.GetMemWrite()) {
//         uint32_t val = registers_->ReadFpr(rs2) & 0xFFFFFFFF;
//         memory_controller_.WriteWord(execution_result_, val);
//     }
// }
void RVSSVM::WriteMemoryFloat()
{
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    if (control_unit_.GetMemRead())
        memory_result_ = memory_controller_.ReadWord(execution_result_);

    if (control_unit_.GetMemWrite()) {
        if (recording_enabled_) {
            MemoryChange mem_change;
            mem_change.address = execution_result_;
            uint32_t old_val = memory_controller_.ReadWord(execution_result_);
            for (int i = 0; i < 4; ++i)
                mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
            uint32_t new_val = registers_->ReadFpr(rs2) & 0xFFFFFFFF;
            for (int i = 0; i < 4; ++i)
                mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
            current_delta_.memory_changes.push_back(mem_change);
        }

        uint32_t val = registers_->ReadFpr(rs2) & 0xFFFFFFFF;
        memory_controller_.WriteWord(execution_result_, val);
    }
}

// void RVSSVM::WriteMemoryDouble()
// {
//     if (registers_->GetIsa() != ISA::RV64) {
//         emit vmError("Double-precision store/read not supported in RV32");
//         return;
//     }
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     if (control_unit_.GetMemRead())
//         memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);

//     if (control_unit_.GetMemWrite())
//         memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
// }

void RVSSVM::WriteMemoryDouble()
{
    if (registers_->GetIsa() != ISA::RV64) {
        emit vmError("Double-precision store/read not supported in RV32");
        return;
    }
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

    if (control_unit_.GetMemRead())
        memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);

    if (control_unit_.GetMemWrite()) {
        if (recording_enabled_) {
            MemoryChange mem_change;
            mem_change.address = execution_result_;
            uint64_t old_val = memory_controller_.ReadDoubleWord(execution_result_);
            for (int i = 0; i < 8; ++i)
                mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
            uint64_t new_val = registers_->ReadFpr(rs2);
            for (int i = 0; i < 8; ++i)
                mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
            current_delta_.memory_changes.push_back(mem_change);
        }

        memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
    }
}

void RVSSVM::WriteBack()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    int32_t imm = ImmGenerator(current_instruction_);

    if (opcode == 0b1110011 && funct3 == 0b000)
        return;
    if (instruction_set::isFInstruction(current_instruction_)) {
        WriteBackFloat();
        return;
    }
    else if (instruction_set::isDInstruction(current_instruction_)) {
        if (registers_->GetIsa() == ISA::RV64)
            WriteBackDouble();
        else
            emit vmError("Double-precision writeback not supported in RV32");
        return;
    }
    else if (opcode == 0b1110011) {
        WriteBackCsr();
        return;
    }

    if (control_unit_.GetRegWrite()) {
        uint64_t new_value = 0;
        switch (opcode) {
        case 0b0110011: // R-type
        case 0b0010011: // I-type
        case 0b0010111: // AUIPC
            new_value = execution_result_;
            break;
        case 0b0000011: // Loads
            new_value = memory_result_;
            break;
        case 0b1100111: // JALR
        case 0b1101111: // JAL
            new_value = next_pc_;
            break;
        case 0b0110111: // LUI
            new_value = (imm << 12);
            break;
        default:
            break;
        }

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 0; // GPR
            change.reg_index = rd;
            change.old_value = registers_->ReadGpr(rd);
            change.new_value = new_value;
            current_delta_.register_changes.push_back(change);
        }

        registers_->WriteGpr(rd, new_value);
        emit gprUpdated(rd, new_value);
    }
}

void RVSSVM::WriteBackFloat()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    if (control_unit_.GetRegWrite()) {
        uint64_t value = execution_result_;
        if (registers_->GetIsa() == ISA::RV32)
            value &= 0xFFFFFFFF;

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 2; // FPR
            change.reg_index = rd;
            change.old_value = registers_->ReadFpr(rd);
            change.new_value = value;
            current_delta_.register_changes.push_back(change);
        }

        registers_->WriteFpr(rd, value);
        emit fprUpdated(rd, value);
    }
}

void RVSSVM::WriteBackDouble()
{
    if (registers_->GetIsa() != ISA::RV64) {
        emit vmError("Double-precision FPR write not allowed in RV32");
        return;
    }
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    if (control_unit_.GetRegWrite()) {

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 2; // FPR
            change.reg_index = rd;
            change.old_value = registers_->ReadFpr(rd);
            change.new_value = execution_result_;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteFpr(rd, execution_result_);
        emit fprUpdated(rd, execution_result_);
    }
}

// void RVSSVM::WriteBackCsr()
// {
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//     uint16_t csr_addr = csr_target_address_;
//     switch (funct3)
//     {
//     case 0b001: // CSRRW
//         registers_->WriteGpr(rd, csr_old_value_);
//         registers_->WriteCsr(csr_addr, csr_write_val_);
//         emit csrUpdated(csr_addr, csr_write_val_);
//         break;
//     case 0b010: // CSRRS
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_write_val_ != 0)
//         {
//             registers_->WriteCsr(csr_addr, csr_old_value_ | csr_write_val_);
//             emit csrUpdated(csr_addr, csr_old_value_ | csr_write_val_);
//         }
//         break;
//     case 0b011: // CSRRC
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_write_val_ != 0)
//         {
//             registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_write_val_);
//             emit csrUpdated(csr_addr, csr_old_value_ & ~csr_write_val_);
//         }
//         break;
//     case 0b101: // CSRRWI
//         registers_->WriteGpr(rd, csr_old_value_);
//         registers_->WriteCsr(csr_addr, csr_uimm_);
//         emit csrUpdated(csr_addr, csr_uimm_);
//         break;
//     case 0b110: // CSRRSI
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_uimm_ != 0)
//         {
//             registers_->WriteCsr(csr_addr, csr_old_value_ | csr_uimm_);
//             emit csrUpdated(csr_addr, csr_old_value_ | csr_uimm_);
//         }
//         break;
//     case 0b111: // CSRRCI
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_uimm_ != 0)
//         {
//             registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_uimm_);
//             emit csrUpdated(csr_addr, csr_old_value_ & ~csr_uimm_);
//         }
//         break;
//     }
// }

void RVSSVM::WriteBackCsr()
{
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint16_t csr_addr = csr_target_address_;

    switch (funct3)
    {
    case 0b001: // CSRRW
        registers_->WriteGpr(rd, csr_old_value_);
        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 1; // CSR
            change.reg_index = csr_addr;
            change.old_value = csr_old_value_;
            change.new_value = csr_write_val_;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteCsr(csr_addr, csr_write_val_);
        emit csrUpdated(csr_addr, csr_write_val_);
        break;

    case 0b010: // CSRRS
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_write_val_ != 0) {
            if (recording_enabled_) {
                RegisterChange change;
                change.reg_type = 1;
                change.reg_index = csr_addr;
                change.old_value = csr_old_value_;
                change.new_value = csr_old_value_ | csr_write_val_;
                current_delta_.register_changes.push_back(change);
            }
            registers_->WriteCsr(csr_addr, csr_old_value_ | csr_write_val_);
            emit csrUpdated(csr_addr, csr_old_value_ | csr_write_val_);
        }
        break;

    case 0b011: // CSRRC
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_write_val_ != 0) {
            if (recording_enabled_) {
                RegisterChange change;
                change.reg_type = 1;
                change.reg_index = csr_addr;
                change.old_value = csr_old_value_;
                change.new_value = csr_old_value_ & ~csr_write_val_;
                current_delta_.register_changes.push_back(change);
            }
            registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_write_val_);
            emit csrUpdated(csr_addr, csr_old_value_ & ~csr_write_val_);
        }
        break;

    case 0b101: // CSRRWI
        registers_->WriteGpr(rd, csr_old_value_);
        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 1;
            change.reg_index = csr_addr;
            change.old_value = csr_old_value_;
            change.new_value = csr_uimm_;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteCsr(csr_addr, csr_uimm_);
        emit csrUpdated(csr_addr, csr_uimm_);
        break;

    case 0b110: // CSRRSI
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_uimm_ != 0) {
            if (recording_enabled_) {
                RegisterChange change;
                change.reg_type = 1;
                change.reg_index = csr_addr;
                change.old_value = csr_old_value_;
                change.new_value = csr_old_value_ | csr_uimm_;
                current_delta_.register_changes.push_back(change);
            }
            registers_->WriteCsr(csr_addr, csr_old_value_ | csr_uimm_);
            emit csrUpdated(csr_addr, csr_old_value_ | csr_uimm_);
        }
        break;

    case 0b111: // CSRRCI
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_uimm_ != 0) {
            if (recording_enabled_) {
                RegisterChange change;
                change.reg_type = 1;
                change.reg_index = csr_addr;
                change.old_value = csr_old_value_;
                change.new_value = csr_old_value_ & ~csr_uimm_;
                current_delta_.register_changes.push_back(change);
            }
            registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_uimm_);
            emit csrUpdated(csr_addr, csr_old_value_ & ~csr_uimm_);
        }
        break;
    }
}

// VM control
void RVSSVM::Run()
{
    ClearStop();
    while (!stop_requested_ && program_counter_ < program_size_)
    {
        Fetch();
        Decode();
        Execute();
        WriteMemory();
        WriteBack();
        instructions_retired_++;
        cycle_s_++;
    }
    if (program_counter_ >= program_size_)
        emit statusChanged("VM_PROGRAM_END");

    DumpRegisters(globals::registers_dump_file_path,*registers_);
    qDebug() << "Dump Registers is finished";
}

void RVSSVM::DebugRun()
{
    ClearStop();
    while (!stop_requested_ && program_counter_ < program_size_)
    {
        current_delta_.old_pc = program_counter_;
        Fetch();
        Decode();
        Execute();
        WriteMemory();
        WriteBack();
        instructions_retired_++;
        cycle_s_++;
        current_delta_.new_pc = program_counter_;
        undo_stack_.push(current_delta_);
        /*while (!redo_stack_.empty())
            redo_stack_.pop()*/;
        current_delta_ = StepDelta();
    }
    if (program_counter_ >= program_size_)
        emit statusChanged("VM_PROGRAM_END");
}

// void RVSSVM::Step()
// {
//     current_delta_.old_pc = program_counter_;
//     if (program_counter_ < program_size_)
//     {
//         Fetch();
//         Decode();
//         Execute();
//         WriteMemory();
//         WriteBack();
//         instructions_retired_++;
//         cycle_s_++;
//         current_delta_.new_pc = program_counter_;
//         undo_stack_.push(current_delta_);
//         while (!redo_stack_.empty())
//             redo_stack_.pop();
//         current_delta_ = StepDelta();

//         DumpRegisters(globals::registers_dump_file_path,*registers_);
//         qDebug() << "Dump Registers is finished";
//     }
// }

void RVSSVM::Step()
{
    // Prepare to record the current step's changes (delta)
    current_delta_.old_pc = program_counter_;
    current_delta_.register_changes.clear();
    current_delta_.memory_changes.clear();

    if (program_counter_ >= program_size_)
        return;

    // Enable recording for this step
    recording_enabled_ = true;

    // Execute one instruction cycle
    Fetch();
    Decode();
    Execute();
    WriteMemory();
    WriteBack();

    // Disable recording after step
    recording_enabled_ = false;

    // Update statistics
    instructions_retired_++;
    cycle_s_++;

    // After step, save new PC
    current_delta_.new_pc = program_counter_;

    // Push this step delta to undo stack
    undo_stack_.push(current_delta_);

    // Clear redo stack
    // while (!redo_stack_.empty())
    //     redo_stack_.pop();

    // Reset delta for next step
    current_delta_ = StepDelta();

    DumpRegisters(globals::registers_dump_file_path, *registers_);
    qDebug() << "Step completed";
}

void RVSSVM::Undo()
{
    if (undo_stack_.empty())
        return;
    StepDelta last = undo_stack_.top();
    undo_stack_.pop();
    for (const auto &change : last.register_changes)
    {
        switch (change.reg_type)
        {
        case 0:
            registers_->WriteGpr(change.reg_index, change.old_value);
            emit gprUpdated(change.reg_index, change.old_value);
            break;
        case 1:
            registers_->WriteCsr(change.reg_index, change.old_value);
            emit csrUpdated(change.reg_index, change.old_value);
            break;
        case 2:
            registers_->WriteFpr(change.reg_index, change.old_value);
            emit fprUpdated(change.reg_index, change.old_value);
            break;
        }
    }
    for (const auto &change : last.memory_changes)
    {
        for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
            memory_controller_.WriteByte(change.address + i, change.old_bytes_vec[i]);
    }
    program_counter_ = last.old_pc;
    instructions_retired_--;
    cycle_s_--;
    // redo_stack_.push(last);

}

void RVSSVM::Reset()
{
    program_counter_ = 0;
    instructions_retired_ = 0;
    cycle_s_ = 0;
    registers_->Reset();
    memory_controller_.Reset();
    control_unit_.Reset();
    branch_flag_ = false;
    next_pc_ = 0;
    execution_result_ = 0;
    memory_result_ = 0;
    return_address_ = 0;
    csr_target_address_ = 0;
    csr_old_value_ = 0;
    csr_write_val_ = 0;
    csr_uimm_ = 0;
    current_delta_.register_changes.clear();
    current_delta_.memory_changes.clear();
    current_delta_.old_pc = 0;
    current_delta_.new_pc = 0;
    undo_stack_ = std::stack<StepDelta>();
    // redo_stack_ = std::stack<StepDelta>();
}
