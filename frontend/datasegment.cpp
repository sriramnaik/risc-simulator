#include "datasegment.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QString>
#include <QDebug>

DataSegment::DataSegment(QWidget *parent)
    : QWidget(parent),
      baseAddress(0x10000000),
      showHex(true),
      doubleWordMode(false),
      currentPage(0),
      itemsPerPage(64),
      currentBaseAddress(0x10000000),
      stackPointer(0)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // Table
    memoryTable = new QTableWidget(this);
    memoryTable->setColumnCount(9); // Address + 8 values
    memoryTable->setHorizontalHeaderLabels({"Address", "+0", "+4", "+8", "+c",
                                            "+10", "+14", "+18", "+1c"});
    memoryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memoryTable->verticalHeader()->setVisible(false);
    memoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoryTable->setSelectionMode(QAbstractItemView::NoSelection);
    memoryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    memoryTable->setStyleSheet(
        "QTableWidget { "
        "    background-color: #1e1e1e; "
        "    color: #e0e0e0; "
        "    gridline-color: #3a3d41; "
        "    border: 1px solid #3a3d41; "
        "    font-family: 'Courier New', monospace; "
        "    font-size: 9pt; "
        "} "
        "QHeaderView::section { "
        "    background-color: #252526; "
        "    color: white; "
        "    border: 1px solid #3a3d41; "
        "    padding: 4px; "
        "    font-weight: bold; "
        "} "
        "QScrollBar:vertical { "
        "    width: 10px; "
        "    background: #2d2d2d; "
        "}");

    mainLayout->addWidget(memoryTable, 1);

    // Controls (bottom bar)
    QHBoxLayout *controls = new QHBoxLayout();
    controls->setSpacing(10);

    prevButton = new QPushButton("<", this);
    nextButton = new QPushButton(">", this);
    prevButton->setFixedWidth(30);
    nextButton->setFixedWidth(30);

    segmentCombo = new QComboBox(this);
    segmentCombo->addItems({"0x10000000 (.data)",
                            "0x7FFFFFF0 (.stack)",
                            "0x20000000 (.heap)"});
    segmentCombo->setStyleSheet(
        "QComboBox { "
        "    background-color: #252526; "
        "    color: white; "
        "    border: 1px solid #3a3d41; "
        "    padding: 4px; "
        "}");

    hexCheckBox = new QCheckBox("Hexadecimal Addresses", this);
    hexCheckBox->setChecked(true);
    hexCheckBox->setStyleSheet("color: #dcdcdc;");

    QCheckBox *hexValuesCheckBox = new QCheckBox("Hexadecimal Values", this);
    hexValuesCheckBox->setChecked(true);
    hexValuesCheckBox->setStyleSheet("color: #dcdcdc;");
    connect(hexValuesCheckBox, &QCheckBox::toggled, this, &DataSegment::updateDisplayMode);

    // Stack Pointer label
    spLabel = new QLabel("SP: 0x00000000", this);
    spLabel->setStyleSheet("color: #00ff00; font-weight: bold; font-size: 9pt;");

    controls->addWidget(prevButton);
    controls->addWidget(nextButton);
    controls->addSpacing(10);
    controls->addWidget(segmentCombo);
    controls->addSpacing(10);
    controls->addWidget(hexCheckBox);
    controls->addWidget(hexValuesCheckBox);
    controls->addStretch();
    controls->addWidget(spLabel);

    mainLayout->addLayout(controls);

    // Connect signals
    connect(prevButton, &QPushButton::clicked, this, &DataSegment::prevPage);
    connect(nextButton, &QPushButton::clicked, this, &DataSegment::nextPage);
    connect(segmentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataSegment::changeSegment);
}

void DataSegment::setMemory(const QVector<quint64> &data, quint64 baseAddr)
{
    memory = data;
    baseAddress = baseAddr;
    currentBaseAddress = baseAddr;
    currentPage = 0;
    populateTable();
}

void DataSegment::clearData()
{
    memoryTable->clearContents();
    memoryTable->setRowCount(0);
    memoryData.clear();
    memory.clear();
}

void DataSegment::updateDisplayMode()
{
    showHex = !showHex;
    populateTable();
}

void DataSegment::updateWordMode()
{
    doubleWordMode = !doubleWordMode;
    populateTable();
}

void DataSegment::changeSegment(int index)
{
    switch (index)
    {
    case 0:
        baseAddress = 0x10000000;
        currentBaseAddress = 0x10000000;
        break; // .data
    case 1:
        baseAddress = 0x7FFFFFF0;
        currentBaseAddress = 0x7FFFFFF0;
        break; // .stack
    case 2:
        baseAddress = 0x20000000;
        currentBaseAddress = 0x20000000;
        break; // .heap
    }
    currentPage = 0;
    populateTable();
}

void DataSegment::prevPage()
{
    if (currentPage > 0)
    {
        currentPage--;
        populateTable();
    }
}

void DataSegment::nextPage()
{
    int maxPage = memory.size() / itemsPerPage;
    if (currentPage < maxPage)
    {
        currentPage++;
        populateTable();
    }
}

void DataSegment::populateTable()
{
    memoryTable->clearContents();

    if (memory.isEmpty())
    {
        memoryTable->setRowCount(0);
        return;
    }

    int bytesPerWord = 4; // 4 bytes per word
    int wordsPerRow = 8;  // 8 values per row (matching column headers)

    int startIndex = currentPage * itemsPerPage;
    int endIndex = qMin(startIndex + itemsPerPage, memory.size());
    int totalWords = endIndex - startIndex;
    int rows = (totalWords + wordsPerRow - 1) / wordsPerRow;
    memoryTable->setRowCount(rows);

    for (int r = 0; r < rows; ++r)
    {
        quint64 addr = baseAddress + (startIndex + r * wordsPerRow) * bytesPerWord;

        // Address column with light blue color
        QTableWidgetItem *addrItem = new QTableWidgetItem(
            QString("0x%1").arg(addr, 8, 16, QLatin1Char('0')).toLower());
        addrItem->setForeground(QBrush(QColor(100, 200, 255)));
        memoryTable->setItem(r, 0, addrItem);

        // Value columns
        for (int c = 0; c < wordsPerRow; ++c)
        {
            int idx = startIndex + r * wordsPerRow + c;
            if (idx < endIndex)
            {
                quint32 val = static_cast<quint32>(memory[idx] & 0xFFFFFFFF);
                QString text = showHex
                                   ? QString("0x%1").arg(val, 8, 16, QLatin1Char('0')).toLower()
                                   : QString::number(val);

                QTableWidgetItem *item = new QTableWidgetItem(text);

                // Color non-zero values differently

                // {
                    item->setForeground(QBrush(QColor(255, 255, 255))); // White
                // }
                // else
                // {
                //     item->setForeground(QBrush(QColor(120, 120, 120))); // Gray
                // }

                memoryTable->setItem(r, c + 1, item);
            }
            else
            {
                memoryTable->setItem(r, c + 1, new QTableWidgetItem(""));
            }
        }
    }

    memoryTable->resizeRowsToContents();
}

void DataSegment::updateMemory(uint64_t address, const QVector<uint8_t> &data)
{
    // Store byte data
    for (int i = 0; i < data.size(); i++)
    {
        memoryData[address + i] = data[i];
    }

    // Convert bytes to words for display
    for (int i = 0; i < data.size(); i += 4)
    {
        uint64_t wordAddr = address + i;
        quint32 word = 0;

        // Assemble word from bytes (little-endian)
        for (int j = 0; j < 4 && (i + j) < data.size(); j++)
        {
            word |= (static_cast<quint32>(data[i + j]) << (j * 8));
        }

        // Calculate word index from address
        int wordIndex = (wordAddr - baseAddress) / 4;

        // Expand memory vector if needed
        if (wordIndex >= memory.size())
        {
            memory.resize(wordIndex + 1);
        }

        if (wordIndex >= 0)
        {
            memory[wordIndex] = word;
        }
    }

    populateTable();
}

void DataSegment::updateStackPointer(uint64_t sp)
{
    if (!spLabel) {
        qDebug() << "ERROR: spLabel is null!";
        return;
    }
    stackPointer = sp;
    spLabel->setText(QString("SP: 0x%1").arg(sp, 8, 16, QLatin1Char('0')).toUpper());
}

void DataSegment::updateTable()
{
    populateTable();
}
  
