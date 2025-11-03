#include "registerpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>

RegisterPanel::RegisterPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    tabs = new QTabWidget(this);

    // Integer registers
    intRegs = new RegisterTable(this);
    QStringList intNames;
    QStringList names = {"zero","ra","sp","gp","tp","t0","t1","t2","s0","s1",
                         "a0","a1","a2","a3","a4","a5","a6","a7",
                         "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
                         "t3","t4","t5","t6"};
    for (int i = 0; i < 32; ++i)
        intNames << QString("x%1:%2").arg(i).arg(names[i]);
    intRegs->initialize(intNames);

    // Set column widths for integer registers
    intRegs->setColumnWidth(0, 100);  // Reg column (smaller)
    intRegs->setColumnWidth(1, 100); // Name column
    intRegs->setColumnWidth(2, 160); // Value column (larger for 18 chars)

    // Adjust row height to fit more rows
    intRegs->verticalHeader()->setDefaultSectionSize(22);
    // registerpanel.cpp (constructor)
    // backendRegisterFile = new RegisterFile(); // or get from VM if you share one!

    // Floating registers
    floatRegs = new RegisterTable(this);
    QStringList floatNames;
    for (int i = 0; i < 32; ++i) {
        QString name;

        if (i >= 0 && i <= 7)
            name = QString("ft%1").arg(i);           // ft0–ft7
        else if (i >= 8 && i <= 9)
            name = QString("fs%1").arg(i - 8);       // fs0–fs1
        else if (i >= 10 && i <= 17)
            name = QString("fa%1").arg(i - 10);      // fa0–fa7
        else if (i >= 18 && i <= 27)
            name = QString("fs%1").arg(i - 16);      // fs2–fs11
        else if (i >= 28 && i <= 31)
            name = QString("ft%1").arg(i - 20);      // ft8–ft11

        floatNames << QString("%1: %2").arg(name).arg(i);
    }

    floatRegs->initialize(floatNames);

    // Set column widths for float registers
    floatRegs->setColumnWidth(0, 100);  // Reg column (smaller)
    floatRegs->setColumnWidth(1, 100); // Name column
    floatRegs->setColumnWidth(2, 160); // Value column (larger for 18 chars)

    // Adjust row height to fit more rows
    floatRegs->verticalHeader()->setDefaultSectionSize(22);

    // CSR Table
    csrTable = new ControlStatusTable(this);
    csrTable->initialize();

    // Adjust CSR table row height
    csrTable->verticalHeader()->setDefaultSectionSize(22);

    tabs->addTab(intRegs, "Integer");
    tabs->addTab(floatRegs, "Float");
    tabs->addTab(csrTable, "Control/Status");

    layout->addWidget(tabs);

    // Create display type selector below the table
    QWidget *controlPanel = new QWidget(this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlPanel);
    controlLayout->setContentsMargins(5, 5, 5, 5);
    controlLayout->setSpacing(10);

    QLabel *label = new QLabel("Display Type:");
    label->setStyleSheet("color: #dcdcdc; font-weight: bold;");

    displayTypeCombo = new QComboBox();
    displayTypeCombo->addItem("Hex");
    displayTypeCombo->addItem("Unsigned");
    displayTypeCombo->addItem("Signed");
    displayTypeCombo->addItem("Float");
    displayTypeCombo->addItem("Double");
    displayTypeCombo->setStyleSheet(
        "QComboBox { background-color: #252526; color: white; border: 1px solid #3a3d41; padding: 4px; min-width: 100px; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { image: none; border: none; }"
        "QComboBox QAbstractItemView { background-color: #252526; color: white; selection-background-color: #3a3d41; }"
        );

    controlLayout->addWidget(label);
    controlLayout->addWidget(displayTypeCombo);
    controlLayout->addStretch();

    controlPanel->setStyleSheet("background-color: #1e1e1e;");

    layout->addWidget(controlPanel);

    // Connect display type combo to update both register tables
    connect(displayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RegisterPanel::onDisplayTypeChanged);
    backendRegisterFile = new RegisterFile();
}

void RegisterPanel::onDisplayTypeChanged(int index)
{
    RegisterTable::DisplayType type = static_cast<RegisterTable::DisplayType>(index);
    intRegs->setDisplayType(type);
    floatRegs->setDisplayType(type);
}

RegisterFile* RegisterPanel::getRegisterFile() {
    return backendRegisterFile;
}


