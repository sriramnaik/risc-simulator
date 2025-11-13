#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include "../../backend/vm_asm_mw.h"
#include <QSlider>
#include <QLabel>

class QTabWidget;
class QStackedWidget;
class CodeEditor;
class BottomPanel;
class RegisterPanel;
class ErrorConsole;
class Assembler;
class RVSSVM;
class RVSSVMPipelined;
struct ErrorMessage;

struct FileTab
{
    CodeEditor *editor;
    QString filePath;
    bool isModified;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showProcessorSelection();

private:
    QTabWidget *tabWidget;
    BottomPanel *bottomPanel;
    RegisterPanel *registerPanel;
    QStackedWidget *emptyStateStack;

    Assembler *assembler;
    ErrorConsole *errorconsole;
    RVSSVM *vm;
    RVSSVM* singleCycleVm = nullptr;
    RVSSVMPipelined* pipelinedVm = nullptr;

    QVector<FileTab> fileTabs;

    CodeEditor *getCurrentEditor();
    int getCurrentTabIndex();
    QString getCurrentFilePath();
    AssembledProgram program;

    QSlider *executionSpeedSlider;
    bool hasShownFprUpdate;
    bool hasShownCsrUpdate;
    int currentTabIndex;

    QLabel *instructionCountLabel;
    QLabel *cycleCountLabel;
    QLabel *executionTimeLabel;

    void updateRegisterTable();
    void updateExecutionInfo();
    void updateTabTitle(int tabIndex);
    void setTabModified(int tabIndex, bool modified);
    bool promptSaveChanges(int tabIndex);
    bool saveToFile(int tabIndex, const QString &filePath);
    void highlightCurrentLine();
    void clearLineHighlight();
    void refreshMemoryDisplay();

    QString lastName = "Single-cycle processor";
    QString lastISA = "RV64";
    QLabel *processorInfoLabel;

private slots:
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onAssemble();
    void onRun();
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onStep();
    void onPause();
    void onStop();
    void onUndo();
    void onReset();
    void onPipelineStageChanged(uint64_t pc, QString stage);

    // void onRunSlow();
};

#endif // MAINWINDOW_H
