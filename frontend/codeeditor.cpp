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

void CodeEditor::paintEvent(QPaintEvent *event)
{

    QPlainTextEdit::paintEvent(event);

    if (!pipelineLabels.isEmpty())
    {
        QPainter painter(viewport());
        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                int lineNumber = blockNumber + 1; // 1-based

                // ✅ Check if this line has any pipeline stages
                if (pipelineLabels.contains(lineNumber)) {
                    const QSet<QString>& stages = pipelineLabels[lineNumber];

                    // Build ordered stage list
                    QStringList stageOrder = {"IF", "ID", "EX", "MEM", "WB"};
                    QStringList orderedStages;

                    for (const QString& stageName : stageOrder) {
                        if (stages.contains(stageName)) {
                            orderedStages.append(stageName);
                        }
                    }

                    // ✅ Draw stages from right to left with proper spacing
                    if (!orderedStages.isEmpty()) {
                        painter.setPen(Qt::white);

                        QFontMetrics fm(font());
                        int xOffset = 0;

                        // Draw in reverse order (right to left)
                        for (int i = orderedStages.size() - 1; i >= 0; --i) {
                            const QString& stageName = orderedStages[i];
                            int stageWidth = fm.horizontalAdvance(stageName);

                            int xPos = viewport()->width() - 50 - xOffset - stageWidth;
                            painter.drawText(xPos, top + fm.ascent(), stageName);

                            xOffset += stageWidth + 10; // 10px spacing between stages
                        }
                    }
                }
            }

            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
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

void CodeEditor::setPipelineLabel(int line, const QString &stage)
{
    // qDebug() << "setPipelineLabel - line:" << line << "stage:" << stage;

    if (stage.endsWith("_CLEAR")) {
        QString baseStage = stage.left(stage.indexOf("_CLEAR"));
        clearPipelineLabels(line, baseStage);
        return;
    }

    // ✅ Add this stage to the set for this line
    if (!pipelineLabels.contains(line)) {
        pipelineLabels[line] = QSet<QString>();
    }
    pipelineLabels[line].insert(stage);

    // qDebug() << "Added" << stage << "to line" << line
    //          << "- now has" << pipelineLabels[line].size() << "stages";

    viewport()->update();
}

void CodeEditor::clearAllPipelineLabels()
{
    pipelineLabels.clear();
    viewport()->update();
}

void CodeEditor::clearPipelineLabels(int line, const QString &stage)
{
    // qDebug() << "clearPipelineLabel - line:" << line << "stage:" << stage;

    if (!pipelineLabels.contains(line)) {
        // qDebug() << "Line" << line << "has no labels";
        return;
    }

    // Remove this specific stage from the line
    pipelineLabels[line].remove(stage);

    // If no stages left, remove the line entry entirely
    if (pipelineLabels[line].isEmpty()) {
        pipelineLabels.remove(line);
        // qDebug() << "Removed line" << line << "entirely (no stages left)";
    } else {
        // qDebug() << "Removed" << stage << "from line" << line
                 // << "- still has" << pipelineLabels[line].size() << "stages";
    }

    viewport()->update();
}

const QMap<int, QSet<QString> > &CodeEditor::getPipelineLabels()
{
    return pipelineLabels;
}


