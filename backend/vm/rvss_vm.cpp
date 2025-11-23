// #include "rvss_vm.h"
// #include "../../utils.h"
// #include "../../globals.h"
// #include "../../common/instructions.h"
// // #include "../../config.h"

// #include <cctype>
// #include <cstdint>
// #include <qdebug.h>
// #include <tuple>
// #include <stack>
// // #include <string>
// #include <atomic>
// #include <QString>

// // Proper Qt constructor/destructor
// RVSSVM::RVSSVM(RegisterFile *sharedRegisters, QObject *parent)
//     : QObject(parent), VmBase()
// {
//     registers_ = sharedRegisters; // Use the shared instance
// }
// RVSSVM::~RVSSVM() = default;

// void RVSSVM::Fetch()
// {
//     current_instruction_ = memory_controller_.ReadWord(program_counter_);
//     UpdateProgramCounter(4);
// }

// void RVSSVM::Decode()
// {
//     control_unit_.SetControlSignals(current_instruction_);
// }

// void RVSSVM::Execute()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//     if (opcode == 0b1110011 && funct3 == 0b000) {
//         HandleSyscall();
//         return;
//     }

//     if (instruction_set::isFInstruction(current_instruction_)) {
//         ExecuteFloat();
//         return;
//     } else if (instruction_set::isDInstruction(current_instruction_)) {
//         if (registers_->GetIsa() == ISA::RV64)
//             ExecuteDouble();
//         else
//             emit vmError("Double-precision not supported in RV32");
//         return;
//     } else if (opcode == 0b1110011) {
//         ExecuteCsr();
//         return;
//     }

//     uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     int32_t imm = ImmGenerator(current_instruction_);

//     uint64_t reg1_value = registers_->ReadGpr(rs1);
//     uint64_t reg2_value = registers_->ReadGpr(rs2);

//     bool overflow = false;
//     if (control_unit_.GetAluSrc()) {
//         reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
//     }

//     alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//     std::tie(execution_result_, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);

//     // Branch instructions
//     if (opcode == 0b1100111 || opcode == 0b1101111) { // JALR/JAL
//         next_pc_ = static_cast<int64_t>(program_counter_);
//         UpdateProgramCounter(-4);
//         return_address_ = program_counter_ + 4;
//         if (opcode == 0b1100111) {
//             UpdateProgramCounter(-program_counter_ + execution_result_);
//         } else {
//             UpdateProgramCounter(imm);
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
//         if (takeBranch) {
//             UpdateProgramCounter(-4);
//             UpdateProgramCounter(imm);
//         }
//     }

//     if (opcode == 0b0010111) { // AUIPC
//         execution_result_ = static_cast<int64_t>(program_counter_) - 4 + (imm << 12);
//     }
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

// void RVSSVM::WriteMemory()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//     if (opcode == 0b1110011 && funct3 == 0b000)
//         return;

//     if (instruction_set::isFInstruction(current_instruction_)) {
//         WriteMemoryFloat();
//         return;
//     }
//     else if (instruction_set::isDInstruction(current_instruction_)) {
//         if (registers_->GetIsa() == ISA::RV64)
//             WriteMemoryDouble();
//         else
//             emit vmError("Double-precision stores not supported in RV32");
//         return;
//     }

//     if (control_unit_.GetMemRead()) {
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
//     }



//     if (control_unit_.GetMemWrite()) {
//         if (recording_enabled_) {
//             MemoryChange mem_change;
//             mem_change.address = execution_result_;

//             // Read old bytes before overwriting
//             switch (funct3) {
//             case 0b000: // SB
//                 mem_change.old_bytes_vec.push_back(memory_controller_.ReadByte(execution_result_));
//                 mem_change.new_bytes_vec.push_back(registers_->ReadGpr(rs2) & 0xFF);
//                 break;
//             case 0b001: // SH
//             {
//                 uint16_t old_val = memory_controller_.ReadHalfWord(execution_result_);
//                 mem_change.old_bytes_vec.push_back(old_val & 0xFF);
//                 mem_change.old_bytes_vec.push_back((old_val >> 8) & 0xFF);
//                 uint16_t new_val = registers_->ReadGpr(rs2) & 0xFFFF;
//                 mem_change.new_bytes_vec.push_back(new_val & 0xFF);
//                 mem_change.new_bytes_vec.push_back((new_val >> 8) & 0xFF);
//                 break;
//             }
//             case 0b010: // SW
//             {
//                 uint32_t old_val = memory_controller_.ReadWord(execution_result_);
//                 for (int i = 0; i < 4; ++i)
//                     mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
//                 uint32_t new_val = registers_->ReadGpr(rs2) & 0xFFFFFFFF;
//                 for (int i = 0; i < 4; ++i)
//                     mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
//                 break;
//             }
//             case 0b011: // SD
//                 if (registers_->GetIsa() == ISA::RV64) {
//                     uint64_t old_val = memory_controller_.ReadDoubleWord(execution_result_);
//                     for (int i = 0; i < 8; ++i)
//                         mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
//                     uint64_t new_val = registers_->ReadGpr(rs2);
//                     for (int i = 0; i < 8; ++i)
//                         mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
//                 }
//                 break;
//             }
//             current_delta_.memory_changes.push_back(mem_change);
//         }
//         switch (funct3) {
//         case 0b000: memory_controller_.WriteByte(execution_result_, registers_->ReadGpr(rs2) & 0xFF); break;
//         case 0b001: memory_controller_.WriteHalfWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFF); break;
//         case 0b010: memory_controller_.WriteWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFFFFFF); break;
//         case 0b011:
//             if (registers_->GetIsa() == ISA::RV64)
//                 memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadGpr(rs2));
//             break;
//         }

//         // Optionally update undo/redo stacks (not abbreviated for brevity).
//     }
// }

// void RVSSVM::WriteBack()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;
//     int32_t imm = ImmGenerator(current_instruction_);

//     if (opcode == 0b1110011 && funct3 == 0b000)
//         return;
//     if (instruction_set::isFInstruction(current_instruction_)) {
//         WriteBackFloat();
//         return;
//     }
//     else if (instruction_set::isDInstruction(current_instruction_)) {
//         if (registers_->GetIsa() == ISA::RV64)
//             WriteBackDouble();
//         else
//             emit vmError("Double-precision writeback not supported in RV32");
//         return;
//     }
//     else if (opcode == 0b1110011) {
//         WriteBackCsr();
//         return;
//     }

//     if (control_unit_.GetRegWrite()) {
//         uint64_t new_value = 0;
//         switch (opcode) {
//         case 0b0110011: // R-type
//         case 0b0010011: // I-type
//         case 0b0010111: // AUIPC
//             new_value = execution_result_;
//             break;
//         case 0b0000011: // Loads
//             new_value = memory_result_;
//             break;
//         case 0b1100111: // JALR
//         case 0b1101111: // JAL
//             new_value = next_pc_;
//             break;
//         case 0b0110111: // LUI
//             new_value = (imm << 12);
//             break;
//         default:
//             break;
//         }

//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 0; // GPR
//             change.reg_index = rd;
//             change.old_value = registers_->ReadGpr(rd);
//             change.new_value = new_value;
//             current_delta_.register_changes.push_back(change);
//         }

//         registers_->WriteGpr(rd, new_value);
//         emit gprUpdated(rd, new_value);
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

//     // Handle FLW (opcode 0b0000111) - Float Load Word
//     if (opcode == 0b0000111) {
//         // Address = GPR[rs1] + imm
//         uint64_t base_addr = registers_->ReadGpr(rs1);
//         execution_result_ = base_addr + imm;
//         // WriteMemoryFloat will handle the actual load
//         return;
//     }

//     // Handle FSW (opcode 0b0100111) - Float Store Word
//     if (opcode == 0b0100111) {
//         // Address = GPR[rs1] + imm
//         uint64_t base_addr = registers_->ReadGpr(rs1);
//         execution_result_ = base_addr + imm;
//         // WriteMemoryFloat will handle the actual store
//         return;
//     }

//     // Default: read from FPR
//     uint64_t reg1_value = registers_->ReadFpr(rs1);
//     uint64_t reg2_value = registers_->ReadFpr(rs2);
//     uint64_t reg3_value = registers_->ReadFpr(rs3);

//     // Validate NaN-boxing for single-precision operands
//     // If upper 32 bits are not all 1s, treat as canonical NaN
//     auto validate_sp = [](uint64_t val) -> uint64_t {
//         if ((val & 0xFFFFFFFF00000000ULL) != 0xFFFFFFFF00000000ULL) {
//             // Not properly NaN-boxed, return canonical NaN
//             // qDebug() << "Warning: FPR value not properly NaN-boxed:" << QString::number(val, 16);
//             return 0x7FC00000; // Canonical single-precision NaN
//         }
//         return val & 0xFFFFFFFF; // Extract lower 32 bits
//     };

//     // For instructions that need GPR source, don't validate
//     if (funct7 != 0b1101000 && funct7 != 0b1111000) {
//         reg1_value = validate_sp(reg1_value);
//         reg2_value = validate_sp(reg2_value);
//         reg3_value = validate_sp(reg3_value);
//     }

//     // Instructions that need GPR source:
//     // FCVT.S.W/FCVT.S.WU (funct7=0b1101000): convert int to float, source is GPR
//     // FMV.W.X (funct7=0b1111000): move bits from GPR to FPR
//     // if (funct7 == 0b1101000 || funct7 == 0b1111000) {
//     //     reg1_value = registers_->ReadGpr(rs1);
//     // }

//     // Note: FCVT.W.S/FCVT.WU.S (funct7=0b1100000) converts float to int
//     // Source is FPR (already read above), destination is GPR (handled in WriteBackFloat)

//     if (control_unit_.GetAluSrc())
//         reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));

//     alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//     std::cout << aluOperation;
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

//     // Handle FLD (opcode 0b0000111, funct3=0b011) - Float Load Double
//     if (opcode == 0b0000111) {
//         uint64_t base_addr = registers_->ReadGpr(rs1);
//         execution_result_ = base_addr + imm;
//         return;
//     }

//     // Handle FSD (opcode 0b0100111, funct3=0b011) - Float Store Double
//     if (opcode == 0b0100111) {
//         uint64_t base_addr = registers_->ReadGpr(rs1);
//         execution_result_ = base_addr + imm;
//         return;
//     }

//     uint64_t reg1_value = registers_->ReadFpr(rs1);
//     uint64_t reg2_value = registers_->ReadFpr(rs2);
//     uint64_t reg3_value = registers_->ReadFpr(rs3);

//     // FCVT.D.W/FCVT.D.WU/FCVT.D.L/FCVT.D.LU (funct7=0b1101001): int to double
//     // FMV.D.X (funct7=0b1111001): move bits from GPR to FPR
//     if (funct7 == 0b1101001 || funct7 == 0b1111001) {
//         reg1_value = registers_->ReadGpr(rs1);
//     }

//     if (control_unit_.GetAluSrc())
//         reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));

//     alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//     std::tie(execution_result_, fcsr_status) = alu::Alu::dfpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

//     registers_->WriteCsr(0x003, fcsr_status);
//     emit csrUpdated(0x003, fcsr_status);
// }

// void RVSSVM::WriteMemoryFloat()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

//     // FLW - Load float from memory to FPR
//     if (control_unit_.GetMemRead()) {
//         uint32_t raw_value = memory_controller_.ReadWord(execution_result_);
//         memory_result_ = 0xFFFFFFFF00000000ULL | raw_value;

//         // qDebug() << "FLW: Loading from address" << execution_result_
//         //          << "raw value:" << QString::number(raw_value, 16)
//         //          << "boxed value:" << QString::number(memory_result_, 16);
//     }

//     // FSW - Store float from FPR to memory
//     if (control_unit_.GetMemWrite()) {

//         uint32_t float_bits = registers_->ReadFpr(rs2) & 0xFFFFFFFF;

//         if (recording_enabled_) {
//             MemoryChange mem_change;
//             mem_change.address = execution_result_;
//             uint32_t old_val = memory_controller_.ReadWord(execution_result_);
//             for (int i = 0; i < 4; ++i)
//                 mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
//             for (int i = 0; i < 4; ++i)
//                 mem_change.new_bytes_vec.push_back((float_bits >> (i * 8)) & 0xFF);
//             current_delta_.memory_changes.push_back(mem_change);
//         }

//         // uint32_t val = registers_->ReadFpr(rs2) & 0xFFFFFFFF;
//         // memory_controller_.WriteWord(execution_result_, val);
//         // qDebug() << "FSW: Storing to address" << execution_result_ << "value:" << val;
//         memory_controller_.WriteWord(execution_result_, float_bits);
//         // qDebug() << "FSW: Storing to address" << execution_result_
//         //          << "value:" << QString::number(float_bits, 16);
//     }
// }

// void RVSSVM::WriteMemoryDouble()
// {
//     if (registers_->GetIsa() != ISA::RV64) {
//         emit vmError("Double-precision store/read not supported in RV32");
//         return;
//     }
//     uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

//     // FLD - Load double from memory
//     if (control_unit_.GetMemRead()) {
//         memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
//         // qDebug() << "FLD: Loading from address" << execution_result_ << "value:" << memory_result_;
//     }

//     // FSD - Store double to memory
//     if (control_unit_.GetMemWrite()) {
//         if (recording_enabled_) {
//             MemoryChange mem_change;
//             mem_change.address = execution_result_;
//             uint64_t old_val = memory_controller_.ReadDoubleWord(execution_result_);
//             for (int i = 0; i < 8; ++i)
//                 mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
//             uint64_t new_val = registers_->ReadFpr(rs2);
//             for (int i = 0; i < 8; ++i)
//                 mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
//             current_delta_.memory_changes.push_back(mem_change);
//         }

//         memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
//         // qDebug() << "FSD: Storing to address" << execution_result_ << "value:" << registers_->ReadFpr(rs2);
//     }
// }

// void RVSSVM::WriteBackFloat()
// {
//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;

//     if (!control_unit_.GetRegWrite())
//         return;

//     // Determine the value to write
//     uint64_t value;

//     // FLW (opcode 0b0000111) - value comes from memory
//     if (opcode == 0b0000111) {
//         value = memory_result_;
//         // qDebug() << "WriteBackFloat FLW: Writing memory_result_" << value << "to FPR" << rd;
//     }
//     // FCVT.W.S / FCVT.WU.S (funct7=0b1100000) - float to int, write to GPR
//     else if (funct7 == 0b1100000) {
//         value = execution_result_;
//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 0; // GPR
//             change.reg_index = rd;
//             change.old_value = registers_->ReadGpr(rd);
//             change.new_value = value;
//             current_delta_.register_changes.push_back(change);
//         }
//         registers_->WriteGpr(rd, value);
//         emit gprUpdated(rd, value);
//         // qDebug() << "WriteBackFloat FCVT: Writing" << value << "to GPR" << rd;
//         return; // Don't write to FPR
//     }
//     // FMV.X.W (funct7=0b1110000) - move bits from FPR to GPR
//     else if (funct7 == 0b1110000) {
//         value = execution_result_;
//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 0; // GPR
//             change.reg_index = rd;
//             change.old_value = registers_->ReadGpr(rd);
//             change.new_value = value;
//             current_delta_.register_changes.push_back(change);
//         }
//         registers_->WriteGpr(rd, value);
//         emit gprUpdated(rd, value);
//         // qDebug() << "WriteBackFloat FMV.X.W: Writing" << value << "to GPR" << rd;
//         return;
//     }
//     // All other float ops - result goes to FPR
//     else {
//         value = execution_result_;
//     }

//     // Mask for RV32
//     // if (registers_->GetIsa() == ISA::RV32)
//     //     value &= 0xFFFFFFFF;

//     if (registers_->GetIsa() == ISA::RV32) {
//         // In RV32, single-precision values must be NaN-boxed:
//         // Upper 32 bits = all 1s, lower 32 bits = the float value
//         uint32_t float_bits = value & 0xFFFFFFFF;
//         value = 0xFFFFFFFF00000000ULL | float_bits;
//         // qDebug() << "RV32: NaN-boxing float value to" << QString::number(value, 16);
//     } else {
//         // In RV64, single-precision floats are also NaN-boxed
//         uint32_t float_bits = value & 0xFFFFFFFF;
//         value = 0xFFFFFFFF00000000ULL | float_bits;
//         // qDebug() << "RV64: NaN-boxing float value to" << QString::number(value, 16);
//     }

//     if (recording_enabled_) {
//         RegisterChange change;
//         change.reg_type = 2; // FPR
//         change.reg_index = rd;
//         change.old_value = registers_->ReadFpr(rd);
//         change.new_value = value;
//         current_delta_.register_changes.push_back(change);
//     }

//     registers_->WriteFpr(rd, value);
//     emit fprUpdated(rd, value);
//     // qDebug() << "WriteBackFloat: Writing" << QString::number(value, 16) << "to FPR" << rd;
// }

// void RVSSVM::WriteBackDouble()
// {
//     if (registers_->GetIsa() != ISA::RV64) {
//         // emit vmError("Double-precision FPR write not allowed in RV32");
//         return;
//     }

//     uint8_t opcode = current_instruction_ & 0b1111111;
//     uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//     uint8_t rd = (current_instruction_ >> 7) & 0b11111;

//     if (!control_unit_.GetRegWrite())
//         return;

//     uint64_t value;

//     // FLD (opcode 0b0000111) - value comes from memory
//     if (opcode == 0b0000111) {
//         value = memory_result_;
//         // qDebug() << "WriteBackDouble FLD: Writing memory_result_" << value << "to FPR" << rd;
//     }
//     // FCVT.W.D / FCVT.L.D etc (funct7=0b1100001) - double to int, write to GPR
//     else if (funct7 == 0b1100001) {
//         value = execution_result_;
//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 0; // GPR
//             change.reg_index = rd;
//             change.old_value = registers_->ReadGpr(rd);
//             change.new_value = value;
//             current_delta_.register_changes.push_back(change);
//         }
//         registers_->WriteGpr(rd, value);
//         emit gprUpdated(rd, value);
//         return;
//     }
//     // FMV.X.D (funct7=0b1110001) - move bits from FPR to GPR
//     else if (funct7 == 0b1110001) {
//         value = execution_result_;
//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 0; // GPR
//             change.reg_index = rd;
//             change.old_value = registers_->ReadGpr(rd);
//             change.new_value = value;
//             current_delta_.register_changes.push_back(change);
//         }
//         registers_->WriteGpr(rd, value);
//         emit gprUpdated(rd, value);
//         return;
//     }
//     else {
//         value = execution_result_;
//     }

//     if (recording_enabled_) {
//         RegisterChange change;
//         change.reg_type = 2; // FPR
//         change.reg_index = rd;
//         change.old_value = registers_->ReadFpr(rd);
//         change.new_value = value;
//         current_delta_.register_changes.push_back(change);
//     }

//     registers_->WriteFpr(rd, value);
//     emit fprUpdated(rd, value);
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
//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 1; // CSR
//             change.reg_index = csr_addr;
//             change.old_value = csr_old_value_;
//             change.new_value = csr_write_val_;
//             current_delta_.register_changes.push_back(change);
//         }
//         registers_->WriteCsr(csr_addr, csr_write_val_);
//         emit csrUpdated(csr_addr, csr_write_val_);
//         break;

//     case 0b010: // CSRRS
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_write_val_ != 0) {
//             if (recording_enabled_) {
//                 RegisterChange change;
//                 change.reg_type = 1;
//                 change.reg_index = csr_addr;
//                 change.old_value = csr_old_value_;
//                 change.new_value = csr_old_value_ | csr_write_val_;
//                 current_delta_.register_changes.push_back(change);
//             }
//             registers_->WriteCsr(csr_addr, csr_old_value_ | csr_write_val_);
//             emit csrUpdated(csr_addr, csr_old_value_ | csr_write_val_);
//         }
//         break;

//     case 0b011: // CSRRC
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_write_val_ != 0) {
//             if (recording_enabled_) {
//                 RegisterChange change;
//                 change.reg_type = 1;
//                 change.reg_index = csr_addr;
//                 change.old_value = csr_old_value_;
//                 change.new_value = csr_old_value_ & ~csr_write_val_;
//                 current_delta_.register_changes.push_back(change);
//             }
//             registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_write_val_);
//             emit csrUpdated(csr_addr, csr_old_value_ & ~csr_write_val_);
//         }
//         break;

//     case 0b101: // CSRRWI
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (recording_enabled_) {
//             RegisterChange change;
//             change.reg_type = 1;
//             change.reg_index = csr_addr;
//             change.old_value = csr_old_value_;
//             change.new_value = csr_uimm_;
//             current_delta_.register_changes.push_back(change);
//         }
//         registers_->WriteCsr(csr_addr, csr_uimm_);
//         emit csrUpdated(csr_addr, csr_uimm_);
//         break;

//     case 0b110: // CSRRSI
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_uimm_ != 0) {
//             if (recording_enabled_) {
//                 RegisterChange change;
//                 change.reg_type = 1;
//                 change.reg_index = csr_addr;
//                 change.old_value = csr_old_value_;
//                 change.new_value = csr_old_value_ | csr_uimm_;
//                 current_delta_.register_changes.push_back(change);
//             }
//             registers_->WriteCsr(csr_addr, csr_old_value_ | csr_uimm_);
//             emit csrUpdated(csr_addr, csr_old_value_ | csr_uimm_);
//         }
//         break;

//     case 0b111: // CSRRCI
//         registers_->WriteGpr(rd, csr_old_value_);
//         if (csr_uimm_ != 0) {
//             if (recording_enabled_) {
//                 RegisterChange change;
//                 change.reg_type = 1;
//                 change.reg_index = csr_addr;
//                 change.old_value = csr_old_value_;
//                 change.new_value = csr_old_value_ & ~csr_uimm_;
//                 current_delta_.register_changes.push_back(change);
//             }
//             registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_uimm_);
//             emit csrUpdated(csr_addr, csr_old_value_ & ~csr_uimm_);
//         }
//         break;
//     }
// }

// // VM control
// void RVSSVM::Run()
// {
//     ClearStop();
//     while (!stop_requested_ && program_counter_ < program_size_)
//     {
//         Fetch();
//         Decode();
//         Execute();
//         WriteMemory();
//         WriteBack();
//         instructions_retired_++;
//         cycle_s_++;
//     }
//     if (program_counter_ >= program_size_)
//         emit statusChanged("VM_PROGRAM_END");

//     DumpRegisters(globals::registers_dump_file_path,*registers_);
//     // qDebug() << "Dump Registers is finished";
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
//         /*while (!redo_stack_.empty())
//             redo_stack_.pop()*/;
//         current_delta_ = StepDelta();
//     }
//     if (program_counter_ >= program_size_)
//         emit statusChanged("VM_PROGRAM_END");
// }

// void RVSSVM::Step()
// {
//     // Prepare to record the current step's changes (delta)
//     current_delta_.old_pc = program_counter_;
//     current_delta_.register_changes.clear();
//     current_delta_.memory_changes.clear();

//     if (program_counter_ >= program_size_)
//         return;

//     // Enable recording for this step
//     recording_enabled_ = true;

//     // Execute one instruction cycle
//     Fetch();
//     Decode();
//     Execute();
//     WriteMemory();
//     WriteBack();

//     // Disable recording after step
//     recording_enabled_ = false;

//     // Update statistics
//     instructions_retired_++;
//     cycle_s_++;

//     // After step, save new PC
//     current_delta_.new_pc = program_counter_;

//     // Push this step delta to undo stack
//     undo_stack_.push(current_delta_);

//     // Clear redo stack
//     // while (!redo_stack_.empty())
//     //     redo_stack_.pop();

//     // Reset delta for next step
//     current_delta_ = StepDelta();

//     DumpRegisters(globals::registers_dump_file_path, *registers_);
//     // qDebug() << "Step completed";
// }

// void RVSSVM::Undo()
// {
//     if (undo_stack_.empty())
//         return;
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
//     // redo_stack_.push(last);

// }

// void RVSSVM::Reset()
// {
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

//     DumpRegisters(globals::registers_dump_file_path,*registers_);
//     // redo_stack_ = std::stack<StepDelta>();
// }


#include "rvss_vm.h"
#include "../../utils.h"
#include "../../globals.h"
#include "../../common/instructions.h"

#include <cctype>
#include <cstdint>
#include <qdebug.h>
#include <tuple>
#include <stack>
#include <atomic>
#include <QString>

// Proper Qt constructor/destructor
RVSSVM::RVSSVM(RegisterFile *sharedRegisters, QObject *parent)
    : QObject(parent), VmBase()
{
    registers_ = sharedRegisters;
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

    qDebug() << "=== EXECUTE STAGE ===";
    qDebug() << "PC:" << QString::number(program_counter_ - 4, 16)
             << "Instruction:" << QString::number(current_instruction_, 16);

    if (opcode == 0b1110011 && funct3 == 0b000) {
        qDebug() << ">>> SYSCALL DETECTED <<<";
        HandleSyscall();
        return;
    }

    if (instruction_set::isFInstruction(current_instruction_)) {
        qDebug() << ">>> FLOAT INSTRUCTION DETECTED <<<";
        ExecuteFloat();
        return;
    } else if (instruction_set::isDInstruction(current_instruction_)) {
        qDebug() << ">>> DOUBLE INSTRUCTION DETECTED <<<";
        if (registers_->GetIsa() == ISA::RV64)
            ExecuteDouble();
        else
            emit vmError("Double-precision not supported in RV32");
        return;
    } else if (opcode == 0b1110011) {
        qDebug() << ">>> CSR INSTRUCTION DETECTED <<<";
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
    if (opcode == 0b1100111 || opcode == 0b1101111) {
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
        case 0b000: takeBranch = (execution_result_ == 0); break;
        case 0b001: takeBranch = (execution_result_ != 0); break;
        case 0b100: takeBranch = (execution_result_ < 0); break;
        case 0b101: takeBranch = (execution_result_ >= 0); break;
        case 0b110: takeBranch = (static_cast<uint64_t>(reg1_value) < static_cast<uint64_t>(reg2_value)); break;
        case 0b111: takeBranch = (static_cast<uint64_t>(reg1_value) >= static_cast<uint64_t>(reg2_value)); break;
        }
        if (takeBranch) {
            UpdateProgramCounter(-4);
            UpdateProgramCounter(imm);
        }
    }

    if (opcode == 0b0010111) {
        execution_result_ = static_cast<int64_t>(program_counter_) - 4 + (imm << 12);
    }
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
    qDebug() << "Syscall number:" << syscall_number;

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

    qDebug() << "=== WRITE MEMORY STAGE ===";

    if (opcode == 0b1110011 && funct3 == 0b000)
        return;

    if (instruction_set::isFInstruction(current_instruction_)) {
        qDebug() << ">>> Calling WriteMemoryFloat()";
        WriteMemoryFloat();
        return;
    }
    else if (instruction_set::isDInstruction(current_instruction_)) {
        qDebug() << ">>> Calling WriteMemoryDouble()";
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

            switch (funct3) {
            case 0b000:
                mem_change.old_bytes_vec.push_back(memory_controller_.ReadByte(execution_result_));
                mem_change.new_bytes_vec.push_back(registers_->ReadGpr(rs2) & 0xFF);
                break;
            case 0b001:
            {
                uint16_t old_val = memory_controller_.ReadHalfWord(execution_result_);
                mem_change.old_bytes_vec.push_back(old_val & 0xFF);
                mem_change.old_bytes_vec.push_back((old_val >> 8) & 0xFF);
                uint16_t new_val = registers_->ReadGpr(rs2) & 0xFFFF;
                mem_change.new_bytes_vec.push_back(new_val & 0xFF);
                mem_change.new_bytes_vec.push_back((new_val >> 8) & 0xFF);
                break;
            }
            case 0b010:
            {
                uint32_t old_val = memory_controller_.ReadWord(execution_result_);
                for (int i = 0; i < 4; ++i)
                    mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
                uint32_t new_val = registers_->ReadGpr(rs2) & 0xFFFFFFFF;
                for (int i = 0; i < 4; ++i)
                    mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
                break;
            }
            case 0b011:
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
    }
}

void RVSSVM::WriteBack()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    int32_t imm = ImmGenerator(current_instruction_);

    qDebug() << "=== WRITE BACK STAGE ===";

    if (opcode == 0b1110011 && funct3 == 0b000)
        return;
    if (instruction_set::isFInstruction(current_instruction_)) {
        qDebug() << ">>> Calling WriteBackFloat()";
        WriteBackFloat();
        return;
    }
    else if (instruction_set::isDInstruction(current_instruction_)) {
        qDebug() << ">>> Calling WriteBackDouble()";
        if (registers_->GetIsa() == ISA::RV64)
            WriteBackDouble();
        else
            emit vmError("Double-precision writeback not supported in RV32");
        return;
    }
    else if (opcode == 0b1110011) {
        qDebug() << ">>> Calling WriteBackCsr()";
        WriteBackCsr();
        return;
    }

    if (control_unit_.GetRegWrite()) {
        uint64_t new_value = 0;
        switch (opcode) {
        case 0b0110011:
        case 0b0010011:
        case 0b0010111:
            new_value = execution_result_;
            break;
        case 0b0000011:
            new_value = memory_result_;
            break;
        case 0b1100111:
        case 0b1101111:
            new_value = next_pc_;
            break;
        case 0b0110111:
            new_value = (imm << 12);
            break;
        default:
            break;
        }

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 0;
            change.reg_index = rd;
            change.old_value = registers_->ReadGpr(rd);
            change.new_value = new_value;
            current_delta_.register_changes.push_back(change);
        }

        registers_->WriteGpr(rd, new_value);
        emit gprUpdated(rd, new_value);
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

    qDebug() << "\n========== ExecuteFloat() ==========";
    qDebug() << "Instruction:" << QString::number(current_instruction_, 16);
    qDebug() << "Opcode:" << QString::number(opcode, 2).rightJustified(7, '0');
    qDebug() << "Funct3:" << QString::number(funct3, 2).rightJustified(3, '0');
    qDebug() << "Funct7:" << QString::number(funct7, 2).rightJustified(7, '0');
    qDebug() << "rs1:" << rs1 << "rs2:" << rs2 << "rs3:" << rs3;

    uint8_t fcsr_status = 0;
    int32_t imm = ImmGenerator(current_instruction_);

    if (rm == 0b111) {
        rm = registers_->ReadCsr(0x002);
        qDebug() << "Using dynamic rounding mode from CSR:" << rm;
    } else {
        qDebug() << "Rounding Mode:" << rm;
    }

    // Handle FLW
    if (opcode == 0b0000111) {
        qDebug() << ">>> FLW Instruction (Load Float Word)";
        uint64_t base_addr = registers_->ReadGpr(rs1);
        qDebug() << "Base GPR[" << rs1 << "]:" << QString::number(base_addr, 16);
        qDebug() << "Immediate:" << imm;
        execution_result_ = base_addr + imm;
        qDebug() << "Target Address:" << QString::number(execution_result_, 16);
        qDebug() << "====================================\n";
        return;
    }

    // Handle FSW
    if (opcode == 0b0100111) {
        qDebug() << ">>> FSW Instruction (Store Float Word)";
        uint64_t base_addr = registers_->ReadGpr(rs1);
        uint64_t fpr_value = registers_->ReadFpr(rs2);
        qDebug() << "Base GPR[" << rs1 << "]:" << QString::number(base_addr, 16);
        qDebug() << "Immediate:" << imm;
        qDebug() << "FPR[" << rs2 << "]:" << QString::number(fpr_value, 16);

        uint32_t float_bits = fpr_value & 0xFFFFFFFF;
        float f_val;
        std::memcpy(&f_val, &float_bits, sizeof(float));
        qDebug() << "FPR[" << rs2 << "] as float:" << f_val;

        execution_result_ = base_addr + imm;
        qDebug() << "Target Address:" << QString::number(execution_result_, 16);
        qDebug() << "====================================\n";
        return;
    }

    // Read FPR values
    uint64_t reg1_value = registers_->ReadFpr(rs1);
    uint64_t reg2_value = registers_->ReadFpr(rs2);
    uint64_t reg3_value = registers_->ReadFpr(rs3);

    qDebug() << "FPR values (raw):";
    qDebug() << "FPR[" << rs1 << "]:" << QString::number(reg1_value, 16);
    qDebug() << "FPR[" << rs2 << "]:" << QString::number(reg2_value, 16);
    qDebug() << "FPR[" << rs3 << "]:" << QString::number(reg3_value, 16);

    // Validate NaN-boxing
    auto validate_sp = [](uint64_t val, int reg_num) -> uint64_t {
        if ((val & 0xFFFFFFFF00000000ULL) != 0xFFFFFFFF00000000ULL) {
            qDebug() << "Warning: FPR[" << reg_num << "] not NaN-boxed:" << QString::number(val, 16);
            return 0x7FC00000;
        }
        return val & 0xFFFFFFFF;
    };

    // Handle GPR source instructions
    if (funct7 == 0b1101000) {
        qDebug() << ">>> FCVT.S.W/WU - Reading from GPR";
        reg1_value = registers_->ReadGpr(rs1);
        qDebug() << "GPR[" << rs1 << "]:" << QString::number(reg1_value, 16) << "(" << (int32_t)reg1_value << ")";
    } else if (funct7 == 0b1111000) {
        qDebug() << ">>> FMV.W.X - Reading from GPR";
        reg1_value = registers_->ReadGpr(rs1);
        qDebug() << "GPR[" << rs1 << "]:" << QString::number(reg1_value, 16);
    } else {
        // Validate and decode float values
        reg1_value = validate_sp(reg1_value, rs1);
        reg2_value = validate_sp(reg2_value, rs2);
        reg3_value = validate_sp(reg3_value, rs3);

        float f1, f2, f3;
        uint32_t temp = reg1_value & 0xFFFFFFFF;
        std::memcpy(&f1, &temp, sizeof(float));
        temp = reg2_value & 0xFFFFFFFF;
        std::memcpy(&f2, &temp, sizeof(float));
        temp = reg3_value & 0xFFFFFFFF;
        std::memcpy(&f3, &temp, sizeof(float));

        qDebug() << "Float values: f1=" << f1 << "f2=" << f2 << "f3=" << f3;
    }

    if (control_unit_.GetAluSrc()) {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
        qDebug() << "Using immediate:" << imm;
    }

    alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
    // qDebug() << "ALU Operation:" << aluOperation;

    std::tie(execution_result_, fcsr_status) = alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    qDebug() << "Execution Result:" << QString::number(execution_result_, 16);
    qDebug() << "FCSR Status:" << QString::number(fcsr_status, 2).rightJustified(8, '0');

    if (funct7 != 0b1100000) {
        uint32_t temp = execution_result_ & 0xFFFFFFFF;
        float result_float;
        std::memcpy(&result_float, &temp, sizeof(float));
        qDebug() << "Result as float:" << result_float;
    } else {
        qDebug() << "Result as integer:" << (int32_t)execution_result_;
    }

    registers_->WriteCsr(0x003, fcsr_status);
    emit csrUpdated(0x003, fcsr_status);
    qDebug() << "====================================\n";
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

    qDebug() << "\n========== ExecuteDouble() ==========";
    qDebug() << "Instruction:" << QString::number(current_instruction_, 16);
    qDebug() << "Opcode:" << QString::number(opcode, 2).rightJustified(7, '0');
    qDebug() << "Funct3:" << QString::number(funct3, 2).rightJustified(3, '0');
    qDebug() << "Funct7:" << QString::number(funct7, 2).rightJustified(7, '0');
    qDebug() << "rs1:" << rs1 << "rs2:" << rs2 << "rs3:" << rs3;

    uint8_t fcsr_status = 0;
    int32_t imm = ImmGenerator(current_instruction_);

    if (rm == 0b111) {
        rm = registers_->ReadCsr(0x002);
        qDebug() << "Using dynamic rounding mode from CSR:" << rm;
    } else {
        qDebug() << "Rounding Mode:" << rm;
    }

    // Handle FLD
    if (opcode == 0b0000111) {
        qDebug() << ">>> FLD Instruction (Load Double)";
        uint64_t base_addr = registers_->ReadGpr(rs1);
        qDebug() << "Base GPR[" << rs1 << "]:" << QString::number(base_addr, 16);
        qDebug() << "Immediate:" << imm;
        execution_result_ = base_addr + imm;
        qDebug() << "Target Address:" << QString::number(execution_result_, 16);
        qDebug() << "====================================\n";
        return;
    }

    // Handle FSD
    if (opcode == 0b0100111) {
        qDebug() << ">>> FSD Instruction (Store Double)";
        uint64_t base_addr = registers_->ReadGpr(rs1);
        uint64_t fpr_value = registers_->ReadFpr(rs2);
        qDebug() << "Base GPR[" << rs1 << "]:" << QString::number(base_addr, 16);
        qDebug() << "Immediate:" << imm;
        qDebug() << "FPR[" << rs2 << "]:" << QString::number(fpr_value, 16);

        double d_val;
        std::memcpy(&d_val, &fpr_value, sizeof(double));
        qDebug() << "FPR[" << rs2 << "] as double:" << d_val;

        execution_result_ = base_addr + imm;
        qDebug() << "Target Address:" << QString::number(execution_result_, 16);
        qDebug() << "====================================\n";
        return;
    }

    // Read FPR values
    uint64_t reg1_value = registers_->ReadFpr(rs1);
    uint64_t reg2_value = registers_->ReadFpr(rs2);
    uint64_t reg3_value = registers_->ReadFpr(rs3);

    qDebug() << "FPR values (raw):";
    qDebug() << "FPR[" << rs1 << "]:" << QString::number(reg1_value, 16);
    qDebug() << "FPR[" << rs2 << "]:" << QString::number(reg2_value, 16);
    qDebug() << "FPR[" << rs3 << "]:" << QString::number(reg3_value, 16);

    // Handle GPR source instructions
    if (funct7 == 0b1101001) {
        qDebug() << ">>> FCVT.D.W/WU/L/LU - Reading from GPR";
        reg1_value = registers_->ReadGpr(rs1);
        qDebug() << "GPR[" << rs1 << "]:" << QString::number(reg1_value, 16) << "(" << (int64_t)reg1_value << ")";
    } else if (funct7 == 0b1111001) {
        qDebug() << ">>> FMV.D.X - Reading from GPR";
        reg1_value = registers_->ReadGpr(rs1);
        qDebug() << "GPR[" << rs1 << "]:" << QString::number(reg1_value, 16);
    } else {
        // Decode double values
        double d1, d2, d3;
        std::memcpy(&d1, &reg1_value, sizeof(double));
        std::memcpy(&d2, &reg2_value, sizeof(double));
        std::memcpy(&d3, &reg3_value, sizeof(double));

        qDebug() << "Double values: d1=" << d1 << "d2=" << d2 << "d3=" << d3;
    }

    if (control_unit_.GetAluSrc()) {
        reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
        qDebug() << "Using immediate:" << imm;
    }

    alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
    // qDebug() << "ALU Operation:" << aluOperation;

    std::tie(execution_result_, fcsr_status) = alu::Alu::dfpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

    qDebug() << "Execution Result:" << QString::number(execution_result_, 16);
    qDebug() << "FCSR Status:" << QString::number(fcsr_status, 2).rightJustified(8, '0');

    if (funct7 != 0b1100001) {
        double result_double;
        std::memcpy(&result_double, &execution_result_, sizeof(double));
        qDebug() << "Result as double:" << result_double;
    } else {
        qDebug() << "Result as integer:" << (int64_t)execution_result_;
    }

    registers_->WriteCsr(0x003, fcsr_status);
    emit csrUpdated(0x003, fcsr_status);
    qDebug() << "====================================\n";
}

void RVSSVM::WriteMemoryFloat()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

    qDebug() << "\n========== WriteMemoryFloat() ==========";
    qDebug() << "Address:" << QString::number(execution_result_, 16);

    if (control_unit_.GetMemRead()) {
        qDebug() << ">>> Memory READ (FLW)";
        uint32_t raw_value = memory_controller_.ReadWord(execution_result_);
        qDebug() << "Raw value from memory:" << QString::number(raw_value, 16);

        float f_val;
        std::memcpy(&f_val, &raw_value, sizeof(float));
        qDebug() << "Value as float:" << f_val;

        memory_result_ = 0xFFFFFFFF00000000ULL | raw_value;
        qDebug() << "NaN-boxed result:" << QString::number(memory_result_, 16);
    }

    if (control_unit_.GetMemWrite()) {
        qDebug() << ">>> Memory WRITE (FSW)";
        uint64_t fpr_full = registers_->ReadFpr(rs2);
        uint32_t float_bits = fpr_full & 0xFFFFFFFF;

        qDebug() << "FPR[" << rs2 << "] full:" << QString::number(fpr_full, 16);
        qDebug() << "Float bits to store:" << QString::number(float_bits, 16);

        float f_val;
        std::memcpy(&f_val, &float_bits, sizeof(float));
        qDebug() << "Value as float:" << f_val;

        if (recording_enabled_) {
            MemoryChange mem_change;
            mem_change.address = execution_result_;
            uint32_t old_val = memory_controller_.ReadWord(execution_result_);
            qDebug() << "Old memory value:" << QString::number(old_val, 16);

            float old_f;
            std::memcpy(&old_f, &old_val, sizeof(float));
            qDebug() << "Old value as float:" << old_f;

            for (int i = 0; i < 4; ++i)
                mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
            for (int i = 0; i < 4; ++i)
                mem_change.new_bytes_vec.push_back((float_bits >> (i * 8)) & 0xFF);
            current_delta_.memory_changes.push_back(mem_change);
        }

        memory_controller_.WriteWord(execution_result_, float_bits);
        qDebug() << "Stored to memory at" << QString::number(execution_result_, 16);
    }
    qDebug() << "========================================\n";
}

void RVSSVM::WriteMemoryDouble()
{
    if (registers_->GetIsa() != ISA::RV64) {
        emit vmError("Double-precision store/read not supported in RV32");
        return;
    }

    uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

    qDebug() << "\n========== WriteMemoryDouble() ==========";
    qDebug() << "Address:" << QString::number(execution_result_, 16);

    if (control_unit_.GetMemRead()) {
        qDebug() << ">>> Memory READ (FLD)";
        memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
        qDebug() << "Raw value from memory:" << QString::number(memory_result_, 16);

        double d_val;
        std::memcpy(&d_val, &memory_result_, sizeof(double));
        qDebug() << "Value as double:" << d_val;
    }

    if (control_unit_.GetMemWrite()) {
        qDebug() << ">>> Memory WRITE (FSD)";
        uint64_t fpr_value = registers_->ReadFpr(rs2);
        qDebug() << "FPR[" << rs2 << "]:" << QString::number(fpr_value, 16);

        double d_val;
        std::memcpy(&d_val, &fpr_value, sizeof(double));
        qDebug() << "Value as double:" << d_val;

        if (recording_enabled_) {
            MemoryChange mem_change;
            mem_change.address = execution_result_;
            uint64_t old_val = memory_controller_.ReadDoubleWord(execution_result_);
            qDebug() << "Old memory value:" << QString::number(old_val, 16);

            double old_d;
            std::memcpy(&old_d, &old_val, sizeof(double));
            qDebug() << "Old value as double:" << old_d;

            for (int i = 0; i < 8; ++i)
                mem_change.old_bytes_vec.push_back((old_val >> (i * 8)) & 0xFF);
            uint64_t new_val = registers_->ReadFpr(rs2);
            for (int i = 0; i < 8; ++i)
                mem_change.new_bytes_vec.push_back((new_val >> (i * 8)) & 0xFF);
            current_delta_.memory_changes.push_back(mem_change);
        }

        memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
        qDebug() << "Stored to memory at" << QString::number(execution_result_, 16);
    }
    qDebug() << "========================================\n";
}

void RVSSVM::WriteBackFloat()
{
    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;

    qDebug() << "\n========== WriteBackFloat() ==========";
    qDebug() << "rd:" << rd << "Opcode:" << QString::number(opcode, 2).rightJustified(7, '0')
             << "Funct7:" << QString::number(funct7, 2).rightJustified(7, '0');

    if (!control_unit_.GetRegWrite()) {
        qDebug() << "RegWrite not enabled, skipping";
        qDebug() << "======================================\n";
        return;
    }

    uint64_t value;

    if (opcode == 0b0000111) {
        qDebug() << ">>> FLW Writeback";
        value = memory_result_;
        qDebug() << "memory_result_:" << QString::number(value, 16);

        uint32_t float_bits = value & 0xFFFFFFFF;
        float f_val;
        std::memcpy(&f_val, &float_bits, sizeof(float));
        qDebug() << "Value as float:" << f_val;
    }
    else if (funct7 == 0b1100000) {
        qDebug() << ">>> FCVT.W.S/WU.S - Writing to GPR";
        value = execution_result_;
        qDebug() << "Integer result:" << (int32_t)value;

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 0;
            change.reg_index = rd;
            change.old_value = registers_->ReadGpr(rd);
            change.new_value = value;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteGpr(rd, value);
        emit gprUpdated(rd, value);
        qDebug() << "Written to GPR[" << rd << "]:" << value;
        qDebug() << "======================================\n";
        return;
    }
    else if (funct7 == 0b1110000) {
        qDebug() << ">>> FMV.X.W - Writing to GPR";
        value = execution_result_;
        qDebug() << "Bit pattern:" << QString::number(value, 16);

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 0;
            change.reg_index = rd;
            change.old_value = registers_->ReadGpr(rd);
            change.new_value = value;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteGpr(rd, value);
        emit gprUpdated(rd, value);
        qDebug() << "Written to GPR[" << rd << "]:" << QString::number(value, 16);
        qDebug() << "======================================\n";
        return;
    }
    else {
        qDebug() << ">>> Standard float operation";
        value = execution_result_;
        qDebug() << "execution_result_:" << QString::number(value, 16);
    }

    // NaN-boxing
    uint32_t float_bits = value & 0xFFFFFFFF;
    value = 0xFFFFFFFF00000000ULL | float_bits;
    qDebug() << "After NaN-boxing:" << QString::number(value, 16);

    float f_val;
    std::memcpy(&f_val, &float_bits, sizeof(float));
    qDebug() << "Value as float:" << f_val;

    if (recording_enabled_) {
        RegisterChange change;
        change.reg_type = 2;
        change.reg_index = rd;
        change.old_value = registers_->ReadFpr(rd);
        change.new_value = value;
        current_delta_.register_changes.push_back(change);
    }

    registers_->WriteFpr(rd, value);
    emit fprUpdated(rd, value);
    qDebug() << "Written to FPR[" << rd << "]:" << QString::number(value, 16);
    qDebug() << "======================================\n";
}

void RVSSVM::WriteBackDouble()
{
    if (registers_->GetIsa() != ISA::RV64) {
        return;
    }

    uint8_t opcode = current_instruction_ & 0b1111111;
    uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;

    qDebug() << "\n========== WriteBackDouble() ==========";
    qDebug() << "rd:" << rd << "Opcode:" << QString::number(opcode, 2).rightJustified(7, '0')
             << "Funct7:" << QString::number(funct7, 2).rightJustified(7, '0');

    if (!control_unit_.GetRegWrite()) {
        qDebug() << "RegWrite not enabled, skipping";
        qDebug() << "=======================================\n";
        return;
    }

    uint64_t value;

    if (opcode == 0b0000111) {
        qDebug() << ">>> FLD Writeback";
        value = memory_result_;
        qDebug() << "memory_result_:" << QString::number(value, 16);

        double d_val;
        std::memcpy(&d_val, &value, sizeof(double));
        qDebug() << "Value as double:" << d_val;
    }
    else if (funct7 == 0b1100001) {
        qDebug() << ">>> FCVT.W.D/L.D - Writing to GPR";
        value = execution_result_;
        qDebug() << "Integer result:" << (int64_t)value;

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 0;
            change.reg_index = rd;
            change.old_value = registers_->ReadGpr(rd);
            change.new_value = value;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteGpr(rd, value);
        emit gprUpdated(rd, value);
        qDebug() << "Written to GPR[" << rd << "]:" << value;
        qDebug() << "=======================================\n";
        return;
    }
    else if (funct7 == 0b1110001) {
        qDebug() << ">>> FMV.X.D - Writing to GPR";
        value = execution_result_;
        qDebug() << "Bit pattern:" << QString::number(value, 16);

        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 0;
            change.reg_index = rd;
            change.old_value = registers_->ReadGpr(rd);
            change.new_value = value;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteGpr(rd, value);
        emit gprUpdated(rd, value);
        qDebug() << "Written to GPR[" << rd << "]:" << QString::number(value, 16);
        qDebug() << "=======================================\n";
        return;
    }
    else {
        qDebug() << ">>> Standard double operation";
        value = execution_result_;
        qDebug() << "execution_result_:" << QString::number(value, 16);

        double d_val;
        std::memcpy(&d_val, &value, sizeof(double));
        qDebug() << "Value as double:" << d_val;
    }

    if (recording_enabled_) {
        RegisterChange change;
        change.reg_type = 2;
        change.reg_index = rd;
        change.old_value = registers_->ReadFpr(rd);
        change.new_value = value;

        double old_d, new_d;
        std::memcpy(&old_d, &change.old_value, sizeof(double));
        std::memcpy(&new_d, &change.new_value, sizeof(double));
        qDebug() << "old:" << old_d << "-> new:" << new_d;

        current_delta_.register_changes.push_back(change);
    }

    registers_->WriteFpr(rd, value);
    emit fprUpdated(rd, value);
    qDebug() << "Written to FPR[" << rd << "]:" << QString::number(value, 16);
    qDebug() << "=======================================\n";
}

void RVSSVM::WriteBackCsr()
{
    uint8_t rd = (current_instruction_ >> 7) & 0b11111;
    uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
    uint16_t csr_addr = csr_target_address_;

    qDebug() << "\n=== WriteBackCsr() ===";
    qDebug() << "CSR:" << QString::number(csr_addr, 16) << "rd:" << rd << "funct3:" << funct3;

    switch (funct3)
    {
    case 0b001: // CSRRW
        qDebug() << ">>> CSRRW";
        registers_->WriteGpr(rd, csr_old_value_);
        if (recording_enabled_) {
            RegisterChange change;
            change.reg_type = 1;
            change.reg_index = csr_addr;
            change.old_value = csr_old_value_;
            change.new_value = csr_write_val_;
            current_delta_.register_changes.push_back(change);
        }
        registers_->WriteCsr(csr_addr, csr_write_val_);
        emit csrUpdated(csr_addr, csr_write_val_);
        break;

    case 0b010: // CSRRS
        qDebug() << ">>> CSRRS";
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
        qDebug() << ">>> CSRRC";
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
        qDebug() << ">>> CSRRWI";
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
        qDebug() << ">>> CSRRSI";
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
        qDebug() << ">>> CSRRCI";
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
    qDebug() << "======================\n";
}

void RVSSVM::Run()
{
    qDebug() << "\n***** RUN MODE STARTED *****\n";
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

    DumpRegisters(globals::registers_dump_file_path, *registers_);
    qDebug() << "\n***** RUN MODE ENDED *****";
    qDebug() << "Instructions:" << instructions_retired_ << "Cycles:" << cycle_s_ << "\n";
}

void RVSSVM::DebugRun()
{
    qDebug() << "\n***** DEBUG RUN MODE STARTED *****\n";
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
        current_delta_ = StepDelta();
    }
    if (program_counter_ >= program_size_)
        emit statusChanged("VM_PROGRAM_END");

    qDebug() << "\n***** DEBUG RUN ENDED *****";
    qDebug() << "Instructions:" << instructions_retired_ << "Cycles:" << cycle_s_ << "\n";
}

void RVSSVM::Step()
{
    qDebug() << "\n╔════════════════════════════════════════╗";
    qDebug() << "║          STEP EXECUTION START          ║";
    qDebug() << "╚════════════════════════════════════════╝";
    qDebug() << "PC:" << QString::number(program_counter_, 16);
    qDebug() << "Instruction count:" << instructions_retired_;

    current_delta_.old_pc = program_counter_;
    current_delta_.register_changes.clear();
    current_delta_.memory_changes.clear();

    if (program_counter_ >= program_size_) {
        qDebug() << "PC beyond program size";
        return;
    }

    recording_enabled_ = true;

    Fetch();
    Decode();
    Execute();
    WriteMemory();
    WriteBack();

    recording_enabled_ = false;

    instructions_retired_++;
    cycle_s_++;
    current_delta_.new_pc = program_counter_;

    qDebug() << "\nStep Summary:";
    qDebug() << "  Old PC:" << QString::number(current_delta_.old_pc, 16);
    qDebug() << "  New PC:" << QString::number(current_delta_.new_pc, 16);
    qDebug() << "  Reg changes:" << current_delta_.register_changes.size();
    qDebug() << "  Mem changes:" << current_delta_.memory_changes.size();

    for (const auto &change : current_delta_.register_changes) {
        QString reg_type;
        switch (change.reg_type) {
        case 0: reg_type = "GPR"; break;
        case 1: reg_type = "CSR"; break;
        case 2: reg_type = "FPR"; break;
        default: reg_type = "???"; break;
        }
        qDebug() << "    " << reg_type << "[" << change.reg_index << "]:"
                 << QString::number(change.old_value, 16) << "->"
                 << QString::number(change.new_value, 16);
    }

    undo_stack_.push(current_delta_);
    current_delta_ = StepDelta();

    DumpRegisters(globals::registers_dump_file_path, *registers_);

    qDebug() << "╔════════════════════════════════════════╗";
    qDebug() << "║          STEP EXECUTION END            ║";
    qDebug() << "╚════════════════════════════════════════╝\n";
}

void RVSSVM::Undo()
{
    qDebug() << "\n=== UNDO ===";

    if (undo_stack_.empty()) {
        qDebug() << "Undo stack empty";
        return;
    }

    StepDelta last = undo_stack_.top();
    undo_stack_.pop();

    qDebug() << "Undoing PC" << QString::number(last.old_pc, 16)
             << "->" << QString::number(last.new_pc, 16);
    qDebug() << "Reg changes:" << last.register_changes.size();
    qDebug() << "Mem changes:" << last.memory_changes.size();

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

            if (registers_->GetIsa() == ISA::RV32 ||
                (change.old_value & 0xFFFFFFFF00000000ULL) == 0xFFFFFFFF00000000ULL) {
                uint32_t float_bits = change.old_value & 0xFFFFFFFF;
                float f_val;
                std::memcpy(&f_val, &float_bits, sizeof(float));
                qDebug() << "  Restored FPR[" << change.reg_index << "] float:" << f_val;
            } else {
                double d_val;
                std::memcpy(&d_val, &change.old_value, sizeof(double));
                qDebug() << "  Restored FPR[" << change.reg_index << "] double:" << d_val;
            }
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

    qDebug() << "PC restored to:" << QString::number(program_counter_, 16);
    qDebug() << "============\n";
}

void RVSSVM::Reset()
{
    qDebug() << "\n***** RESET *****";

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

    DumpRegisters(globals::registers_dump_file_path, *registers_);

    qDebug() << "Reset complete";
    qDebug() << "*****************\n";
}
