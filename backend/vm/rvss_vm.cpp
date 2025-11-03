// #include "rvss_vm.h"
// // #include "../../utils.h"
// // #include "../../globals.h"
// #include "../../common/instructions.h"
// // #include "../../config.h"

// #include <cctype>
// #include <cstdint>
// #include <tuple>
// #include <stack>
// #include <string>
// // #include <algorithm>
// // #include <thread>
// // #include <mutex>
// // #include <condition_variable>
// #include <atomic>
// #include <QString>

// // Proper Qt constructor/destructor
// RVSSVM::RVSSVM(RegisterFile *sharedRegisters, QObject *parent)
//     : QObject(parent), VmBase()
// {
//   registers_ = sharedRegisters; // Use the shared instance
// }
// RVSSVM::~RVSSVM() = default;

// void RVSSVM::Fetch()
// {
//   current_instruction_ = memory_controller_.ReadWord(program_counter_);
//   UpdateProgramCounter(4);
// }

// void RVSSVM::Decode()
// {
//   control_unit_.SetControlSignals(current_instruction_);
// }

// void RVSSVM::Execute()
// {
//     std::string str = registers_->GetIsa()  == ISA::RV32 ?  "Yes" : "NO";
//     std::cout << str << std::endl;
//   // uint8_t opcode = current_instruction_ & 0b1111111;
//   // uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//   // if (opcode == 0b1110011 && funct3 == 0b000)
//   // {
//   //   HandleSyscall();
//   //   return;
//   // }

//   // if (instruction_set::isFInstruction(current_instruction_))
//   // {
//   //   ExecuteFloat();
//   //   return;
//   // }
//   // else if (instruction_set::isDInstruction(current_instruction_))
//   // {
//   //   ExecuteDouble();
//   //   return;
//   // }
//   // else if (opcode == 0b1110011)
//   // {
//   //   ExecuteCsr();
//   //   return;
//   // }

//   // uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//   // uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//   // int32_t imm = ImmGenerator(current_instruction_);

//   // uint64_t reg1_value = registers_->ReadGpr(rs1);
//   // uint64_t reg2_value = registers_->ReadGpr(rs2);

//   // bool overflow = false;

//   // if (control_unit_.GetAluSrc())
//   // {
//   //   reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
//   // }

//   // alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//   // std::tie(execution_result_, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);

//   uint8_t opcode = current_instruction_ & 0b1111111;
//   uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//   if (opcode == 0b1110011 && funct3 == 0b000) {
//       HandleSyscall();
//       return;
//   }

//   if (instruction_set::isFInstruction(current_instruction_)) {
//       ExecuteFloat();
//       return;
//   }
//   else if (instruction_set::isDInstruction(current_instruction_)) {
//       if (registers_->GetIsa() == ISA::RV64) {
//           ExecuteDouble();
//       } else {
//           emit vmError("Double-precision not supported in RV32");
//       }
//       return;
//   }
//   else if (opcode == 0b1110011) {
//       ExecuteCsr();
//       return;
//   }

//   uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//   uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//   int32_t imm = ImmGenerator(current_instruction_);

//   uint64_t reg1_value = registers_->ReadGpr(rs1);
//   uint64_t reg2_value = registers_->ReadGpr(rs2);

//   bool overflow = false;
//   if (control_unit_.GetAluSrc()) {
//       reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
//   }

//   alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//   std::tie(execution_result_, overflow) = alu_.execute(aluOperation, reg1_value, reg2_value);

//   // Mask for RV32
//   // if (registers_->GetIsa() == ISA::RV32)
//   //     execution_result_ &= 0xFFFFFFFF;

//   // branch logic (JAL, JALR, BEQ, etc.)
//   if (control_unit_.GetBranch())
//   {
//     if (opcode == 0b1100111 || opcode == 0b1101111)
//     {
//       next_pc_ = static_cast<int64_t>(program_counter_);
//       UpdateProgramCounter(-4);
//       return_address_ = program_counter_ + 4;
//       if (opcode == 0b1100111)
//       {
//         UpdateProgramCounter(-program_counter_ + execution_result_);
//       }
//       else
//       {
//         UpdateProgramCounter(imm);
//       }
//     }
//     // else if (instruction_set::isBranchInstruction(opcode))
//     else if (opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbeq).opcode ||
//              opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbne).opcode ||
//              opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kblt).opcode ||
//              opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbge).opcode ||
//              opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbltu).opcode ||
//              opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbgeu).opcode)
//     {
//       switch (funct3)
//       {
//       case 0b000:
//         branch_flag_ = (execution_result_ == 0);
//         break; // BEQ
//       case 0b001:
//         branch_flag_ = (execution_result_ != 0);
//         break; // BNE
//       case 0b100:
//         branch_flag_ = (execution_result_ == 1);
//         break; // BLT
//       case 0b101:
//         branch_flag_ = (execution_result_ == 0);
//         break; // BGE
//       case 0b110:
//         branch_flag_ = (execution_result_ == 1);
//         break; // BLTU
//       case 0b111:
//         branch_flag_ = (execution_result_ == 0);
//         break; // BGEU
//       }
//     }
//   }

//   if (branch_flag_ && /*instruction_set::isBranchInstruction(opcode)*/
//           opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbeq).opcode ||
//       opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbne).opcode ||
//       opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kblt).opcode ||
//       opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbge).opcode ||
//       opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbltu).opcode ||
//       opcode == instruction_set::instruction_encoding_map.at(instruction_set::Instruction::kbgeu).opcode)
//   {
//     UpdateProgramCounter(-4);
//     UpdateProgramCounter(imm);
//   }

//   if (opcode == 0b0010111)
//   { // AUIPC
//     execution_result_ = static_cast<int64_t>(program_counter_) - 4 + (imm << 12);
//   }
// }

// void RVSSVM::ExecuteFloat()
// {
//   uint8_t opcode = current_instruction_ & 0b1111111;
//   uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//   uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//   uint8_t rm = funct3;
//   uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//   uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//   uint8_t rs3 = (current_instruction_ >> 27) & 0b11111;

//   uint8_t fcsr_status = 0;
//   int32_t imm = ImmGenerator(current_instruction_);

//   if (rm == 0b111)
//     rm = registers_->ReadCsr(0x002);

//   uint64_t reg1_value = registers_->ReadFpr(rs1);
//   uint64_t reg2_value = registers_->ReadFpr(rs2);
//   uint64_t reg3_value = registers_->ReadFpr(rs3);

//   if (funct7 == 0b1101000 || funct7 == 0b1111000 || opcode == 0b0000111 || opcode == 0b0100111)
//   {
//     reg1_value = registers_->ReadGpr(rs1);
//   }

//   if (control_unit_.GetAluSrc())
//   {
//     reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
//   }

//   alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//   std::tie(execution_result_, fcsr_status) = alu::Alu::fpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);

//   registers_->WriteCsr(0x003, fcsr_status);
//   emit csrUpdated(0x003, fcsr_status);
// }

// void RVSSVM::ExecuteDouble()
// {
//   uint8_t opcode = current_instruction_ & 0b1111111;
//   uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//   uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
//   uint8_t rm = funct3;
//   uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//   uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//   uint8_t rs3 = (current_instruction_ >> 27) & 0b11111;

//   uint8_t fcsr_status = 0;
//   int32_t imm = ImmGenerator(current_instruction_);

//   uint64_t reg1_value = registers_->ReadFpr(rs1);
//   uint64_t reg2_value = registers_->ReadFpr(rs2);
//   uint64_t reg3_value = registers_->ReadFpr(rs3);

//   if (funct7 == 0b1101001 || funct7 == 0b1111001 || opcode == 0b0000111 || opcode == 0b0100111)
//   {
//     reg1_value = registers_->ReadGpr(rs1);
//   }

//   if (control_unit_.GetAluSrc())
//   {
//     reg2_value = static_cast<uint64_t>(static_cast<int64_t>(imm));
//   }

//   alu::AluOp aluOperation = control_unit_.GetAluSignal(current_instruction_, control_unit_.GetAluOp());
//   std::tie(execution_result_, fcsr_status) = alu::Alu::dfpexecute(aluOperation, reg1_value, reg2_value, reg3_value, rm);
// }

// void RVSSVM::ExecuteCsr()
// {
//   uint8_t rs1 = (current_instruction_ >> 15) & 0b11111;
//   uint16_t csr = (current_instruction_ >> 20) & 0xFFF;
//   uint64_t csr_val = registers_->ReadCsr(csr);

//   csr_target_address_ = csr;
//   csr_old_value_ = csr_val;
//   csr_write_val_ = registers_->ReadGpr(rs1);
//   csr_uimm_ = rs1;
// }

// void RVSSVM::HandleSyscall()
// {
//   uint64_t syscall_number = registers_->ReadGpr(17);
//   switch (syscall_number)
//   {
//   case SYSCALL_PRINT_INT:
//     emit syscallOutput(QString("[Syscall output: %1]").arg((qint64)registers_->ReadGpr(10)));
//     break;
//   case SYSCALL_PRINT_FLOAT:
//   {
//     float float_value;
//     uint64_t raw = registers_->ReadGpr(10);
//     std::memcpy(&float_value, &raw, sizeof(float_value));
//     emit syscallOutput(QString("[Syscall output: %1]").arg(float_value));
//     break;
//   }
//   case SYSCALL_PRINT_DOUBLE:
//   {
//     double double_value;
//     uint64_t raw = registers_->ReadGpr(10);
//     std::memcpy(&double_value, &raw, sizeof(double_value));
//     emit syscallOutput(QString("[Syscall output: %1]").arg(double_value));
//     break;
//   }
//   case SYSCALL_PRINT_STRING:
//     emit syscallOutput("[Syscall output: ...string... (not implemented yet)]");
//     break;
//   case SYSCALL_EXIT:
//     stop_requested_ = true;
//     emit vmError(QString("VM exited with code: %1").arg(registers_->ReadGpr(10)));
//     break;
//   default:
//     emit vmError(QString("Unknown syscall: %1").arg(syscall_number));
//     break;
//   }
// }

// // void RVSSVM::WriteMemory() {
// //     // Use logic from your paste (see original for details)
// //     // ... typical memory load/store code ...
// // }

// // void RVSSVM::WriteMemoryFloat() {
// //     // ... as in your original logic ...
// // }
// // void RVSSVM::WriteMemoryDouble() {
// //     // ... as in your original logic ...
// // }

// void RVSSVM::WriteMemory()
// {
//   // uint8_t opcode = current_instruction_ & 0b1111111;
//   // uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//   // uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//   // if (opcode == 0b1110011 && funct3 == 0b000)
//   // {
//   //   return;
//   // }

//   // if (instruction_set::isFInstruction(current_instruction_))
//   // { // RV64 F
//   //   WriteMemoryFloat();
//   //   return;
//   // }
//   // else if (instruction_set::isDInstruction(current_instruction_))
//   // {
//   //   WriteMemoryDouble();
//   //   return;
//   // }

//   // if (control_unit_.GetMemRead())
//   // {
//   //   switch (funct3)
//   //   {
//   //   case 0b000:
//   //   { // LB
//   //     memory_result_ = static_cast<int8_t>(memory_controller_.ReadByte(execution_result_));
//   //     break;
//   //   }
//   //   case 0b001:
//   //   { // LH
//   //     memory_result_ = static_cast<int16_t>(memory_controller_.ReadHalfWord(execution_result_));
//   //     break;
//   //   }
//   //   case 0b010:
//   //   { // LW
//   //     memory_result_ = static_cast<int32_t>(memory_controller_.ReadWord(execution_result_));
//   //     break;
//   //   }
//   //   case 0b011:
//   //   { // LD
//   //     memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
//   //     break;
//   //   }
//   //   case 0b100:
//   //   { // LBU
//   //     memory_result_ = static_cast<uint8_t>(memory_controller_.ReadByte(execution_result_));
//   //     break;
//   //   }
//   //   case 0b101:
//   //   { // LHU
//   //     memory_result_ = static_cast<uint16_t>(memory_controller_.ReadHalfWord(execution_result_));
//   //     break;
//   //   }
//   //   case 0b110:
//   //   { // LWU
//   //     memory_result_ = static_cast<uint32_t>(memory_controller_.ReadWord(execution_result_));
//   //     break;
//   //   }
//   //   }
//   // }

//   uint8_t opcode = current_instruction_ & 0b1111111;
//   uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;
//   uint8_t funct3 = (current_instruction_ >> 12) & 0b111;

//   if (opcode == 0b1110011 && funct3 == 0b000)
//       return;

//   if (instruction_set::isFInstruction(current_instruction_)) {
//       WriteMemoryFloat();
//       return;
//   }
//   else if (instruction_set::isDInstruction(current_instruction_)) {
//       if (registers_->GetIsa() == ISA::RV64)
//           WriteMemoryDouble();
//       else
//           emit vmError("Double-precision stores not supported in RV32");
//       return;
//   }

//   if (control_unit_.GetMemRead()) {
//       switch (funct3) {
//       case 0b000: memory_result_ = static_cast<int8_t>(memory_controller_.ReadByte(execution_result_)); break;
//       case 0b001: memory_result_ = static_cast<int16_t>(memory_controller_.ReadHalfWord(execution_result_)); break;
//       case 0b010: memory_result_ = static_cast<int32_t>(memory_controller_.ReadWord(execution_result_)); break;
//       case 0b011:
//           if (registers_->GetIsa() == ISA::RV64)
//               memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
//           break;
//       case 0b100: memory_result_ = static_cast<uint8_t>(memory_controller_.ReadByte(execution_result_)); break;
//       case 0b101: memory_result_ = static_cast<uint16_t>(memory_controller_.ReadHalfWord(execution_result_)); break;
//       case 0b110: memory_result_ = static_cast<uint32_t>(memory_controller_.ReadWord(execution_result_)); break;
//       }
//   }

//   // Write logic...
//   if (control_unit_.GetMemWrite()) {
//       switch (funct3) {
//       case 0b000: memory_controller_.WriteByte(execution_result_, registers_->ReadGpr(rs2) & 0xFF); break;
//       case 0b001: memory_controller_.WriteHalfWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFF); break;
//       case 0b010: memory_controller_.WriteWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFFFFFF); break;
//       case 0b011:
//           if (registers_->GetIsa() == ISA::RV64)
//               memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadGpr(rs2));
//           break;
//       }

//   uint64_t writeAddress = 0;
//   QVector<uint8_t> writtenBytes;

//   uint64_t addr = 0;
//   std::vector<uint8_t> old_bytes_vec;
//   std::vector<uint8_t> new_bytes_vec;

//   // TODO: use direct read to read memory for undo/redo functionality, i.e. ReadByte -> ReadByte_d

//   if (control_unit_.GetMemWrite())
//   {
//     switch (funct3)
//     {
//     case 0b000:
//     { // SB
//       addr = execution_result_;
//       old_bytes_vec.push_back(memory_controller_.ReadByte(addr));
//       memory_controller_.WriteByte(execution_result_, registers_->ReadGpr(rs2) & 0xFF);
//       new_bytes_vec.push_back(memory_controller_.ReadByte(addr));
//       break;
//     }
//     case 0b001:
//     { // SH
//       addr = execution_result_;
//       for (size_t i = 0; i < 2; ++i)
//       {
//         old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//       }
//       memory_controller_.WriteHalfWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFF);
//       for (size_t i = 0; i < 2; ++i)
//       {
//         new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//       }
//       break;
//     }
//     case 0b010:
//     { // SW
//       addr = execution_result_;
//       for (size_t i = 0; i < 4; ++i)
//       {
//         old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//       }
//       memory_controller_.WriteWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFFFFFF);
//       for (size_t i = 0; i < 4; ++i)
//       {
//         new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//       }
//       break;
//     }
//     case 0b011:
//     { // SD
//       addr = execution_result_;
//       for (size_t i = 0; i < 8; ++i)
//       {
//         old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//       }
//       memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadGpr(rs2) & 0xFFFFFFFFFFFFFFFF);
//       for (size_t i = 0; i < 8; ++i)
//       {
//         new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//       }
//       break;
//     }
//     }

//     if (!writtenBytes.isEmpty())
//     {
//       emit memoryUpdated(writeAddress, writtenBytes);
//     }
//   }


//   if (old_bytes_vec != new_bytes_vec)
//   {
//     current_delta_.memory_changes.push_back({addr,
//                                              old_bytes_vec,
//                                              new_bytes_vec});
//   }
// }

// }
// // void RVSSVM::WriteMemoryFloat()
// // {
// //   uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

// //   if (control_unit_.GetMemRead())
// //   { // FLW
// //     memory_result_ = memory_controller_.ReadWord(execution_result_);
// //   }

// //   uint64_t addr = 0;
// //   std::vector<uint8_t> old_bytes_vec;
// //   std::vector<uint8_t> new_bytes_vec;

// //   if (control_unit_.GetMemWrite())
// //   { // FSW
// //     addr = execution_result_;
// //     for (size_t i = 0; i < 4; ++i)
// //     {
// //       old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
// //     }
// //     uint32_t val = registers_->ReadFpr(rs2) & 0xFFFFFFFF;
// //     memory_controller_.WriteWord(execution_result_, val);
// //     // new_bytes_vec.push_back(memory_controller_.ReadByte(addr));
// //     for (size_t i = 0; i < 4; ++i)
// //     {
// //       new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
// //     }
// //   }


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
//       uint64_t addr = 0;
//       std::vector<uint8_t> old_bytes_vec;
//       std::vector<uint8_t> new_bytes_vec;

//       if (control_unit_.GetMemWrite())
//       { // FSD
//         addr = execution_result_;
//         for (size_t i = 0; i < 8; ++i)
//         {
//           old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//         }
//         memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
//         for (size_t i = 0; i < 8; ++i)
//         {
//           new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
//         }
//       }

//       if (old_bytes_vec != new_bytes_vec)
//       {
//         current_delta_.memory_changes.push_back({addr, old_bytes_vec, new_bytes_vec});
//       }

//     // if (control_unit_.GetMemWrite()) {
//     //     memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
//     // }
// }


// // void RVSSVM::WriteMemoryDouble()
// // {
// //   uint8_t rs2 = (current_instruction_ >> 20) & 0b11111;

// //   if (control_unit_.GetMemRead())
// //   { // FLD
// //     memory_result_ = memory_controller_.ReadDoubleWord(execution_result_);
// //   }

// //   uint64_t addr = 0;
// //   std::vector<uint8_t> old_bytes_vec;
// //   std::vector<uint8_t> new_bytes_vec;

// //   if (control_unit_.GetMemWrite())
// //   { // FSD
// //     addr = execution_result_;
// //     for (size_t i = 0; i < 8; ++i)
// //     {
// //       old_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
// //     }
// //     memory_controller_.WriteDoubleWord(execution_result_, registers_->ReadFpr(rs2));
// //     for (size_t i = 0; i < 8; ++i)
// //     {
// //       new_bytes_vec.push_back(memory_controller_.ReadByte(addr + i));
// //     }
// //   }

// //   if (old_bytes_vec != new_bytes_vec)
// //   {
// //     current_delta_.memory_changes.push_back({addr, old_bytes_vec, new_bytes_vec});
// //   }
// // }

// // void RVSSVM::WriteBack()
// // {
// //   uint8_t opcode = current_instruction_ & 0b1111111;
// //   uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
// //   uint8_t rd = (current_instruction_ >> 7) & 0b11111;
// //   int32_t imm = ImmGenerator(current_instruction_);

// //   if (opcode == 0b1110011 && funct3 == 0b000)
// //   {
// //     return;
// //   }
// //   if (instruction_set::isFInstruction(current_instruction_))
// //   {
// //     WriteBackFloat();
// //     return;
// //   }
// //   else if (instruction_set::isDInstruction(current_instruction_))
// //   {
// //     WriteBackDouble();
// //     return;
// //   }
// //   else if (opcode == 0b1110011)
// //   {
// //     WriteBackCsr();
// //     return;
// //   }
// //   if (control_unit_.GetRegWrite())
// //   {
// //     switch (opcode)
// //     {
// //     case 0b0110011:
// //     case 0b0010011:
// //     case 0b0010111:
// //       registers_->WriteGpr(rd, execution_result_);
// //       emit gprUpdated(rd, execution_result_);
// //       break;
// //     case 0b0000011:
// //       registers_->WriteGpr(rd, memory_result_);
// //       emit gprUpdated(rd, memory_result_);
// //       break;
// //     case 0b1100111:
// //     case 0b1101111:
// //       registers_->WriteGpr(rd, next_pc_);
// //       emit gprUpdated(rd, next_pc_);
// //       break;
// //     case 0b0110111:
// //       registers_->WriteGpr(rd, (imm << 12));
// //       emit gprUpdated(rd, (imm << 12));
// //       break;
// //     default:
// //       break;
// //     }
// //   }
// // }

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
//         uint64_t value = execution_result_;
//         if (registers_->GetIsa() == ISA::RV32)
//             value &= 0xFFFFFFFF;
//         registers_->WriteGpr(rd, value);
//         emit gprUpdated(rd, value);
//     }

//     if (control_unit_.GetRegWrite())
//       {
//         switch (opcode)
//         {
//         case 0b0110011:
//         case 0b0010011:
//         case 0b0010111:
//           registers_->WriteGpr(rd, execution_result_);
//           emit gprUpdated(rd, execution_result_);
//           break;
//         case 0b0000011:
//           registers_->WriteGpr(rd, memory_result_);
//           emit gprUpdated(rd, memory_result_);
//           break;
//         case 0b1100111:
//         case 0b1101111:
//           registers_->WriteGpr(rd, next_pc_);
//           emit gprUpdated(rd, next_pc_);
//           break;
//         case 0b0110111:
//           registers_->WriteGpr(rd, (imm << 12));
//           emit gprUpdated(rd, (imm << 12));
//           break;
//         default:
//           break;
//         }
// }
// }

// // void RVSSVM::WriteBackFloat()
// // {
// //   uint8_t opcode = current_instruction_ & 0b1111111;
// //   uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
// //   uint8_t rd = (current_instruction_ >> 7) & 0b11111;
// //   if (control_unit_.GetRegWrite())
// //   {
// //     if (funct7 == 0b1010000 || funct7 == 0b1100000 || funct7 == 0b1110000)
// //     {
// //       registers_->WriteGpr(rd, execution_result_);
// //       emit gprUpdated(rd, execution_result_);
// //     }
// //     else if (opcode == 0b0000111)
// //     {
// //       registers_->WriteFpr(rd, memory_result_);
// //       emit fprUpdated(rd, memory_result_);
// //     }
// //     else
// //     {
// //       registers_->WriteFpr(rd, execution_result_);
// //       emit fprUpdated(rd, execution_result_);
// //     }
// //   }
// // }

// void RVSSVM::WriteBackFloat()
// {
//     // unchanged, RV32 supports F, mask if needed
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

// // void RVSSVM::WriteBackDouble()
// // {
// //   uint8_t opcode = current_instruction_ & 0b1111111;
// //   uint8_t funct7 = (current_instruction_ >> 25) & 0b1111111;
// //   uint8_t rd = (current_instruction_ >> 7) & 0b11111;
// //   if (control_unit_.GetRegWrite())
// //   {
// //     if (funct7 == 0b1010001 || funct7 == 0b1100001 || funct7 == 0b1110001)
// //     {
// //       registers_->WriteGpr(rd, execution_result_);
// //       emit gprUpdated(rd, execution_result_);
// //     }
// //     else if (opcode == 0b0000111)
// //     {
// //       registers_->WriteFpr(rd, memory_result_);
// //       emit fprUpdated(rd, memory_result_);
// //     }
// //     else
// //     {
// //       registers_->WriteFpr(rd, execution_result_);
// //       emit fprUpdated(rd, execution_result_);
// //     }
// //   }
// // }

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
//   uint8_t rd = (current_instruction_ >> 7) & 0b11111;
//   uint8_t funct3 = (current_instruction_ >> 12) & 0b111;
//   uint16_t csr_addr = csr_target_address_;
//   switch (funct3)
//   {
//   case 0b001: // CSRRW
//     registers_->WriteGpr(rd, csr_old_value_);
//     registers_->WriteCsr(csr_addr, csr_write_val_);
//     emit csrUpdated(csr_addr, csr_write_val_);
//     break;
//   case 0b010: // CSRRS
//     registers_->WriteGpr(rd, csr_old_value_);
//     if (csr_write_val_ != 0)
//     {
//       registers_->WriteCsr(csr_addr, csr_old_value_ | csr_write_val_);
//       emit csrUpdated(csr_addr, csr_old_value_ | csr_write_val_);
//     }
//     break;
//   case 0b011: // CSRRC
//     registers_->WriteGpr(rd, csr_old_value_);
//     if (csr_write_val_ != 0)
//     {
//       registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_write_val_);
//       emit csrUpdated(csr_addr, csr_old_value_ & ~csr_write_val_);
//     }
//     break;
//   case 0b101: // CSRRWI
//     registers_->WriteGpr(rd, csr_old_value_);
//     registers_->WriteCsr(csr_addr, csr_uimm_);
//     emit csrUpdated(csr_addr, csr_uimm_);
//     break;
//   case 0b110: // CSRRSI
//     registers_->WriteGpr(rd, csr_old_value_);
//     if (csr_uimm_ != 0)
//     {
//       registers_->WriteCsr(csr_addr, csr_old_value_ | csr_uimm_);
//       emit csrUpdated(csr_addr, csr_old_value_ | csr_uimm_);
//     }
//     break;
//   case 0b111: // CSRRCI
//     registers_->WriteGpr(rd, csr_old_value_);
//     if (csr_uimm_ != 0)
//     {
//       registers_->WriteCsr(csr_addr, csr_old_value_ & ~csr_uimm_);
//       emit csrUpdated(csr_addr, csr_old_value_ & ~csr_uimm_);
//     }
//     break;
//   }
// }

// // VM control
// void RVSSVM::Run()
// {
//   // std::cout << "Entered the Run function" << std::endl;
//   ClearStop();
//   std::cout << program_counter_ << " " << program_size_ << std::endl;
//   while (!stop_requested_ && program_counter_ < program_size_)
//   {
//     std::cout << "Entered the Run function loop " << program_counter_ << std::endl;
//     Fetch();
//     Decode();
//     Execute();
//     WriteMemory();
//     WriteBack();
//     instructions_retired_++;
//     cycle_s_++;
//   }
//   if (program_counter_ >= program_size_)
//     emit statusChanged("VM_PROGRAM_END");
//   // Optionally dump state...
// }

// void RVSSVM::DebugRun()
// {
//   ClearStop();
//   while (!stop_requested_ && program_counter_ < program_size_)
//   {
//     current_delta_.old_pc = program_counter_;
//     Fetch();
//     Decode();
//     Execute();
//     WriteMemory();
//     WriteBack();
//     instructions_retired_++;
//     cycle_s_++;
//     current_delta_.new_pc = program_counter_;
//     undo_stack_.push(current_delta_);
//     while (!redo_stack_.empty())
//       redo_stack_.pop();
//     current_delta_ = StepDelta();
//   }
//   if (program_counter_ >= program_size_)
//     emit statusChanged("VM_PROGRAM_END");
// }

// void RVSSVM::Step()
// {
//   current_delta_.old_pc = program_counter_;
//   if (program_counter_ < program_size_)
//   {
//     Fetch();
//     Decode();
//     Execute();
//     WriteMemory();
//     WriteBack();
//     instructions_retired_++;
//     cycle_s_++;
//     current_delta_.new_pc = program_counter_;
//     undo_stack_.push(current_delta_);
//     while (!redo_stack_.empty())
//       redo_stack_.pop();
//     current_delta_ = StepDelta();
//   }
// }

// void RVSSVM::Undo()
// {
//   if (undo_stack_.empty())
//     return;
//   StepDelta last = undo_stack_.top();
//   undo_stack_.pop();
//   for (const auto &change : last.register_changes)
//   {
//     switch (change.reg_type)
//     {
//     case 0:
//       registers_->WriteGpr(change.reg_index, change.old_value);
//       emit gprUpdated(change.reg_index, change.old_value);
//       break;
//     case 1:
//       registers_->WriteCsr(change.reg_index, change.old_value);
//       emit csrUpdated(change.reg_index, change.old_value);
//       break;
//     case 2:
//       registers_->WriteFpr(change.reg_index, change.old_value);
//       emit fprUpdated(change.reg_index, change.old_value);
//       break;
//     }
//   }
//   for (const auto &change : last.memory_changes)
//   {
//     for (size_t i = 0; i < change.old_bytes_vec.size(); ++i)
//       memory_controller_.WriteByte(change.address + i, change.old_bytes_vec[i]);
//   }
//   program_counter_ = last.old_pc;
//   instructions_retired_--;
//   cycle_s_--;
//   redo_stack_.push(last);
// }

// void RVSSVM::Redo()
// {
//   if (redo_stack_.empty())
//     return;
//   StepDelta next = redo_stack_.top();
//   redo_stack_.pop();
//   for (const auto &change : next.register_changes)
//   {
//     switch (change.reg_type)
//     {
//     case 0:
//       registers_->WriteGpr(change.reg_index, change.new_value);
//       emit gprUpdated(change.reg_index, change.new_value);
//       break;
//     case 1:
//       registers_->WriteCsr(change.reg_index, change.new_value);
//       emit csrUpdated(change.reg_index, change.new_value);
//       break;
//     case 2:
//       registers_->WriteFpr(change.reg_index, change.new_value);
//       emit fprUpdated(change.reg_index, change.new_value);
//       break;
//     }
//   }
//   for (const auto &change : next.memory_changes)
//   {
//     for (size_t i = 0; i < change.new_bytes_vec.size(); ++i)
//       memory_controller_.WriteByte(change.address + i, change.new_bytes_vec[i]);
//   }
//   program_counter_ = next.new_pc;
//   instructions_retired_++;
//   cycle_s_++;
//   undo_stack_.push(next);
// }

// void RVSSVM::Reset()
// {
//   program_counter_ = 0;
//   instructions_retired_ = 0;
//   cycle_s_ = 0;
//   registers_->Reset();
//   memory_controller_.Reset();
//   control_unit_.Reset();
//   branch_flag_ = false;
//   next_pc_ = 0;
//   execution_result_ = 0;
//   memory_result_ = 0;
//   return_address_ = 0;
//   csr_target_address_ = 0;
//   csr_old_value_ = 0;
//   csr_write_val_ = 0;
//   csr_uimm_ = 0;
//   current_delta_.register_changes.clear();
//   current_delta_.memory_changes.clear();
//   current_delta_.old_pc = 0;
//   current_delta_.new_pc = 0;
//   undo_stack_ = std::stack<StepDelta>();
//   redo_stack_ = std::stack<StepDelta>();
// }


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
