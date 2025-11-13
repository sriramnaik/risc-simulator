#ifndef VM_ASM_MW_H
#define VM_ASM_MW_H

#include <map>
// #include <unordered_map>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include <ostream>

#include "assembler/parser.h"

struct AssembledProgram {
  std::map<unsigned int, unsigned int> line_number_instruction_number_mapping;
  std::map<unsigned int, unsigned int> instruction_number_line_number_mapping;
  std::map<unsigned int, unsigned int> instruction_number_disassembly_mapping;

  std::vector<std::pair<ICUnit, bool>> intermediate_code;

  // std::vector<std::pair<std::string, SymbolData>> symbol_table;
  

  std::map<std::string, SymbolData> symbol_table;
  int errorCount ;

  std::string filename;
  std::vector<std::variant<uint8_t, uint16_t, uint32_t, uint64_t, std::string, float, double>> data_buffer;
  std::vector<uint32_t> text_buffer;
};



inline std::ostream &operator<<(std::ostream &os, const AssembledProgram &program)
{
    os << "AssembledProgram for file: " << program.filename << "\n";

    os << "line_number_instruction_number_mapping (size=" << program.line_number_instruction_number_mapping.size() << "):\n";
    for (const auto &pair : program.line_number_instruction_number_mapping)
        os << "  Line " << pair.first << " -> Inst#" << pair.second << "\n";

    os << "instruction_number_line_number_mapping (size=" << program.instruction_number_line_number_mapping.size() << "):\n";
    for (const auto &pair : program.instruction_number_line_number_mapping)
        os << "  Inst#" << pair.first << " -> Line " << pair.second << "\n";

    os << "instruction_number_disassembly_mapping (size=" << program.instruction_number_disassembly_mapping.size() << "):\n";
    for (const auto &pair : program.instruction_number_disassembly_mapping)
        os << "  Inst#" << pair.first << " -> DisasmLine " << pair.second << "\n";

    os << "text_buffer (size=" << program.text_buffer.size() << "):\n";
    for (size_t i = 0; i < program.text_buffer.size(); ++i)
        os << "  [0x" << std::hex << program.text_buffer[i] << std::dec << "]\n";

    os << "data_buffer (size=" << program.data_buffer.size() << "):\n";
    for (size_t i = 0; i < program.data_buffer.size(); ++i) {
        os << "  [" << i << "] ";
        std::visit([&os](auto &&val) { os << val; }, program.data_buffer[i]);
        os << "\n";
    }

    os << "intermediate_code (size=" << program.intermediate_code.size() << ")\n";
    for (size_t i = 0; i < program.intermediate_code.size(); ++i)
        os << "  [" << i << "] ICUnit (not shown), used=" << program.intermediate_code[i].second << "\n";

    os << "symbol_table (size=" << program.symbol_table.size() << "):\n";
    for (const auto &entry : program.symbol_table)
        os << "  " << entry.first << " : SymbolData (not shown)\n";

    return os;
}


#endif // VM_ASM_MW_H
