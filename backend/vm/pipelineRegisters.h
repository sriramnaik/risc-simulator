
#ifndef PIPELINE_REGISTERS_H
#define PIPELINE_REGISTERS_H

#include <cstdint>
#include "alu.h"

    struct IFIDRegister {
    uint32_t instruction = 0;
    uint64_t pc = 0;
    bool valid = false;

    void Reset() {
        instruction = 0;
        pc = 0;
        valid = false;
    }
};

struct IDEXRegister {
    // Control signals
    bool reg_write = false;
    bool mem_to_reg = false;
    bool mem_read = false;
    bool mem_write = false;
    bool branch = false;
    bool alu_src = false;
    alu::AluOp alu_op = alu::AluOp::kNone;

    // Data
    uint64_t pc = 0;
    uint64_t read_data1 = 0;
    uint64_t read_data2 = 0;
    int32_t immediate = 0;

    // Register addresses
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t rd = 0;

    uint8_t funct3 = 0;
    uint8_t funct7 = 0;
    bool is_float = false;
    bool is_double = false;

    bool valid = false;

    void Reset() {
        reg_write = mem_to_reg = mem_read = mem_write = branch = alu_src = false;
        alu_op = alu::AluOp::kNone;
        pc = read_data1 = read_data2 = 0;
        immediate = 0;
        rs1 = rs2 = rd = 0;
        funct3 = funct7 = 0;
        is_float = is_double = false;
        valid = false;
    }
};

struct EXMEMRegister {
    bool reg_write = false;
    bool mem_to_reg = false;
    bool mem_read = false;
    bool mem_write = false;

    uint64_t alu_result = 0;
    uint64_t write_data = 0;
    uint64_t branch_target = 0;
    bool branch_taken = false;

    uint8_t rd = 0;
    uint8_t mem_size = 0;
    bool is_float = false;
    bool is_double = false;

    bool valid = false;

    void Reset() {
        reg_write = mem_to_reg = mem_read = mem_write = false;
        alu_result = write_data = branch_target = 0;
        branch_taken = false;
        rd = 0;
        mem_size = 0;
        is_float = is_double = false;
        valid = false;
    }
};

struct MEMWBRegister {
    bool reg_write = false;
    bool mem_to_reg = false;

    uint64_t alu_result = 0;
    uint64_t mem_data = 0;

    uint8_t rd = 0;
    bool is_float = false;
    bool is_double = false;

    bool valid = false;

    void Reset() {
        reg_write = mem_to_reg = false;
        alu_result = mem_data = 0;
        rd = 0;
        is_float = is_double = false;
        valid = false;
    }
};

#endif // PIPELINE_REGISTERS_H
