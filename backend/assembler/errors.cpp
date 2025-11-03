/** @cond DOXYGEN_IGNORE */
/**
 * File Name: errors.cpp
 * Author: Vishank Singh
 * Github: https://github.com/VishankSingh
 */
/** @endcond */

#include "errors.h"

#include <ostream>
#include <iomanip>

namespace errors {

std::ostream &operator<<(std::ostream &os, const SyntaxError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const UnexpectedTokenError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const ImmediateOutOfRangeError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const MisalignedImmediateError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const UnexpectedOperandError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const InvalidLabelRefError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const LabelRedefinitionError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::ostream &operator<<(std::ostream &os, const InvalidRegisterError &error) {
  os << error.filename << ":" << error.line_number << ":" << error.column_number << ": "
     << ANSI_code_red << "[ERROR]" << ANSI_code_reset << " "
     << error.main_message << "\n"
     << std::setw(6) << std::right << error.line_number << " | " << error.line_text << "\n"
     << std::setw(6) << " " << " | " << ANSI_code_red << std::setw(error.column_number) << std::right << "^"
     << ANSI_code_reset << "\n"
     << std::setw(6) << " " << " | " << error.sub_message << "\n"
     << std::setw(6) << " " << " | " << "\n"
     << std::setw(0);
  return os;
}

std::vector<std::string> extractAllErrorMessages(
    const std::vector<std::variant<
        SyntaxError,
        UnexpectedTokenError,
        ImmediateOutOfRangeError,
        MisalignedImmediateError,
        UnexpectedOperandError,
        InvalidLabelRefError,
        LabelRedefinitionError,
        InvalidRegisterError
        >> &all_errors)
{
    std::vector<std::string> messages;
    for (const auto &error : all_errors) {
        std::ostringstream oss;
        std::visit([&oss](auto &&err) {
            oss << err;  // uses its respective overload
        }, error);
        messages.push_back(oss.str());
    }
    return messages;
}

} // namespace errors
