/**
 * @file hazard_unit.cpp
 * @brief Hazard detection unit implementation
 */

#include "hazardUnit.h"

bool HazardDetectionUnit::DetectLoadUseHazard(uint8_t ex_rd, bool ex_mem_read, uint8_t id_rs1, uint8_t id_rs2) const
{
    if (!ex_mem_read)
        return false; // EX stage is not a load
    if (ex_rd == 0)
        return false; // x0 is not a hazard
    return (ex_rd == id_rs1) || (ex_rd == id_rs2);
}

bool HazardDetectionUnit::DetectEXHazard(uint8_t ex_rd, bool ex_reg_write, uint8_t id_rs1, uint8_t id_rs2) const
{
    if (!ex_reg_write)
        return false; // EX stage doesn't write to register
    if (ex_rd == 0)
        return false; // x0 is not a hazard
    return (ex_rd == id_rs1) || (ex_rd == id_rs2);
}

bool HazardDetectionUnit::DetectMEMHazard(uint8_t mem_rd, bool mem_reg_write, uint8_t id_rs1, uint8_t id_rs2) const
{
    if (!mem_reg_write)
        return false; // MEM stage doesn't write to register
    if (mem_rd == 0)
        return false; // x0 is not a hazard
    return (mem_rd == id_rs1) || (mem_rd == id_rs2);
}
