#include "mainwindow.h"
#include "codeeditor.h"
#include "registerpanel.h"
#include "bottompanel.h"
// #include "errorconsole.h"
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

    cycleCountLabel = new QLabel("Cycles: 0");
    cycleCountLabel->setStyleSheet("color: #dcdcdc; font-size: 10pt;");

    executionLayout->addWidget(executionTitle);
    executionLayout->addWidget(instructionCountLabel);
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
        QString("Processor: %1 | ISA: %2").arg(lastName, lastISA)
        );
    processorInfoLabel->setStyleSheet("color: #cccccc; font-size: 9pt; padding: 2px 10px;");
    statusBar->addPermanentWidget(processorInfoLabel);
    setStatusBar(statusBar);

    // --- Initialize backend ---
    // qDebug() << "[MainWindow] Initializing backend components";
    // qDebug() << "[MainWindow] registerPanel =" << registerPanel;
    // qDebug() << "[MainWindow] registerPanel->getRegisterFile() =" << registerPanel->getRegisterFile();

    assembler = new Assembler(registerPanel->getRegisterFile(), this);
    vm = new RVSSVM(registerPanel->getRegisterFile(), this);
    errorconsole = bottomPanel->getConsole();
    DataSegment *dataSegment = bottomPanel->getDataSegment();

    // qDebug() << "[MainWindow] assembler =" << assembler;
    // qDebug() << "[MainWindow] vm =" << vm;
    // qDebug() << "[MainWindow] errorconsole =" << errorconsole;

    if (assembler && errorconsole)
    {
        connect(assembler, &Assembler::errorsAvailable,
                errorconsole, &ErrorConsole::addMessages);
        // qDebug() << "[MainWindow] Connected assembler to error console";
    }

    // Connect VM signals with SAFETY CHECKS
    // qDebug() << "[MainWindow] Connecting VM signals";

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

    // connect(vm, &RVSSVMPipelined::pipelineStageChanged,
    //         this, &MainWindow::onPipelineStageChanged);



    // registerPanel->setregisterBitWidth(lastISA == "RV32" ? 32 : 64);

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

MainWindow::~MainWindow()
{
    // qDebug() << "[MainWindow] Destructor";
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
    // qDebug() << "[onAssemble] START";
    CodeEditor *editor = getCurrentEditor();

    if (!editor)
    {
        QMessageBox::warning(this, "Warning", "No file is open!");
        return;
    }

    QString filePath = getCurrentFilePath();
    if (filePath.isEmpty())
    {
        QMessageBox::warning(this, "No File Open", "Please save the file before assembling.");
        return;
    }

    if (currentTabIndex >= 0 && currentTabIndex < fileTabs.size() && fileTabs[currentTabIndex].isModified)
    {
        bool saved = saveToFile(currentTabIndex, filePath);
        if (!saved)
            return;
    }

    if (!assembler)
    {
        QMessageBox::critical(this, "Error", "Assembler not initialized!");
        return;
    }

    if (editor) {
        // editor->clearLineHighlight();    // remove old highlights
        editor->clearPipelineLabels();   // remove old pipeline stage text
    }
    std::string cppFilePath = filePath.toStdString();
    std::string startMsg = "Assemble: assembling " + cppFilePath;
    errorconsole->addMessages({startMsg});
    editor->clearPipelineLabels();

    // qDebug() << "[onAssemble] Running assembler";
    program = assembler->assemble(cppFilePath);

    if (program.errorCount == 0)
    {
        std::string successMsg = "Assemble: operation completed successfully.";
        errorconsole->addMessages({successMsg});


        vm->LoadProgram(program);

        uint64_t dataSegmentBase = 0x10000000;
        size_t memorySize = 512; // 512 byte

        DataSegment *dataSegment = bottomPanel->getDataSegment();
        std::vector<uint8_t> memoryBytes = vm->GetMemoryRange(dataSegmentBase, memorySize);

        // Convert bytes to words for display
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

        // Set memory starting at data segment base address
        dataSegment->clearData();
        dataSegment->setMemory(memoryWords, 0x10000000);
        updateRegisterTable();
        updateExecutionInfo();
        highlightCurrentLine();
        vm->Reset();
    }
    else {
        bottomPanel->changeTab();
    }

    // qDebug() << "[onAssemble] Updating register table";
    // qDebug() << "[onAssemble] DONE";
}

void MainWindow::onRun()
{
    // qDebug() << "[onRun] START";
    CodeEditor *editor = getCurrentEditor();
    if (!editor)
    {
        // qDebug() << "[onRun] No editor";
        QMessageBox::warning(this, "Warning", "No file is open!");
        return;
    }

    if (!vm)
    {
        // qDebug() << "[onRun] VM is null!";
        QMessageBox::critical(this, "Error", "VM not initialized!");
        return;
    }

    if (program.errorCount != 0)
    {
        // qDebug() << "[onRun] Assembly errors:" << program.errorCount;
        QMessageBox::warning(this, "Assembly Error", "Please fix assembly errors before running.");
        return;
    }

    try
    {
        // qDebug() << "[onRun] Blocking VM signals";
        vm->blockSignals(true);
        editor->clearPipelineLabels();

        // qDebug() << "[onRun] clearLineHighlight";
        clearLineHighlight();

        // qDebug() << "[onRun] vm->Reset()";
        vm->Reset();

        // qDebug() << "[onRun] vm->LoadProgram()";.
        vm->LoadProgram(program);

        // ✅ Load actual memory from VM
        DataSegment *dataSegment = bottomPanel->getDataSegment();
        // After assemble & LoadProgram:
        uint64_t base = 0x10000000;
        size_t bytes = 512; // set as needed
        std::vector<uint8_t> mem = vm->GetMemoryRange(base, bytes);

        QVector<quint64> memoryWords;
        for (size_t i = 0; i < mem.size(); i += 4)
        {
            uint32_t word = 0;
            for (size_t j = 0; j < 4 && (i + j) < mem.size(); j++)
            {
                word |= (static_cast<uint32_t>(mem[i + j]) << (j * 8));
            }
            memoryWords.append(word);
        }
        dataSegment->clearData();
        dataSegment->setMemory(memoryWords, base);

        updateExecutionInfo();
        // qDebug() << "[onRun] Unblocking VM signals";
        vm->blockSignals(false);

        int speed = executionSpeedSlider->value();
        // qDebug() << "[onRun] Speed =" << speed;

        if (speed > 30)
        {
            // Fast mode
            // qDebug() << "[onRun] Fast mode";
            // qDebug() << "[onRun] Calling vm->Run()";
            vm->Run();
            // qDebug() << "[onRun] vm->Run() completed";

            // qDebug() << "[onRun] updateRegisterTable";
            updateRegisterTable();

            // qDebug() << "[onRun] highlightCurrentLine";
            highlightCurrentLine();


            updateExecutionInfo();

            // ✅ Refresh memory display after execution
            refreshMemoryDisplay();

            QString msg = QString("Execution finished!\nInstructions: %1\nCycles: %2")
                              .arg(vm->instructions_retired_)
                              .arg(vm->cycle_s_);
            QMessageBox::information(this, "Complete", msg);
            // qDebug() << "[onRun] DONE (fast mode)";
        }
        else
        {
            // Animated mode
            // qDebug() << "[onRun] Animated mode";
            int delayMs = 1000 / speed;

            QTimer *stepTimer = new QTimer(this);
            connect(stepTimer, &QTimer::timeout, this, [this, stepTimer]()
                    {
                        // if (vm->GetProgramCounter() >= vm->GetProgramSize() || vm->IsStopRequested()) {
                        //     stepTimer->stop();
                        //     stepTimer->deleteLater();

                        //     updateExecutionInfo();
                        //     refreshMemoryDisplay();  // ✅ Refresh at end

                        //     QString msg = QString("Execution finished!\nInstructions: %1\nCycles: %2")
                        //                       .arg(vm->instructions_retired_)
                        //                       .arg(vm->cycle_s_);
                        //     QMessageBox::information(this, "Complete", msg);
                        //     return;
                        // }

                        if ((vm->GetProgramCounter() >= vm->GetProgramSize() && vm->IsPipelineEmpty()) || vm->IsStopRequested()) {
                            stepTimer->stop();
                            stepTimer->deleteLater();

                            updateExecutionInfo();
                            refreshMemoryDisplay();  // ✅ Refresh at end

                            QString msg = QString("Execution finished!\nInstructions: %1\nCycles: %2")
                                              .arg(vm->instructions_retired_)
                                              .arg(vm->cycle_s_);
                            QMessageBox::information(this, "Complete", msg);
                            return;
                        }

                        vm->Step();
                        updateRegisterTable();
                        highlightCurrentLine();
                        updateExecutionInfo(); });

            stepTimer->start(delayMs);
            // qDebug() << "[onRun] DONE (animated mode started)";
        }
    }
    catch (const std::exception &ex)
    {
        // qDebug() << "[onRun] EXCEPTION:" << ex.what();
        vm->blockSignals(false);
        QMessageBox::critical(this, "Error", ex.what());
        updateRegisterTable();
    }
}

void MainWindow::onStep()
{
    // qDebug() << "[onStep] START";
    if (!vm)
        return;

    vm->Step();
    updateRegisterTable();
    highlightCurrentLine();
    updateExecutionInfo();
    // qDebug() << "[onStep] DONE";
}

void MainWindow::onPause()
{
    // qDebug() << "[onPause]";
    if (vm)
    {
        vm->RequestStop();
    }
}

void MainWindow::onStop()
{
    // qDebug() << "[onStop] START";
    if (vm)
    {
        vm->RequestStop();
        vm->Reset();
        updateRegisterTable();
        clearLineHighlight();
    }
    // qDebug() << "[onStop] DONE";
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
    // qDebug() << "[updateRegisterTable] START";
    try
    {
        if (!registerPanel)
        {
            // qDebug() << "[updateRegisterTable] ERROR: registerPanel is null!";
            return;
        }

        RegisterFile *registers = registerPanel->getRegisterFile();
        if (!registers)
        {
            // qDebug() << "[updateRegisterTable] ERROR: registers is null!";
            return;
        }

        RegisterTable *reg_table = registerPanel->getRegTable();
        if (!reg_table)
        {
            // qDebug() << "[updateRegisterTable] ERROR: reg_table is null!";
            return;
        }

        uint64_t pc_value = vm ? vm->GetProgramCounter() : 0;
        // qDebug() << "[updateRegisterTable] PC =" << pc_value;

        QVector<quint64> reg_values;

        // First add all 32 GPRs
        for (uint64_t v : registers->GetGprValues())
            reg_values << static_cast<quint64>(v);

        // Then add PC at the end
        reg_values << static_cast<quint64>(pc_value);

        // qDebug() << "[updateRegisterTable] reg_values.size() =" << reg_values.size();
        // qDebug() << "[updateRegisterTable] reg_table->rowCount() =" << reg_table->rowCount();

        reg_table->updateAllRegisters(reg_values);
        // qDebug() << "[updateRegisterTable] DONE";
    }
    catch (const std::exception &e)
    {
        // qDebug() << "[updateRegisterTable] EXCEPTION:" << e.what();
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
    ProcessorWindow dlg(this);
    dlg.setInitialSelection(lastName, lastISA); // Restore last selection!
    if (dlg.exec() == QDialog::Accepted)
    {
        lastName = dlg.selectedProcessorName();
        lastISA = dlg.selectedISA();
        // Use/save these values as needed
        processorInfoLabel->setText(QString("Processor: %1 | ISA: %2").arg(lastName, lastISA));
        ISA selected = (lastISA == "RV32") ? ISA::RV32 : ISA::RV64;

        if (lastName == "Single-cycle processor") {
            vm = new RVSSVM(registerPanel->getRegisterFile(), this); // Standard VM
        } else if (lastName == "Pipelined processor") {
            vm = new RVSSVMPipelined(registerPanel->getRegisterFile(), this); // Use pipelined backend
        }
        vm->registers_->SetIsa(selected);
        vm->Reset(); // Optionally reset VM state to match ISA
        auto *pipelinedVm = qobject_cast<RVSSVMPipelined *>(vm);
        if (pipelinedVm) {
            qDebug() << "Pipelined VM connected!";
            connect(pipelinedVm, &RVSSVMPipelined::pipelineStageChanged,
                    this, &MainWindow::onPipelineStageChanged);
        } else {
            qDebug() << "Not pipelined.";
        }
    }
}

void MainWindow::onPipelineStageChanged(uint64_t pc, QString stage)
{
    CodeEditor *editor = getCurrentEditor();
    if (!editor)
        return;

    // CLEAR stage: remove highlight + label
    if (stage.endsWith("_CLEAR"))
    {
        QString baseStage = stage.left(stage.indexOf("_CLEAR"));
        editor->setPipelineLabel(0, stage); // remove label text
        return;
    }

    // Convert PC -> instruction index
    unsigned int instrIndex = static_cast<unsigned int>(pc / 4);

    // Convert instruction index -> source line number
    if (program.instruction_number_line_number_mapping.find(instrIndex) ==
        program.instruction_number_line_number_mapping.end())
        return;

    unsigned int sourceLine = program.instruction_number_line_number_mapping[instrIndex]; // 1-based

    // Highlight that line and show the stage label
    editor->setPipelineLabel(pc, stage);
}

