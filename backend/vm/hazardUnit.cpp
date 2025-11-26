

// #include "hazardUnit.h"

// bool HazardDetectionUnit::DetectLoadUseHazard(uint8_t ex_rd, bool ex_mem_read,
//                                      uint8_t rs1, uint8_t rs2,
//                                      bool rs1_is_float, bool rs2_is_float,
//                                      bool ex_is_float) const
// {
//     if (!ex_mem_read) return false;

//     // Check rs1 hazard (with register file matching)
//     if (rs1 != 0 || rs1_is_float)  // Allow f0 for FPR
//     {
//         if (ex_rd == rs1 && ex_is_float == rs1_is_float)
//             return true;
//     }

//     // Check rs2 hazard
//     if (rs2 != 0 || rs2_is_float)
//     {
//         if (ex_rd == rs2 && ex_is_float == rs2_is_float)
//             return true;
//     }

//     return false;
// }


// hazardUnit.cpp -- corrected hazard detection

#include "hazardUnit.h"

// ex_rd: destination register in EX stage
// ex_mem_read: true if EX stage is performing a load (memory read)
// rs1/rs2: register indices of the instruction in ID stage
// rs1_is_float/rs2_is_float: register-file type for the ID-stage operands
// ex_is_float: register-file type for the EX-stage destination
bool HazardDetectionUnit::DetectLoadUseHazard(uint8_t ex_rd, bool ex_mem_read,
                                              uint8_t rs1, uint8_t rs2,
                                              bool rs1_is_float, bool rs2_is_float,
                                              bool ex_is_float) const
{
    // Only a load in EX stage can create a classic load-use hazard
    if (!ex_mem_read)
        return false;

    // If EX-stage load writes to different register file than the ID-stage operand -> no hazard
    // Check rs1
    if (rs1_is_float == ex_is_float) // same register file
    {
        // GPR x0 is architectural zero; it does not cause a hazard if rs==0 and is GPR
        if (!(rs1 == 0 && !rs1_is_float))
        {
            if (ex_rd == rs1)
                return true;
        }
    }

    // Check rs2
    if (rs2_is_float == ex_is_float) // same register file
    {
        if (!(rs2 == 0 && !rs2_is_float))
        {
            if (ex_rd == rs2)
                return true;
        }
    }

    return false;
}

bool HazardDetectionUnit::DetectEXHazard(uint8_t ex_rd, bool ex_reg_write, uint8_t id_rs1, uint8_t id_rs2) const
{
    if (!ex_reg_write)
        return false;
    if (ex_rd == 0)
        return false; // x0 cannot cause a hazard
    return (ex_rd == id_rs1) || (ex_rd == id_rs2);
}

bool HazardDetectionUnit::DetectMEMHazard(uint8_t mem_rd, bool mem_reg_write, uint8_t id_rs1, uint8_t id_rs2) const
{
    if (!mem_reg_write)
        return false;
    if (mem_rd == 0)
        return false; // x0 cannot cause a hazard
    return (mem_rd == id_rs1) || (mem_rd == id_rs2);
}


// bool HazardDetectionUnit::DetectEXHazard(uint8_t ex_rd, bool ex_reg_write, uint8_t id_rs1, uint8_t id_rs2) const
// {
//     if (!ex_reg_write)
//         return false; // EX stage doesn't write to register
//     if (ex_rd == 0)
//         return false; // x0 is not a hazard
//     return (ex_rd == id_rs1) || (ex_rd == id_rs2);
// }

// bool HazardDetectionUnit::DetectMEMHazard(uint8_t mem_rd, bool mem_reg_write, uint8_t id_rs1, uint8_t id_rs2) const
// {
//     if (!mem_reg_write)
//         return false; // MEM stage doesn't write to register
//     if (mem_rd == 0)
//         return false; // x0 is not a hazard
//     return (mem_rd == id_rs1) || (mem_rd == id_rs2);
// }
