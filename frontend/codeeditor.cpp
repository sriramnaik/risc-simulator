#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QFont>
#include <QVBoxLayout>
#include <cmath>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    // Connect signals
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    // Increase font size and set fixed-width font
    QFont font("Consolas", 13);
    setFont(font);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Dark theme for editor
    setStyleSheet(
        "QPlainTextEdit { "
        "background-color: #1e1e1e; "
        "color: #dcdcdc; "
        "border: none; "
        "padding-left: 4px; "
        "selection-background-color: #264f78; "
        "}");

    // Allow full-width highlight to show
    viewport()->setAutoFillBackground(false);

    // Give line number area a divider style
    lineNumberArea->setStyleSheet(
        "QWidget { "
        "background-color: #1e1e1e; "
        "border-right: 2px solid #b0b0b0; "
        "}");

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);

    // Also update viewport to repaint pipeline labels
    if (!pipelineLabels.isEmpty())
        viewport()->update();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    int lineAreaWidth = lineNumberAreaWidth();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineAreaWidth, cr.height()));

    setViewportMargins(lineAreaWidth, 0, 0, 0);
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(70, 70, 90, 100);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);

        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::highlightLine(int lineNumber)
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(40, 40, 40);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);

    QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
    if (block.isValid())
    {
        QTextCursor cursor(block);
        selection.cursor = cursor;
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = qMax(5, 1 + (int)log10(qMax(1, document()->blockCount())));
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    // Paint line numbers on the left
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(30, 30, 30));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int lineNumber = blockNumber + 1;

    int top = static_cast<int>(blockBoundingGeometry(block)
                                   .translated(contentOffset())
                                   .top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    // Draw line numbers
    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(lineNumber);
            painter.setPen(QColor(150, 150, 150));
            painter.drawText(0, top, lineNumberArea->width() - 4,
                             fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++lineNumber;
    }
}

// void CodeEditor::paintEvent(QPaintEvent *event)
// {
//     // Call parent to draw the text editor content
//     QPlainTextEdit::paintEvent(event);

//     // Paint pipeline stage labels on the right side of viewport (only if pipelineLabels is not empty)
//     if (!pipelineLabels.isEmpty())
//     {
//         QPainter painter(viewport());
//         QTextBlock block = firstVisibleBlock();
//         int blockNumber = block.blockNumber();

//         int top = static_cast<int>(blockBoundingGeometry(block)
//                                        .translated(contentOffset())
//                                        .top());
//         int bottom = top + static_cast<int>(blockBoundingRect(block).height());
//         int rightMargin = viewport()->width() - 50; // adjust spacing as needed

//         while (block.isValid() && top <= event->rect().bottom())
//         {
//             if (block.isVisible() && bottom >= event->rect().top())
//             {
//                 uint64_t pc = blockNumber * 4; // assuming each instruction = 4 bytes
//                 if (pipelineLabels.contains(pc))
//                 {
//                     QString stage = pipelineLabels[pc];
//                     painter.setPen(Qt::white);
//                     painter.setFont(font());
//                     painter.drawText(rightMargin, top + fontMetrics().ascent(), stage);
//                 }
//             }

//             block = block.next();
//             top = bottom;
//             bottom = top + static_cast<int>(blockBoundingRect(block).height());
//             ++blockNumber;
//         }
//     }
// }

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    if (!pipelineLabels.isEmpty())
    {
        QPainter painter(viewport());
        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();

        int top = static_cast<int>(blockBoundingGeometry(block)
                                       .translated(contentOffset())
                                       .top());
        int bottom = top + static_cast<int>(blockBoundingRect(block).height());

        int rightMargin = viewport()->width() - 50; // adjust spacing as needed

        while (block.isValid() && top <= event->rect().bottom())
        {
            if (block.isVisible() && bottom >= event->rect().top())
            {
                int lineNumber = blockNumber + 1;  // 1-based line number
                if (pipelineLabels.contains(lineNumber))
                {
                    QString stage = pipelineLabels[lineNumber];
                    painter.setPen(Qt::white);
                    painter.setFont(font());
                    painter.drawText(rightMargin, top + fontMetrics().ascent(), stage);
                }
            }

            block = block.next();
            top = bottom;
            bottom = top + static_cast<int>(blockBoundingRect(block).height());
            ++blockNumber;
        }
    }
}


void CodeEditor::goToLine(int lineNumber)
{
    QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
    if (!block.isValid())
        return;

    QTextCursor cursor(block);
    setTextCursor(cursor);
    centerCursor();
}

// void CodeEditor::highlightPipelineStages(const QMap<int, QString> &stages)
// {
//     QList<QTextEdit::ExtraSelection> extraSelections;

//     for (auto it = stages.constBegin(); it != stages.constEnd(); ++it)
//     {
//         int line = it.key();
//         QTextBlock block = document()->findBlockByNumber(line - 1);
//         if (block.isValid())
//         {
//             QTextEdit::ExtraSelection selection;
//             QColor color(255, 215, 0, 80); // gold transparent
//             selection.format.setBackground(color);
//             selection.format.setProperty(QTextFormat::FullWidthSelection, true);
//             QTextCursor cursor(block);
//             selection.cursor = cursor;
//             selection.cursor.clearSelection();
//             extraSelections.append(selection);
//         }
//     }

//     setExtraSelections(extraSelections);
// }

// void CodeEditor::setPipelineLabel(uint64_t pc, const QString &stage,int line)
// {
//     if (stage.endsWith("_CLEAR"))
//     {
//         QString baseStage = stage.left(stage.indexOf("_CLEAR"));

//         // remove any label with this stage name (regardless of PC)
//         QList<uint64_t> keysToRemove;
//         for (auto it = pipelineLabels.begin(); it != pipelineLabels.end(); ++it)
//         {
//             if (it.value() == baseStage)
//                 keysToRemove.append(it.key());
//         }
//         for (uint64_t key : keysToRemove)
//             pipelineLabels.remove(key);

//         // remove highlight for that stage

//         viewport()->update();
//         return;
//     }

//     // NORMAL CASE: add/replace the label for this PC
//     // First remove existing entry that had this same stage (so stage appears only once)
//     QList<uint64_t> toRemove;
//     for (auto it = pipelineLabels.begin(); it != pipelineLabels.end(); ++it)
//     {
//         if (it.value() == stage)
//             toRemove.append(it.key());
//     }
//     for (uint64_t key : toRemove)
//         pipelineLabels.remove(key);

//     // Now set label
//     pipelineLabels[pc] = stage;

//     // Update highlight for this stage (pc->line conversion done in highlightLineForStage)
//     // NOTE: highlightLineForStage expects a 1-based sourceLine (matching your other helpers)
//     int sourceLine = (int)(pc / 4) + 1;

//     viewport()->update();
// }

// void CodeEditor::setPipelineLabel(int line, const QString &stage)
// {
//     if (stage.endsWith("_CLEAR"))
//     {
//         QString baseStage = stage.left(stage.indexOf("_CLEAR"));

//         // Remove any label with this stage name (regardless of line)
//         QList<int> keysToRemove;
//         for (auto it = pipelineLabels.begin(); it != pipelineLabels.end(); ++it)
//         {
//             if (it.value() == baseStage)
//                 keysToRemove.append(it.key());
//         }
//         for (int key : keysToRemove)
//             pipelineLabels.remove(key);

//         viewport()->update();
//         return;
//     }

//     // NORMAL CASE: add/replace the label for this line
//     // Remove existing entry that had the same stage (so only one of each stage exists)
//     QList<int> toRemove;
//     for (auto it = pipelineLabels.begin(); it != pipelineLabels.end(); ++it)
//     {
//         if (it.value() == stage)
//             toRemove.append(it.key());
//     }
//     for (int key : toRemove)
//         pipelineLabels.remove(key);

//     // Set label for this line
//     pipelineLabels[line] = stage;

//     // Redraw the viewport
//     viewport()->update();
// }

void CodeEditor::setPipelineLabel(int line, const QString &stage)
{
    if (stage.endsWith("_CLEAR")) {
        QString baseStage = stage.left(stage.indexOf("_CLEAR"));

        // Remove the label only for the given line if the stage matches
        if (pipelineLabels.contains(line) && pipelineLabels.value(line) == baseStage)
            pipelineLabels.remove(line);

        viewport()->update();
        return;
    }

    // Remove any existing label with this stage (ensure only one instance per stage exists)
    QList<int> toRemove;
    for (auto it = pipelineLabels.begin(); it != pipelineLabels.end(); ++it) {
        if (it.value() == stage)
            toRemove.append(it.key());
    }
    for (int key : toRemove)
        pipelineLabels.remove(key);

    // Set label for this line
    pipelineLabels[line] = stage;

    viewport()->update();
}

void CodeEditor::clearPipelineLabels()
{
    pipelineLabels.clear();
    viewport()->update();
}




