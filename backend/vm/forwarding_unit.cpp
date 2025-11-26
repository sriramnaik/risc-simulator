// #include "forwarding_unit.h"

// ForwardingUnit::ForwardingSource ForwardingUnit::GetRs1Source(
//     bool ex_reg_write, uint8_t ex_rd,
//     bool mem_reg_write, uint8_t mem_rd,
//     uint8_t rs1,
//     bool is_float_rs1,
//     bool ex_is_float,
//     bool mem_is_float) const
// {
//     // EX stage forwarding
//     if (ex_reg_write &&
//         is_float_rs1 == ex_is_float &&   // same register file
//         ex_rd == rs1)
//     {
//         if (!is_float_rs1 && ex_rd == 0)
//             return ForwardingSource::FROM_REG_FILE;

//         return ForwardingSource::FROM_EX_MEM;
//     }

//     // MEM stage forwarding
//     if (mem_reg_write &&
//         is_float_rs1 == mem_is_float &&  // same register file
//         mem_rd == rs1)
//     {
//         if (!is_float_rs1 && mem_rd == 0)
//             return ForwardingSource::FROM_REG_FILE;

//         return ForwardingSource::FROM_MEM_WB;
//     }

//     // ❌ You forgot this
//     return ForwardingSource::FROM_REG_FILE;
// }

// ForwardingUnit::ForwardingSource ForwardingUnit::GetRs2Source(
//     bool ex_reg_write, uint8_t ex_rd,
//     bool mem_reg_write, uint8_t mem_rd,
//     uint8_t rs2,
//     bool is_float_rs2,
//     bool ex_is_float,
//     bool mem_is_float) const
// {
//     // EX stage forwarding
//     if (ex_reg_write &&
//         is_float_rs2 == ex_is_float &&   // same register file
//         ex_rd == rs2)
//     {
//         if (!is_float_rs2 && ex_rd == 0)
//             return ForwardingSource::FROM_REG_FILE;

//         return ForwardingSource::FROM_EX_MEM;
//     }

//     // MEM stage forwarding
//     if (mem_reg_write &&
//         is_float_rs2 == mem_is_float &&  // same register file
//         mem_rd == rs2)
//     {
//         if (!is_float_rs2 && mem_rd == 0)
//             return ForwardingSource::FROM_REG_FILE;

//         return ForwardingSource::FROM_MEM_WB;
//     }

//     // ❌ You forgot this too
//     return ForwardingSource::FROM_REG_FILE;
// }


// forwarding_unit.cpp -- corrected GetRs1Source / GetRs2Source
#include "forwarding_unit.h"

// Helper: true when register is the architectural zero for GPRs only
static inline bool IsGprZero(uint8_t rd, bool is_float)
{
    // x0 is architectural zero; f0 is NOT.
    return (!is_float && rd == 0);
}

ForwardingUnit::ForwardingSource ForwardingUnit::GetRs1Source(
    bool ex_reg_write, uint8_t ex_rd,
    bool mem_reg_write, uint8_t mem_rd,
    uint8_t rs1,
    bool is_float_rs1,
    bool ex_is_float,
    bool mem_is_float) const
{
    // Priority: EX stage forwarding first, then MEM stage, else register file.
    // Match must require same register-file (GPR vs FPR).
    if (ex_reg_write && (ex_is_float == is_float_rs1) && (ex_rd == rs1))
    {
        // If this is a GPR read and destination is x0 -> still read register file (x0)
        if (IsGprZero(ex_rd, ex_is_float))
            return ForwardingSource::FROM_REG_FILE;

        // Otherwise forward from EX/MEM result
        return ForwardingSource::FROM_EX_MEM;
    }

    if (mem_reg_write && (mem_is_float == is_float_rs1) && (mem_rd == rs1))
    {
        if (IsGprZero(mem_rd, mem_is_float))
            return ForwardingSource::FROM_REG_FILE;

        return ForwardingSource::FROM_MEM_WB;
    }

    // Default: read from register file
    return ForwardingSource::FROM_REG_FILE;
}

ForwardingUnit::ForwardingSource ForwardingUnit::GetRs2Source(
    bool ex_reg_write, uint8_t ex_rd,
    bool mem_reg_write, uint8_t mem_rd,
    uint8_t rs2,
    bool is_float_rs2,
    bool ex_is_float,
    bool mem_is_float) const
{
    if (ex_reg_write && (ex_is_float == is_float_rs2) && (ex_rd == rs2))
    {
        if (IsGprZero(ex_rd, ex_is_float))
            return ForwardingSource::FROM_REG_FILE;

        return ForwardingSource::FROM_EX_MEM;
    }

    if (mem_reg_write && (mem_is_float == is_float_rs2) && (mem_rd == rs2))
    {
        if (IsGprZero(mem_rd, mem_is_float))
            return ForwardingSource::FROM_REG_FILE;

        return ForwardingSource::FROM_MEM_WB;
    }

    return ForwardingSource::FROM_REG_FILE;
}

