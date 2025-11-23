#include "mainwindow.h"
#include "VMExecutionThread.h"
#include "codeeditor.h"
#include "registerpanel.h"
#include "bottompanel.h"
#include "../backend/assembler/assembler.h"
#include "../backend/vm/rvss_vm.h"
#include "../backend/vm/rvss_vm_pipelined.h"
#include "processorwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QToolBar>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include <QLabel>
#include <QStatusBar>
#include <QPainter>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QTextCursor>
#include <QTabWidget>
#include <QTabBar>
#include <QStackedWidget>
// #include <QDebug>
#include <QSlider>
#include <qapplication.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      tabWidget(nullptr),
      bottomPanel(nullptr),
      registerPanel(nullptr),
      emptyStateStack(nullptr),
      assembler(nullptr),
      errorconsole(nullptr),
      vm(nullptr),
      executionSpeedSlider(nullptr),
      currentTabIndex(-1),
      lastName("Single-cycle processor"),
      lastISA("RV64")

{
    // qDebug() << "[MainWindow] Constructor START";

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(8, 8, 8, 8);
    topLayout->setSpacing(4);

    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // --- Toolbar ---
    // qDebug() << "[MainWindow] Creating toolbar";
    QToolBar *toolBar = new QToolBar("Main Toolbar", this);
    toolBar->setIconSize(QSize(32, 32));
    toolBar->setMovable(false);
    toolBar->setStyleSheet(
        "QToolBar { background-color: #252526; border: none; padding: 1px; }"
        "QToolButton { color: white; }");

    QAction *processor = new QAction(QIcon(), "Processor", this);
    QAction *newFileAction = new QAction(QIcon("icons/newFile.png"), "New File", this);
    QAction *openFileAction = new QAction(QIcon("icons/openFile.png"), "Open File", this);
    QAction *saveFileAction = new QAction(QIcon("icons/SaveFile.png"), "Save File", this);
    QAction *saveAsFileAction = new QAction(QIcon("icons/SaveAs.png"), "Save As", this);
    QAction *assembleAction = new QAction(QIcon("icons/processo.png"), "Assemble", this);
    QAction *runAction = new QAction(QIcon("icons/run.png"), "Run", this);
    QAction *stepAction = new QAction(QIcon("icons/step.png"), "Step", this);
    QAction *undoAction = new QAction(QIcon("icons/undo.png"), "Undo", this);
    QAction *resetAction = new QAction(QIcon("icons/reset.png"), "reset", this);
    QAction *pauseAction = new QAction(QIcon("icons/pause.png"), "Pause", this);
    QAction *stopAction = new QAction(QIcon("icons/stop.png"), "Stop", this);

    toolBar->addAction(processor);
    toolBar->addAction(newFileAction);
    toolBar->addAction(openFileAction);
    toolBar->addAction(saveFileAction);
    toolBar->addAction(saveAsFileAction);
    toolBar->addAction(assembleAction);
    toolBar->addAction(runAction);
    toolBar->addSeparator();
    toolBar->addAction(stepAction);
    toolBar->addAction(pauseAction);
    toolBar->addAction(stopAction);
    toolBar->addAction(undoAction);
    toolBar->addAction(resetAction);
    toolBar->addSeparator();

    // Speed control
    QWidget *speedControlWidget = new QWidget(this);
    QVBoxLayout *speedLayout = new QVBoxLayout(speedControlWidget);
    speedLayout->setContentsMargins(5, 2, 5, 2);
    speedLayout->setSpacing(2);

    QLabel *speedDisplayLabel = new QLabel("Speed: MAX", this);
    speedDisplayLabel->setStyleSheet("color: #00ff00; font-size: 9pt; font-weight: bold;");
    speedDisplayLabel->setAlignment(Qt::AlignCenter);

    executionSpeedSlider = new QSlider(Qt::Horizontal, this);
    executionSpeedSlider->setRange(1, 31);
    executionSpeedSlider->setValue(31);
    executionSpeedSlider->setFixedWidth(200);

    connect(executionSpeedSlider, &QSlider::valueChanged, this, [speedDisplayLabel](int value)
            {
        if (value > 30) {
            speedDisplayLabel->setText("Speed: MAX");
            speedDisplayLabel->setStyleSheet("color: #00ff00; font-size: 9pt; font-weight: bold;");
        } else {
            speedDisplayLabel->setText(QString("Speed: %1 inst/sec").arg(value));
            speedDisplayLabel->setStyleSheet("color: #dcdcdc; font-size: 9pt; font-weight: bold;");
        } });

    speedLayout->addWidget(speedDisplayLabel);
    speedLayout->addWidget(executionSpeedSlider);
    toolBar->addWidget(speedControlWidget);
    leftLayout->addWidget(toolBar);

    // --- Tab Widget ---
    // qDebug() << "[MainWindow] Creating tab widget";
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setDocumentMode(true);
    tabWidget->hide();

    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    QVBoxLayout *editorLayout = new QVBoxLayout();
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(2);

    QWidget *emptyEditorBackground = new QWidget(this);
    emptyEditorBackground->setStyleSheet("background-color: #1e1e1e; border: none;");

    QStackedWidget *stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(emptyEditorBackground);
    stackedWidget->addWidget(tabWidget);
    stackedWidget->setCurrentIndex(0);

    emptyStateStack = stackedWidget;
    editorLayout->addWidget(stackedWidget, 1);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    editorLayout->addWidget(line);

    // qDebug() << "[MainWindow] Creating bottom panel";
    bottomPanel = new BottomPanel(this);
    bottomPanel->setFixedHeight(250);
    editorLayout->addWidget(bottomPanel, 0);

    QWidget *editorWidget = new QWidget(this);
    editorWidget->setLayout(editorLayout);
    leftLayout->addWidget(editorWidget);

    // --- Right side (registers + execution info) ---
    // qDebug() << "[MainWindow] Creating register panel";
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(5);

    registerPanel = new RegisterPanel(this);
    rightLayout->addWidget(registerPanel, 1);

    QWidget *executionInfoPanel = new QWidget(this);
    executionInfoPanel->setStyleSheet("background-color: #1e1e1e; border: 1px solid #3a3d41;");
    QVBoxLayout *executionLayout = new QVBoxLayout(executionInfoPanel);
    executionLayout->setContentsMargins(8, 8, 8, 8);
    executionLayout->setSpacing(4);

    QLabel *executionTitle = new QLabel("Execution Info");
    executionTitle->setStyleSheet("color: #dcdcdc; font-weight: bold; font-size: 11pt;");

    // Create labels as member variables so we can update them
    instructionCountLabel = new QLabel("Instructions: 0");
    instructionCountLabel->setStyleSheet("color: #dcdcdc; font-size: 10pt;");

    cpiLabel = new QLabel("CPI : 0.00");
    cpiLabel->setStyleSheet("color: #dcdcdc; font-size: 10pt;");

    cycleCountLabel = new QLabel("Cycles: 0");
    cycleCountLabel->setStyleSheet("color: #dcdcdc; font-size: 10pt;");

    executionLayout->addWidget(executionTitle);
    executionLayout->addWidget(instructionCountLabel);
    executionLayout->addWidget(cpiLabel);
    executionLayout->addWidget(cycleCountLabel);
    // executionLayout->addWidget(executionTimeLabel);
    rightLayout->addWidget(executionInfoPanel, 0);

    // ✅ ADD THESE TWO CRITICAL LINES THAT WERE MISSING:
    topLayout->addWidget(leftWidget, 3);
    topLayout->addWidget(rightWidget, 1);
    mainLayout->addLayout(topLayout);

    // Then continue with status bar...
    QStatusBar *statusBar = new QStatusBar(this);
    statusBar->setStyleSheet("QStatusBar { background-color: #3e3e42; color: #cccccc; border: none; }");
    processorInfoLabel = new QLabel(
        QString("Processor: %1 | ISA: %2").arg(lastName, lastISA));
    processorInfoLabel->setStyleSheet("color: #cccccc; font-size: 9pt; padding: 2px 10px;");
    statusBar->addPermanentWidget(processorInfoLabel);
    setStatusBar(statusBar);

    assembler = new Assembler(registerPanel->getRegisterFile(), this);
    singleCycleVm = new RVSSVM(registerPanel->getRegisterFile(), this);
    pipelinedVm = new RVSSVMPipelined(registerPanel->getRegisterFile(), this);
    vm = singleCycleVm;
    errorconsole = bottomPanel->getConsole();
    DataSegment *dataSegment = bottomPanel->getDataSegment();

    if (assembler && errorconsole)
    {
        connect(assembler, &Assembler::errorsAvailable,
                errorconsole, &ErrorConsole::addMessages);
    }

    executionThread_ = new VMExecutionThread(vm, this);

    connect(executionThread_, &VMExecutionThread::executionFinished,
            this, &MainWindow::onExecutionFinished);

    connect(executionThread_, &VMExecutionThread::executionError,
            this, &MainWindow::onExecutionError);

    connect(executionThread_, &VMExecutionThread::stepCompleted,
            this, &MainWindow::onPeriodicUpdate);

    // Create update timer for GUI refresh during execution
    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::onPeriodicUpdate);

    connect(vm, &RVSSVM::gprUpdated, this, [this, dataSegment](int index, quint64 value)
            {
        // qDebug() << "[gprUpdated] index=" << index << "value=" << value;
        try {
            if (!registerPanel) {
                // qDebug() << "[gprUpdated] ERROR: registerPanel is null!";
                return;
            }

            QTabWidget* tabs = registerPanel->getTabWidget();
            if (!tabs) {
                // qDebug() << "[gprUpdated] ERROR: tabWidget is null!";
                return;
            }

            RegisterTable* regTable = registerPanel->getRegTable();
            if (!regTable) {
                // qDebug() << "[gprUpdated] ERROR: regTable is null!";
                return;
            }
            if (index == 2) {
                dataSegment->updateStackPointer(static_cast<uint64_t>(value));
            }

            // qDebug() << "[gprUpdated] Switching to tab 0";
            tabs->setCurrentIndex(0);

            // qDebug() << "[gprUpdated] Updating register";
            regTable->updateRegister(index, static_cast<quint64>(value));
            // qDebug() << "[gprUpdated] DONE";
        } catch (const std::exception& e) {
            // qDebug() << "[gprUpdated] EXCEPTION:" << e.what();
        } });

    connect(vm, &RVSSVM::fprUpdated, this, [this](int index, quint64 value)
            {
        // qDebug() << "[fprUpdated] index=" << index << "value=" << value;
        try {
            if (!registerPanel) {
                // qDebug() << "[fprUpdated] ERROR: registerPanel is null!";
                return;
            }

            QTabWidget* tabs = registerPanel->getTabWidget();
            if (!tabs) {
                // qDebug() << "[fprUpdated] ERROR: tabWidget is null!";
                return;
            }

            RegisterTable* fprTable = registerPanel->getFprTable();
            if (!fprTable) {
                // qDebug() << "[fprUpdated] ERROR: fprTable is null!";
                return;
            }

            // qDebug() << "[fprUpdated] Switching to tab 1";
            tabs->setCurrentIndex(1);

            // qDebug() << "[fprUpdated] Updating register";
            fprTable->updateRegister(index, static_cast<quint64>(value));
            // qDebug() << "[fprUpdated] DONE";
        } catch (const std::exception& e) {
            // qDebug() << "[fprUpdated] EXCEPTION:" << e.what();
        } });

    connect(vm, &RVSSVM::csrUpdated, this, [this](int index, quint64 value)
            {
                // qDebug() << "[csrUpdated] index=" << index << "value=" << value;
                // CSR table handling if needed
            });

    connect(vm, &RVSSVM::memoryUpdated, bottomPanel->getDataSegment(),
            &DataSegment::updateMemory);

    // --- Connect toolbar actions ---
    // qDebug() << "[MainWindow] Connecting toolbar actions";
    connect(newFileAction, &QAction::triggered, this, &MainWindow::onNewFile);
    connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(saveFileAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    connect(saveAsFileAction, &QAction::triggered, this, &MainWindow::onSaveAsFile);
    connect(assembleAction, &QAction::triggered, this, &MainWindow::onAssemble);
    connect(runAction, &QAction::triggered, this, &MainWindow::onRun);
    connect(stepAction, &QAction::triggered, this, &MainWindow::onStep);
    connect(pauseAction, &QAction::triggered, this, &MainWindow::onPause);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStop);
    connect(undoAction, &QAction::triggered, this, &MainWindow::onUndo);
    connect(resetAction, &QAction::triggered, this, &MainWindow::onReset);
    connect(processor, &QAction::triggered, this, &MainWindow::showProcessorSelection);

    // --- Window setup ---
    // qDebug() << "[MainWindow] Setting up window";
    setWindowTitle("RISC-V Simulator");
    QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    setGeometry(availableGeometry);
    show();

    QTimer::singleShot(100, this, [this, availableGeometry]()
                       {
#ifdef Q_OS_WIN
                           setGeometry(availableGeometry);
                           setWindowState(Qt::WindowMaximized);
#else
                           showMaximized();
#endif
                       });

    // qDebug() << "[MainWindow] Constructor COMPLETE";
}

CodeEditor *MainWindow::getCurrentEditor()
{
    int index = tabWidget->currentIndex();
    if (index >= 0 && index < fileTabs.size())
    {
        return fileTabs[index].editor;
    }
    return nullptr;
}

int MainWindow::getCurrentTabIndex()
{
    return tabWidget->currentIndex();
}

QString MainWindow::getCurrentFilePath()
{
    if (currentTabIndex < 0 || currentTabIndex >= fileTabs.size())
        return QString();
    return fileTabs[currentTabIndex].filePath;
}

void MainWindow::updateTabTitle(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= fileTabs.size())
        return;

    QString title;
    if (fileTabs[tabIndex].filePath.isEmpty())
    {
        title = "Untitled";
    }
    else
    {
        title = QFileInfo(fileTabs[tabIndex].filePath).fileName();
    }

    if (fileTabs[tabIndex].isModified)
    {
        title += " *";
    }

    tabWidget->setTabText(tabIndex, title);
}

void MainWindow::setTabModified(int tabIndex, bool modified)
{
    if (tabIndex < 0 || tabIndex >= fileTabs.size())
        return;

    fileTabs[tabIndex].isModified = modified;
    updateTabTitle(tabIndex);
}

void MainWindow::onTabChanged(int index)
{
    currentTabIndex = index;
}

void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0 || index >= fileTabs.size())
        return;

    if (promptSaveChanges(index))
    {
        tabWidget->removeTab(index);
        delete fileTabs[index].editor;
        fileTabs.removeAt(index);

        if (fileTabs.isEmpty())
        {
            tabWidget->hide();
            emptyStateStack->setCurrentIndex(0);
        }
    }
}

void MainWindow::onNewFile()
{
    // qDebug() << "[onNewFile] START";
    if (fileTabs.isEmpty())
    {
        tabWidget->show();
        emptyStateStack->setCurrentIndex(1);
    }

    CodeEditor *editor = new CodeEditor(this);
    int index = tabWidget->addTab(editor, "Untitled");
    tabWidget->setCurrentIndex(index);

    FileTab tab;
    tab.editor = editor;
    tab.filePath = "";
    tab.isModified = false;
    fileTabs.append(tab);

    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool changed)
            {
        for (int i = 0; i < fileTabs.size(); ++i) {
            if (fileTabs[i].editor == editor) {
                setTabModified(i, changed);
                break;
            }
        } });

    updateTabTitle(index);
    editor->setFocus();
    // qDebug() << "[onNewFile] DONE";
}

void MainWindow::onOpenFile()
{
    // qDebug() << "[onOpenFile] START";
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open Assembly File", QString(), "Assembly Files (*.s *.asm);;All Files (*)");

    if (fileName.isEmpty())
        return;

    for (int i = 0; i < fileTabs.size(); ++i)
    {
        if (fileTabs[i].filePath == fileName)
        {
            tabWidget->setCurrentIndex(i);
            QMessageBox::information(this, "Info", "File is already open!");
            return;
        }
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Could not open file: " + fileName);
        return;
    }

    QString content = QTextStream(&file).readAll();
    file.close();

    if (fileTabs.isEmpty())
    {
        tabWidget->show();
        emptyStateStack->setCurrentIndex(1);
    }

    CodeEditor *editor = new CodeEditor(this);
    editor->setPlainText(content);

    int index = tabWidget->addTab(editor, QFileInfo(fileName).fileName());
    tabWidget->setCurrentIndex(index);

    FileTab tab;
    tab.editor = editor;
    tab.filePath = fileName;
    tab.isModified = false;
    fileTabs.append(tab);

    connect(editor->document(), &QTextDocument::modificationChanged, this, [this, editor](bool changed)
            {
        for (int i = 0; i < fileTabs.size(); ++i) {
            if (fileTabs[i].editor == editor) {
                setTabModified(i, changed);
                break;
            }
        } });

    editor->document()->setModified(false);
    updateTabTitle(index);
    // qDebug() << "[onOpenFile] DONE";
}

bool MainWindow::saveToFile(int tabIndex, const QString &filePath)
{
    if (tabIndex < 0 || tabIndex >= fileTabs.size())
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "Error", "Could not save file: " + filePath);
        return false;
    }

    QTextStream out(&file);
    out << fileTabs[tabIndex].editor->toPlainText();
    file.close();

    fileTabs[tabIndex].filePath = filePath;
    fileTabs[tabIndex].editor->document()->setModified(false);
    setTabModified(tabIndex, false);

    statusBar()->showMessage("File saved successfully: " + QFileInfo(filePath).fileName(), 3000);
    return true;
}

void MainWindow::onSaveFile()
{
    int index = getCurrentTabIndex();
    if (index < 0)
        return;

    if (fileTabs[index].filePath.isEmpty())
    {
        onSaveAsFile();
    }
    else
    {
        saveToFile(index, fileTabs[index].filePath);
    }
}

void MainWindow::onSaveAsFile()
{
    int index = getCurrentTabIndex();
    if (index < 0)
        return;

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Assembly File", QString(), "Assembly Files (*.s *.asm);;All Files (*)");

    if (!fileName.isEmpty())
    {
        if (saveToFile(index, fileName))
        {
            updateTabTitle(index);
        }
    }
}

void MainWindow::onAssemble()
{
    CodeEditor *editor = getCurrentEditor();

    if (!editor)
    {
        QMessageBox::warning(this, "Warning", "No file is open!");
        return;
    }

    // ✅ STOP any running execution before assembling
    if (executionThread_ && executionThread_->isRunning())
    {
        executionThread_->requestStop();
        executionThread_->wait(2000);
        updateTimer_->stop();
    }

    QString filePath = getCurrentFilePath();
    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "No File Open", "Please save the file before assembling.");
        return;
    }

    if (currentTabIndex >= 0 && currentTabIndex < fileTabs.size() &&
        fileTabs[currentTabIndex].isModified)
    {
        bool saved = saveToFile(currentTabIndex, filePath);
        if (!saved)
            return;
    }

    if (registerPanel)
    {
        registerPanel->resetAllTables();
    }

    if (!assembler)
    {
        QMessageBox::critical(this, "Error", "Assembler not initialized!");
        return;
    }

    if (editor)
    {
        editor->clearAllPipelineLabels();
    }

    std::string cppFilePath = filePath.toStdString();
    std::string startMsg = "Assemble: assembling " + cppFilePath;
    errorconsole->addMessages({startMsg});

    program = assembler->assemble(cppFilePath);

    if (program.errorCount == 0)
    {
        std::string successMsg = "Assemble: operation completed successfully.";
        errorconsole->addMessages({successMsg});

        // ✅ Reset VM and load new program
        vm->Reset();
        vm->LoadProgram(program);

        uint64_t dataSegmentBase = 0x10000000;
        size_t memorySize = 1024 *  32;

        DataSegment *dataSegment = bottomPanel->getDataSegment();
        std::vector<uint8_t> memoryBytes = vm->GetMemoryRange(dataSegmentBase, memorySize);

        QVector<quint64> memoryWords;
        for (size_t i = 0; i < memoryBytes.size(); i += 4)
        {
            uint32_t word = 0;
            for (size_t j = 0; j < 4 && (i + j) < memoryBytes.size(); j++)
            {
                word |= (static_cast<uint32_t>(memoryBytes[i + j]) << (j * 8));
            }
            memoryWords.append(word);
        }

        dataSegment->clearData();
        dataSegment->setMemory(memoryWords, 0x10000000);

        // ✅ Update all displays
        updateRegisterTable();
        updateExecutionInfo();
        highlightCurrentLine();
    }
    else
    {
        bottomPanel->changeTab();
    }
}

// void MainWindow::onRun()
// {
//     CodeEditor *editor = getCurrentEditor();
//     if (!editor)
//     {
//         QMessageBox::warning(this, "Warning", "No file is open!");
//         return;
//     }

//     if (!vm)
//     {
//         QMessageBox::critical(this, "Error", "VM not initialized!");
//         return;
//     }

//     if (program.errorCount != 0)
//     {
//         QMessageBox::warning(this, "Assembly Error",
//                              "Please fix assembly errors before running.");
//         return;
//     }

//     if (executionThread_ && executionThread_->isRunning())
//     {
//         QMessageBox::information(this, "Info", "Execution already in progress!");
//         return;
//     }

//     onReset();
//     vm->stop_requested_ = false;

//     int speed = executionSpeedSlider->value();

//     // ✅ DEBUG: Check VM type and initial state
//     RVSSVMPipelined *pipeVm = qobject_cast<RVSSVMPipelined *>(vm);
//     // qDebug() << "========= EXECUTION START =========";
//     // qDebug() << "VM Type:" << (pipeVm ? "PIPELINED" : "SINGLE-CYCLE");
//     // qDebug() << "Program Size:" << vm->GetProgramSize();
//     // qDebug() << "Initial PC:" << vm->GetProgramCounter();
//     // qDebug() << "Pipeline Empty:" << vm->IsPipelineEmpty();
//     // qDebug() << "===================================";

//     if (speed > 30)
//     {
//         // === FAST MODE: Run in background thread ===
//         executionThread_->setVM(vm);
//         executionThread_->setMaxInstructions(10000);

//         editor->clearAllPipelineLabels();
//         clearLineHighlight();
//         updateExecutionInfo();

//         executionThread_->start();
//         updateTimer_->start(100);

//         statusBar()->showMessage("Running in background...", 0);
//     }
//     else
//     {
//         // === ANIMATED MODE: Step-by-step with timer ===
//         editor->clearAllPipelineLabels();
//         clearLineHighlight();
//         updateExecutionInfo();

//         int delayMs = 1000 / speed;
//         QTimer *stepTimer = new QTimer(this);

//         // ✅ DEBUG counter
//         int stepCount = 0;

//         connect(stepTimer, &QTimer::timeout, this, [this, stepTimer, &stepCount, pipeVm]()
//                 {
//             stepCount++;

//             uint64_t pc = vm->GetProgramCounter();
//             uint64_t progSize = vm->GetProgramSize();
//             bool pipeEmpty = vm->IsPipelineEmpty();
//             bool stopReq = vm->IsStopRequested();

//             // ✅ DEBUG: Print every 10 steps or near end
//             // if (stepCount % 10 == 0 || pc >= progSize) {
//                 // qDebug() << "Step" << stepCount
//                 //          << "| PC:" << pc
//                 //          << "/ Size:" << progSize
//                 //          << "| Pipeline Empty:" << pipeEmpty
//                 //          << "| Stop Req:" << stopReq
//                 //          << "| Instr Retired:" << vm->instructions_retired_;
//             // }

//             if ((pc >= progSize && pipeEmpty) || stopReq) {
//                 // qDebug() << "========= EXECUTION END =========";
//                 // qDebug() << "Reason:" << (stopReq ? "STOP REQUESTED" : "PROGRAM END");
//                 // qDebug() << "Final PC:" << pc;
//                 // qDebug() << "Pipeline Empty:" << pipeEmpty;
//                 // qDebug() << "Total Steps:" << stepCount;
//                 // qDebug() << "=================================";

//                 stepTimer->stop();
//                 stepTimer->deleteLater();

//                 updateExecutionInfo();
//                 refreshMemoryDisplay();

//                 QString msg = QString("Execution finished!\nInstructions: %1\nCycles: %2")
//                                   .arg(vm->instructions_retired_)
//                                   .arg(vm->cycle_s_);
//                 QMessageBox::information(this, "Complete", msg);
//                 return;
//             }

//             vm->Step();
//             updateRegisterTable();
//             highlightCurrentLine();
//             updateExecutionInfo(); });

//         stepTimer->start(delayMs);
//     }
// }

void MainWindow::onStep()
{
    // qDebug() << "[onStep] START";
    if (!vm)
        return;
    // qDebug() << pipelinedVm;
    if (program.errorCount != 0)
    {
        return;
    }
    vm->Step();
    QCoreApplication::processEvents();

    updateRegisterTable();
    highlightCurrentLine();
    updateExecutionInfo();
    refreshMemoryDisplay();
}

void MainWindow::onUndo()
{
    // CodeEditor *editor = getCurrentEditor();
    if (program.errorCount != 0)
    {
        return;
    }

    if (vm)
    {
        vm->Undo(); // Undo last editing operation in the current editor.
        updateRegisterTable();
        highlightCurrentLine();
        updateExecutionInfo();
        refreshMemoryDisplay();
        QCoreApplication::processEvents();
    }
}

void MainWindow::onReset()
{
    if (!vm)
        return;

    // ✅ Stop any running execution
    if (executionThread_ && executionThread_->isRunning())
    {
        executionThread_->requestStop();
        executionThread_->wait(1000);
        updateTimer_->stop();
    }

    vm->RequestStop();
    vm->Reset();

    if (program.errorCount == 0)
    {
        vm->LoadProgram(program);
    }

    updateRegisterTable();
    updateExecutionInfo();
    refreshMemoryDisplay();
    clearLineHighlight();

    CodeEditor *editor = getCurrentEditor();
    if (editor)
    {
        editor->clearAllPipelineLabels();
        editor->setFocus();
    }
}

void MainWindow::highlightCurrentLine()
{
    CodeEditor *editor = getCurrentEditor();
    if (!editor || !vm)
        return;

    uint64_t pc = vm->GetProgramCounter();
    unsigned int instructionNum = pc / 4;

    if (program.instruction_number_line_number_mapping.count(instructionNum) == 0)
        return;

    unsigned int sourceLine = program.instruction_number_line_number_mapping[instructionNum];

    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, sourceLine - 1);
    cursor.select(QTextCursor::LineUnderCursor);
    editor->highlightLine(sourceLine);
    // QTextEdit::ExtraSelection highlight;
    // highlight.cursor = cursor;
    // highlight.format.setBackground(QColor(100, 150, 255, 100));
    // highlight.format.setProperty(QTextFormat::FullWidthSelection, true);

    // QList<QTextEdit::ExtraSelection> extras;
    // extras << highlight;
    // editor->setExtraSelections(extras);
}

void MainWindow::clearLineHighlight()
{
    CodeEditor *editor = getCurrentEditor();
    if (!editor)
        return;

    editor->setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

bool MainWindow::promptSaveChanges(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= fileTabs.size())
        return true;

    if (fileTabs[tabIndex].isModified)
    {
        QString fileName = fileTabs[tabIndex].filePath.isEmpty()
                               ? "Untitled"
                               : QFileInfo(fileTabs[tabIndex].filePath).fileName();

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            "Do you want to save changes to \"" + fileName + "\"?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save)
        {
            if (fileTabs[tabIndex].filePath.isEmpty())
            {
                QString savePath = QFileDialog::getSaveFileName(
                    this, "Save Assembly File", QString(), "Assembly Files (*.s *.asm);;All Files (*)");
                if (savePath.isEmpty())
                    return false;
                return saveToFile(tabIndex, savePath);
            }
            else
            {
                return saveToFile(tabIndex, fileTabs[tabIndex].filePath);
            }
        }
        else if (reply == QMessageBox::Cancel)
        {
            return false;
        }
    }
    return true;
}

void MainWindow::updateRegisterTable()
{
    try
    {
        if (!registerPanel)
        {
            return;
        }

        RegisterFile *registers = registerPanel->getRegisterFile();
        if (!registers)
        {
            return;
        }

        RegisterTable *reg_table = registerPanel->getRegTable();
        RegisterTable *fpr_table = registerPanel->getFprTable();

        if (!reg_table || !fpr_table)
        {
            qDebug() << "[updateRegisterTable] ERROR: tables are null!";
            return;
        }

        uint64_t pc_value = vm ? vm->GetProgramCounter() : 0;

        // INTEGER REGISTERS (32 GPRs + PC)
        QVector<quint64> reg_values;
        for (uint64_t v : registers->GetGprValues())
            reg_values << static_cast<quint64>(v);
        reg_values << static_cast<quint64>(pc_value); // Add PC

        // FLOATING POINT REGISTERS (32 FPRs + PC)
        QVector<quint64> fpr_values;
        for (uint64_t v : registers->GetFprValues())
            fpr_values << static_cast<quint64>(v);
        fpr_values << static_cast<quint64>(pc_value); // Add PC for size match

        reg_table->updateAllRegisters(reg_values);
        fpr_table->updateAllRegisters(fpr_values);
    }
    catch (const std::exception &e)
    {
        qDebug() << "[updateRegisterTable] EXCEPTION:" << e.what();
    }
}

void MainWindow::updateExecutionInfo()
{
    if (!vm)
        return;

    instructionCountLabel->setText(QString("Instructions: %1").arg(vm->instructions_retired_));
    cycleCountLabel->setText(QString("Cycles: %1").arg(vm->cycle_s_));

    // Calculate execution time (if you track it)
    // For now, showing cycles as approximate time
    // double timeMs = vm->cycle_s_ * 0.001; // Placeholder calculation
    // executionTimeLabel->setText(QString("Time: %1 ms").arg(timeMs, 0, 'f', 2));
    double cpi = 0.0;
    if(vm->instructions_retired_ > 0){
        cpi = static_cast<double>(vm->cycle_s_)/ vm->instructions_retired_;

        cpiLabel->setText(QString("CPI : %1").arg(cpi,0,'f',2));
    }
    else {
        cpiLabel->setText(QString("CPI : %1").arg(cpi,0,'f',2));
    }
}

void MainWindow::refreshMemoryDisplay()
{
    if (!vm)
        return;

    DataSegment *dataSegment = bottomPanel->getDataSegment();
    uint64_t dataSegmentBase = 0x10000000;
    size_t memorySize = 512;

    // Read actual memory from VM
    std::vector<uint8_t> memoryBytes = vm->GetMemoryRange(dataSegmentBase, memorySize);

    // Convert to words
    QVector<quint64> memoryWords;
    for (size_t i = 0; i < memoryBytes.size(); i += 4)
    {
        uint32_t word = 0;
        for (size_t j = 0; j < 4 && (i + j) < memoryBytes.size(); j++)
        {
            word |= (static_cast<uint32_t>(memoryBytes[i + j]) << (j * 8));
        }
        memoryWords.append(word);
    }

    dataSegment->setMemory(memoryWords, dataSegmentBase);

    // Also update byte-level map
    QVector<uint8_t> qMemoryBytes;
    for (auto byte : memoryBytes)
    {
        qMemoryBytes.append(byte);
    }
    dataSegment->updateMemory(dataSegmentBase, qMemoryBytes);
}

void MainWindow::showProcessorSelection()
{
    // ✅ Stop execution before switching
    if (executionThread_ && executionThread_->isRunning())
    {
        executionThread_->requestStop();
        executionThread_->wait(2000);
        updateTimer_->stop();
    }

    ProcessorWindow dlg(this);
    dlg.setInitialSelection(lastName, lastISA);

    if (dlg.exec() == QDialog::Accepted)
    {
        lastName = dlg.selectedProcessorName();
        lastISA = dlg.selectedISA();

        processorInfoLabel->setText(QString("Processor: %1 | ISA: %2")
                                        .arg(lastName, lastISA));
        ISA selected = (lastISA == "RV32") ? ISA::RV32 : ISA::RV64;

        // ✅ Disconnect old pipelined signals
        RVSSVMPipelined *oldPipeVm = qobject_cast<RVSSVMPipelined *>(vm);
        if (oldPipeVm)
        {
            disconnect(oldPipeVm, &RVSSVMPipelined::pipelineStageChanged,
                       this, &MainWindow::onPipelineStageChanged);
        }

        // Switch VM
        if (lastName == "Single-cycle processor")
        {
            vm = singleCycleVm;
        }
        else
        {
            vm = pipelinedVm;

            RVSSVMPipelined *pipeVm = qobject_cast<RVSSVMPipelined *>(vm);
            if (pipeVm)
            {
                // Configure pipeline based on selection
                if (lastName == "5-stage processor w/o forwarding or hazard detection")
                {
                    vm->SetPipelineConfig(false, false, false, false);
                }
                else if (lastName == "5-stage processor with forwarding and hazard detection")
                {
                    vm->SetPipelineConfig(true, true, false, false);
                }
                else if (lastName == "5-stage processor w/o hazard detection")
                {
                    vm->SetPipelineConfig(false, true, false, false);
                }
                else if (lastName == "5-stage processor w/o forwarding unit")
                {
                    vm->SetPipelineConfig(true, false, false, false);
                }
                else if (lastName == "5-stage processor with static Branch prediction")
                {
                    qDebug() << "static Branch";
                    vm->SetPipelineConfig(true, true, true, false);
                }
                else if (lastName == "5-stage processor with dynamic 1-bit Branch prediction")
                {
                    vm->SetPipelineConfig(true, true, true, true);
                }

                // ✅ Connect new pipelined signals
                connect(pipeVm, &RVSSVMPipelined::pipelineStageChanged,
                        this, &MainWindow::onPipelineStageChanged);
            }
        }

        // ✅ Update thread's VM pointer
        executionThread_->setVM(vm);

        // Reset and update displays
        vm->Reset();
        if (program.errorCount == 0)
        {
            vm->LoadProgram(program);
        }

        vm->registers_->SetIsa(selected);
        updateRegisterTable();
        updateExecutionInfo();
        refreshMemoryDisplay();
        clearLineHighlight();

        CodeEditor *editor = getCurrentEditor();
        if (editor)
        {
            editor->clearAllPipelineLabels();
        }
    }
}

void MainWindow::onPipelineStageChanged(uint64_t pc, QString stage)
{
    CodeEditor *editor = getCurrentEditor();
    if (!editor)
        return;

    // Convert PC to line number
    unsigned int instrIndex = static_cast<unsigned int>(pc / 4);
    auto it = program.instruction_number_line_number_mapping.find(instrIndex);
    if (it == program.instruction_number_line_number_mapping.end())
    {
        // qDebug() << "No mapping found for PC" << pc;
        if (stage.endsWith("_CLEAR"))
        {
            QString baseStage = stage.left(stage.indexOf("_CLEAR"));
            QMap<int, QSet<QString>> labels = editor->getPipelineLabels();

            for (auto lineIt = labels.begin(); lineIt != labels.end(); ++lineIt)
            {
                if (lineIt.value().contains(baseStage))
                {
                    editor->clearPipelineLabels(lineIt.key(), baseStage);
                }
            }
            return;
        }
    }
    unsigned int sourceLine = it->second; // 1-based

    // Handle CLEAR or SET
    if (stage.endsWith("_CLEAR"))
    {
        QString baseStage = stage.left(stage.indexOf("_CLEAR"));
        editor->clearPipelineLabels(sourceLine, baseStage);
        // qDebug() << "Cleared" << baseStage << "from line" << sourceLine;
    }
    else
    {
        editor->setPipelineLabel(sourceLine, stage);
        // qDebug() << "Set" << stage << "at line" << sourceLine;
    }
}

// void MainWindow::onExecutionFinished(uint64_t instructions, uint64_t cycles)
// {
//     updateTimer_->stop();

//     // Final UI update
//     updateRegisterTable();
//     highlightCurrentLine();
//     updateExecutionInfo();
//     refreshMemoryDisplay();

//     statusBar()->showMessage("Execution complete", 3000);

//     QString msg = QString("Execution finished!\nInstructions: %1\nCycles: %2")
//                       .arg(instructions)
//                       .arg(cycles);
//     QMessageBox::information(this, "Complete", msg);
// }

// void MainWindow::onExecutionError(QString message)
// {
//     updateTimer_->stop();

//     // Update UI with final state
//     updateRegisterTable();
//     highlightCurrentLine();
//     updateExecutionInfo();
//     refreshMemoryDisplay();

//     statusBar()->showMessage("Execution stopped", 3000);
//     QMessageBox::critical(this, "Execution Error", message);
// }

// void MainWindow::onPeriodicUpdate()
// {
//     // Update GUI periodically during execution
//     updateRegisterTable();
//     highlightCurrentLine();
//     updateExecutionInfo();

//     // ✅ For pipelined mode, also update pipeline labels
//     CodeEditor *editor = getCurrentEditor();
//     if (editor)
//     {
//         RVSSVMPipelined *pipeVm = qobject_cast<RVSSVMPipelined *>(vm);
//         if (pipeVm)
//         {
//             // Pipeline labels are updated via signals, but we can force a repaint
//             editor->viewport()->update();
//         }
//     }
// }

void MainWindow::onStop()
{
    if (executionThread_ && executionThread_->isRunning())
    {
        executionThread_->requestStop();
        executionThread_->wait(1000); // Wait up to 1 second
    }

    updateTimer_->stop();

    if (vm)
    {
        vm->RequestStop();
        vm->Reset();
        updateRegisterTable();
        clearLineHighlight();
        updateExecutionInfo();
    }

    statusBar()->showMessage("Execution stopped", 2000);
}

void MainWindow::onPause()
{
    if (executionThread_ && executionThread_->isRunning())
    {
        executionThread_->requestStop();
    }

    if (vm)
    {
        vm->RequestStop();
    }

    updateTimer_->stop();
    statusBar()->showMessage("Execution paused", 2000);
}

MainWindow::~MainWindow()
{
    if (executionThread_)
    {
        executionThread_->requestStop();
        if (!executionThread_->wait(2000))
        {
            executionThread_->terminate();
            executionThread_->wait();
        }
    }
}

void MainWindow::onRun()
{
    CodeEditor *editor = getCurrentEditor();
    if (!editor)
    {
        QMessageBox::warning(this, "Warning", "No file is open!");
        return;
    }

    if (!vm)
    {
        QMessageBox::critical(this, "Error", "VM not initialized!");
        return;
    }

    if (program.errorCount != 0)
    {
        QMessageBox::warning(this, "Assembly Error",
                             "Please fix assembly errors before running.");
        return;
    }

    if (executionThread_ && executionThread_->isRunning())
    {
        QMessageBox::information(this, "Info", "Execution already in progress!");
        return;
    }

    onReset();
    vm->stop_requested_ = false;

    int speed = executionSpeedSlider->value();

    // ✅ DEBUG: Check VM type and initial state
    RVSSVMPipelined *pipeVm = qobject_cast<RVSSVMPipelined *>(vm);
    qDebug() << "========= EXECUTION START =========";
    qDebug() << "VM Type:" << (pipeVm ? "PIPELINED" : "SINGLE-CYCLE");
    qDebug() << "Program Size:" << vm->GetProgramSize();
    qDebug() << "Initial PC:" << vm->GetProgramCounter();
    qDebug() << "Pipeline Empty:" << vm->IsPipelineEmpty();
    qDebug() << "===================================";

    if (speed > 30)
    {
        // === FAST MODE: Run in background thread ===
        executionThread_->setVM(vm);
        executionThread_->setMaxInstructions(1000000); // ✅ Increased from 10000

        editor->clearAllPipelineLabels();
        clearLineHighlight();
        updateExecutionInfo();

        executionThread_->start();
        updateTimer_->start(100);

        statusBar()->showMessage("Running in background...", 0);
    }
    else
    {
        // === ANIMATED MODE: Step-by-step with timer ===
        editor->clearAllPipelineLabels();
        clearLineHighlight();
        updateExecutionInfo();

        int delayMs = 1000 / speed;
        QTimer *stepTimer = new QTimer(this);

        // ✅ Step counter and safety limit
        int *stepCount = new int(0);
        const int MAX_STEPS = 100000; // Safety limit

        connect(stepTimer, &QTimer::timeout, this, [this, stepTimer, stepCount, pipeVm, MAX_STEPS]()
                {
                    (*stepCount)++;

                    // ✅ CRITICAL: Emergency stop after too many steps
                    if (*stepCount > MAX_STEPS)
                    {
                        qDebug() << "\n========== EMERGENCY STOP ==========";
                        qDebug() << "Exceeded" << MAX_STEPS << "steps - stopping execution";
                        qDebug() << "Final PC:" << vm->GetProgramCounter();
                        qDebug() << "Instructions Retired:" << vm->instructions_retired_;

                        stepTimer->stop();
                        stepTimer->deleteLater();
                        delete stepCount;

                        updateExecutionInfo();
                        refreshMemoryDisplay();

                        QMessageBox::warning(this, "Execution Stopped",
                                             QString("Execution stopped after %1 steps to prevent infinite loop.\n\n"
                                                     "Instructions executed: %2\n"
                                                     "Cycles: %3")
                                                 .arg(MAX_STEPS)
                                                 .arg(vm->instructions_retired_)
                                                 .arg(vm->cycle_s_));
                        return;
                    }

                    uint64_t pc = vm->GetProgramCounter();
                    uint64_t progSize = vm->GetProgramSize();
                    bool pipeEmpty = vm->IsPipelineEmpty();
                    bool stopReq = vm->IsStopRequested();

                    // ✅ DEBUG: Print every 100 steps or near end
                    if (*stepCount % 100 == 0 || pc >= progSize)
                    {
                        qDebug() << "[Step" << *stepCount << "]"
                                 << "PC:" << QString::number(pc, 16)
                                 << "/ Size:" << QString::number(progSize, 16)
                                 << "| Pipeline Empty:" << pipeEmpty
                                 << "| Stop Req:" << stopReq
                                 << "| Instr:" << vm->instructions_retired_;
                    }

                    // ✅ CRITICAL FIX: Better termination check
                    // For pipelined: PC >= size AND pipeline empty
                    // For single-cycle: PC >= size OR stop requested
                    bool shouldStop = false;
                    if (pipeVm)
                    {
                        // Pipelined VM: must drain pipeline
                        shouldStop = (pc >= progSize && pipeEmpty) || stopReq;
                    }
                    else
                    {
                        // Single-cycle VM: immediate stop
                        shouldStop = (pc >= progSize) || stopReq;
                    }

                    if (shouldStop)
                    {
                        qDebug() << "\n========= EXECUTION END =========";
                        qDebug() << "Reason:" << (stopReq ? "STOP REQUESTED" : "PROGRAM END");
                        qDebug() << "Final PC:" << pc;
                        qDebug() << "Pipeline Empty:" << pipeEmpty;
                        qDebug() << "Total Steps:" << *stepCount;
                        qDebug() << "Instructions:" << vm->instructions_retired_;
                        qDebug() << "Cycles:" << vm->cycle_s_;
                        qDebug() << "=================================";

                        stepTimer->stop();
                        stepTimer->deleteLater();
                        delete stepCount;

                        updateExecutionInfo();
                        refreshMemoryDisplay();

                        QString msg = QString("Execution finished!\n\n"
                                              "Instructions: %1\n"
                                              "Cycles: %2\n"
                                              "CPI: %3")
                                          .arg(vm->instructions_retired_)
                                          .arg(vm->cycle_s_)
                                          .arg(vm->instructions_retired_ > 0
                                                   ? QString::number((double)vm->cycle_s_ / vm->instructions_retired_, 'f', 2)
                                                   : "N/A");
                        QMessageBox::information(this, "Complete", msg);
                        return;
                    }

                    // Execute one step
                    vm->Step();

                    // Update UI
                    updateRegisterTable();
                    highlightCurrentLine();
                    updateExecutionInfo();
                });

        stepTimer->start(delayMs);
    }
}

void MainWindow::onExecutionFinished(uint64_t instructions, uint64_t cycles)
{
    updateTimer_->stop();

    qDebug() << "\n========= onExecutionFinished =========";
    qDebug() << "Instructions:" << instructions;
    qDebug() << "Cycles:" << cycles;
    qDebug() << "=======================================";

    // Final UI update
    updateRegisterTable();
    highlightCurrentLine();
    updateExecutionInfo();
    refreshMemoryDisplay();

    statusBar()->showMessage("Execution complete", 3000);

    double cpi = instructions > 0 ? (double)cycles / instructions : 0.0;
    QString msg = QString("Execution finished!\n\n"
                          "Instructions: %1\n"
                          "Cycles: %2\n"
                          "CPI: %3")
                      .arg(instructions)
                      .arg(cycles)
                      .arg(cpi, 0, 'f', 2);
    QMessageBox::information(this, "Complete", msg);
}

void MainWindow::onExecutionError(QString message)
{
    updateTimer_->stop();

    qDebug() << "\n========= onExecutionError =========";
    qDebug() << "Error:" << message;
    qDebug() << "====================================";

    // Update UI with final state
    updateRegisterTable();
    highlightCurrentLine();
    updateExecutionInfo();
    refreshMemoryDisplay();

    statusBar()->showMessage("Execution stopped", 3000);
    QMessageBox::critical(this, "Execution Error", message);
}

void MainWindow::onPeriodicUpdate()
{
    // Update GUI periodically during execution
    updateRegisterTable();
    highlightCurrentLine();
    updateExecutionInfo();

    // ✅ For pipelined mode, also update pipeline labels
    CodeEditor *editor = getCurrentEditor();
    if (editor)
    {
        RVSSVMPipelined *pipeVm = qobject_cast<RVSSVMPipelined *>(vm);
        if (pipeVm)
        {
            // Pipeline labels are updated via signals, but we can force a repaint
            editor->viewport()->update();
        }
    }

    // ✅ Check if execution should stop
    uint64_t pc = vm->GetProgramCounter();
    uint64_t progSize = vm->GetProgramSize();
    bool pipeEmpty = vm->IsPipelineEmpty();

    RVSSVMPipelined *pipeVm = qobject_cast<RVSSVMPipelined *>(vm);
    if (pipeVm)
    {
        if (pc >= progSize && pipeEmpty)
        {
            qDebug() << "onPeriodicUpdate: Detected program end";
            onExecutionFinished(vm->instructions_retired_, vm->cycle_s_);
        }
    }
    else
    {
        if (pc >= progSize)
        {
            qDebug() << "onPeriodicUpdate: Detected program end";
            onExecutionFinished(vm->instructions_retired_, vm->cycle_s_);
        }
    }
}
