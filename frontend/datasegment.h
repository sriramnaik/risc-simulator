#ifndef DATASEGMENT_H
#define DATASEGMENT_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QVector>
#include <QSpinBox>

class DataSegment : public QWidget
{
    Q_OBJECT

public:
    explicit DataSegment(QWidget *parent = nullptr);
    void setMemory(const QVector<quint64> &data, quint64 baseAddr);
    void updateStackPointer(uint64_t sp);
    void updateMemory(uint64_t address, const QVector<uint8_t> &data);
    void updateTable();
    void clearData();

private slots:
    void updateDisplayMode();
    void updateWordMode();
    void changeSegment(int index);
    void prevPage();
    void nextPage();

private:
    void populateTable();

    // QTableWidget *table;
    QPushButton *clearButton;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QComboBox *segmentCombo;
    QComboBox *wordModeCombo;
    QCheckBox *hexCheckBox;
    QLabel *addrLabel;
    QLabel *spLabel;

    QVector<quint64> memory;
    quint64 baseAddress;
    bool showHex;
    bool doubleWordMode;
    int currentPage;
    int itemsPerPage;
    uint64_t stackPointer;
    QTableWidget *memoryTable;
    QSpinBox *baseAddressInput;
    QPushButton *refreshButton;

    uint64_t currentBaseAddress;

    QMap<uint64_t, uint8_t> memoryData;

    static constexpr int ROWS = 64;
    static constexpr int BYTES_PER_ROW = 16;
};

#endif // DATASEGMENT_H
