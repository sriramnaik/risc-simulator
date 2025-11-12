#include "processorwindow.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

ProcessorWindow::ProcessorWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Select Processor");
    resize(340, 160);

    isaCombo   = new QComboBox(this);
    stageCombo = new QComboBox(this);

    okButton     = new QPushButton("OK", this);
    cancelButton = new QPushButton("Cancel", this);

    // Populate ISA options
    isaCombo->addItems({"RV32", "RV64"});

    // Populate stage options for RV32 by default
    stageCombo->addItems({
        "5-stage processor w/o forwarding or hazard detection",
        "5-stage processor with forwarding and hazard detection",
        "5-stage processor w/o hazard detection",
        "5-stage processor w/o forwarding unit",
        "5-stage processor with static Branch prediction",
        "Single-cycle processor"
    });

    // Labels for summary (plain text values)
    isaLabel   = new QLabel(this);
    stageLabel = new QLabel(this);

    // --- Form layout for the top controls ---
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("ISA:", isaCombo);
    formLayout->addRow("Stage:", stageCombo);

    // --- Summary: each row is horizontal (label + value) ---
    QVBoxLayout *summaryLayout = new QVBoxLayout;

    // Row: Selected ISA: <value>
    QHBoxLayout *isaRow = new QHBoxLayout;
    QLabel *isaText = new QLabel("Selected ISA:", this);
    isaRow->addWidget(isaText);
    isaRow->addWidget(isaLabel);
    isaRow->addStretch(1);               // push items to left, keep them compact
    summaryLayout->addLayout(isaRow);

    // Row: Selected Processor: <value>
    QHBoxLayout *procRow = new QHBoxLayout;
    QLabel *procText = new QLabel("Selected Processor:", this);
    procRow->addWidget(procText);
    procRow->addWidget(stageLabel);
    procRow->addStretch(1);
    summaryLayout->addLayout(procRow);

    // --- OK / Cancel layout ---
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    // --- Main layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(8);
    mainLayout->addLayout(summaryLayout);
    mainLayout->addStretch(1);
    mainLayout->addLayout(buttonLayout);

    // --- Connections ---
    connect(isaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProcessorWindow::onISAChanged);
    connect(stageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProcessorWindow::onStageChanged);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Initialize summary labels to current selections
    onISAChanged(isaCombo->currentIndex());
    onStageChanged(stageCombo->currentIndex());
}


void ProcessorWindow::onISAChanged(int index)
{
    QString isa = isaCombo->itemText(index);
    m_isa = isa;
    isaLabel->setText(isa);

    // Change stage options based on ISA
    stageCombo->clear();
    if (isa == "RV32") {
        stageCombo->addItems({"5-stage processor w/o forwarding or hazard detection",
                              "5-stage processor with forwarding and hazard detection",
                              "5-stage processor w/o hazard detection",
                              "5-stage processor w/o forwarding unit",
                              "5-stage processor with static Branch prediction",
                              "Single-cycle processor"
                              });
    } else {
        stageCombo->addItems({"5-stage processor w/o forwarding or hazard detection",
                              "5-stage processor with forwarding and hazard detection",
                              "5-stage processor w/o hazard detection",
                              "5-stage processor w/o forwarding unit",
                              "5-stage processor with static Branch prediction",
                              "Single-cycle processor"
                              });
    }
    // Update summary for stage
    onStageChanged(stageCombo->currentIndex());
}

void ProcessorWindow::onStageChanged(int index)
{
    QString stage = stageCombo->itemText(index);
    m_stage = stage;
    stageLabel->setText(stage);
}

void ProcessorWindow::setInitialSelection(const QString& stage, const QString& isa)
{
    // Set ISA
    int isaIndex = isaCombo->findText(isa);
    if (isaIndex >= 0)
        isaCombo->setCurrentIndex(isaIndex);
    else
        isaCombo->setCurrentIndex(0);

    // Set Stage
    int stageIndex = stageCombo->findText(stage);
    if (stageIndex >= 0)
        stageCombo->setCurrentIndex(stageIndex);
    else
        stageCombo->setCurrentIndex(0);

    // Labels will auto-update due to connected slots
}


QString ProcessorWindow::selectedProcessorName() const { return m_stage; }
QString ProcessorWindow::selectedISA() const { return m_isa; }

