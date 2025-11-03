#ifndef REGISTERS_H
#define REGISTERS_H

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

enum class ISA { RV32, RV64 };

class RegisterFile {
public:
    static constexpr size_t NUM_GPR = 32;
    static constexpr size_t NUM_FPR = 32;
    static constexpr size_t NUM_CSR = 4096;

    RegisterFile();
    void Reset();
    uint64_t pc;

    uint64_t ReadGpr(size_t reg) const;
    void WriteGpr(size_t reg, uint64_t value);

    uint64_t ReadFpr(size_t reg) const;
    void WriteFpr(size_t reg, uint64_t value);

    uint64_t ReadCsr(size_t reg) const;
    void WriteCsr(size_t reg, uint64_t value);

    std::vector<uint64_t> GetGprValues() const;
    std::vector<uint64_t> GetFprValues() const;

    void ModifyRegister(const std::string& reg_name, uint64_t value);

    static std::vector<std::string> GprAbiNames();

    void SetIsa(ISA isa) { current_isa = isa; }
    ISA GetIsa() const { return current_isa; }
private:
    std::array<uint64_t, NUM_GPR> gpr_;
    std::array<uint64_t, NUM_FPR> fpr_;
    std::array<uint64_t, NUM_CSR> csr_;
    ISA current_isa = ISA::RV64; // default
};

// These are available for all assembler, parser, and UI components
extern const std::unordered_map<std::string, int> abi_name_to_gpr_index;
extern const std::unordered_set<std::string> valid_general_purpose_registers;
extern const std::unordered_set<std::string> valid_floating_point_registers;
extern const std::unordered_set<std::string> valid_csr_registers;
extern const std::unordered_map<std::string, int> csr_to_address;
extern const std::unordered_map<std::string, std::string> reg_alias_to_name;

bool IsValidGeneralPurposeRegister(const std::string& reg);
bool IsValidFloatingPointRegister(const std::string& reg);
bool IsValidCsr(const std::string& reg);

#endif // REGISTERS_H
