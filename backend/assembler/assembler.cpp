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
// #include <iostream>
// #include <algorithm>

// Assembler::Assembler(QObject* parent)
//     : QObject(parent)
// {}
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






