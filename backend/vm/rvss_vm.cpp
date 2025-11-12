#include "rvss_vm.h"
// #include "../../utils.h"
// #include "../../globals.h"
#include "../../common/instructions.h"
// #include "../../config.h"

#include <cctype>
#include <cstdint>
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

void RVSSVM::WriteMemoryFloat()
{
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    if (control_unit_.GetMemRead())
        memory_result_ = memory_controller_.ReadWord(execution_result_);

    if (control_unit_.GetMemWrite()) {
        uint32_t val = registers_->ReadFpr(rs2) & 0xFFFFFFFF;
        memory_controller_.WriteWord(execution_result_, val);
    }
}

void RVSSVM::WriteMemoryDouble()
{
    if (registers_->GetIsa() != ISA::RV64) {
        emit vmError("Double-precision store/read not supported in RV32");
        return;
    }
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
    if (control_unit_.GetMemRead())
        memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);

    if (control_unit_.GetMemWrite())
        memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
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
        switch (opcode) {
        case 0b0110011: // R-type
        case 0b0010011: // I-type
        case 0b0010111: // AUIPC
            registers_->WriteGpr(rd, execution_result_);
            emit gprUpdated(rd, execution_result_);
            break;
        case 0b0000011: // Loads
            registers_->WriteGpr(rd, memory_result_);
            emit gprUpdated(rd, memory_result_);
            break;
        case 0b1100111: // JALR
        case 0b1101111: // JAL
            registers_->WriteGpr(rd, next_pc_);
            emit gprUpdated(rd, next_pc_);
            break;
        case 0b0110111: // LUI
            registers_->WriteGpr(rd, (imm << 12));
            emit gprUpdated(rd, (imm << 12));
            break;
        default:
            break;
        }
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
        registers_->WriteFpr(rd, execution_result_);
        emit fprUpdated(rd, execution_result_);
    }
}

void RVSSVM::WriteBackCsr()
{
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint16_t csr_addr = csr_target_address_;
    switch (funct3)
    {
    case 0b001: // CSRRW
        registers_->WriteGpr(rd, csr_old_value_);
        registers_->WriteCsr(csr_addr, csr_write_val_);
        emit csrUpdated(csr_addr, csr_write_val_);
        break;
    case 0b010: // CSRRS
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_write_val_ != 0)
        {
            registers_->WriteCsr(csr_addr, csr_old_value_ | csr_write_val_);
            emit csrUpdated(csr_addr, csr_old_value_ | csr_write_val_);
        }
        break;
    case 0b011: // CSRRC
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_write_val_ != 0)
        {
            registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_write_val_);
            emit csrUpdated(csr_addr, csr_old_value_ & ~csr_write_val_);
        }
        break;
    case 0b101: // CSRRWI
        registers_->WriteGpr(rd, csr_old_value_);
        registers_->WriteCsr(csr_addr, csr_uimm_);
        emit csrUpdated(csr_addr, csr_uimm_);
        break;
    case 0b110: // CSRRSI
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_uimm_ != 0)
        {
            registers_->WriteCsr(csr_addr, csr_old_value_ | csr_uimm_);
            emit csrUpdated(csr_addr, csr_old_value_ | csr_uimm_);
        }
        break;
    case 0b111: // CSRRCI
        registers_->WriteGpr(rd, csr_old_value_);
        if (csr_uimm_ != 0)
        {
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
        while (!redo_stack_.empty())
            redo_stack_.pop();
        current_delta_ = StepDelta();
    }
    if (program_counter_ >= program_size_)
        emit statusChanged("VM_PROGRAM_END");
}

void RVSSVM::Step()
{
    current_delta_.old_pc = program_counter_;
    if (program_counter_ < program_size_)
    {
        Fetch();
        Decode();
        Execute();
        WriteMemory();
        WriteBack();
        instructions_retired_++;
        cycle_s_++;
        current_delta_.new_pc = program_counter_;
        undo_stack_.push(current_delta_);
        while (!redo_stack_.empty())
            redo_stack_.pop();
        current_delta_ = StepDelta();
    }
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
    redo_stack_.push(last);
}

void RVSSVM::Redo()
{
    if (redo_stack_.empty())
        return;
    StepDelta next = redo_stack_.top();
    redo_stack_.pop();
    for (const auto &change : next.register_changes)
    {
        switch (change.reg_type)
        {
        case 0:
            registers_->WriteGpr(change.reg_index, change.new_value);
            emit gprUpdated(change.reg_index, change.new_value);
            break;
        case 1:
            registers_->WriteCsr(change.reg_index, change.new_value);
            emit csrUpdated(change.reg_index, change.new_value);
            break;
        case 2:
            registers_->WriteFpr(change.reg_index, change.new_value);
            emit fprUpdated(change.reg_index, change.new_value);
            break;
        }
    }
    for (const auto &change : next.memory_changes)
    {
        for (size_t i = 0; i < change.new_bytes_vec.size(); ++i)
            memory_controller_.WriteByte(change.address + i, change.new_bytes_vec[i]);
    }
    program_counter_ = next.new_pc;
    instructions_retired_++;
    cycle_s_++;
    undo_stack_.push(next);
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
    redo_stack_ = std::stack<StepDelta>();
}


// #include "rvss_vm.h"
// #include "../../common/instructions.h"

// #include <cctype>
// #include <cstdint>
// #include <tuple>
// #include <stack>
// #include <QString>
// #include <QDebug>  // Include for qDebug

// RVSSVM::RVSSVM(RegisterFile *sharedRegisters, QObject *parent)
//     : QObject(parent), VmBase()
// {
//     registers_ = sharedRegisters; // Use the shared instance
//     qDebug() << "RVSSVM Constructor called. ISA =" << static_cast<int>(registers_->GetIsa());
// }
// RVSSVM::~RVSSVM() {
//     qDebug() << "RVSSVM Destructor called";
// }

// void RVSSVM::Fetch()
// {
//     current_instruction_ = memory_controller_.ReadWord(program_counter_);
//     qDebug() << "Fetch: PC=0x" << QString::number(program_counter_, 16) << " Instruction=0x" << QString::number(current_instruction_, 16);
//     UpdateProgramCounter(4);
// }

// void RVSSVM::Decode()
// {
//     control_unit_.SetControlSignals(current_instruction_);
//     qDebug() << "Decode: Instruction=0x" << QString::number(current_instruction_, 16)
//              << " Control Signals set.";
// }

// void RVSSVM::Execute()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//     uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     int32_t imm = ImmGenerator(current_instruction_);
//     uint64_t reg1_value = registers_->ReadGpr(rs1);
//     uint64_t reg2_value = registers_->ReadGpr(rs2);

//     qDebug() << "Execute: instruction=0x" << QString::number(current_instruction_, 16)
//              << " opcode=" << QString::number(opcode, 2).rightJustified(7, '0')
//              << " funct3=" << QString::number(funct3, 2).rightJustified(3, '0')
//              << " rs1=" << rs1 << " reg1_value=0x" << QString::number(reg1_value, 16)
//              << " rs2=" << rs2 << " reg2_value=0x" << QString::number(reg2_value, 16)
//              << " imm=" << imm;

//     if (opcode == 0b1110011 && funct3 == 0b000) {
//         qDebug() << "Syscall detected.";
//         HandleSyscall();
//         return;
//     }

//     if (instruction_set::isFInstruction(current_instruction_)) {
//         qDebug() << "Floating point instruction detected.";
//         ExecuteFloat();
//         return;
//     } else if (instruction_set::isDInstruction(current_instruction_)) {
//         if (registers_->GetIsa() == ISA::RV64)
//             ExecuteDouble();
//         else {
//             emit vmError("Double-precision not supported in RV32");
//             qDebug() << "Double-precision not supported in RV32";
//         }
//         return;
//     } else if (opcode == 0b1110011) {
//         ExecuteCsr();
//         return;
//     }

//     bool overflow = false;
//     if (control_unit_.GetAluSrc()) {
//         reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
//         qDebug() << "ALU src is immediate, reg2_value set to imm: " << imm;
//     }

//     alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//     std::tie(execution_result_, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);
//     qDebug() << "ALU executed, result=0x" << QString::number(execution_result_, 16) << " overflow=" << overflow;

//     // Branch instructions handling
//     if (opcode == 0b1100111 || opcode == 0b1101111) { // JALR, JAL
//         next_pc_ = static_cast<int64_t>(program_counter_);
//         UpdateProgramCounter(-4);
//         return_address_ = program_counter_ + 4;
//         if (opcode == 0b1100111) {
//             UpdateProgramCounter(-program_counter_ + execution_result_);
//             qDebug() << "JALR branch taken to 0x" << QString::number(program_counter_,16);
//         } else {
//             UpdateProgramCounter(imm);
//             qDebug() << "JAL branch taken to 0x" << QString::number(program_counter_,16);
//         }
//     }
//     else if (opcode == 0b1100011) {
//         bool takeBranch = false;
//         switch (funct3) {
//         case 0b000: takeBranch = (execution_result_ == 0); break; // BEQ
//         case 0b001: takeBranch = (execution_result_ != 0); break; // BNE
//         case 0b100: takeBranch = (execution_result_ < 0); break; // BLT
//         case 0b101: takeBranch = (execution_result_ >= 0); break; // BGE
//         case 0b110: takeBranch = (static_cast<uint64_t>(reg1_value) < static_cast<uint64_t>(reg2_value)); break; // BLTU
//         case 0b111: takeBranch = (static_cast<uint64_t>(reg1_value) >= static_cast<uint64_t>(reg2_value)); break; // BGEU
//         }
//         qDebug() << "Branch instruction funct3=" << funct3 << " takeBranch=" << takeBranch;
//         if (takeBranch) {
//             UpdateProgramCounter(-4);
//             UpdateProgramCounter(imm);
//             qDebug() << "Branch taken to 0x" << QString::number(program_counter_,16);
//         }
//     }

//     if (opcode == 0b0010111) { // AUIPC
//         execution_result_ = static_cast<int64_t>(program_counter_) - 4 + (imm << 12);
//         qDebug() << "AUIPC execution_result=0x" << QString::number(execution_result_, 16);
//     }
// }

// void RVSSVM::WriteMemory()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//     if (opcode == 0b1110011 && funct3 == 0b000) {
//         qDebug() << "WriteMemory skipped for syscall.";
//         return;
//     }

//     if (instruction_set::isFInstruction(current_instruction_)) {
//         qDebug() << "WriteMemory: floating point load/store.";
//         WriteMemoryFloat();
//         return;
//     }
//     else if (instruction_set::isDInstruction(current_instruction_)) {
//         if (registers_->GetIsa() == ISA::RV64)
//             WriteMemoryDouble();
//         else {
//             emit vmError("Double-precision stores not supported in RV32");
//             qDebug() << "Double-precision stores not supported in RV32";
//         }
//         return;
//     }

//     if (control_unit_.GetMemRead()) {
//         qDebug() << "WriteMemory (Load): opcode=" << QString::number(opcode, 2).rightJustified(7, '0')
//         << " funct3=" << QString::number(funct3, 2).rightJustified(3, '0')
//         << " address=0x" << QString::number(execution_result_, 16);
//         switch (funct3) {
//         case 0b000: memory_result_ = static_cast<int8_t>(memory_controller_.ReadByte(execution_result_)); break;
//         case 0b001: memory_result_ = static_cast<int16_t>(memory_controller_.ReadHalfWord(execution_result_)); break;
//         case 0b010: memory_result_ = static_cast<int32_t>(memory_controller_.ReadWord(execution_result_)); break;
//         case 0b011:
//             if (registers_->GetIsa() == ISA::RV64)
//                 memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
//             break;
//         case 0b100: memory_result_ = static_cast<uint8_t>(memory_controller_.ReadByte(execution_result_)); break;
//         case 0b101: memory_result_ = static_cast<uint16_t>(memory_controller_.ReadHalfWord(execution_result_)); break;
//         case 0b110: memory_result_ = static_cast<uint32_t>(memory_controller_.ReadWord(execution_result_)); break;
//         }
//         qDebug() << "Load memory_result_=0x" << QString::number(memory_result_, 16);
//     }

//     if (control_unit_.GetMemWrite()) {
//         qDebug() << "WriteMemory (Store): opcode=" << QString::number(opcode, 2).rightJustified(7, '0')
//         << " funct3=" << QString::number(funct3, 2).rightJustified(3, '0')
//         << " address=0x" << QString::number(execution_result_, 16);
//         switch (funct3) {
//         case 0b000: memory_controller_.WriteByte(execution_result_, registers_->ReadGpr(rs2) & 0xFF); break;
//         case 0b001: memory_controller_.WriteHalfWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFF); break;
//         case 0b010: memory_controller_.WriteWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFFFFFF); break;
//         case 0b011:
//             if (registers_->GetIsa() == ISA::RV64)
//                 memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadGpr(rs2));
//             break;
//         }
//         qDebug() << "Store data=0x" << QString::number(registers_->ReadGpr(rs2), 16);
//     }
// }

// void RVSSVM::WriteBack()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;
//     int32_t imm = ImmGenerator(current_instruction_);

//     if (opcode == 0b1110011 && funct3 == 0b000) {
//         qDebug() << "WriteBack skipped for syscall.";
//         return;
//     }
//     if (instruction_set::isFInstruction(current_instruction_)) {
//         WriteBackFloat();
//         return;
//     }
//     else if (instruction_set::isDInstruction(current_instruction_)) {
//         if (registers_->GetIsa() == ISA::RV64)
//             WriteBackDouble();
//         else {
//             emit vmError("Double-precision writeback not supported in RV32");
//             qDebug() << "Double-precision writeback not supported in RV32";
//         }
//         return;
//     }
//     else if (opcode == 0b1110011) {
//         WriteBackCsr();
//         return;
//     }

//     if (control_unit_.GetRegWrite()) {
//         uint64_t value_to_write = 0;
//         switch (opcode) {
//         case 0b0110011: // R-type
//         case 0b0010011: // I-type
//         case 0b0010111: // AUIPC
//             value_to_write = execution_result_;
//             registers_->WriteGpr(rd, value_to_write);
//             break;
//         case 0b0000011: // Loads
//             value_to_write = memory_result_;
//             registers_->WriteGpr(rd, value_to_write);
//             break;
//         case 0b1100111: // JALR
//         case 0b1101111: // JAL
//             value_to_write = next_pc_;
//             registers_->WriteGpr(rd, value_to_write);
//             break;
//         case 0b0110111: // LUI
//             value_to_write = (imm << 12);
//             registers_->WriteGpr(rd, value_to_write);
//             break;
//         default:
//             break;
//         }
//         qDebug() << "WriteBack: opcode=" << QString::number(opcode, 2).rightJustified(7, '0')
//                  << " rd=" << rd << " writing value=0x" << QString::number(value_to_write, 16);
//         emit gprUpdated(rd, value_to_write);
//     }
// }

// void RVSSVM::Run()
// {
//     ClearStop();
//     qDebug() << "VM Run started.";
//     while (!stop_requested_ && program_counter_ < program_size_)
//     {
//         qDebug() << "Run cycle. PC=0x" << QString::number(program_counter_, 16);
//         Fetch();
//         Decode();
//         Execute();
//         WriteMemory();
//         WriteBack();
//         instructions_retired_++;
//         cycle_s_++;
//     }
//     if (program_counter_ >= program_size_) {
//         emit statusChanged("VM_PROGRAM_END");
//         qDebug() << "VM program reached end. PC=0x" << QString::number(program_counter_, 16);
//     }
// }

// void RVSSVM::Step()
// {
//     qDebug() << "VM Step started. PC=0x" << QString::number(program_counter_, 16);
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
//         qDebug() << "VM Step done. PC=0x" << QString::number(program_counter_, 16);
//     }
// }

// void RVSSVM::Undo()
// {
//     qDebug() << "Undo called.";
//     if (undo_stack_.empty()) {
//         qDebug() << "Undo stack empty, nothing to undo.";
//         return;
//     }
//     StepDelta last = undo_stack_.top();
//     undo_stack_.pop();
//     for (const auto &change : last.register_changes)
//     {
//         switch (change.reg_type)
//         {
//         case 0:
//             registers_->WriteGpr(change.reg_index, change.old_value);
//             emit gprUpdated(change.reg_index, change.old_value);
//             break;
//         case 1:
//             registers_->WriteCsr(change.reg_index, change.old_value);
//             emit csrUpdated(change.reg_index, change.old_value);
//             break;
//         case 2:
//             registers_->WriteFpr(change.reg_index, change.old_value);
//             emit fprUpdated(change.reg_index, change.old_value);
//             break;
//         }
//     }
//     for (const auto &change : last.memory_changes)
//     {
//         for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
//             memory_controller_.WriteByte(change.address + i, change.old_bytes_vec[i]);
//     }
//     program_counter_ = last.old_pc;
//     instructions_retired_--;
//     cycle_s_--;
//     redo_stack_.push(last);
//     qDebug() << "Undo done. Restored PC=0x" << QString::number(program_counter_,16);
// }

// void RVSSVM::Redo()
// {
//     qDebug() << "Redo called.";
//     if (redo_stack_.empty()) {
//         qDebug() << "Redo stack empty, nothing to redo.";
//         return;
//     }
//     StepDelta next = redo_stack_.top();
//     redo_stack_.pop();
//     for (const auto &change : next.register_changes)
//     {
//         switch (change.reg_type)
//         {
//         case 0:
//             registers_->WriteGpr(change.reg_index, change.new_value);
//             emit gprUpdated(change.reg_index, change.new_value);
//             break;
//         case 1:
//             registers_->WriteCsr(change.reg_index, change.new_value);
//             emit csrUpdated(change.reg_index, change.new_value);
//             break;
//         case 2:
//             registers_->WriteFpr(change.reg_index, change.new_value);
//             emit fprUpdated(change.reg_index, change.new_value);
//             break;
//         }
//     }
//     for (const auto &change : next.memory_changes)
//     {
//         for (size_t i = 0; i < change.new_bytes_vec.size(); ++i)
//             memory_controller_.WriteByte(change.address + i, change.new_bytes_vec[i]);
//     }
//     program_counter_ = next.new_pc;
//     instructions_retired_++;
//     cycle_s_++;
//     undo_stack_.push(next);
//     qDebug() << "Redo done. Advanced PC=0x" << QString::number(program_counter_,16);
// }

// void RVSSVM::Reset()
// {
//     qDebug() << "VM Reset.";
//     program_counter_ = 0;
//     instructions_retired_ = 0;
//     cycle_s_ = 0;
//     registers_->Reset();
//     memory_controller_.Reset();
//     control_unit_.Reset();
//     branch_flag_ = false;
//     next_pc_ = 0;
//     execution_result_ = 0;
//     memory_result_ = 0;
//     return_address_ = 0;
//     csr_target_address_ = 0;
//     csr_old_value_ = 0;
//     csr_write_val_ = 0;
//     csr_uimm_ = 0;
//     current_delta_.register_changes.clear();
//     current_delta_.memory_changes.clear();
//     current_delta_.old_pc = 0;
//     current_delta_.new_pc = 0;
//     undo_stack_ = std::stack<StepDelta>();
//     redo_stack_ = std::stack<StepDelta>();
// }

// void RVSSVM::HandleSyscall()
// {
//     uint64_t syscall_number = registers_->ReadGpr(17);
//     switch (syscall_number)
//     {
//     case SYSCALL_PRINT_INT:
//         emit syscallOutput(QString("[Syscall output: %1]").arg((qint64)registers_->ReadGpr(10)));
//         break;
//     case SYSCALL_PRINT_FLOAT:
//     {
//         float float_value;
//         uint64_t raw = registers_->ReadGpr(10);
//         std::memcpy(&float_value, &raw, sizeof(float_value));
//         emit syscallOutput(QString("[Syscall output: %1]").arg(float_value));
//         break;
//     }
//     case SYSCALL_PRINT_DOUBLE:
//     {
//         double double_value;
//         uint64_t raw = registers_->ReadGpr(10);
//         std::memcpy(&double_value, &raw, sizeof(double_value));
//         emit syscallOutput(QString("[Syscall output: %1]").arg(double_value));
//         break;
//     }
//     case SYSCALL_PRINT_STRING:
//         emit syscallOutput("[Syscall output: ...string... (not implemented yet)]");
//         break;
//     case SYSCALL_EXIT:
//         stop_requested_ = true;
//         emit vmError(QString("VM exited with code: %1").arg(registers_->ReadGpr(10)));
//         break;
//     default:
//         emit vmError(QString("Unknown syscall: %1").arg(syscall_number));
//         break;
//     }
// }

// void RVSSVM::ExecuteFloat()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//     uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//     uint8_t rm = funct3;
//     uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     uint8_t rs3 = (current_instruction_ >> 27) & 0b11111;

//     uint8_t fcsr_status = 0;
//     int32_t imm = ImmGenerator(current_instruction_);

//     if (rm == 0b111)
//         rm = registers_->ReadCsr(0x002);

//     uint64_t reg1_value = registers_->ReadFpr(rs1);
//     uint64_t reg2_value = registers_->ReadFpr(rs2);
//     uint64_t reg3_value = registers_->ReadFpr(rs3);

//     if (funct7 == 0b1101000 || funct7 == 0b1111000 || opcode == 0b0000111 || opcode == 0b0100111)
//         reg1_value = registers_->ReadGpr(rs1);

//     if (control_unit_.GetAluSrc())
//         reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));

//     alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//     std::tie(execution_result_, fcsr_status) = alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

//     registers_->WriteCsr(0x003, fcsr_status);
//     emit csrUpdated(0x003, fcsr_status);
// }

// void RVSSVM::ExecuteDouble()
// {
//     if (registers_->GetIsa() != ISA::RV64) {
//         emit vmError("Double-precision not supported in RV32");
//         return;
//     }
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//     uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//     uint8_t rm = funct3;
//     uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     uint8_t rs3 = (current_instruction_ >> 27) & 0b11111;

//     uint8_t fcsr_status = 0;
//     int32_t imm = ImmGenerator(current_instruction_);

//     uint64_t reg1_value = registers_->ReadFpr(rs1);
//     uint64_t reg2_value = registers_->ReadFpr(rs2);
//     uint64_t reg3_value = registers_->ReadFpr(rs3);

//     if (funct7 == 0b1101001 || funct7 == 0b1111001 || opcode == 0b0000111 || opcode == 0b0100111)
//         reg1_value = registers_->ReadGpr(rs1);

//     if (control_unit_.GetAluSrc())
//         reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));

//     alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//     std::tie(execution_result_, fcsr_status) = alu::Alu::dfpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);
// }

// void RVSSVM::ExecuteCsr()
// {
//     uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//     uint16_t csr = (current_instruction_ >> 20) & 0xFFF;
//     uint64_t csr_val = registers_->ReadCsr(csr);

//     csr_target_address_ = csr;
//     csr_old_value_ = csr_val;
//     csr_write_val_ = registers_->ReadGpr(rs1);
//     csr_uimm_ = rs1;
// }

// void RVSSVM::WriteBackFloat()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;
//     if (control_unit_.GetRegWrite()) {
//         uint64_t value = execution_result_;
//         if (registers_->GetIsa() == ISA::RV32)
//             value &= 0xFFFFFFFF;
//         registers_->WriteFpr(rd, value);
//         emit fprUpdated(rd, value);
//     }
// }

// void RVSSVM::WriteBackDouble()
// {
//     if (registers_->GetIsa() != ISA::RV64) {
//         emit vmError("Double-precision FPR write not allowed in RV32");
//         return;
//     }
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;
//     if (control_unit_.GetRegWrite()) {
//         registers_->WriteFpr(rd, execution_result_);
//         emit fprUpdated(rd, execution_result_);
//     }
// }

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

// void RVSSVM::DebugRun()
// {
//     ClearStop();
//     while (!stop_requested_ && program_counter_ < program_size_)
//     {
//         current_delta_.old_pc = program_counter_;
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
//     }
//     if (program_counter_ >= program_size_)
//         emit statusChanged("VM_PROGRAM_END");
// }
