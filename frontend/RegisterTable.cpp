#include "registertable.h"
#include <QHeaderView>
#include <QFontMetrics>
#include <QTimer>
#include <cstring>
// #include <QDebug>

RegisterTable::RegisterTable(QWidget *parent)
    : QTableWidget(parent), displayType(DisplayType::Hex),
      evenBrush(QColor(30, 30, 30)), oddBrush(QColor(40, 40, 40)),
      highlightBrush(QColor(64, 156, 255, 80))
{
    // qDebug() << "[RegisterTable] Constructor";
    setColumnCount(3);
    setHorizontalHeaderLabels({"Reg Name", "Number", "Value"});

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);

    int charWidth = fontMetrics().horizontalAdvance("0x0000000000000000") + fontMetrics().horizontalAdvance("000");
    horizontalHeader()->resizeSection(2, charWidth);

    verticalHeader()->setVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    verticalHeader()->setDefaultSectionSize(20);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setStyleSheet(
        "QTableWidget { background-color: #1e1e1e; color: #dcdcdc; border: none; }"
        "QHeaderView::section { background-color: #252526; color: white; padding: 4px; }"
        "QTableWidget::item:selected { background: #3a3d41; }"
        "QTableWidget::item { padding: 2px; }");
}

void RegisterTable::initialize(const QStringList &regNames)
{
    // qDebug() << "[RegisterTable::initialize] regNames.size() =" << regNames.size();
    int count = regNames.size() + 1;
    setRowCount(count);
    registerValues.fill(0, count);
    // qDebug() << "[RegisterTable::initialize] rowCount() =" << rowCount();

    // Add all GPRs
    for (int i = 0; i < regNames.size(); ++i)
    {
        QString reg = regNames[i];
        setItem(i, 0, new QTableWidgetItem(reg.section(":", 0, 0)));
        setItem(i, 1, new QTableWidgetItem(reg.section(":", 1, 1)));
        setItem(i, 2, new QTableWidgetItem(" 0x 0000 0000 0000 0000"));
        applyAlternatingRowColor(i);

        item(i, 0)->setTextAlignment(Qt::AlignCenter);
        item(i, 1)->setTextAlignment(Qt::AlignCenter);
        item(i, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }

    // Add PC at bottom
    int pcRow = regNames.size();
    setItem(pcRow, 0, new QTableWidgetItem("PC"));
    setItem(pcRow, 1, new QTableWidgetItem("-"));
    setItem(pcRow, 2, new QTableWidgetItem(" 0x 0000 0000 0040 0000"));
    applyAlternatingRowColor(pcRow);

    item(pcRow, 0)->setTextAlignment(Qt::AlignCenter);
    item(pcRow, 1)->setTextAlignment(Qt::AlignCenter);
    item(pcRow, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // qDebug() << "[RegisterTable::initialize] DONE";
}

QWidget *RegisterTable::createDisplayTypeSelector()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    QLabel *label = new QLabel("Display Type:");
    label->setStyleSheet("color: #dcdcdc;");

    QComboBox *combo = new QComboBox();
    combo->addItem("Hex");
    combo->addItem("Unsigned");
    combo->addItem("Signed");
    combo->addItem("Float");
    combo->addItem("Double");

    combo->setCurrentIndex(static_cast<int>(displayType));
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, combo](int index)
            {
        auto type = static_cast<DisplayType>(index);
        setDisplayType(type);
        emit displayTypeChanged(type); });

    layout->addWidget(label);
    layout->addWidget(combo);
    layout->addStretch();

    return container;
}

void RegisterTable::setDisplayType(DisplayType type)
{
    displayType = type;
    updateAllDisplayValues();
}

void RegisterTable::updateRegister(int index, quint64 value)
{
    // qDebug() << "[RegisterTable::updateRegister] index=" << index << "value=" << value << "rows=" << rowCount();

    if (index < 0 || index >= registerValues.size())
    {
        // qDebug() << "[RegisterTable::updateRegister] ERROR: Out of bounds! index=" << index << "size=" << registerValues.size();
        return;
    }

    bool isPC = (index == registerValues.size() - 1);
    quint64 oldValue = registerValues[index];
    registerValues[index] = value;

    auto *valueItem = item(index, 2);
    if (valueItem)
    {
        // valueItem->setText(formatValue(value));
        QString formatted = formatValue(value);
        // qDebug() << "[updateRegister] formatValue=" << formatted;
        valueItem->setText(formatted);
    }
    else
    {
        // qDebug() << "[RegisterTable::updateRegister] ERROR: item at row" << index << "is null!";
    }

    if (!isPC && value != oldValue)
    {
        for (int j = 0; j < 3; ++j)
        {
            if (item(index, j))
            {
                item(index, j)->setBackground(highlightBrush);
            }
        }

        QTimer::singleShot(500, this, [this, index]()
                           { applyAlternatingRowColor(index); });
    }

    // qDebug() << "[RegisterTable::updateRegister] DONE";
}

void RegisterTable::updateAllRegisters(const QVector<quint64> &values)
{
    // qDebug() << "[RegisterTable::updateAllRegisters] values.size()=" << values.size() << "registerValues.size()=" << registerValues.size();

    if (values.size() != registerValues.size())
    {
        // qDebug() << "[RegisterTable::updateAllRegisters] ERROR: Size mismatch!";
        return;
    }

    blockSignals(true);
    setUpdatesEnabled(false);

    for (int i = 0; i < values.size(); ++i)
        updateRegister(i, values[i]);

    blockSignals(false);
    setUpdatesEnabled(true);
    viewport()->update();

    // qDebug() << "[RegisterTable::updateAllRegisters] DONE";
}

void RegisterTable::updateAllDisplayValues()
{
    setUpdatesEnabled(false);
    for (int i = 0; i < registerValues.size(); ++i)
    {
        auto *valueItem = item(i, 2);
        if (valueItem)
            valueItem->setText(formatValue(registerValues[i]));
    }
    setUpdatesEnabled(true);
    viewport()->update();
}

QString RegisterTable::formatValue(quint64 value) const
{
    switch (displayType)
    {
    case DisplayType::Hex:
    {
        QString hex = QString("%1").arg(static_cast<quint64>(value), 16, 16, QLatin1Char('0'));
        QString grouped;
        // qDebug() << "[formatValue] HEX=" << hex;
        for (int i = 0; i < hex.length(); ++i)
        {
            if (i > 0 && i % 4 == 0)
                grouped += " ";
            grouped += hex[i];
        }
        return "  0x " + grouped.toUpper();
    }
    case DisplayType::Unsigned:
        return QString(" %1").arg(static_cast<quint64>(value));
    case DisplayType::Signed:
    {
        qint64 signedValue = static_cast<qint64>(static_cast<qint64>(value));
        return QString(" %1").arg(signedValue);
    }
    case DisplayType::Float:
    {
        float floatValue;
        std::memcpy(&floatValue, &value, sizeof(float));
        return QString(" %1").arg(floatValue, 0, 'f', 6);
    }
    case DisplayType::Double:
    {
        float floatValue;
        std::memcpy(&floatValue, &value, sizeof(float));
        return QString(" %1").arg(static_cast<double>(floatValue), 0, 'f', 6);
    }
    default:
        return QString(" 0x%1").arg(static_cast<quint64>(value), 16, 16, QLatin1Char('0'));
    }
}

void RegisterTable::applyAlternatingRowColor(int index)
{
    QBrush bgBrush = (index % 2 == 0) ? evenBrush : oddBrush;
    for (int j = 0; j < 3; ++j)
    {
        if (item(index, j))
        {
            item(index, j)->setBackground(bgBrush);
        }
    }
}
