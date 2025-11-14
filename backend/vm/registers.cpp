#include "registers.h"
#include <stdexcept>
#include <iostream>

// #include <algorithm>

// --------- Register, ABI, and Alias Data ---------

const std::unordered_map<std::string, int> abi_name_to_gpr_index {
    {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
    {"t0", 5}, {"t1", 6}, {"t2", 7},
    {"s0", 8}, {"fp", 8}, {"s1", 9},
    {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15},
    {"a6", 16}, {"a7", 17},
    {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24},
    {"s9", 25}, {"s10", 26}, {"s11", 27},
    {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31},
    // raw xN aliases
    {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3}, {"x4", 4}, {"x5", 5}, {"x6", 6}, {"x7", 7},
    {"x8", 8}, {"x9", 9}, {"x10", 10}, {"x11", 11}, {"x12", 12}, {"x13", 13}, {"x14", 14},
    {"x15", 15}, {"x16", 16}, {"x17", 17}, {"x18", 18}, {"x19", 19}, {"x20", 20}, {"x21", 21},
    {"x22", 22}, {"x23", 23}, {"x24", 24}, {"x25", 25}, {"x26", 26}, {"x27", 27}, {"x28", 28},
    {"x29", 29}, {"x30", 30}, {"x31", 31}
};

const std::unordered_set<std::string> valid_general_purpose_registers = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9",
    "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
    "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29",
    "x30", "x31",
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
    "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

const std::unordered_set<std::string> valid_floating_point_registers = {
    "f0","f1","f2","f3","f4","f5","f6","f7","f8","f9",
    "f10","f11","f12","f13","f14","f15","f16","f17","f18","f19",
    "f20","f21","f22","f23","f24","f25","f26","f27","f28","f29",
    "f30","f31",
    // ABI aliases
    "ft0","ft1","ft2","ft3","ft4","ft5","ft6","ft7",
    "fs0","fs1","fa0","fa1","fa2","fa3","fa4","fa5",
    "fa6","fa7","fs2","fs3","fs4","fs5","fs6","fs7",
    "fs8","fs9","fs10","fs11","ft8","ft9","ft10","ft11",
    "ft12","ft13","ft14","ft15","ft16","ft17","ft18","ft19",
    "ft20","ft21","ft22","ft23","ft24","ft25","ft26","ft27",
    "ft28","ft29","ft30","ft31"
};

const std::unordered_set<std::string> valid_csr_registers = {
    "fflags","frm","fcsr"
};

const std::unordered_map<std::string, int> csr_to_address {
    {"fflags", 0x001},
    {"frm",    0x002},
    {"fcsr",   0x003}
};

const std::unordered_map<std::string, std::string> reg_alias_to_name = {
    // integer GPR aliases
    {"zero", "x0"}, {"ra", "x1"}, {"sp", "x2"}, {"gp", "x3"}, {"tp", "x4"},
    {"t0", "x5"}, {"t1", "x6"}, {"t2", "x7"},
    {"s0", "x8"}, {"fp", "x8"}, {"s1", "x9"},
    {"a0", "x10"}, {"a1", "x11"}, {"a2", "x12"}, {"a3", "x13"},
    {"a4", "x14"}, {"a5", "x15"}, {"a6", "x16"}, {"a7", "x17"},
    {"s2", "x18"}, {"s3", "x19"}, {"s4", "x20"}, {"s5", "x21"},
    {"s6", "x22"}, {"s7", "x23"}, {"s8", "x24"}, {"s9", "x25"},
    {"s10", "x26"}, {"s11", "x27"}, {"t3", "x28"}, {"t4", "x29"},
    {"t5", "x30"}, {"t6", "x31"},
    // direct xN
    {"x0", "x0"}, {"x1", "x1"}, {"x2", "x2"}, {"x3", "x3"}, {"x4", "x4"}, {"x5", "x5"}, {"x6", "x6"}, {"x7", "x7"},
    {"x8", "x8"}, {"x9", "x9"}, {"x10", "x10"}, {"x11", "x11"}, {"x12", "x12"}, {"x13", "x13"}, {"x14", "x14"}, {"x15", "x15"},
    {"x16", "x16"}, {"x17", "x17"}, {"x18", "x18"}, {"x19", "x19"}, {"x20", "x20"}, {"x21", "x21"}, {"x22", "x22"},
    {"x23", "x23"}, {"x24", "x24"}, {"x25", "x25"}, {"x26", "x26"}, {"x27", "x27"}, {"x28", "x28"}, {"x29", "x29"},
    {"x30", "x30"}, {"x31", "x31"},
    // Floating point FPR ABI aliases
    {"ft0","f0"},{"ft1","f1"},{"ft2","f2"},{"ft3","f3"},{"ft4","f4"},{"ft5","f5"},{"ft6","f6"},{"ft7","f7"},
    {"fs0","f8"},{"fs1","f9"},{"fa0","f10"},{"fa1","f11"},{"fa2","f12"},{"fa3","f13"},{"fa4","f14"},{"fa5","f15"},
    {"fa6","f16"},{"fa7","f17"},{"fs2","f18"},{"fs3","f19"},{"fs4","f20"},{"fs5","f21"},{"fs6","f22"},{"fs7","f23"},
    {"fs8","f24"},{"fs9","f25"},{"fs10","f26"},{"fs11","f27"},
    {"ft8","f28"},{"ft9","f29"},{"ft10","f30"},{"ft11","f31"},
    // direct fN
    {"f0","f0"},{"f1","f1"},{"f2","f2"},{"f3","f3"},{"f4","f4"},{"f5","f5"},{"f6","f6"},{"f7","f7"},
    {"f8","f8"},{"f9","f9"},{"f10","f10"},{"f11","f11"},{"f12","f12"},{"f13","f13"},{"f14","f14"},{"f15","f15"},
    {"f16","f16"},{"f17","f17"},{"f18","f18"},{"f19","f19"},{"f20","f20"},{"f21","f21"},{"f22","f22"},
    {"f23","f23"},{"f24","f24"},{"f25","f25"},{"f26","f26"},{"f27","f27"},{"f28","f28"},{"f29","f29"},
    {"f30","f30"},{"f31","f31"},
    // CSRs
    {"fflags","fflags"},{"frm","frm"},{"fcsr","fcsr"}
};


// --------- RegisterFile Implementation ---------

RegisterFile::RegisterFile() { Reset(); }

void RegisterFile::Reset() {
    gpr_.fill(0);
    fpr_.fill(0);
    csr_.fill(0);
    gpr_[2] = 0x00200000;    // sp
    gpr_[3] = 0x10008000;    // gp
    gpr_[4] = 0x10010000;    // tp
    csr_[0x002] = 0b000;     // RNE IEEE 754
}

// uint64_t RegisterFile::ReadGpr(size_t reg) const {
//     if (reg >= NUM_GPR) throw std::out_of_range("Invalid GPR index");
//     if (reg == 0) return 0;
//     return gpr_[reg];
// }

void RegisterFile::WriteGpr(size_t reg, uint64_t value) {
    if (reg >= NUM_GPR) throw std::out_of_range("Invalid GPR index");
    if (reg == 0) return;
    // std::cout << "[WRITEGPR] ISA: " << (current_isa == ISA::RV64 ? "RV64" : "RV32") << " Value: " << std::hex << value << std::endl;
    if (current_isa == ISA::RV32)
        gpr_[reg] = value & 0xFFFFFFFF;
    else
        gpr_[reg] = value;
}

uint64_t RegisterFile::ReadGpr(size_t reg) const {
    if (reg >= NUM_GPR) throw std::out_of_range("Invalid GPR index");
    if (reg == 0) return 0;
    // std::cout << "[READGPR] ISA: " << (current_isa == ISA::RV64 ? "RV64" : "RV32") << " Value: " << std::hex << gpr_[reg] << std::endl;
    if (current_isa == ISA::RV32)
        return gpr_[reg] & 0xFFFFFFFF;
    else
        return gpr_[reg];
}

// void RegisterFile::WriteGpr(size_t reg, uint64_t value) {
//     if (reg >= NUM_GPR) throw std::out_of_range("Invalid GPR index");
//     if (reg == 0) return;
//     gpr_[reg] = value;
// }
uint64_t RegisterFile::ReadFpr(size_t reg) const {
    if (reg >= NUM_FPR) throw std::out_of_range("Invalid FPR index");
    return fpr_[reg];
}
void RegisterFile::WriteFpr(size_t reg, uint64_t value) {
    if (reg >= NUM_FPR) throw std::out_of_range("Invalid FPR index");
    fpr_[reg] = value;
}
uint64_t RegisterFile::ReadCsr(size_t reg) const {
    if (reg >= NUM_CSR) throw std::out_of_range("Invalid CSR index");
    return csr_[reg];
}
void RegisterFile::WriteCsr(size_t reg, uint64_t value) {
    if (reg >= NUM_CSR) throw std::out_of_range("Invalid CSR index");
    csr_[reg] = value;
}
std::vector<uint64_t> RegisterFile::GetGprValues() const {
    return std::vector<uint64_t>(gpr_.begin(), gpr_.end());
}
std::vector<uint64_t> RegisterFile::GetFprValues() const {
    return std::vector<uint64_t>(fpr_.begin(), fpr_.end());
}
void RegisterFile::ModifyRegister(const std::string& reg_name, uint64_t value) {
    std::string canonical = reg_alias_to_name.at(reg_name);
    if (IsValidGeneralPurposeRegister(canonical))
        WriteGpr(std::stoi(canonical.substr(1)), value);
    else if (IsValidFloatingPointRegister(canonical))
        WriteFpr(std::stoi(canonical.substr(1)), value);
    else if (IsValidCsr(canonical))
        WriteCsr(csr_to_address.at(canonical), value);
    else
        throw std::invalid_argument("Unknown register: " + reg_name);
}
std::vector<std::string> RegisterFile::GprAbiNames() {
    static std::vector<std::string> names {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2","s0", "s1","a0","a1","a2","a3","a4","a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    return names;
}

// ------ VALIDATION FUNCTIONS ------
bool IsValidGeneralPurposeRegister(const std::string& reg) {
    return valid_general_purpose_registers.count(reg) > 0;
}
bool IsValidFloatingPointRegister(const std::string& reg) {
    return valid_floating_point_registers.count(reg) > 0;
}
bool IsValidCsr(const std::string& reg) {
    return valid_csr_registers.count(reg) > 0;
}
