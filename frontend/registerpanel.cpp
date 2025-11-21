#include "registerpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>

RegisterPanel::RegisterPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    tabs = new QTabWidget(this);

    // ========== INTEGER TAB WITH CONTROL PANEL ==========
    QWidget *intTab = new QWidget();
    QVBoxLayout *intLayout = new QVBoxLayout(intTab);
    intLayout->setContentsMargins(0, 0, 0, 0);
    intLayout->setSpacing(5);

    // Integer register table
    intRegs = new RegisterTable(this);
    intRegs->setRegisterType(RegisterTable::RegisterType::Integer);

    QStringList intNames;
    QStringList names = {"zero","ra","sp","gp","tp","t0","t1","t2","s0","s1",
                         "a0","a1","a2","a3","a4","a5","a6","a7",
                         "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
                         "t3","t4","t5","t6"};
    for (int i = 0; i < 32; ++i)
        intNames << QString("x%1:%2").arg(i).arg(names[i]);
    intRegs->initialize(intNames);

    intRegs->setColumnWidth(0, 100);
    intRegs->setColumnWidth(1, 100);
    intRegs->setColumnWidth(2, 160);
    intRegs->verticalHeader()->setDefaultSectionSize(22);

    // Integer control panel
    QWidget *intControlPanel = new QWidget();
    QHBoxLayout *intControlLayout = new QHBoxLayout(intControlPanel);
    intControlLayout->setContentsMargins(8, 5, 8, 5);
    intControlLayout->setSpacing(10);

    QLabel *intLabel = new QLabel("Display Type:");
    // intLabel->setStyleSheet("color: #dcdcdc; font-weight: bold;");
    intLabel->setStyleSheet("color: #dcdcdc;");

    intDisplayTypeCombo = new QComboBox();
    intDisplayTypeCombo->addItem("Hex");
    intDisplayTypeCombo->addItem("Unsigned");
    intDisplayTypeCombo->addItem("Signed");

    intDisplayTypeCombo->setStyleSheet(
        "QComboBox { background-color: #252526; color: white; border: 1px solid #3a3d41; padding: 4px; min-width: 120px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { background-color: #252526; color: white; selection-background-color: #3a3d41; }"
        );

    intControlLayout->addWidget(intLabel);
    intControlLayout->addWidget(intDisplayTypeCombo);
    intControlLayout->addStretch();
    intControlPanel->setStyleSheet("background-color: #2d2d30; border-top: 1px solid #3a3d41;");

    intLayout->addWidget(intRegs);
    intLayout->addWidget(intControlPanel);

    // ========== FLOAT TAB WITH CONTROL PANEL ==========
    QWidget *floatTab = new QWidget();
    QVBoxLayout *floatLayout = new QVBoxLayout(floatTab);
    floatLayout->setContentsMargins(0, 0, 0, 0);
    floatLayout->setSpacing(5);

    // Float register table
    floatRegs = new RegisterTable(this);
    floatRegs->setRegisterType(RegisterTable::RegisterType::FloatingPoint);

    QStringList floatNames;
    for (int i = 0; i < 32; ++i) {
        QString name;
        if (i >= 0 && i <= 7)
            name = QString("ft%1").arg(i);
        else if (i >= 8 && i <= 9)
            name = QString("fs%1").arg(i - 8);
        else if (i >= 10 && i <= 17)
            name = QString("fa%1").arg(i - 10);
        else if (i >= 18 && i <= 27)
            name = QString("fs%1").arg(i - 16);
        else if (i >= 28 && i <= 31)
            name = QString("ft%1").arg(i - 20);

        floatNames << QString("%1:%2").arg(name).arg(i);
    }
    floatRegs->initialize(floatNames);

    floatRegs->setColumnWidth(0, 100);
    floatRegs->setColumnWidth(1, 100);
    floatRegs->setColumnWidth(2, 160);
    floatRegs->verticalHeader()->setDefaultSectionSize(22);

    // Float control panel
    QWidget *floatControlPanel = new QWidget();
    QHBoxLayout *floatControlLayout = new QHBoxLayout(floatControlPanel);
    floatControlLayout->setContentsMargins(8, 5, 8, 5);
    floatControlLayout->setSpacing(10);

    QLabel *floatLabel = new QLabel("Display Type:");
    // floatLabel->setStyleSheet("color: #dcdcdc; font-weight: bold;");

    floatDisplayTypeCombo = new QComboBox();
    floatDisplayTypeCombo->addItem("Hex");
    floatDisplayTypeCombo->addItem("Float (Single Precision)");
    floatDisplayTypeCombo->addItem("Double (Double Precision)");

    floatDisplayTypeCombo->setStyleSheet(
        "QComboBox { color: white; border: 1px solid #3a3d41; padding: 4px; min-width: 180px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { background-color: #252526; color: white; selection-background-color: #3a3d41; }"
        );

    floatControlLayout->addWidget(floatLabel);
    floatControlLayout->addWidget(floatDisplayTypeCombo);
    floatControlLayout->addStretch();
    floatControlPanel->setStyleSheet("background-color: #2d2d30; border-top: 1px solid #3a3d41;");

    floatLayout->addWidget(floatRegs);
    floatLayout->addWidget(floatControlPanel);

    // ========== CSR TAB ==========
    csrTable = new ControlStatusTable(this);
    csrTable->initialize();
    csrTable->verticalHeader()->setDefaultSectionSize(22);

    // ========== ADD TABS ==========
    tabs->addTab(intTab, "Integer");
    tabs->addTab(floatTab, "Float");
    tabs->addTab(csrTable, "Control/Status");

    mainLayout->addWidget(tabs);

    // ========== CONNECT SIGNALS ==========
    // Connect integer combo
    connect(intDisplayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                RegisterTable::DisplayType type = static_cast<RegisterTable::DisplayType>(index);
                intRegs->setDisplayType(type);
            });

    // Connect float combo
    connect(floatDisplayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                RegisterTable::DisplayType type;
                if (index == 0) type = RegisterTable::DisplayType::Hex;
                else if (index == 1) type = RegisterTable::DisplayType::Float;
                else type = RegisterTable::DisplayType::Double;

                floatRegs->setDisplayType(type);
            });

    backendRegisterFile = new RegisterFile();
}

RegisterFile* RegisterPanel::getRegisterFile() {
    return backendRegisterFile;
}

void RegisterPanel::resetAllTables()
{
    if (intRegs) {
        intRegs->reset();
    }
    if (floatRegs) {
        floatRegs->reset();
    }
    if (csrTable) {
        csrTable->reset();  // You'll need to add this to ControlStatusTable too
    }
}

