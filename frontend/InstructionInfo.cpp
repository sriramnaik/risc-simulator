
#include "InstructionInfo.h"////;

void InstructionDatabase::initialize() {
    // ==================== LOAD/STORE INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "ld", "ld rd, offset(rs1)",
        "Load Doubleword: Load 64-bit value from memory into rd",
        "Load/Store",
        {"ld x1, 0(x2)", "ld a0, 8(sp)", "ld t0, -16(s0)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "lw", "lw rd, offset(rs1)",
        "Load Word: Load 32-bit value from memory, sign-extend to 64 bits",
        "Load/Store",
        {"lw x1, 0(x2)", "lw a0, 4(sp)", "lw t0, 100(s1)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "lwu", "lwu rd, offset(rs1)",
        "Load Word Unsigned: Load 32-bit value from memory, zero-extend to 64 bits",
        "Load/Store",
        {"lwu x1, 0(x2)", "lwu a0, 4(sp)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "lh", "lh rd, offset(rs1)",
        "Load Halfword: Load 16-bit value from memory, sign-extend to 64 bits",
        "Load/Store",
        {"lh x1, 0(x2)", "lh a0, 2(sp)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "lhu", "lhu rd, offset(rs1)",
        "Load Halfword Unsigned: Load 16-bit value from memory, zero-extend to 64 bits",
        "Load/Store",
        {"lhu x1, 0(x2)", "lhu a0, 2(sp)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "lb", "lb rd, offset(rs1)",
        "Load Byte: Load 8-bit value from memory, sign-extend to 64 bits",
        "Load/Store",
        {"lb x1, 0(x2)", "lb a0, 1(sp)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "lbu", "lbu rd, offset(rs1)",
        "Load Byte Unsigned: Load 8-bit value from memory, zero-extend to 64 bits",
        "Load/Store",
        {"lbu x1, 0(x2)", "lbu a0, 1(sp)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "sd", "sd rs2, offset(rs1)",
        "Store Doubleword: Store 64-bit value from rs2 to memory",
        "Load/Store",
        {"sd x1, 0(x2)", "sd a0, 8(sp)", "sd t0, -16(s0)"},
        "S-Type"
        ));

    addInstruction(InstructionInfo(
        "sw", "sw rs2, offset(rs1)",
        "Store Word: Store lower 32 bits of rs2 to memory",
        "Load/Store",
        {"sw x1, 0(x2)", "sw a0, 4(sp)", "sw zero, 0(t0)"},
        "S-Type"
        ));

    addInstruction(InstructionInfo(
        "sh", "sh rs2, offset(rs1)",
        "Store Halfword: Store lower 16 bits of rs2 to memory",
        "Load/Store",
        {"sh x1, 0(x2)", "sh a0, 2(sp)"},
        "S-Type"
        ));

    addInstruction(InstructionInfo(
        "sb", "sb rs2, offset(rs1)",
        "Store Byte: Store lower 8 bits of rs2 to memory",
        "Load/Store",
        {"sb x1, 0(x2)", "sb a0, 1(sp)"},
        "S-Type"
        ));

    // ==================== FLOATING-POINT LOAD/STORE ====================
    addInstruction(InstructionInfo(
        "flw", "flw fd, offset(rs1)",
        "Floating-point Load Word: Load 32-bit float from memory, NaN-box to 64 bits",
        "Floating-Point Load/Store",
        {"flw f0, 0(x2)", "flw fa0, 8(sp)", "flw ft1, 100(s0)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "fld", "fld fd, offset(rs1)",
        "Floating-point Load Doubleword: Load 64-bit double from memory",
        "Floating-Point Load/Store",
        {"fld f0, 0(x2)", "fld fa0, 8(sp)", "fld ft1, 16(s0)"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "fsw", "fsw fs2, offset(rs1)",
        "Floating-point Store Word: Store 32-bit float to memory",
        "Floating-Point Load/Store",
        {"fsw f0, 0(x2)", "fsw fa0, 8(sp)", "fsw ft1, 100(s0)"},
        "S-Type"
        ));

    addInstruction(InstructionInfo(
        "fsd", "fsd fs2, offset(rs1)",
        "Floating-point Store Doubleword: Store 64-bit double to memory",
        "Floating-Point Load/Store",
        {"fsd f0, 0(x2)", "fsd fa0, 8(sp)", "fsd ft1, 16(s0)"},
        "S-Type"
        ));

    // ==================== ARITHMETIC INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "add", "add rd, rs1, rs2",
        "Add: rd = rs1 + rs2",
        "Arithmetic",
        {"add x1, x2, x3", "add a0, a0, a1", "add t0, t1, t2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "addi", "addi rd, rs1, imm",
        "Add Immediate: rd = rs1 + imm",
        "Arithmetic",
        {"addi x1, x2, 10", "addi sp, sp, -16", "addi a0, zero, 5"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "sub", "sub rd, rs1, rs2",
        "Subtract: rd = rs1 - rs2",
        "Arithmetic",
        {"sub x1, x2, x3", "sub a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "mul", "mul rd, rs1, rs2",
        "Multiply: rd = (rs1 * rs2)[63:0]",
        "Arithmetic",
        {"mul x1, x2, x3", "mul a0, a1, a2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "div", "div rd, rs1, rs2",
        "Divide: rd = rs1 / rs2 (signed)",
        "Arithmetic",
        {"div x1, x2, x3", "div a0, a1, a2"},
        "R-Type"
        ));

    // ==================== FLOATING-POINT ARITHMETIC ====================
    addInstruction(InstructionInfo(
        "fadd.s", "fadd.s fd, fs1, fs2",
        "Floating-point Add Single: fd = fs1 + fs2 (32-bit)",
        "Floating-Point Arithmetic",
        {"fadd.s f0, f1, f2", "fadd.s fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fadd.d", "fadd.d fd, fs1, fs2",
        "Floating-point Add Double: fd = fs1 + fs2 (64-bit)",
        "Floating-Point Arithmetic",
        {"fadd.d f0, f1, f2", "fadd.d fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fsub.s", "fsub.s fd, fs1, fs2",
        "Floating-point Subtract Single: fd = fs1 - fs2 (32-bit)",
        "Floating-Point Arithmetic",
        {"fsub.s f0, f1, f2", "fsub.s fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fsub.d", "fsub.d fd, fs1, fs2",
        "Floating-point Subtract Double: fd = fs1 - fs2 (64-bit)",
        "Floating-Point Arithmetic",
        {"fsub.d f0, f1, f2", "fsub.d fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fmul.s", "fmul.s fd, fs1, fs2",
        "Floating-point Multiply Single: fd = fs1 * fs2 (32-bit)",
        "Floating-Point Arithmetic",
        {"fmul.s f0, f1, f2", "fmul.s fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fmul.d", "fmul.d fd, fs1, fs2",
        "Floating-point Multiply Double: fd = fs1 * fs2 (64-bit)",
        "Floating-Point Arithmetic",
        {"fmul.d f0, f1, f2", "fmul.d fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fdiv.s", "fdiv.s fd, fs1, fs2",
        "Floating-point Divide Single: fd = fs1 / fs2 (32-bit)",
        "Floating-Point Arithmetic",
        {"fdiv.s f0, f1, f2", "fdiv.s fa0, fa1, fa2"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "fdiv.d", "fdiv.d fd, fs1, fs2",
        "Floating-point Divide Double: fd = fs1 / fs2 (64-bit)",
        "Floating-Point Arithmetic",
        {"fdiv.d f0, f1, f2", "fdiv.d fa0, fa1, fa2"},
        "R-Type"
        ));

    // ==================== LOGICAL INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "and", "and rd, rs1, rs2",
        "Bitwise AND: rd = rs1 & rs2",
        "Logical",
        {"and x1, x2, x3", "and a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "andi", "andi rd, rs1, imm",
        "Bitwise AND Immediate: rd = rs1 & imm",
        "Logical",
        {"andi x1, x2, 0xFF", "andi a0, a0, 15"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "or", "or rd, rs1, rs2",
        "Bitwise OR: rd = rs1 | rs2",
        "Logical",
        {"or x1, x2, x3", "or a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "ori", "ori rd, rs1, imm",
        "Bitwise OR Immediate: rd = rs1 | imm",
        "Logical",
        {"ori x1, x2, 0xFF", "ori a0, a0, 1"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "xor", "xor rd, rs1, rs2",
        "Bitwise XOR: rd = rs1 ^ rs2",
        "Logical",
        {"xor x1, x2, x3", "xor a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "xori", "xori rd, rs1, imm",
        "Bitwise XOR Immediate: rd = rs1 ^ imm",
        "Logical",
        {"xori x1, x2, -1", "xori a0, a0, 1"},
        "I-Type"
        ));

    // ==================== SHIFT INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "sll", "sll rd, rs1, rs2",
        "Shift Left Logical: rd = rs1 << rs2",
        "Shift",
        {"sll x1, x2, x3", "sll a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "slli", "slli rd, rs1, shamt",
        "Shift Left Logical Immediate: rd = rs1 << shamt",
        "Shift",
        {"slli x1, x2, 2", "slli a0, a0, 3"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "srl", "srl rd, rs1, rs2",
        "Shift Right Logical: rd = rs1 >> rs2 (zero-extend)",
        "Shift",
        {"srl x1, x2, x3", "srl a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "srli", "srli rd, rs1, shamt",
        "Shift Right Logical Immediate: rd = rs1 >> shamt (zero-extend)",
        "Shift",
        {"srli x1, x2, 2", "srli a0, a0, 3"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "sra", "sra rd, rs1, rs2",
        "Shift Right Arithmetic: rd = rs1 >> rs2 (sign-extend)",
        "Shift",
        {"sra x1, x2, x3", "sra a0, a0, a1"},
        "R-Type"
        ));

    addInstruction(InstructionInfo(
        "srai", "srai rd, rs1, shamt",
        "Shift Right Arithmetic Immediate: rd = rs1 >> shamt (sign-extend)",
        "Shift",
        {"srai x1, x2, 2", "srai a0, a0, 3"},
        "I-Type"
        ));

    // ==================== BRANCH INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "beq", "beq rs1, rs2, label",
        "Branch if Equal: if (rs1 == rs2) pc += offset",
        "Branch",
        {"beq x1, x2, loop", "beq a0, zero, end", "beq t0, t1, skip"},
        "B-Type"
        ));

    addInstruction(InstructionInfo(
        "bne", "bne rs1, rs2, label",
        "Branch if Not Equal: if (rs1 != rs2) pc += offset",
        "Branch",
        {"bne x1, x2, loop", "bne a0, zero, continue"},
        "B-Type"
        ));

    addInstruction(InstructionInfo(
        "blt", "blt rs1, rs2, label",
        "Branch if Less Than: if (rs1 < rs2) pc += offset (signed)",
        "Branch",
        {"blt x1, x2, loop", "blt a0, a1, skip"},
        "B-Type"
        ));

    addInstruction(InstructionInfo(
        "bge", "bge rs1, rs2, label",
        "Branch if Greater or Equal: if (rs1 >= rs2) pc += offset (signed)",
        "Branch",
        {"bge x1, x2, loop", "bge a0, zero, positive"},
        "B-Type"
        ));

    addInstruction(InstructionInfo(
        "bltu", "bltu rs1, rs2, label",
        "Branch if Less Than Unsigned: if (rs1 < rs2) pc += offset (unsigned)",
        "Branch",
        {"bltu x1, x2, loop", "bltu a0, a1, skip"},
        "B-Type"
        ));

    addInstruction(InstructionInfo(
        "bgeu", "bgeu rs1, rs2, label",
        "Branch if Greater or Equal Unsigned: if (rs1 >= rs2) pc += offset (unsigned)",
        "Branch",
        {"bgeu x1, x2, loop", "bgeu a0, a1, continue"},
        "B-Type"
        ));

    // ==================== JUMP INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "jal", "jal rd, label",
        "Jump and Link: rd = pc + 4, pc += offset",
        "Jump",
        {"jal ra, function", "jal x1, loop", "jal zero, skip"},
        "J-Type"
        ));

    addInstruction(InstructionInfo(
        "jalr", "jalr rd, rs1, offset",
        "Jump and Link Register: rd = pc + 4, pc = rs1 + offset",
        "Jump",
        {"jalr ra, 0(t0)", "jalr zero, 0(ra)", "jalr x1, 8(x2)"},
        "I-Type"
        ));

    // ==================== UPPER IMMEDIATE ====================
    addInstruction(InstructionInfo(
        "lui", "lui rd, imm",
        "Load Upper Immediate: rd = imm << 12",
        "Upper Immediate",
        {"lui x1, 0x12345", "lui a0, 0x80000"},
        "U-Type"
        ));

    addInstruction(InstructionInfo(
        "auipc", "auipc rd, imm",
        "Add Upper Immediate to PC: rd = pc + (imm << 12)",
        "Upper Immediate",
        {"auipc x1, 0x12345", "auipc a0, 0"},
        "U-Type"
        ));

    // ==================== SYSTEM INSTRUCTIONS ====================
    addInstruction(InstructionInfo(
        "ecall", "ecall",
        "Environment Call: Make a system call",
        "System",
        {"ecall"},
        "I-Type"
        ));

    addInstruction(InstructionInfo(
        "ebreak", "ebreak",
        "Environment Break: Trigger a breakpoint",
        "System",
        {"ebreak"},
        "I-Type"
        ));

    // Add more instructions as needed...
}

void InstructionDatabase::addInstruction(const InstructionInfo& info) {
    instructions_[info.mnemonic.toLower()] = info;
}

const InstructionInfo* InstructionDatabase::getInfo(const QString& mnemonic) const {
    auto it = instructions_.find(mnemonic.toLower());
    return (it != instructions_.end()) ? &it.value() : nullptr;
}

QStringList InstructionDatabase::getAllMnemonics() const {
    return instructions_.keys();
}

QStringList InstructionDatabase::getCompletions(const QString& prefix) const {
    QStringList completions;
    QString lowerPrefix = prefix.toLower();

    for (const auto& mnemonic : instructions_.keys()) {
        if (mnemonic.startsWith(lowerPrefix)) {
            completions << mnemonic;
        }
    }

    completions.sort();
    return completions;
}
