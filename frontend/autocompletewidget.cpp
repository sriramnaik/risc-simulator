#include "autocompletewidget.h"
#include <QApplication>
#include <QScreen>

AutoCompleteWidget::AutoCompleteWidget(QWidget* parent)
    : QListWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setStyleSheet(
        "QListWidget {"
        "    background-color: #252526;"
        "    color: #cccccc;"
        "    border: 1px solid #454545;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 11pt;"
        "    padding: 2px;"
        "}"
        "QListWidget::item {"
        "    padding: 4px 8px;"
        "    border: none;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #094771;"
        "    color: white;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #2a2d2e;"
        "}"
        );

    connect(this, &QListWidget::itemClicked, this, &AutoCompleteWidget::onItemClicked);
}

void AutoCompleteWidget::showCompletions(const QStringList& completions, const QPoint& position) {
    clear();

    if (completions.isEmpty()) {
        hide();
        return;
    }

    addItems(completions);
    setCurrentRow(0);

    // Calculate size
    int width = 200;
    int height = qMin(completions.size() * 28 + 4, 300);

    resize(width, height);

    // Position near cursor, but keep on screen
    QPoint pos = position;
    QRect screenGeometry = QApplication::primaryScreen()->geometry();

    if (pos.x() + width > screenGeometry.right()) {
        pos.setX(screenGeometry.right() - width);
    }
    if (pos.y() + height > screenGeometry.bottom()) {
        pos.setY(pos.y() - height - 20);
    }

    move(pos);
    show();
    raise();
}

QString AutoCompleteWidget::getSelectedCompletion() const {
    QListWidgetItem* item = currentItem();
    return item ? item->text() : QString();
}

void AutoCompleteWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
        if (currentItem()) {
            emit completionSelected(currentItem()->text());
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
        QListWidget::keyPressEvent(event);
        break;
    }
}

void AutoCompleteWidget::focusOutEvent(QFocusEvent* event) {
    Q_UNUSED(event);
    hide();
}

void AutoCompleteWidget::onItemClicked(QListWidgetItem* item) {
    if (item) {
        emit completionSelected(item->text());
        hide();
    }
}

