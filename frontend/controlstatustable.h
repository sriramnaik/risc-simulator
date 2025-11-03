#ifndef CONTROLSTATUSTABLE_H
#define CONTROLSTATUSTABLE_H

#include <QTableWidget>

class ControlStatusTable : public QTableWidget {
    Q_OBJECT
public:
    explicit ControlStatusTable(QWidget *parent = nullptr);
    void initialize();
    void updateCSR(const QString &name, quint32 value);
};
#endif

