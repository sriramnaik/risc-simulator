#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QFont>
#include <QVBoxLayout>

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

    setFont(QFont("Consolas", 11));
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Dark theme for editor
    // setStyleSheet(
    //     "QPlainTextEdit { background-color: #1e1e1e; color: #dcdcdc; }"
    //     );

    QWidget *editorContainer = new QWidget();
    QVBoxLayout *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    // editorLayout->addWidget(editor);
    editorContainer->setStyleSheet(
        "QWidget { "
        "border: 2px solid #3a3d41; "
        "border-radius: 4px; "
        "background-color: #1e1e1e; "
        "}");

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    updateLineNumberArea(viewport()->rect(), 0);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = qMax(5, 1 + (int)log10(qMax(1, document()->blockCount())));
    // qMax(3, ...) ensures minimum 3 digits

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
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
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                      lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(40, 40, 40); // dark highlight
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(30, 30, 30));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber() + 1;

    int top = static_cast<int>(blockBoundingGeometry(block)
                                   .translated(contentOffset())
                                   .top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber);
            painter.setPen(QColor(150, 150, 150));
            painter.drawText(0, top, lineNumberArea->width() - 2,
                             fontMetrics().height(), Qt::AlignRight, number);

            if (pipelineStageNames.contains(blockNumber))
            {
                QString stageName = pipelineStageNames.value(blockNumber);
                painter.setPen(QColor(200, 200, 100)); // distinct color
                QFont font = painter.font();
                font.setPointSize(8);
                painter.setFont(font);
                painter.drawText(lineNumberArea->width() - 50, top, 50,
                                 fontMetrics().height(), Qt::AlignLeft, stageName);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::goToLine(int lineNumber)
{
    QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
    if (!block.isValid())
        return;

    QTextCursor cursor(block);
    setTextCursor(cursor);
    centerCursor(); // centers in view
}

// To set pipeline stages (call this when updating highlight):
void CodeEditor::setPipelineStages(const QMap<int, QString> &stages)
{
    pipelineStageNames = stages;
    if(lineNumberArea)
        lineNumberArea->update();
}

void CodeEditor::highlightPipelineStages(const QMap<int, QString>& stages)
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    for (auto it = stages.constBegin(); it != stages.constEnd(); ++it) {
        int line = it.key();
        QTextBlock block = document()->findBlockByNumber(line - 1);
        if (block.isValid()) {
            QTextEdit::ExtraSelection selection;
            QColor color(255, 215, 0, 80); // Gold-ish transparent highlight
            selection.format.setBackground(color);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            QTextCursor cursor(block);
            selection.cursor = cursor;
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
    }

    setExtraSelections(extraSelections);
}
