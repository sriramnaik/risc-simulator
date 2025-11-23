// #include "autocompletewidget.h"
// #include <QApplication>
// #include <QScreen>

// AutoCompleteWidget::AutoCompleteWidget(QWidget* parent)
//     : QListWidget(parent)
// {
//     // ✅ CRITICAL FIX: Use Qt::Tool instead of Qt::Popup to prevent focus stealing
//     setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
//     setFocusPolicy(Qt::NoFocus);

//     // ✅ CRITICAL FIX: Prevent the widget from activating and stealing focus
//     setAttribute(Qt::WA_ShowWithoutActivating);

//     setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//     setSelectionMode(QAbstractItemView::SingleSelection);
//     setMaximumHeight(200);
//     setMinimumWidth(400);

//     setStyleSheet(
//         "QListWidget {"
//         "    background-color: #252526;"
//         "    color: #cccccc;"
//         "    border: 1px solid #454545;"
//         "    font-family: 'Consolas', 'Courier New', monospace;"
//         "    font-size: 11pt;"
//         "    padding: 2px;"
//         "}"
//         "QListWidget::item {"
//         "    padding: 4px 8px;"
//         "    border: none;"
//         "}"
//         "QListWidget::item:selected {"
//         "    background-color: #094771;"
//         "    color: white;"
//         "}"
//         "QListWidget::item:hover {"
//         "    background-color: #2a2d2e;"
//         "}"
//         );

//     connect(this, &QListWidget::itemClicked, this, &AutoCompleteWidget::onItemClicked);
// }

// void AutoCompleteWidget::showCompletions(const QVector<InstructionInfo>& instructions, const QPoint& position) {
//     clear();

//     if (instructions.isEmpty()) {
//         hide();
//         return;
//     }

//     int maxWidth = 500;

//     // ✅ Add items with styled formatting
//     for (const InstructionInfo& info : instructions) {
//         QListWidgetItem* item = new QListWidgetItem();

//         // Store the instruction mnemonic as user data
//         item->setData(Qt::UserRole, info.mnemonic);

//         // For plain text version (since QListWidget doesn't support HTML directly)
//         QString plainText = QString("%1 %")
//                                 .arg(info.syntax)
//                                 .arg(info.description);

//         item->setText(plainText);

//         // Calculate size
//         QFontMetrics fm(font());
//         QStringList lines = plainText.split('\n');
//         int itemHeight = lines.count() * (fm.height() + 2) + 8;

//         for (const QString& line : lines) {
//             int lineWidth = fm.horizontalAdvance(line) + 40;
//             maxWidth = qMax(maxWidth, lineWidth);
//         }

//         item->setSizeHint(QSize(maxWidth, itemHeight));
//         addItem(item);
//     }

//     if (count() > 0) {
//         setCurrentRow(0);
//     }

//     move(position);
//     setFixedWidth(qMin(maxWidth, 700));

//     int totalHeight = 0;
//     for (int i = 0; i < count(); ++i) {
//         totalHeight += item(i)->sizeHint().height();
//     }
//     setFixedHeight(qMin(totalHeight + 10, 350));

//     show();
//     raise();
// }

// QString AutoCompleteWidget::getSelectedCompletion() const {
//     QListWidgetItem* item = currentItem();
//     return item ? item->text() : QString();
// }

// void AutoCompleteWidget::keyPressEvent(QKeyEvent* event) {
//     switch (event->key()) {
//     case Qt::Key_Return:
//     case Qt::Key_Enter:
//     case Qt::Key_Tab:
//         if (currentItem()) {
//             QString instructionName = currentItem()->data(Qt::UserRole).toString();
//             emit completionSelected(instructionName);
//         }
//         hide();
//         event->accept();
//         break;

//     case Qt::Key_Escape:
//         hide();
//         emit cancelled();
//         event->accept();
//         break;

//     case Qt::Key_Up:
//         if (currentRow() > 0) {
//             setCurrentRow(currentRow() - 1);
//         }
//         event->accept();
//         break;

//     case Qt::Key_Down:
//         if (currentRow() < count() - 1) {
//             setCurrentRow(currentRow() + 1);
//         }
//         event->accept();
//         break;

//     default:
//         QListWidget::keyPressEvent(event);
//         break;
//     }
// }

// void AutoCompleteWidget::focusOutEvent(QFocusEvent* event) {
//     Q_UNUSED(event);
//     // ✅ CRITICAL FIX: Don't hide on focus out - let it stay visible as passive suggestions
//     // hide();  // ← REMOVED - this was hiding the widget when typing in editor
// }

// void AutoCompleteWidget::onItemClicked(QListWidgetItem* item) {
//     if (item) {
//         // ✅ Emit ONLY the mnemonic stored in UserRole
//         QString instructionName = item->data(Qt::UserRole).toString();
//         emit completionSelected(instructionName);
//         hide();
//     }
// }

#include "autocompletewidget.h"
#include <QApplication>
#include <QScreen>
#include<QFontMetrics>

AutoCompleteWidget::AutoCompleteWidget(QWidget* parent)
    : QListWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setMaximumHeight(250);
    setMinimumWidth(350);
    setUniformItemSizes(false);

    setStyleSheet(
        "QListWidget {"
        "    background-color: #252526;"
        "    color: #cccccc;"
        "    border: 1px solid #3e3e42;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 9pt;"  // ✅ Smaller font
        "    padding: 4px;"
        "}"
        "QListWidget::item {"
        "    padding: 5px 8px;"
        "    border-bottom: 1px solid #2d2d30;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #094771;"
        "    color: white;"
        "    border-left: 2px solid #007acc;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        );

    connect(this, &QListWidget::itemClicked, this, &AutoCompleteWidget::onItemClicked);
}

void AutoCompleteWidget::showCompletions(const QVector<InstructionInfo>& instructions, const QPoint& position) {
    clear();

    if (instructions.isEmpty()) {
        hide();
        return;
    }

    QFontMetrics fm(font());
    int maxWidth = 350;

    // ✅ Add items with compact, readable formatting
    for (const InstructionInfo& info : instructions) {
        QListWidgetItem* item = new QListWidgetItem();

        // Store the instruction mnemonic as user data
        item->setData(Qt::UserRole, info.mnemonic);

        // ✅ Compact format: syntax on first line, description indented below
        QString plainText = QString("%1 %2")
                                .arg(info.syntax)
                                .arg(info.description);

        item->setText(plainText);

        // ✅ Calculate proper size for multi-line items
        QStringList lines = plainText.split('\n');
        int itemHeight = 0;

        for (const QString& line : lines) {
            itemHeight += fm.height() + 1;
            int lineWidth = fm.horizontalAdvance(line) + 25;
            maxWidth = qMax(maxWidth, lineWidth);
        }

        itemHeight += 6; // Add padding
        item->setSizeHint(QSize(maxWidth, itemHeight));
        addItem(item);
    }

    if (count() > 0) {
        setCurrentRow(0);
    }

    // ✅ Adjust width based on content
    int finalWidth = qMin(maxWidth, 600);
    setFixedWidth(finalWidth);

    // ✅ Calculate and set proper height
    int totalHeight = 4; // Top padding
    for (int i = 0; i < count(); ++i) {
        totalHeight += item(i)->sizeHint().height();
    }
    totalHeight += 4; // Bottom padding
    setFixedHeight(qMin(totalHeight, 250));

    // ✅ Position widget - align left edge with cursor position
    move(position);

    show();
    raise();
}

QString AutoCompleteWidget::getSelectedCompletion() const {
    QListWidgetItem* item = currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void AutoCompleteWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
        if (currentItem()) {
            QString instructionName = currentItem()->data(Qt::UserRole).toString();
            emit completionSelected(instructionName);
        }
        hide();
        event->accept();
        break;

    case Qt::Key_Escape:
        hide();
        emit cancelled();
        event->accept();
        break;

    case Qt::Key_Up:
        if (currentRow() > 0) {
            setCurrentRow(currentRow() - 1);
        }
        event->accept();
        break;

    case Qt::Key_Down:
        if (currentRow() < count() - 1) {
            setCurrentRow(currentRow() + 1);
        }
        event->accept();
        break;

    default:
        event->ignore();
        break;
    }
}

void AutoCompleteWidget::focusOutEvent(QFocusEvent* event) {
    Q_UNUSED(event);
    // hide();
}

bool AutoCompleteWidget::event(QEvent* event) {
    return QListWidget::event(event);
}

void AutoCompleteWidget::hideEvent(QHideEvent* event) {
    QListWidget::hideEvent(event);
    // emit cancelled();
}

void AutoCompleteWidget::onItemClicked(QListWidgetItem* item) {
    if (item) {
        QString instructionName = item->data(Qt::UserRole).toString();
        emit completionSelected(instructionName);
        hide();
    }
}
