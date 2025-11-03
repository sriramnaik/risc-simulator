
#ifndef RVSS_VM_PIPELINED_H
#define RVSS_VM_PIPELINED_H

#include <QObject>
#include <QMap>
#include "rvss_control_unit.h"
#include "rvss_vm.h"

#include <cstdint>

class RVSSVMPipelined : public RVSSVM
{
    Q_OBJECT

private:
    RVSSControlUnit control_unit_;

    struct IF_ID {
        bool valid = false;
        uint64_t pc = 0;
        uint32_t instruction = 0;
    } if_id_, if_id_next_;

    struct ID_EX {
        bool valid = false;
        uint64_t pc = 0;
        uint32_t instruction = 0;

        uint8_t rs1 = 0, rs2 = 0, rd = 0;
        uint8_t funct3 = 0, funct7 = 0;
        int32_t imm = 0;

        uint64_t reg1_value = 0, reg2_value = 0;

        bool reg_write = false, mem_read = false, mem_write = false;
        bool mem_to_reg = false, alu_src = false, branch = false, is_float = false;
    } id_ex_, id_ex_next_;

    struct EX_MEM {
        bool valid = false;
        uint64_t pc = 0;
        uint32_t instruction = 0;

        uint8_t rd = 0;
        bool reg_write = false, mem_read = false, mem_write = false, mem_to_reg = false;

        uint64_t alu_result = 0, reg2_value = 0;
        bool branch_taken = false;
        uint64_t branch_target = 0;
    } ex_mem_, ex_mem_next_;

    struct MEM_WB {
        bool valid = false;
        uint8_t rd = 0;
        bool reg_write = false, mem_to_reg = false;
        uint64_t alu_result = 0, mem_data = 0;
        uint64_t pc = 0;
    } mem_wb_, mem_wb_next_;

    bool pc_update_pending_ = false;
    uint64_t pc_update_value_ = 0;

    void IF_stage();
    void ID_stage();
    void EX_stage();
    void MEM_stage();
    void WB_stage();

    void advance_pipeline_registers();

public:
    explicit RVSSVMPipelined(RegisterFile *sharedRegisters, QObject *parent = nullptr);
    ~RVSSVMPipelined() override;

    QMap<uint64_t, int> pcToLineMap;
    void setPcToLineMap(const QMap<uint64_t, int>& map) { pcToLineMap = map; }

    void Run() override;
    void DebugRun() override;
    void Step() override;
    void Undo() override;
    void Redo() override;
    void Reset() override;

    const IF_ID& getIfId() const { return if_id_; }
    const ID_EX& getIdEx() const { return id_ex_; }
    const EX_MEM& getExMem() const { return ex_mem_; }
    const MEM_WB& getMemWb() const { return mem_wb_; }
};

#endif // RVSS_VM_PIPELINED_H

