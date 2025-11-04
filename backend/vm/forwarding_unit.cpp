// Implementation of forwarding unit
#include "forwarding_unit.h"

ForwardingUnit::ForwardingSource ForwardingUnit::GetRs1Source(bool ex_reg_write, uint8_t ex_rd,
                                                              bool mem_reg_write, uint8_t mem_rd,
                                                              uint8_t rs1) const
{
    // Prioritize the most recent stage (EX/MEM) over MEM/WB
    if (ex_reg_write && ex_rd != 0 && ex_rd == rs1)
        return ForwardingSource::FROM_EX_MEM;
    if (mem_reg_write && mem_rd != 0 && mem_rd == rs1)
        return ForwardingSource::FROM_MEM_WB;
    return ForwardingSource::FROM_REG_FILE;
}

ForwardingUnit::ForwardingSource ForwardingUnit::GetRs2Source(bool ex_reg_write, uint8_t ex_rd,
                                                              bool mem_reg_write, uint8_t mem_rd,
                                                              uint8_t rs2) const
{
    if (ex_reg_write && ex_rd != 0 && ex_rd == rs2)
        return ForwardingSource::FROM_EX_MEM;
    if (mem_reg_write && mem_rd != 0 && mem_rd == rs2)
        return ForwardingSource::FROM_MEM_WB;
    return ForwardingSource::FROM_REG_FILE;
}
