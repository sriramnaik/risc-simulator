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
    branch_taken_this_cycle_ = false;

    // ---- Important: Reset program counter and show initial IF ----
    program_counter_ = 0;
    if (program_size_ > 0)
        emit pipelineStageChanged(program_counter_, "IF");
    else
        emit pipelineStageChanged(0, "IF_CLEAR");
}

void RVSSVMPipelined::IF_stage()
{
    // If branch was taken, flush this stage
    if (branch_taken_this_cycle_)
    {
        if_id_next_.valid = false;
        return;
    }

    // If there's a pending PC update (branch target), use it.
    if (pc_update_pending_)
    {
        // If branch target is out of program range, do not fetch.
        if (pc_update_value_ >= program_size_)
        {
            if_id_next_.valid = false;
            pc_update_pending_ = false;
            pc_update_value_ = 0;
            return;
        }

        program_counter_ = pc_update_value_;
        pc_update_pending_ = false;
        pc_update_value_ = 0;
    }

    // Do not fetch if PC is outside program size
    if (program_counter_ >= program_size_)
    {
        if_id_next_.valid = false;
        return;
    }

    // Normal fetch
    if_id_next_.pc = program_counter_;
    // memory_controller_.PrintMemory(program_counter_, 1);
    // std::cout << memory_controller_.ReadWord(program_counter_) << std::endl;
    if_id_next_.instruction = memory_controller_.ReadWord(program_counter_);
    if_id_next_.valid = true;
    // std::cout << "instruction: " << if_id_next_.instruction << " " << if_id_next_.valid << " " << "pc : " << if_id_next_.pc << std::endl;
    program_counter_ += 4;
}

void RVSSVMPipelined::ID_stage()
{
    // std::cout << "  [ID_STAGE] START" << std::endl;
    // std::cout << "  [ID_STAGE] branch_taken_this_cycle: " << branch_taken_this_cycle_ << std::endl;
    // std::cout << "  [ID_STAGE] if_id_.valid: " << if_id_.valid << std::endl;

    if (branch_taken_this_cycle_)
    {
        // std::cout << "  [ID_STAGE] Flushing due to branch" << std::endl;
        id_ex_next_.valid = false;
        // std::cout << "  [ID_STAGE] END (flushed)" << std::endl;
        return;
    }

    if (!if_id_.valid)
    {
        // std::cout << "  [ID_STAGE] No valid instruction from IF" << std::endl;
        id_ex_next_.valid = false;
        // std::cout << "  [ID_STAGE] END (bubble)" << std::endl;
        return;
    }

    uint32_t instr = if_id_.instruction;
    // std::cout << "  [ID_STAGE] Decoding instruction: 0x" << std::hex << instr << std::dec
              // << " at PC=" << if_id_.pc << std::endl;

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

    // std::cout << "  [ID_STAGE] Decoded: opcode=0x" << std::hex << (int)opcode << std::dec
    //           << " rs1=" << (int)id_ex_next_.rs1
    //           << " rs2=" << (int)id_ex_next_.rs2
    //           << " rd=" << (int)id_ex_next_.rd
    //           << " imm=" << id_ex_next_.imm << std::endl;

    id_ex_next_.reg1_value = registers_->ReadGpr(id_ex_next_.rs1);
    id_ex_next_.reg2_value = registers_->ReadGpr(id_ex_next_.rs2);

    // std::cout << "  [ID_STAGE] Register values: x" << (int)id_ex_next_.rs1 << "=" << id_ex_next_.reg1_value
    //           << " x" << (int)id_ex_next_.rs2 << "=" << id_ex_next_.reg2_value << std::endl;

    control_unit_.SetControlSignals(instr);
    id_ex_next_.reg_write = control_unit_.GetRegWrite();
    id_ex_next_.mem_read = control_unit_.GetMemRead();
    id_ex_next_.mem_write = control_unit_.GetMemWrite();
    id_ex_next_.mem_to_reg = control_unit_.GetMemToReg();
    id_ex_next_.alu_src = control_unit_.GetAluSrc();
    id_ex_next_.branch = control_unit_.GetBranch();
    id_ex_next_.is_float = instruction_set::isFInstruction(instr) || instruction_set::isDInstruction(instr);

    // std::cout << "  [ID_STAGE] Control signals: " << std::endl;
    // std::cout << "  [ID_STAGE]   reg_write=" << id_ex_next_.reg_write << std::endl;
    // std::cout << "  [ID_STAGE]   mem_read=" << id_ex_next_.mem_read << std::endl;
    // std::cout << "  [ID_STAGE]   mem_write=" << id_ex_next_.mem_write << std::endl;
    // std::cout << "  [ID_STAGE]   mem_to_reg=" << id_ex_next_.mem_to_reg << std::endl;
    // std::cout << "  [ID_STAGE]   alu_src=" << id_ex_next_.alu_src << std::endl;
    // std::cout << "  [ID_STAGE]   branch=" << id_ex_next_.branch << std::endl;

    // std::cout << "  [ID_STAGE] END (success)" << std::endl;
}

void RVSSVMPipelined::EX_stage()
{
    // std::cout << "  [EX_STAGE] START" << std::endl;
    // std::cout << "  [EX_STAGE] id_ex_.valid: " << id_ex_.valid << std::endl;

    if (!id_ex_.valid)
    {
        // std::cout << "  [EX_STAGE] No valid instruction from ID" << std::endl;
        ex_mem_next_.valid = false;
        // std::cout << "  [EX_STAGE] END (bubble)" << std::endl;
        return;
    }

    // std::cout << "  [EX_STAGE] Processing instruction at PC=" << id_ex_.pc << std::endl;
    // std::cout << "  [EX_STAGE] id_ex_.rd=" << (int)id_ex_.rd << std::endl;
    // std::cout << "  [EX_STAGE] id_ex_.reg_write=" << id_ex_.reg_write << std::endl;
    // std::cout << "  [EX_STAGE] id_ex_.mem_read=" << id_ex_.mem_read << std::endl;
    // std::cout << "  [EX_STAGE] id_ex_.mem_write=" << id_ex_.mem_write << std::endl;
    // std::cout << "  [EX_STAGE] id_ex_.branch=" << id_ex_.branch << std::endl;

    ex_mem_next_.valid = true;
    ex_mem_next_.pc = id_ex_.pc;
    ex_mem_next_.instruction = id_ex_.instruction;
    ex_mem_next_.rd = id_ex_.rd;
    ex_mem_next_.reg_write = id_ex_.reg_write;
    ex_mem_next_.mem_read = id_ex_.mem_read;
    ex_mem_next_.mem_write = id_ex_.mem_write;
    ex_mem_next_.mem_to_reg = id_ex_.mem_to_reg;
    ex_mem_next_.reg2_value = id_ex_.reg2_value;

    // std::cout << "  [EX_STAGE] COPIED to ex_mem_next_:" << std::endl;
    // std::cout << "  [EX_STAGE]   ex_mem_next_.rd=" << (int)ex_mem_next_.rd << std::endl;
    // std::cout << "  [EX_STAGE]   ex_mem_next_.reg_write=" << ex_mem_next_.reg_write << std::endl;

    uint64_t op1 = id_ex_.reg1_value;
    uint64_t op2 = id_ex_.alu_src ? static_cast<uint64_t>(static_cast<int64_t>(id_ex_.imm)) : id_ex_.reg2_value;

    // std::cout << "  [EX_STAGE] ALU operands: op1=" << op1 << " op2=" << op2 << std::endl;

    alu::AluOp aluOperation = control_unit_.GetAluSignal(id_ex_.instruction, control_unit_.GetAluOp());
    bool overflow = false;
    std::tie(ex_mem_next_.alu_result, overflow) = alu_.execute(aluOperation, op1, op2);

    // std::cout << "  [EX_STAGE] ALU result=" << ex_mem_next_.alu_result << std::endl;

    ex_mem_next_.branch_taken = false;

    if (id_ex_.branch)
    {
        // std::cout << "  [EX_STAGE] Branch instruction detected" << std::endl;
        uint8_t funct3 = id_ex_.funct3;
        bool take = false;
        switch (funct3)
        {
        case 0b000:
            take = (ex_mem_next_.alu_result == 0);
            break; // BEQ
        case 0b001:
            take = (ex_mem_next_.alu_result != 0);
            break; // BNE
        case 0b100:
            take = (ex_mem_next_.alu_result == 1);
            break; // BLT
        case 0b101:
            take = (ex_mem_next_.alu_result == 0);
            break; // BGE
        case 0b110:
            take = (ex_mem_next_.alu_result == 1);
            break; // BLTU
        case 0b111:
            take = (ex_mem_next_.alu_result == 0);
            break; // BGEU
        }

        // std::cout << "  [EX_STAGE] Branch take=" << take << std::endl;

        if (take)
        {
            ex_mem_next_.branch_taken = true;
            ex_mem_next_.branch_target = static_cast<uint64_t>(static_cast<int64_t>(id_ex_.pc)) + id_ex_.imm;
            pc_update_pending_ = true;
            pc_update_value_ = ex_mem_next_.branch_target;
            branch_taken_this_cycle_ = true;
            // std::cout << "  [EX_STAGE] Branch taken to " << ex_mem_next_.branch_target << std::endl;
        }
    }

    // std::cout << "  [EX_STAGE] END (success)" << std::endl;
}

void RVSSVMPipelined::MEM_stage()
{
    // std::cout << "  [MEM_STAGE] START" << std::endl;
    // std::cout << "  [MEM_STAGE] ex_mem_.valid: " << ex_mem_.valid << std::endl;

    if (!ex_mem_.valid)
    {
        // std::cout << "  [MEM_STAGE] No valid instruction from EX" << std::endl;
        mem_wb_next_.valid = false;
        // std::cout << "  [MEM_STAGE] END (bubble)" << std::endl;
        return;
    }

    // std::cout << "  [MEM_STAGE] Processing instruction at PC=" << ex_mem_.pc << std::endl;
    // std::cout << "  [MEM_STAGE] ex_mem_.rd=" << (int)ex_mem_.rd << std::endl;
    // std::cout << "  [MEM_STAGE] ex_mem_.reg_write=" << ex_mem_.reg_write << std::endl;
    // std::cout << "  [MEM_STAGE] ex_mem_.mem_read=" << ex_mem_.mem_read << std::endl;
    // std::cout << "  [MEM_STAGE] ex_mem_.mem_write=" << ex_mem_.mem_write << std::endl;
    // std::cout << "  [MEM_STAGE] ex_mem_.alu_result=" << ex_mem_.alu_result << std::endl;

    mem_wb_next_.valid = true;
    mem_wb_next_.rd = ex_mem_.rd;
    mem_wb_next_.reg_write = ex_mem_.reg_write;
    mem_wb_next_.mem_to_reg = ex_mem_.mem_to_reg;
    mem_wb_next_.alu_result = ex_mem_.alu_result;
    mem_wb_next_.pc = ex_mem_.pc;

    // std::cout << "  [MEM_STAGE] Set mem_wb_next_.rd=" << (int)mem_wb_next_.rd << std::endl;
    // std::cout << "  [MEM_STAGE] Set mem_wb_next_.reg_write=" << mem_wb_next_.reg_write << std::endl;

    if (ex_mem_.mem_read)
    {
        // std::cout << "  [MEM_STAGE] Performing memory read" << std::endl;
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
        // std::cout << "  [MEM_STAGE] Memory read result: " << mem_wb_next_.mem_data << std::endl;
    }

    if (ex_mem_.mem_write)
    {
        // std::cout << "  [MEM_STAGE] Performing memory write" << std::endl;
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

    // std::cout << "  [MEM_STAGE] END (success)" << std::endl;
}

void RVSSVMPipelined::WB_stage()
{
    // std::cout << "  [WB_STAGE] START" << std::endl;
    // std::cout << "  [WB_STAGE] mem_wb_.valid=" << mem_wb_.valid << std::endl;
    // std::cout << "  [WB_STAGE] mem_wb_.rd=" << (int)mem_wb_.rd << std::endl;
    // std::cout << "  [WB_STAGE] mem_wb_.reg_write=" << mem_wb_.reg_write << std::endl;
    // std::cout << "  [WB_STAGE] mem_wb_.alu_result=" << mem_wb_.alu_result << std::endl;
    // std::cout << "  [WB_STAGE] mem_wb_.mem_data=" << mem_wb_.mem_data << std::endl;
    // std::cout << "  [WB_STAGE] mem_wb_.mem_to_reg=" << mem_wb_.mem_to_reg << std::endl;

    if (!mem_wb_.valid)
    {
        // std::cout << "  [WB_STAGE] END (not valid)" << std::endl;
        return;
    }

    uint64_t write_val = mem_wb_.mem_to_reg ? mem_wb_.mem_data : mem_wb_.alu_result;

    // std::cout << "  [WB_STAGE] Calculated write_val=" << write_val << std::endl;
    // std::cout << "  [WB_STAGE] Will write to x" << (int)mem_wb_.rd << " value=" << write_val << std::endl;

    if (mem_wb_.reg_write)
    {
        // std::cout << "  [WB_STAGE] reg_write is TRUE" << std::endl;
        if (mem_wb_.rd != 0)
        {
            // std::cout << "  [WB_STAGE] rd != 0, performing write" << std::endl;
            // std::cout << "  [WB_STAGE] BEFORE WriteGpr: x" << (int)mem_wb_.rd << std::endl;

            registers_->WriteGpr(mem_wb_.rd, write_val);

            // std::cout << "  [WB_STAGE] AFTER WriteGpr" << std::endl;

            // Verify the write
            uint64_t verify = registers_->ReadGpr(mem_wb_.rd);
            // std::cout << "  [WB_STAGE] Verification read: x" << (int)mem_wb_.rd
            // << " = " << verify << std::endl;
        }
        else
        {
            // std::cout << "  [WB_STAGE] rd=0, skipping write (x0 is hardwired to 0)" << std::endl;
        }
    }
    else
    {
        // std::cout << "  [WB_STAGE] reg_write=FALSE, skipping write" << std::endl;
    }

    if (mem_wb_.valid)
    {
        instructions_retired_++;
        // std::cout << "  [WB_STAGE] Instructions retired: " << instructions_retired_ << std::endl;
    }

    // std::cout << "  [WB_STAGE] END" << std::endl;
}

void RVSSVMPipelined::advance_pipeline_registers()
{
    // std::cout << "  [ADVANCE] === BEFORE ADVANCE ===" << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_next_.valid=" << mem_wb_next_.valid << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_next_.rd=" << (int)mem_wb_next_.rd << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_next_.reg_write=" << mem_wb_next_.reg_write << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_next_.alu_result=" << mem_wb_next_.alu_result << std::endl;

    // NOW advance all pipeline registers
    mem_wb_ = mem_wb_next_;
    ex_mem_ = ex_mem_next_;
    id_ex_ = id_ex_next_;
    if_id_ = if_id_next_;

    // std::cout << "  [ADVANCE] === AFTER ADVANCE ===" << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_.valid=" << mem_wb_.valid << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_.rd=" << (int)mem_wb_.rd << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_.reg_write=" << mem_wb_.reg_write << std::endl;
    // std::cout << "  [ADVANCE] mem_wb_.alu_result=" << mem_wb_.alu_result << std::endl;

    // Clear next registers for next cycle
    mem_wb_next_ = MEM_WB();
    ex_mem_next_ = EX_MEM();
    id_ex_next_ = ID_EX();
    if_id_next_ = IF_ID();

    // Rest of your emit code...
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

    branch_taken_this_cycle_ = false;

    // std::cout << "  [ADVANCE] === COMPLETE ===" << std::endl;
}

void RVSSVMPipelined::Run()
{
    // Reset();
    bool pipeline_empty = false;

    while (!stop_requested_)
    {

        // If there is nothing to do (no in-flight instructions and nothing left to fetch) break
        bool pipeline_has_work = (if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
        bool fetch_remaining = (program_counter_ < program_size_);

        // std::cout << "[CYCLE] Pipeline has work: " << pipeline_has_work
        //           << " (IF:" << if_id_.valid << " ID:" << id_ex_.valid
        //           << " EX:" << ex_mem_.valid << " MEM:" << mem_wb_.valid << ")" << std::endl;
        // std::cout << "[CYCLE] Fetch remaining: " << fetch_remaining
        //           << " (PC=" << program_counter_ << ", size=" << program_size_ << ")" << std::endl;

        if (!pipeline_has_work && !fetch_remaining)
            break;

        // one cycle
        IF_stage();
        ID_stage();
        EX_stage();
        MEM_stage();
        WB_stage();
        advance_pipeline_registers();

        cycle_s_++;
    }
    if (program_counter_ >= program_size_ && pipeline_empty)
    {
        output_status_ = "VM_PROGRAM_END";
    }
}

void RVSSVMPipelined::DebugRun()
{
    Run();
}

void RVSSVMPipelined::Step()
{
    // IF_stage();
    // ID_stage();
    // EX_stage();
    // MEM_stage();
    // WB_stage();
    // advance_pipeline_registers();
    // cycle_s_++;
    bool pipeline_has_work = (if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
    bool fetch_remaining = (program_counter_ < program_size_);

    // If no more work and no more instructions left to fetch, stop
    // std::cout << "[CYCLE] Pipeline has work: " << pipeline_has_work
    //           << " (IF:" << if_id_.valid << " ID:" << id_ex_.valid
    //           << " EX:" << ex_mem_.valid << " MEM:" << mem_wb_.valid << ")" << std::endl;
    // std::cout << "[CYCLE] Fetch remaining: " << fetch_remaining
    //           << " (PC=" << program_counter_ << ", size=" << program_size_ << ")" << std::endl;

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

    // Normal pipeline cycle
    IF_stage();
    ID_stage();
    EX_stage();
    MEM_stage();
    WB_stage();
    advance_pipeline_registers();

    cycle_s_++;
}

void RVSSVMPipelined::Undo()
{
    std::cerr << "Undo not supported in pipelined VM" << std::endl;
}

void RVSSVMPipelined::Redo()
{
    std::cerr << "Redo not supported in pipelined VM" << std::endl;
}

bool RVSSVMPipelined::IsPipelineEmpty() const
{
    // Example: assuming you have pipeline registers IF/ID, ID/EX, EX/MEM, MEM/WB
    return !(if_id_.valid || id_ex_.valid || ex_mem_.valid || mem_wb_.valid);
}

// void RVSSVMPipelined::DumpPipelineState()
// {
//     auto show_if = [&](const IF_ID &r) {
//         std::cout << "[IF_ID] valid=" << r.valid << " pc=" << r.pc << " instr=0x" << std::hex << r.instruction << std::dec << std::endl;
//     };
//     auto show_id = [&](const ID_EX &r) {
//         std::cout << "[ID_EX] valid=" << r.valid << " pc=" << r.pc << " rd=" << int(r.rd)
//         << " rs1=" << int(r.rs1) << " rs2=" << int(r.rs2)
//         << " imm=" << r.imm << " reg1=0x" << std::hex << r.reg1_value << std::dec
//         << " reg2=0x" << std::hex << r.reg2_value << std::dec << std::endl;
//     };
//     auto show_ex = [&](const EX_MEM &r) {
//         std::cout << "[EX_MEM] valid=" << r.valid << " pc=" << r.pc << " rd=" << int(r.rd)
//         << " alu_res=0x" << std::hex << r.alu_result << std::dec
//         << " reg2=0x" << std::hex << r.reg2_value << std::dec
//         << " mem_read=" << r.mem_read << " mem_write=" << r.mem_write
//         << " branch_taken=" << r.branch_taken << std::endl;
//     };
//     auto show_mem = [&](const MEM_WB &r) {
//         std::cout << "[MEM_WB] valid=" << r.valid << " pc=" << r.pc << " rd=" << int(r.rd)
//         << " reg_write=" << r.reg_write
//         << " mem_to_reg=" << r.mem_to_reg
//         << " alu_res=0x" << std::hex << r.alu_result
//         << " mem_data=0x" << r.mem_data << std::dec << std::endl;
//     };

//     std::cout << "===== PIPELINE STATE =====" << std::endl;
//     show_if(if_id_);
//     show_id(id_ex_);
//     show_ex(ex_mem_);
//     show_mem(mem_wb_);
//     std::cout << "pc=" << program_counter_ << " program_size=" << program_size_
//               << " instructions_retired=" << instructions_retired_
//               << " cycles=" << cycle_s_
//               << " pc_update_pending=" << pc_update_pending_
//               << std::endl;
//     std::cout << "==========================" << std::endl;
// }
