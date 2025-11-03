#include "controlstatustable.h"
#include <QHeaderView>
#include <QBrush>
#include <QTimer>

ControlStatusTable::ControlStatusTable(QWidget *parent)
    : QTableWidget(parent)
{
    setColumnCount(2);
    setHorizontalHeaderLabels({"CSR", "Value"});
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setStyleSheet(
        "QTableWidget { background-color: #1e1e1e; color: #dcdcdc; border: none; }"
        "QHeaderView::section { background-color: #252526; color: white; }"
        );
}

void ControlStatusTable::initialize()
{
    QStringList csrList = {"pc", "mstatus", "mtvec", "mepc", "mcause"};
    setRowCount(csrList.size());

    for (int i = 0; i < csrList.size(); ++i) {
        setItem(i, 0, new QTableWidgetItem(csrList[i]));
        setItem(i, 1, new QTableWidgetItem("0x00000000"));
        for (int j = 0; j < 2; ++j)
            item(i, j)->setTextAlignment(Qt::AlignCenter);
    }
}

void ControlStatusTable::updateCSR(const QString &name, quint32 value)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (item(i, 0)->text() == name) {
            auto *valItem = item(i, 1);
            valItem->setText(QString("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper());
            valItem->setBackground(QColor(255, 255, 0, 80));
            QTimer::singleShot(300, [valItem]() { valItem->setBackground(Qt::transparent); });
            return;
        }
    }
}
