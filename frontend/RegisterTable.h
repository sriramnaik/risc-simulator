#ifndef REGISTERTABLE_H
#define REGISTERTABLE_H

#include <QTableWidget>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>

class RegisterTable : public QTableWidget
{
    Q_OBJECT

public:
    enum class DisplayType
    {
        Hex = 0,
        Unsigned,
        Signed,
        Float,
        Double
    };

    explicit RegisterTable(QWidget *parent = nullptr);

    void initialize(const QStringList &regNames);
    QWidget *createDisplayTypeSelector();
    void setDisplayType(DisplayType type);
    // void setBitWidth(int) {bitwidth = val;}

    // Update one register's value (backend connects to this)
public slots:
    void updateRegister(int index, quint64 value);

    // Update all register values at once (bulk refresh)
    void updateAllRegisters(const QVector<quint64> &values);

signals:
    void displayTypeChanged(DisplayType type);

private:
    QString formatValue(quint64 value) const;
    void updateAllDisplayValues();
    void applyAlternatingRowColor(int index);

    QVector<quint32> registerValues;
    DisplayType displayType;

    // Helper brushes for row colors
    QBrush evenBrush;
    QBrush oddBrush;
    QBrush highlightBrush; // Single highlight color


};

#endif // REGISTERTABLE_H
