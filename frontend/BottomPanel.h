// #pragma once
// #include <QWidget>
// #include <QPushButton>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include "ErrorConsole.h"
// #include "DataSegment.h"
// #include "memoryviewer.h"

// class BottomPanel : public QWidget
// {
//     Q_OBJECT
// public:
//     explicit BottomPanel(QWidget *parent = nullptr);

//     void togglePanel(bool showData); // programmatic toggle
//     ErrorConsole* getConsole();
//     DataSegment* getDataSegment();
//     MemoryViewer* getMemoryViewer() const { return memoryViewer; }

// private:
//     QVBoxLayout *mainLayout;
//     QHBoxLayout *buttonLayout;
//     QPushButton *consoleButton;
//     QPushButton *dataButton;

//     ErrorConsole *console;
//     DataSegment *dataSegment;
//     MemoryViewer *memoryViewer;

// private slots:
//     void handleConsoleClicked();
//     void handleDataClicked();
// };

#ifndef BOTTOMPANEL_H
#define BOTTOMPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include "ErrorConsole.h"
#include "DataSegment.h"

class BottomPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BottomPanel(QWidget *parent = nullptr);

    ErrorConsole *getConsole() { return console; }
    DataSegment *getDataSegment() { return dataSegment; }

    void togglePanel(bool showData);
    void changeTab();

private slots:
    void handleConsoleClicked();
    void handleDataClicked();

private:
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonLayout;

    QPushButton *consoleButton;
    QPushButton *dataButton;

    ErrorConsole *console;
    DataSegment *dataSegment;
};

#endif // BOTTOMPANEL_H
