#include "assembler.h"
#include "../utils.h"
#include "../globals.h"
#include "lexer.h"
#include "../vm_asm_mw.h"

#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>
#include <QString>
#include <iomanip>
#include <sstream>

Assembler::Assembler(RegisterFile* regs,QObject* parent)
    : QObject(parent), registers_(regs) {}

AssembledProgram Assembler::assemble(const std::string &filename) {

    std::unique_ptr<Lexer> lexer;
  try {
    lexer = std::make_unique<Lexer>(filename);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  std::vector<Token> tokens = lexer->getTokenList();
  // int previous_line = -1;
  // for (const Token& token : tokens) {
  //     if (token.line_number != previous_line) {
  //         if (previous_line != -1) {
  //             std::cout << std::endl;
  //         }
  //         previous_line = token.line_number;
  //     }
  //     std::cout << token << std::endl;
  // }

  Parser parser(lexer->getFilename(), tokens);
  parser.parse();

  AssembledProgram program;
  program.filename = filename;

  // std::cout << "file is parsed " << std::endl;

  if (parser.getErrorCount()==0) {
      // std::cout << "file is parsed " << std::endl;

    std::vector<uint32_t> machine_code_bits = generateMachineCode(parser.getIntermediateCode());

    program.data_buffer = parser.getDataBuffer();
    program.intermediate_code = parser.getIntermediateCode();
    program.text_buffer = machine_code_bits;
    program.instruction_number_line_number_mapping = parser.getInstructionNumberLineNumberMapping();

    program.line_number_instruction_number_mapping = [&]() {
      std::map<unsigned int, unsigned int> line_number_instruction_number_mapping;
      if (program.instruction_number_line_number_mapping.empty()) {
        return line_number_instruction_number_mapping;
      }
      unsigned int prev_instruction = 0;
      unsigned int prev_line = 1;

      for (const auto &[instruction, line] : program.instruction_number_line_number_mapping) {
        for (unsigned int i = prev_line; i <= line; ++i) {
          line_number_instruction_number_mapping[i] = prev_instruction;
        }
        prev_instruction += 1;
        prev_line = line + 1;
      }
      return line_number_instruction_number_mapping;
    }();

    program.symbol_table = parser.getSymbolTable();
    program.errorCount = 0;

    // std::cout<<"Before the DumpDisassembly call "<< std::endl;
    if (registers_) {
        registers_->Reset();

        // Optionally, log/emit as needed
    }
    DumpDisasssembly(globals::disassembly_file_path, program);
    DumpErrors(globals::errors_dump_file_path, parser.getErrors());
    // std::cout << program << std::endl;

    // DumpNoErrors(globals::errors_dump_file_path);

  } else {

      // parser.printErrors();
      const std::vector<std::string> errors = errors::extractAllErrorMessages(parser.getAllErrors());
      program.errorCount = errors.size();

      QVector<std::string> qErrors;
      for (const auto &e : errors) {
          // QString qstr = QString::fromStdString(e);
          qErrors.append(e);
      }

      // for (const auto &msg : errors)
      //         std::cout << msg << std::endl;
      // for (const auto &msg : qErrors)
      //     std::cout << msg << std::endl;

      emit errorsAvailable(qErrors);

    DumpErrors(globals::errors_dump_file_path, parser.getErrors());
    // if (globals::verbose_errors_print) {
    //   parser.printErrors();
    // }
    // throw std::runtime_error("Failed to parse file: " + filename);

    // ErrorConsole::addMessages(Error);

  }

  // std::cout << program << std::endl;
  return program;
}


QString Assembler::GenerateDisassemblyString(const AssembledProgram &program) {
    const std::map<std::string, SymbolData>& symbol_table = program.symbol_table;
    const std::vector<std::pair<ICUnit, bool>>& intermediate_code = program.intermediate_code;
    const std::vector<uint32_t>& text_buffer = program.text_buffer;

    std::unordered_map<uint64_t, std::string> label_for_address;
    for (const auto& [name, data] : symbol_table) {
        if (!data.isData) {
            label_for_address[data.address] = name;
        }
    }
    if (label_for_address.find(0) == label_for_address.end()) {
        label_for_address[0] = "start";
    }

    unsigned int instruction_index = 0;
    unsigned int line_number = 1;
    size_t max_address = intermediate_code.size() * 4;
    int hex_digits = 1;
    size_t temp = max_address;
    while (temp >>= 4) ++hex_digits;

    std::stringstream out;

    while (instruction_index < intermediate_code.size()) {
        const auto& [ICBlock, isData] = intermediate_code[instruction_index];
        uint64_t current_address = instruction_index * 4;

        auto it = label_for_address.find(current_address);
        if (it != label_for_address.end()) {
            if (line_number > 1) {
                out << std::endl;
                ++line_number;
            }
            out << std::setw(16) << std::setfill('0') << std::hex
                << current_address
                << std::dec << std::setfill(' ')
                << " <" << it->second << ">:" << std::endl;
            ++line_number;
        }

        out << "  "
            << std::setw(hex_digits) << std::setfill(' ') << std::right << std::hex
            << current_address
            << std::dec << std::left << std::setw(0)
            << ": ";

        if (instruction_index < text_buffer.size()) {
            uint32_t raw = text_buffer[instruction_index];
            out << std::setfill('0') << std::setw(8) << std::right << std::hex
                << raw
                << std::dec << std::setfill(' ') << "             ";
        } else {
            out << " ????????             ";
        }

        out << ICBlock << std::endl;
        ++line_number;
        ++instruction_index;
    }

    return QString::fromStdString(out.str());
}


