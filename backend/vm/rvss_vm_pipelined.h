
#ifndef RVSS_VM_PIPELINED_H
#define RVSS_VM_PIPELINED_H

#include <QObject>
#include <QMap>
#include "rvss_control_unit.h"
#include "rvss_vm.h"
#include "hazardUnit.h"
#include "forwarding_unit.h"

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
        bool predicted_taken = false;
    } if_id_, if_id_next_;

    struct ID_EX {
        bool valid = false;
        uint64_t pc = 0;
        uint32_t instruction = 0;
        bool is_syscall;

        uint8_t rs1 = 0, rs2 = 0, rd = 0;
        uint8_t funct3 = 0, funct7 = 0;
        int32_t imm = 0;

        bool rs1_is_float = false;  // NEW
        bool rs2_is_float = false;  // NEW

        uint64_t reg1_value = 0, reg2_value = 0,reg3_value;

        bool reg_write = false, mem_read = false, mem_write = false;
        bool mem_to_reg = false, alu_src = false, branch = false, is_float = false;

        // bool is_float = false;  // Add this
        bool predicted_taken = false;

    } id_ex_, id_ex_next_;

    struct EX_MEM {
        bool valid = false;
        uint64_t pc = 0;
        uint32_t instruction = 0;

        uint8_t rd = 0;
        bool reg_write = false, mem_read = false, mem_write = false, mem_to_reg = false;

        uint64_t alu_result = 0, reg2_value = 0;
        bool branch_taken = false;
         bool is_float = false;
        uint64_t branch_target = 0;
         bool is_syscall = false;
    } ex_mem_, ex_mem_next_;

    struct MEM_WB {
        bool valid = false;
        uint8_t rd = 0;
        bool reg_write = false, mem_to_reg = false;
        uint64_t alu_result = 0, mem_data = 0;
        uint64_t pc = 0;
        bool is_float = false;
        bool is_syscall = false;
        uint32_t instruction = 0;
    } mem_wb_, mem_wb_next_;

    struct PipelineStepDelta {
        // PC state
        uint64_t old_pc;
        uint64_t new_pc;

        // Pipeline register snapshots
        IF_ID old_if_id;
        ID_EX old_id_ex;
        EX_MEM old_ex_mem;
        MEM_WB old_mem_wb;

        IF_ID new_if_id;
        ID_EX new_id_ex;
        EX_MEM new_ex_mem;
        MEM_WB new_mem_wb;

        // Register/memory changes (same as before)
        std::vector<RegisterChange> register_changes;
        std::vector<MemoryChange> memory_changes;

        // Pipeline control state
        bool old_stall;
        bool new_stall;
        bool old_pc_update_pending;
        uint64_t old_pc_update_value;

        // Statistics
        uint64_t old_cycle;
        uint64_t old_instructions_retired;
        uint64_t old_stall_cycles;
    };

    bool pc_update_pending_ = false;
    uint64_t pc_update_value_ = 0;
    // bool branch_taken_this_cycle_ = false;

    void IF_stage();
    void ID_stage();
    void EX_stage();
    void MEM_stage();
    void WB_stage();

    // void advance_pipeline_registers();

public:
    explicit RVSSVMPipelined(RegisterFile *sharedRegisters, QObject *parent = nullptr);
    ~RVSSVMPipelined() override;

    void LoadProgram(const AssembledProgram& program) override;
    QMap<uint64_t, int> pcToLineMap;
    void setPcToLineMap(const QMap<uint64_t, int>& map) { pcToLineMap = map; }
    bool IsPipelineEmpty() const override;

    void Run() override;
    void DebugRun() override;
    void Step() override;
    void Undo() override;
    // void Redo() override;
    void Reset() override;
    std::stack<PipelineStepDelta> pipeline_undo_stack_;

    const IF_ID& getIfId() const { return if_id_; }
    const ID_EX& getIdEx() const { return id_ex_; }
    const EX_MEM& getExMem() const { return ex_mem_; }
    const MEM_WB& getMemWb() const { return mem_wb_; }

    bool hazard_detection_enabled_;
    bool forwarding_enabled_;

    bool branch_prediction_enabled_;  // enables branch prediction (static or dynamic)
    bool dynamic_branch_prediction_enabled_; // enables dynamic mode when branch_prediction_enabled_ is true

    static constexpr int BHT_SIZE = 256;
    std::vector<bool> branch_history_table_;      // 1-bit dynamic predictor bits
    std::vector<uint64_t> branch_target_buffer_;  // branch target addresses


    void SetForwardingEnabled(bool enabled) { forwarding_enabled_ = enabled; }
    bool GetForwardingEnabled() const { return forwarding_enabled_; }

    ForwardingUnit forwarding_unit_;

    HazardDetectionUnit hazard_unit_;
    bool stall_ = false; // when true, IF/ID is frozen and ID/EX gets a bubble
    bool flush_pipeline_;

    void DumpPipelineState();
    void advance_pipeline_registers();
    void SetPipelineConfig(bool hazardEnabled,
                           bool forwardingEnabled,
                           bool branchPredictionEnabled,
                           bool dynamicPredictionEnabled) override;

    void DumpBranchPredictionTables(const std::filesystem::path &filepath);
    void PrintBranchPredictionTables();

private:
    std::map<std::string, uint64_t> stage_to_pc_;

signals:
    void pipelineStageChanged(uint64_t pc, QString stageName);
};

#endif // RVSS_VM_PIPELINED_H

