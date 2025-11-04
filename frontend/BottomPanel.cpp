// #include "bottompanel.h"

// BottomPanel::BottomPanel(QWidget *parent) : QWidget(parent)
// {
//     QVBoxLayout *layout = new QVBoxLayout(this);
//     layout->setContentsMargins(0,0,0,0);

//     console = new ErrorConsole(this);
//     dataSegment = new DataSegment(this);

//     // Show console by default
//     console->setVisible(true);
//     dataSegment->setVisible(false);

//     layout->addWidget(console);
//     layout->addWidget(dataSegment);
// }

// void BottomPanel::togglePanel(bool showData)
// {
//     console->setVisible(!showData);
//     dataSegment->setVisible(showData);
// }

// ErrorConsole* BottomPanel::getConsole() { return console; }
// DataSegment* BottomPanel::getDataSegment() { return dataSegment; }


#include "BottomPanel.h"

BottomPanel::BottomPanel(QWidget *parent) : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(2);

    // --- Buttons at top of panel ---
    buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(0);

    consoleButton = new QPushButton("Console", this);
    dataButton = new QPushButton("Data Segment", this);

    consoleButton->setCheckable(true);
    dataButton->setCheckable(true);
    consoleButton->setChecked(true); // console default

    // Button styling
    auto buttonStyle = "QPushButton { background-color: #3a3d41; color: white; border: none; padding: 6px; }"
                       "QPushButton:checked { background-color: #505356; }";
    consoleButton->setStyleSheet(buttonStyle);
    dataButton->setStyleSheet(buttonStyle);

    buttonLayout->addWidget(consoleButton);
    buttonLayout->addWidget(dataButton);

    mainLayout->addLayout(buttonLayout);

    // --- Panels ---
    console = new ErrorConsole(this);
    dataSegment = new DataSegment(this);

    console->setVisible(true);
    dataSegment->setVisible(false);

    mainLayout->addWidget(console);
    mainLayout->addWidget(dataSegment);

    // --- Connect buttons ---
    connect(consoleButton, &QPushButton::clicked, this, &BottomPanel::handleConsoleClicked);
    connect(dataButton, &QPushButton::clicked, this, &BottomPanel::handleDataClicked);
}

void BottomPanel::handleConsoleClicked()
{
    consoleButton->setChecked(true);
    dataButton->setChecked(false);
    console->setVisible(true);
    dataSegment->setVisible(false);
}

void BottomPanel::handleDataClicked()
{
    consoleButton->setChecked(false);
    dataButton->setChecked(true);
    console->setVisible(false);
    dataSegment->setVisible(true);
}

void BottomPanel::togglePanel(bool showData)
{
    if(showData) handleDataClicked();
    else handleConsoleClicked();
}

void BottomPanel::changeTab(){
    handleConsoleClicked();
}
