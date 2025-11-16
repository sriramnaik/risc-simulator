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

    enum class RegisterType
    {
        Integer,        // GPR registers
        FloatingPoint   // FPR registers
    };

    explicit RegisterTable(QWidget *parent = nullptr);

    void initialize(const QStringList &regNames);
    QWidget *createDisplayTypeSelector();
    void setDisplayType(DisplayType type);
    void setRegisterType(RegisterType type);  // Set whether this is GPR or FPR

public slots:
    void updateRegister(int index, quint64 value);
    void updateAllRegisters(const QVector<quint64> &values);

signals:
    void displayTypeChanged(DisplayType type);

private:
    QString formatValue(quint64 value) const;
    void updateAllDisplayValues();
    void applyAlternatingRowColor(int index);

    // ✅ PRIVATE member variables
    QVector<quint64> registerValues;  // Changed from quint32 to quint64
    DisplayType displayType;
    RegisterType registerType;  // ✅ MOVED TO PRIVATE

    // Helper brushes for row colors
    QBrush evenBrush;
    QBrush oddBrush;
    QBrush highlightBrush;
};

#endif // REGISTERTABLE_H
