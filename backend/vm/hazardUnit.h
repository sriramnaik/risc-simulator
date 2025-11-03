// #ifndef HAZARDUNIT_H
// #define HAZARDUNIT_H

// #endif // HAZARDUNIT_H
/**
 * @file hazard_unit.h
 * @brief Hazard detection and forwarding unit
 */
#ifndef HAZARD_UNIT_H
#define HAZARD_UNIT_H

#include <cstdint>
#include "pipelineRegisters.h"

enum class ForwardSource {
    NONE,
    EX_MEM,
    MEM_WB
};

class HazardUnit {
public:
    HazardUnit() = default;
    ~HazardUnit() = default;

    bool DetectLoadUseHazard(const IDEXRegister& id_ex,
                             uint8_t rs1, uint8_t rs2) const {
        if (id_ex.mem_read && id_ex.valid) {
            if ((id_ex.rd == rs1 && rs1 != 0) ||
                (id_ex.rd == rs2 && rs2 != 0)) {
                return true;
            }
        }
        return false;
    }

    ForwardSource GetForwardA(const EXMEMRegister& ex_mem,
                              const MEMWBRegister& mem_wb,
                              uint8_t rs1) const {
        if (ex_mem.reg_write && ex_mem.rd != 0 &&
            ex_mem.rd == rs1 && ex_mem.valid) {
            return ForwardSource::EX_MEM;
        }

        if (mem_wb.reg_write && mem_wb.rd != 0 &&
            mem_wb.rd == rs1 && mem_wb.valid) {
            return ForwardSource::MEM_WB;
        }

        return ForwardSource::NONE;
    }

    ForwardSource GetForwardB(const EXMEMRegister& ex_mem,
                              const MEMWBRegister& mem_wb,
                              uint8_t rs2) const {
        if (ex_mem.reg_write && ex_mem.rd != 0 &&
            ex_mem.rd == rs2 && ex_mem.valid) {
            return ForwardSource::EX_MEM;
        }

        if (mem_wb.reg_write && mem_wb.rd != 0 &&
            mem_wb.rd == rs2 && mem_wb.valid) {
            return ForwardSource::MEM_WB;
        }

        return ForwardSource::NONE;
    }

    bool DetectBranchHazard(const EXMEMRegister& ex_mem) const {
        return ex_mem.branch_taken && ex_mem.valid;
    }
};

#endif // HAZARD_UNIT_H
