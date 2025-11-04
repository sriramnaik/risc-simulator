#ifndef FORWARDING_UNIT_H
#define FORWARDING_UNIT_H

#include <cstdint>

// Forwarding unit determines whether an operand should be taken from the
// register file or forwarded from a later pipeline stage (EX/MEM or MEM/WB).
class ForwardingUnit
{
public:
    enum class ForwardingSource
    {
        FROM_REG_FILE, // default: read from register file
        FROM_EX_MEM,   // forward from EX/MEM pipeline register
        FROM_MEM_WB    // forward from MEM/WB pipeline register
    };

    ForwardingSource GetRs1Source(bool ex_reg_write, uint8_t ex_rd,
                                  bool mem_reg_write, uint8_t mem_rd,
                                  uint8_t rs1) const;

    ForwardingSource GetRs2Source(bool ex_reg_write, uint8_t ex_rd,
                                  bool mem_reg_write, uint8_t mem_rd,
                                  uint8_t rs2) const;
};

#endif // FORWARDING_UNIT_H
