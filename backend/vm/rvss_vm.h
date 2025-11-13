#ifndef RVSSVM_H
#define RVSSVM_H

#include <QObject>
#include "../vm_base.h"
#include "rvss_control_unit.h"
#include "memory_controller.h"//;
#include "forwarding_unit.h"
#include "hazardUnit.h"
#include <stack>
#include <vector>
#include <atomic>
#include <cstdint>

struct RegisterChange {
    unsigned int reg_index;
    unsigned int reg_type; // 0 for GPR, 1 for CSR, 2 for FPR
    uint64_t old_value;
    uint64_t new_value;
};

struct MemoryChange {
    uint64_t address;
    std::vector<uint8_t> old_bytes_vec;
    std::vector<uint8_t> new_bytes_vec;
};

struct StepDelta {
    uint64_t old_pc;
    uint64_t new_pc;
    std::vector<RegisterChange> register_changes;
    std::vector<MemoryChange> memory_changes;
};


// --- Change 1: Inherit from QObject *first* and vm_base second via virtual, or only from QObject! ---
class RVSSVM : public QObject, public VmBase
{
    Q_OBJECT

public:
    // --- Change 2: Proper QObject constructor ---
    explicit RVSSVM(RegisterFile* sharedRegisters, QObject *parent = nullptr);

    ~RVSSVM();

    RVSSControlUnit control_unit_;
    std::atomic<bool> stop_requested_ = false;

    std::stack<StepDelta> undo_stack_;
    std::stack<StepDelta> redo_stack_;

    StepDelta current_delta_;
    int64_t execution_result_{};
    int64_t memory_result_{};
    uint64_t return_address_{};
    bool branch_flag_ = false;
    int64_t next_pc_{};
    uint16_t csr_target_address_{};
    uint64_t csr_old_value_{};
    uint64_t csr_write_val_{};
    uint8_t csr_uimm_{};

    void Fetch();
    void Decode();
    void Execute();
    void ExecuteFloat();
    void ExecuteDouble();
    void ExecuteCsr();
    void HandleSyscall();
    void WriteMemory();
    void WriteMemoryFloat();
    void WriteMemoryDouble();
    void WriteBack();
    void WriteBackFloat();
    void WriteBackDouble();
    void WriteBackCsr();

    void Run() override;
    void DebugRun() override;
    void Step() override;
    void Undo() override;
    // void Redo() override;
    void Reset() override;
    void RequestStop() { stop_requested_ = true; }
    bool IsStopRequested() const { return stop_requested_; }
    void ClearStop() { stop_requested_ = false; }
    void PrintType() { std::cout << "rvssvm" << std::endl; }

    std::vector<uint8_t> GetMemoryRange(uint64_t address, size_t size) {
        std::vector<uint8_t> data;
        for (size_t i = 0; i < size; i++) {
            data.push_back(memory_controller_.ReadByte(address + i));
        }
        return data;
    }

    virtual bool IsPipelineEmpty() const { return true; }
    virtual void SetPipelineConfig(bool hazardEnabled,
                                   bool forwardingEnabled,
                                   bool branchPredictionEnabled,
                                   bool dynamicPredictionEnabled) {return;}

    void DumpPipelineState() {return ;}

signals:
    void gprUpdated(int index, quint64 value);
    void csrUpdated(int index, quint64 value);
    void fprUpdated(int index, quint64 value);
    void memoryUpdated(quint64 address, QVector<quint8> data);
    void vmError(const QString &msg);
    void syscallOutput(const QString &msg);
    void statusChanged(const QString &msg);
};

#endif // RVSSVM_H
