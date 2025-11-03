#ifndef REGISTERPANEL_H
#define REGISTERPANEL_H

#include <QWidget>
#include <QTabWidget>
#include <QComboBox>
#include "registertable.h"
#include "controlstatustable.h"
#include "../../backend/vm/registers.h"

class RegisterPanel : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterPanel(QWidget *parent = nullptr);

    RegisterTable *getRegTable() const { return intRegs; }
    RegisterTable *getFprTable() const { return floatRegs; }
    ControlStatusTable *getCsrTable() const { return csrTable; }
    RegisterFile *getRegisterFile();
    QTabWidget *getTabWidget() const { return tabs; }
    void setregisterBitWidth(int val) { registerBitWidth = val ;}

private slots:
    void onDisplayTypeChanged(int index);

private:
    QTabWidget *tabs;
    RegisterTable *intRegs;
    RegisterTable *floatRegs;
    ControlStatusTable *csrTable;
    QComboBox *displayTypeCombo;
    RegisterFile *backendRegisterFile;
    int registerBitWidth = 64;

};

#endif // REGISTERPANEL_H
