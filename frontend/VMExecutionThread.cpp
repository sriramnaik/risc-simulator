#include "VMExecutionThread.h"
#include "../backend/vm/rvss_vm.h"
#include "../backend/vm/rvss_vm_pipelined.h"
#include <QDebug>

VMExecutionThread::VMExecutionThread(RVSSVM* vm, QObject* parent)
    : QThread(parent)
    , vm_(vm)
    , running_(false)
    , stop_requested_(false)
    , max_instructions_(1000000) // Default limit: 1 million instructions
{
}

VMExecutionThread::~VMExecutionThread()
{
    requestStop();
    if (isRunning()) {
        wait(5000); // Wait max 5 seconds
        if (isRunning()) {
            terminate(); // Force terminate if still running
            wait();
        }
    }
}

void VMExecutionThread::setVM(RVSSVM* vm)
{
    QMutexLocker locker(&mutex_);
    vm_ = vm;
}

void VMExecutionThread::setMaxInstructions(uint64_t max)
{
    QMutexLocker locker(&mutex_);
    max_instructions_ = max;
}

void VMExecutionThread::requestStop()
{
    stop_requested_ = true;
    if (vm_) {
        vm_->RequestStop();
    }
}

void VMExecutionThread::run()
{
    if (!vm_) {
        emit executionError("VM is not initialized");
        return;
    }

    running_ = true;
    stop_requested_ = false;

    try {
        uint64_t instruction_count = 0;
        uint64_t cycles_without_progress = 0;
        uint64_t last_instruction_count = 0;

        // Check if we're using pipelined VM
        RVSSVMPipelined* pipelinedVm = dynamic_cast<RVSSVMPipelined*>(vm_);
        bool isPipelined = (pipelinedVm != nullptr);

        while (!stop_requested_ && instruction_count < max_instructions_) {

            // For pipelined: check if both PC is at end AND pipeline is empty
            // For single-cycle: just check if PC is at end
            bool atEnd = vm_->GetProgramCounter() >= vm_->GetProgramSize();
            bool pipelineEmpty = vm_->IsPipelineEmpty();

            if (atEnd && pipelineEmpty) {
                qDebug() << "[VMExecutionThread] Execution complete - PC at end and pipeline empty";
                break;
            }

            // Execute one step
            vm_->Step();
            instruction_count++;

            // Detect infinite loops by checking if instructions are retiring
            if (instruction_count % 1000 == 0) {
                if (vm_->instructions_retired_ == last_instruction_count) {
                    cycles_without_progress++;

                    // If no progress for 10,000 cycles, likely infinite loop
                    if (cycles_without_progress > 10) {
                        emit executionError(
                            QString("Execution stopped: No progress detected.\n"
                                    "Instructions retired: %1\n"
                                    "Cycles: %2\n"
                                    "Possible infinite loop or pipeline stall!")
                                .arg(vm_->instructions_retired_)
                                .arg(vm_->cycle_s_)
                            );
                        break;
                    }
                } else {
                    cycles_without_progress = 0;
                    last_instruction_count = vm_->instructions_retired_;
                }
            }

            // Emit signal periodically for GUI updates (every 100 steps)
            if (instruction_count % 100 == 0) {
                emit stepCompleted();

                // Allow thread to process events and check for stop request
                QThread::msleep(1); // Small delay to keep GUI responsive
            }
        }

        if (instruction_count >= max_instructions_) {
            emit executionError(
                QString("Execution stopped: Maximum step limit (%1) reached.\n"
                        "Instructions retired: %2\n"
                        "Cycles: %3\n"
                        "Possible infinite loop detected!")
                    .arg(max_instructions_)
                    .arg(vm_->instructions_retired_)
                    .arg(vm_->cycle_s_)
                );
        } else {
            qDebug() << "[VMExecutionThread] Normal completion";
        }

        emit executionFinished(vm_->instructions_retired_, vm_->cycle_s_);

    } catch (const std::exception& ex) {
        emit executionError(QString("Execution error: %1").arg(ex.what()));
    }

    running_ = false;
}
