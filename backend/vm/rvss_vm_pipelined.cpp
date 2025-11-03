// // #include "rvss_vm_pipelined.h"

// // rvss_vm_pipelined::rvss_vm_pipelined() {}

// #include "rvss_vm_pipelined.h"

// #include "../common/instructions.h"
// // #include "../../globals.h"
// // #include "../../utils.h"

// #include <iostream>
// #include <tuple>
// #include <QDebug>

// using instruction_set::Instruction;
// // using instruction_set::get_instr_encoding;

// // RVSSVMPipelined::RVSSVMPipelined() : VmBase() {
// //     // initialize PC, registers, etc. reuse VmBase defaults
// // }
// RVSSVMPipelined::RVSSVMPipelined(RegisterFile *sharedRegisters, QObject *parent)
//     : RVSSVM(sharedRegisters, parent)
// {
//     registers_ = sharedRegisters; // Use the shared instance
// }

// RVSSVMPipelined::~RVSSVMPipelined() = default;

// void RVSSVMPipelined::Reset()
// {
//     RVSSVM::Reset();
//     if_id_ = IF_ID();
//     if_id_next_ = IF_ID();
//     id_ex_ = ID_EX();
//     id_ex_next_ = ID_EX();
//     ex_mem_ = EX_MEM();
//     ex_mem_next_ = EX_MEM();
//     mem_wb_ = MEM_WB();
//     mem_wb_next_ = MEM_WB();
//     pc_update_pending_ = false;
//     pc_update_value_ = 0;
// }

// // void RVSSVMPipelined::IF_stage()
// // {
// //     if (!if_id_.valid) {
// //         id_ex_next_.valid = false;
// //         return;
// //     }
// //     // Fetch instruction at program_counter_
// //     if (!pc_update_pending_)
// //     {
// //         if_id_next_.pc = program_counter_;
// //         if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
// //         if_id_next_.valid = true;
// //         // advance PC for next fetch
// //         program_counter_ += 4;
// //     }
// //     else
// //     {
// //         // If some other stage set PC (branch from EX), honor it next cycle
// //         program_counter_ = pc_update_value_;
// //         pc_update_pending_ = false;
// //         pc_update_value_ = 0;
// //         // fetch at new PC
// //         if_id_next_.pc = program_counter_;
// //         if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
// //         if_id_next_.valid = true;
// //         program_counter_ += 4;
// //     }
// // }

// // void RVSSVMPipelined::ID_stage()
// // {
// //     if (!if_id_.valid)
// //     {
// //         id_ex_next_.valid = false;
// //         return;
// //     }

// //     uint32_t instr = if_id_.instruction;
// //     id_ex_next_.valid = true;
// //     id_ex_next_.pc = if_id_.pc;
// //     id_ex_next_.instruction = instr;

// //     uint8_t opcode = instr & 0b1111111;
// //     id_ex_next_.rs1 = (instr >> 15) & 0b11111;
// //     id_ex_next_.rs2 = (instr >> 20) & 0b11111;
// //     id_ex_next_.rd = (instr >> 7) & 0b11111;
// //     id_ex_next_.funct3 = (instr >> 12) & 0b111;
// //     id_ex_next_.funct7 = (instr >> 25) & 0b1111111;
// //     id_ex_next_.imm = ImmGenerator(instr);

// //     // read registers
// //     id_ex_next_.reg1_value = registers_->ReadGpr(id_ex_next_.rs1);
// //     id_ex_next_.reg2_value = registers_->ReadGpr(id_ex_next_.rs2);

// //     // set control signals via control unit
// //     control_unit_.SetControlSignals(instr);
// //     id_ex_next_.reg_write = control_unit_.GetRegWrite();
// //     id_ex_next_.mem_read = control_unit_.GetMemRead();
// //     id_ex_next_.mem_write = control_unit_.GetMemWrite();
// //     id_ex_next_.mem_to_reg = control_unit_.GetMemToReg();
// //     id_ex_next_.alu_src = control_unit_.GetAluSrc();
// //     id_ex_next_.branch = control_unit_.GetBranch();
// //     id_ex_next_.is_float = instruction_set::isFInstruction(instr) || instruction_set::isDInstruction(instr);
// // }

// // void RVSSVMPipelined::EX_stage()
// // {
// //     if (!id_ex_.valid)
// //     {
// //         ex_mem_next_.valid = false;
// //         return;
// //     }

// //     ex_mem_next_.valid = true;
// //     ex_mem_next_.pc = id_ex_.pc;
// //     ex_mem_next_.instruction = id_ex_.instruction;
// //     ex_mem_next_.rd = id_ex_.rd;
// //     ex_mem_next_.reg_write = id_ex_.reg_write;
// //     ex_mem_next_.mem_read = id_ex_.mem_read;
// //     ex_mem_next_.mem_write = id_ex_.mem_write;
// //     ex_mem_next_.mem_to_reg = id_ex_.mem_to_reg;
// //     ex_mem_next_.reg2_value = id_ex_.reg2_value;

// //     // select alu input
// //     uint64_t op1 = id_ex_.reg1_value;
// //     uint64_t op2 = id_ex_.alu_src ? static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm)) : id_ex_.reg2_value;

// //     // for floating point and CSR some instructions are handled elsewhere in single-cycle; here we'll call the ALU generically
// //     alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
// //     bool overflow = false;
// //     std::tie(ex_mem_next_.alu_result, overflow) = alu_.execute(aluOperation, op1, op2);

// //     // branch handling: compute branch target and set PC update pending immediately
// //     if (id_ex_.branch)
// //     {
// //         int32_t imm = id_ex_.imm;
// //         // simple branch decision similar to single-cycle: rely on alu result
// //         uint8_t funct3 = id_ex_.funct3;
// //         bool take = false;
// //         switch (funct3)
// //         {
// //         case 0b000:
// //             take = (ex_mem_next_.alu_result == 0);
// //             break; // BEQ
// //         case 0b001:
// //             take = (ex_mem_next_.alu_result != 0);
// //             break; // BNE
// //         case 0b100:
// //             take = (ex_mem_next_.alu_result == 1);
// //             break; // BLT (alu gave slt)
// //         case 0b101:
// //             take = (ex_mem_next_.alu_result == 0);
// //             break; // BGE
// //         case 0b110:
// //             take = (ex_mem_next_.alu_result == 1);
// //             break; // BLTU
// //         case 0b111:
// //             take = (ex_mem_next_.alu_result == 0);
// //             break; // BGEU
// //         default:
// //             take = false;
// //             break;
// //         }

// //         if (take)
// //         {
// //             // branch target computed relative to id_ex_.pc (which is inst addr)
// //             ex_mem_next_.branch_taken = true;
// //             ex_mem_next_.branch_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc) + id_ex_.imm);
// //             // set PC to target for next IF
// //             pc_update_pending_ = true;
// //             pc_update_value_ = ex_mem_next_.branch_target;
// //         }
// //         else
// //         {
// //             ex_mem_next_.branch_taken = false;
// //         }
// //     }
// //     else
// //     {
// //         ex_mem_next_.branch_taken = false;
// //     }
// // }

// // void RVSSVMPipelined::MEM_stage()
// // {
// //     if (!ex_mem_.valid)
// //     {
// //         mem_wb_next_.valid = false;
// //         return;
// //     }

// //     mem_wb_next_.valid = true;
// //     mem_wb_next_.rd = ex_mem_.rd;
// //     mem_wb_next_.reg_write = ex_mem_.reg_write;
// //     mem_wb_next_.mem_to_reg = ex_mem_.mem_to_reg;
// //     mem_wb_next_.alu_result = ex_mem_.alu_result;
// //     mem_wb_next_.pc = ex_mem_.pc;

// //     // memory operations
// //     if (ex_mem_.mem_read)
// //     {
// //         uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
// //         switch (funct3)
// //         {
// //         case 0b000:
// //             mem_wb_next_.mem_data = static_cast<int8_t>(memory_controller_.ReadByte(ex_mem_.alu_result));
// //             break; // LB
// //         case 0b001:
// //             mem_wb_next_.mem_data = static_cast<int16_t>(memory_controller_.ReadHalfWord(ex_mem_.alu_result));
// //             break; // LH
// //         case 0b010:
// //             mem_wb_next_.mem_data = static_cast<int32_t>(memory_controller_.ReadWord(ex_mem_.alu_result));
// //             break; // LW
// //         case 0b011:
// //             mem_wb_next_.mem_data = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
// //             break; // LD
// //         case 0b100:
// //             mem_wb_next_.mem_data = memory_controller_.ReadByte(ex_mem_.alu_result);
// //             break; // LBU
// //         case 0b101:
// //             mem_wb_next_.mem_data = memory_controller_.ReadHalfWord(ex_mem_.alu_result);
// //             break; // LHU
// //         case 0b110:
// //             mem_wb_next_.mem_data = memory_controller_.ReadWord(ex_mem_.alu_result);
// //             break; // LWU
// //         default:
// //             mem_wb_next_.mem_data = 0;
// //             break;
// //         }
// //     }

// //     if (ex_mem_.mem_write)
// //     {
// //         uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
// //         switch (funct3)
// //         {
// //         case 0b000:
// //             memory_controller_.WriteByte(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFF);
// //             break; // SB
// //         case 0b001:
// //             memory_controller_.WriteHalfWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFF);
// //             break; // SH
// //         case 0b010:
// //             memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF);
// //             break; // SW
// //         case 0b011:
// //             memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value);
// //             break; // SD
// //         default:
// //             break;
// //         }
// //     }
// // }

// // void RVSSVMPipelined::WB_stage()
// // {
// //     if (!mem_wb_.valid)
// //         return;

// //     if (mem_wb_.reg_write)
// //     {
// //         uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
// //         // zero register x0 should remain zero
// //         if (mem_wb_.rd != 0)
// //         {
// //             uint64_t old = registers_->ReadGpr(mem_wb_.rd);
// //             registers_->WriteGpr(mem_wb_.rd, write_val);
// //             if (old != write_val)
// //             {
// //                 // no undo/redo tracking in pipelined stub; could be added later
// //             }
// //         }
// //     }
// // }

// // void RVSSVMPipelined::advance_pipeline_registers()
// // {
// //     // advance pipeline: WB consumes mem_wb_, MEM consumes ex_mem_, etc.
// //     mem_wb_ = mem_wb_next_;
// //     ex_mem_ = ex_mem_next_;
// //     id_ex_ = id_ex_next_;
// //     if_id_ = if_id_next_;
// //     // ex_mem_next_.pc = id_ex_.pc;

// //     // clear next stage holders
// //     mem_wb_next_ = MEM_WB();
// //     ex_mem_next_ = EX_MEM();
// //     id_ex_next_ = ID_EX();
// //     if_id_next_ = IF_ID();
// // }

// // void RVSSVMPipelined::Run()
// // {
// //     // ClearStop();
// //     // uint64_t instruction_executed = 0;
// //     // std::cout << "Running the pipelined version " << std::endl;

// //     // // pipeline warmup: ensure registers are cleared
// //     // Reset();

// //     // while (!stop_requested_ && program_counter_ < program_size_) {
// //     //     // if (instruction_executed > vm_config::config.getInstructionExecutionLimit()) break;

// //     //     // Progress pipeline in reverse order to model same-cycle behavior
// //     //     IF_stage();
// //     //     ID_stage();
// //     //     EX_stage();
// //     //     MEM_stage();
// //     //     WB_stage();

// //     //     advance_pipeline_registers();

// //     //     instructions_retired_ += (mem_wb_.valid && mem_wb_.reg_write) ? 1 : 0;
// //     //     instruction_executed++;
// //     //     cycle_s_++;
// //     // }

// //     // if (program_counter_ >= program_size_) {
// //     //     std::cout << "VM_PROGRAM_END" << std::endl;
// //     //     output_status_ = "VM_PROGRAM_END";
// //     // }
// //     // DumpRegisters(globals::registers_dump_file_path, registers_);
// //     // DumpState(globals::vm_state_dump_file_path);

// //     uint64_t instruction_executed = 0;
// //     Reset();

// //     bool pipeline_empty = false;

// //     while (!stop_requested_ && (!pipeline_empty || program_counter_ < program_size_))
// //     {
// //         IF_stage();
// //         ID_stage();
// //         EX_stage();
// //         MEM_stage();
// //         WB_stage();

// //         advance_pipeline_registers();

// //         instructions_retired_ += mem_wb_.valid ? 1 : 0;
// //         instruction_executed++;
// //         cycle_s_++;

// //         pipeline_empty = !(if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
// //     }

// //     if (program_counter_ >= program_size_ && pipeline_empty)
// //     {
// //         std::cout << "VM_PROGRAM_END" << std::endl;
// //         output_status_ = "VM_PROGRAM_END";
// //     }
// // }

// void RVSSVMPipelined::IF_stage()
// {
//     qDebug() << "IF_stage entry. PC:" << program_counter_ << "if_id_.valid:" << if_id_.valid;
//     // Always fetch instruction unless branch PC update pending
//     if (!pc_update_pending_) {
//         if_id_next_.pc = program_counter_;
//         if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
//         if_id_next_.valid = true;
//         program_counter_ += 4;
//         qDebug() << "IF_stage fetched at PC:" << if_id_next_.pc;
//     } else {
//         program_counter_ = pc_update_value_;
//         pc_update_pending_ = false;
//         pc_update_value_ = 0;
//         if_id_next_.pc = program_counter_;
//         if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
//         if_id_next_.valid = true;
//         program_counter_ += 4;
//         qDebug() << "IF_stage branch taken. New PC:" << if_id_next_.pc;
//     }
// }

// void RVSSVMPipelined::ID_stage()
// {
//     qDebug() << "ID_stage entry. if_id_.valid:" << if_id_.valid;

//     if (!if_id_.valid) {
//         id_ex_next_.valid = false;
//         qDebug() << "ID_stage no valid instruction, clearing id_ex_next_ valid flag.";
//         return;
//     }

//     uint32_t instr = if_id_.instruction;
//     id_ex_next_.valid = true;
//     id_ex_next_.pc = if_id_.pc;
//     id_ex_next_.instruction = instr;

//     // ... Other decoding remains unchanged ...

//     qDebug() << "ID_stage decoded instr at PC:" << id_ex_next_.pc
//              << "rs1:" << id_ex_next_.rs1 << "rs2:" << id_ex_next_.rs2
//              << "rd:" << id_ex_next_.rd;
// }

// void RVSSVMPipelined::EX_stage()
// {
//     qDebug() << "EX_stage entry. id_ex_.valid:" << id_ex_.valid;

//     if (!id_ex_.valid) {
//         ex_mem_next_.valid = false;
//         qDebug() << "EX_stage no valid instruction, clearing ex_mem_next_ valid flag.";
//         return;
//     }

//     ex_mem_next_.valid = true;
//     ex_mem_next_.pc = id_ex_.pc;
//     ex_mem_next_.instruction = id_ex_.instruction;
//     ex_mem_next_.rd = id_ex_.rd;
//     ex_mem_next_.reg_write = id_ex_.reg_write;
//     ex_mem_next_.mem_read = id_ex_.mem_read;
//     ex_mem_next_.mem_write = id_ex_.mem_write;
//     ex_mem_next_.mem_to_reg = id_ex_.mem_to_reg;
//     ex_mem_next_.reg2_value = id_ex_.reg2_value;

//     // ALU operation
//     uint64_t op1 = id_ex_.reg1_value;
//     uint64_t op2 = id_ex_.alu_src ? static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm)) : id_ex_.reg2_value;
//     alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
//     bool overflow = false;
//     std::tie(ex_mem_next_.alu_result, overflow) = alu_.execute(aluOperation, op1, op2);

//     qDebug() << "EX_stage ALU op result:" << ex_mem_next_.alu_result
//              << "Op1:" << op1 << "Op2:" << op2;

//     // Branch handling debug
//     if (id_ex_.branch) {
//         qDebug() << "EX_stage branch instruction detected.";
//         // ... Branch detection as before ...
//         // Log branch taken or not
//         if (ex_mem_next_.branch_taken) {
//             qDebug() << "EX_stage branch taken to target:" << ex_mem_next_.branch_target;
//         } else {
//             qDebug() << "EX_stage branch not taken";
//         }
//     }
// }

// void RVSSVMPipelined::MEM_stage()
// {
//     qDebug() << "MEM_stage entry. ex_mem_.valid:" << ex_mem_.valid;

//     if (!ex_mem_.valid) {
//         mem_wb_next_.valid = false;
//         qDebug() << "MEM_stage no valid instruction, clearing mem_wb_next_ valid flag.";
//         return;
//     }

//     mem_wb_next_.valid = true;
//     mem_wb_next_.rd = ex_mem_.rd;
//     mem_wb_next_.reg_write = ex_mem_.reg_write;
//     mem_wb_next_.mem_to_reg = ex_mem_.mem_to_reg;
//     mem_wb_next_.alu_result = ex_mem_.alu_result;
//     mem_wb_next_.pc = ex_mem_.pc;

//     // Memory operations debug
//     if (ex_mem_.mem_read) {
//         qDebug() << "MEM_stage performing memory read.";
//         // ... Continue operations ...
//     }

//     if (ex_mem_.mem_write) {
//         qDebug() << "MEM_stage performing memory write.";
//         // ... Continue operations ...
//     }
// }

// void RVSSVMPipelined::WB_stage()
// {
//     qDebug() << "WB_stage entry. mem_wb_.valid:" << mem_wb_.valid;

//     if (!mem_wb_.valid) {
//         qDebug() << "WB_stage no valid instruction, return.";
//         return;
//     }

//     if (mem_wb_.reg_write) {
//         uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
//         if (mem_wb_.rd != 0) {
//             uint64_t old = registers_->ReadGpr(mem_wb_.rd);
//             registers_->WriteGpr(mem_wb_.rd, write_val);
//             if (old != write_val) {
//                 qDebug() << "WB_stage register x" << mem_wb_.rd
//                          << " updated from:" << old << " to:" << write_val;
//             }
//         }
//     }
// }

// void RVSSVMPipelined::advance_pipeline_registers()
// {
//     qDebug() << "Advancing pipeline registers:";
//     qDebug() << "  Before advance: if_id_.valid =" << if_id_.valid
//              << ", id_ex_.valid =" << id_ex_.valid
//              << ", ex_mem_.valid =" << ex_mem_.valid
//              << ", mem_wb_.valid =" << mem_wb_.valid;

//     mem_wb_ = mem_wb_next_;
//     ex_mem_ = ex_mem_next_;
//     id_ex_ = id_ex_next_;
//     if_id_ = if_id_next_;

//     mem_wb_next_ = MEM_WB();
//     ex_mem_next_ = EX_MEM();
//     id_ex_next_ = ID_EX();
//     if_id_next_ = IF_ID();

//     qDebug() << "  After advance: if_id_.valid =" << if_id_.valid
//              << ", id_ex_.valid =" << id_ex_.valid
//              << ", ex_mem_.valid =" << ex_mem_.valid
//              << ", mem_wb_.valid =" << mem_wb_.valid;
// }

// void RVSSVMPipelined::Run()
// {
//     qDebug() << "Starting pipelined Run() method.";
//     Reset();
//     uint64_t instruction_executed = 0;
//     bool pipeline_empty = false;

//     while (!stop_requested_ && (!pipeline_empty || program_counter_ < program_size_)) {
//         IF_stage();
//         ID_stage();
//         EX_stage();
//         MEM_stage();
//         WB_stage();

//         advance_pipeline_registers();

//         instructions_retired_ += mem_wb_.valid ? 1 : 0;
//         instruction_executed++;
//         cycle_s_++;

//         pipeline_empty = !(if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);

//         qDebug() << "Cycle:" << cycle_s_ << "PC:" << program_counter_
//                  << "Instructions executed:" << instruction_executed
//                  << "Pipeline empty:" << pipeline_empty;
//     }

//     if (program_counter_ >= program_size_ && pipeline_empty) {
//         qDebug() << "Program execution completed.";
//         output_status_ = "VM_PROGRAM_END";
//     }
// }

// void RVSSVMPipelined::DebugRun()
// {
//     // For now, DebugRun will behave same as Run but step with some printing
//     Run();
// }

// void RVSSVMPipelined::Step()
// {
//     // Single cycle step: run one cycle of pipeline
//     // IF_stage();
//     // ID_stage();
//     // EX_stage();
//     // MEM_stage();
//     // WB_stage();
//     // advance_pipeline_registers();
//     // cycle_s_++;

//     qDebug() << "======= Pipeline Step: Cycle" << cycle_s_ << " =======";

//     IF_stage();
//     qDebug() << "After IF_stage: if_id_.valid =" << if_id_.valid << ", if_id_.pc =" << if_id_.pc;

//     ID_stage();
//     qDebug() << "After ID_stage: id_ex_.valid =" << id_ex_.valid << ", id_ex_.pc =" << id_ex_.pc;

//     EX_stage();
//     qDebug() << "After EX_stage: ex_mem_.valid =" << ex_mem_.valid << ", ex_mem_.pc =" << ex_mem_.pc;

//     MEM_stage();
//     qDebug() << "After MEM_stage: mem_wb_.valid =" << mem_wb_.valid << ", mem_wb_.pc =" << mem_wb_.pc;

//     WB_stage();

//     qDebug() << "Before advance: if_id_next_.valid =" << if_id_next_.valid
//              << ", id_ex_next_.valid =" << id_ex_next_.valid
//              << ", ex_mem_next_.valid =" << ex_mem_next_.valid
//              << ", mem_wb_next_.valid =" << mem_wb_next_.valid;

//     advance_pipeline_registers();

//     qDebug() << "After advance: if_id_.valid =" << if_id_.valid
//              << ", id_ex_.valid =" << id_ex_.valid
//              << ", ex_mem_.valid =" << ex_mem_.valid
//              << ", mem_wb_.valid =" << mem_wb_.valid;

//     cycle_s_++;
// }

// void RVSSVMPipelined::Undo()
// {
//     std::cerr << "Undo not supported in pipelined VM stub" << std::endl;
// }

// void RVSSVMPipelined::Redo()
// {
//     std::cerr << "Redo not supported in pipelined VM stub" << std::endl;
// }


#include "rvss_vm_pipelined.h"
#include "../common/instructions.h"
#include <iostream>
#include <QDebug>

using instruction_set::Instruction;

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
}

void RVSSVMPipelined::IF_stage()
{
    qDebug() << "IF_stage entry. PC:" << program_counter_ << "if_id_.valid:" << if_id_.valid;
    if (!pc_update_pending_) {
        if_id_next_.pc = program_counter_;
        if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_next_.valid = true;
        program_counter_ += 4;
        qDebug() << "IF_stage fetched at PC:" << if_id_next_.pc;
    } else {
        program_counter_ = pc_update_value_;
        pc_update_pending_ = false;
        pc_update_value_ = 0;
        if_id_next_.pc = program_counter_;
        if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
        if_id_next_.valid = true;
        program_counter_ += 4;
        qDebug() << "IF_stage branch taken. New PC:" << if_id_next_.pc;
    }
}

void RVSSVMPipelined::ID_stage()
{
    qDebug() << "ID_stage entry. if_id_.valid:" << if_id_.valid;

    if (!if_id_.valid) {
        id_ex_next_.valid = false;
        qDebug() << "ID_stage no valid instruction, clearing id_ex_next_.";
        return;
    }

    uint32_t instr = if_id_.instruction;
    id_ex_next_.valid = true;
    id_ex_next_.pc = if_id_.pc;
    id_ex_next_.instruction = instr;

    uint8_t opcode = instr & 0b1111111;
    id_ex_next_.rs1 = (instr >> 15) & 0b11111;
    id_ex_next_.rs2 = (instr >> 20) & 0b11111;
    id_ex_next_.rd = (instr >> 7) & 0b11111;
    id_ex_next_.funct3 = (instr >> 12) & 0b111;
    id_ex_next_.funct7 = (instr >> 25) & 0b1111111;
    id_ex_next_.imm = ImmGenerator(instr);

    id_ex_next_.reg1_value = registers_->ReadGpr(id_ex_next_.rs1);
    id_ex_next_.reg2_value = registers_->ReadGpr(id_ex_next_.rs2);

    control_unit_.SetControlSignals(instr);
    id_ex_next_.reg_write = control_unit_.GetRegWrite();
    id_ex_next_.mem_read = control_unit_.GetMemRead();
    id_ex_next_.mem_write = control_unit_.GetMemWrite();
    id_ex_next_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_next_.alu_src = control_unit_.GetAluSrc();
    id_ex_next_.branch = control_unit_.GetBranch();
    id_ex_next_.is_float = instruction_set::isFInstruction(instr) || instruction_set::isDInstruction(instr);

    qDebug() << "ID_stage decoded PC:" << id_ex_next_.pc << "rs1:" << id_ex_next_.rs1 << "rs2:" << id_ex_next_.rs2 << "rd:" << id_ex_next_.rd;
}

void RVSSVMPipelined::EX_stage()
{
    qDebug() << "EX_stage entry. id_ex_.valid:" << id_ex_.valid;

    if (!id_ex_.valid) {
        ex_mem_next_.valid = false;
        qDebug() << "EX_stage no valid instruction, clearing ex_mem_next_.";
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
    ex_mem_next_.reg2_value = id_ex_.reg2_value;

    uint64_t op1 = id_ex_.reg1_value;
    uint64_t op2 = id_ex_.alu_src ? static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm)) : id_ex_.reg2_value;

    alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
    bool overflow = false;
    std::tie(ex_mem_next_.alu_result, overflow) = alu_.execute(aluOperation, op1, op2);

    qDebug() << "EX_stage ALU result:" << ex_mem_next_.alu_result << "Op1:" << op1 << "Op2:" << op2;

    if (id_ex_.branch)
    {
        int32_t imm = id_ex_.imm;
        uint8_t funct3 = id_ex_.funct3;
        bool take = false;
        switch (funct3)
        {
        case 0b000: take = (ex_mem_next_.alu_result == 0); break; // BEQ
        case 0b001: take = (ex_mem_next_.alu_result != 0); break; // BNE
        case 0b100: take = (ex_mem_next_.alu_result == 1); break; // BLT
        case 0b101: take = (ex_mem_next_.alu_result == 0); break; // BGE
        case 0b110: take = (ex_mem_next_.alu_result == 1); break; // BLTU
        case 0b111: take = (ex_mem_next_.alu_result == 0); break; // BGEU
        }
        ex_mem_next_.branch_taken = take;
        if (take) {
            ex_mem_next_.branch_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc)) + id_ex_.imm;
            pc_update_pending_ = true;
            pc_update_value_ = ex_mem_next_.branch_target;
        }
    }
    else
        ex_mem_next_.branch_taken = false;
}

void RVSSVMPipelined::MEM_stage()
{
    qDebug() << "MEM_stage entry. ex_mem_.valid:" << ex_mem_.valid;

    if (!ex_mem_.valid) {
        mem_wb_next_.valid = false;
        return;
    }

    mem_wb_next_.valid = true;
    mem_wb_next_.rd = ex_mem_.rd;
    mem_wb_next_.reg_write = ex_mem_.reg_write;
    mem_wb_next_.mem_to_reg = ex_mem_.mem_to_reg;
    mem_wb_next_.alu_result = ex_mem_.alu_result;
    mem_wb_next_.pc = ex_mem_.pc;

    if (ex_mem_.mem_read) {
        uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
        switch (funct3)
        {
        case 0b000:
            mem_wb_next_.mem_data = static_cast<int8_t>(memory_controller_.ReadByte(ex_mem_.alu_result));
            break;
        case 0b001:
            mem_wb_next_.mem_data = static_cast<int16_t>(memory_controller_.ReadHalfWord(ex_mem_.alu_result));
            break;
        case 0b010:
            mem_wb_next_.mem_data = static_cast<int32_t>(memory_controller_.ReadWord(ex_mem_.alu_result));
            break;
        case 0b011:
            mem_wb_next_.mem_data = memory_controller_.ReadDoubleWord(ex_mem_.alu_result);
            break;
        case 0b100:
            mem_wb_next_.mem_data = memory_controller_.ReadByte(ex_mem_.alu_result);
            break;
        case 0b101:
            mem_wb_next_.mem_data = memory_controller_.ReadHalfWord(ex_mem_.alu_result);
            break;
        case 0b110:
            mem_wb_next_.mem_data = memory_controller_.ReadWord(ex_mem_.alu_result);
            break;
        default:
            mem_wb_next_.mem_data = 0;
            break;
        }
    }

    if (ex_mem_.mem_write) {
        uint8_t funct3 = (ex_mem_.instruction >> 12) & 0b111;
        switch (funct3)
        {
        case 0b000:
            memory_controller_.WriteByte(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFF);
            break;
        case 0b001:
            memory_controller_.WriteHalfWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFF);
            break;
        case 0b010:
            memory_controller_.WriteWord(ex_mem_.alu_result, ex_mem_.reg2_value & 0xFFFFFFFF);
            break;
        case 0b011:
            memory_controller_.WriteDoubleWord(ex_mem_.alu_result, ex_mem_.reg2_value);
            break;
        default:
            break;
        }
    }
}

void RVSSVMPipelined::WB_stage()
{
    qDebug() << "WB_stage entry. mem_wb_.valid:" << mem_wb_.valid;

    if (!mem_wb_.valid)
        return;

    if (mem_wb_.reg_write) {
        uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;
        if (mem_wb_.rd != 0) {
            uint64_t old = registers_->ReadGpr(mem_wb_.rd);
            registers_->WriteGpr(mem_wb_.rd, write_val);
            if (old != write_val) {
                qDebug() << "WB_stage register x" << mem_wb_.rd << " updated from:" << old << " to:" << write_val;
            }
        }
    }
}

void RVSSVMPipelined::advance_pipeline_registers()
{
    qDebug() << "Advancing pipeline registers:";
    qDebug() << "  Before advance: if_id_.valid =" << if_id_.valid << ", id_ex_.valid =" << id_ex_.valid
             << ", ex_mem_.valid =" << ex_mem_.valid << ", mem_wb_.valid =" << mem_wb_.valid;
    mem_wb_ = mem_wb_next_;
    ex_mem_ = ex_mem_next_;
    id_ex_ = id_ex_next_;
    if_id_ = if_id_next_;
    mem_wb_next_ = MEM_WB();
    ex_mem_next_ = EX_MEM();
    id_ex_next_ = ID_EX();
    if_id_next_ = IF_ID();
    qDebug() << "  After advance: if_id_.valid =" << if_id_.valid << ", id_ex_.valid =" << id_ex_.valid
             << ", ex_mem_.valid =" << ex_mem_.valid << ", mem_wb_.valid =" << mem_wb_.valid;
}

void RVSSVMPipelined::Run()
{
    qDebug() << "Starting pipelined Run() method.";
    Reset();
    uint64_t instruction_executed = 0;
    bool pipeline_empty = false;
    while (!stop_requested_ && (!pipeline_empty || program_counter_ < program_size_))
    {
        IF_stage();
        ID_stage();
        EX_stage();
        MEM_stage();
        WB_stage();
        advance_pipeline_registers();
        instructions_retired_ += mem_wb_.valid ? 1 : 0;
        instruction_executed++;
        cycle_s_++;
        pipeline_empty = !(if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
        qDebug() << "Cycle:" << cycle_s_ << "PC:" << program_counter_ << "Pipeline empty:" << pipeline_empty;
    }
    if (program_counter_ >= program_size_ && pipeline_empty)
    {
        qDebug() << "Program execution completed.";
        output_status_ = "VM_PROGRAM_END";
    }
}

void RVSSVMPipelined::DebugRun()
{
    Run();
}

void RVSSVMPipelined::Step()
{
    qDebug() << "======= Pipeline Step: Cycle" << cycle_s_ << " =======";
    IF_stage();
    qDebug() << "After IF_stage: if_id_.valid =" << if_id_.valid << ", if_id_.pc =" << if_id_.pc;
    ID_stage();
    qDebug() << "After ID_stage: id_ex_.valid =" << id_ex_.valid << ", id_ex_.pc =" << id_ex_.pc;
    EX_stage();
    qDebug() << "After EX_stage: ex_mem_.valid =" << ex_mem_.valid << ", ex_mem_.pc =" << ex_mem_.pc;
    MEM_stage();
    qDebug() << "After MEM_stage: mem_wb_.valid =" << mem_wb_.valid << ", mem_wb_.pc =" << mem_wb_.pc;
    WB_stage();
    qDebug() << "Before advance: if_id_next_.valid =" << if_id_next_.valid << ", id_ex_next_.valid =" << id_ex_next_.valid
             << ", ex_mem_next_.valid =" << ex_mem_next_.valid << ", mem_wb_next_.valid =" << mem_wb_next_.valid;
    advance_pipeline_registers();
    qDebug() << "After advance: if_id_.valid =" << if_id_.valid << ", id_ex_.valid =" << id_ex_.valid
             << ", ex_mem_.valid =" << ex_mem_.valid << ", mem_wb_.valid =" << mem_wb_.valid;
    cycle_s_++;
}

void RVSSVMPipelined::Undo()
{
    std::cerr << "Undo not supported in pipelined VM stub" << std::endl;
}

void RVSSVMPipelined::Redo()
{
    std::cerr << "Redo not supported in pipelined VM stub" << std::endl;
}


