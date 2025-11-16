
#ifndef VMEXECUTIONTHREAD_H
#define VMEXECUTIONTHREAD_H

#include <QThread>
#include <QMutex>
#include <atomic>

class RVSSVM; // Forward declaration

class VMExecutionThread : public QThread
{
    Q_OBJECT

public:
    explicit VMExecutionThread(RVSSVM* vm, QObject* parent = nullptr);
    ~VMExecutionThread();

    void setVM(RVSSVM* vm);
    void setMaxInstructions(uint64_t max);
    void requestStop();
    bool isRunning() const { return running_; }

signals:
    void stepCompleted();
    void executionFinished(uint64_t instructions, uint64_t cycles);
    void executionError(QString message);

protected:
    void run() override;

private:
    RVSSVM* vm_;
    std::atomic<bool> running_;
    std::atomic<bool> stop_requested_;
    uint64_t max_instructions_;
    QMutex mutex_;
};

#endif // VMEXECUTIONTHREAD_H
