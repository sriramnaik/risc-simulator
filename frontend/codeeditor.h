#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QTimer>


class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void goToLine(int lineNumber);
    void setPipelineStages(const QMap<int, QString> &stages);
    void highlightLine(int lineNumber);
    void paintEvent(QPaintEvent *event) override;
    QMap<int, QString> getPipelineStages() const;
    // void setPipelineLabel(uint64_t pc, const QString &stage,int line);
    void setPipelineLabel(int line,const QString &stage);
    void clearPipelineLabels(int line,const QString &stage);
    QMap<QString, int> stageHighlights;   // stage -> line
    void highlightLineForStage(const QString &stage, int sourceLine);
    void updateHighlights();
    void clearHighlightForStage(const QString &stage);
    void clearAllPipelineLabels();
    // QMap<int, QString>  pipelineLabels;

    const QMap<int, QSet<QString>>& getPipelineLabels();
    void lineNumberAreaClicked(const QPoint &pos);

protected:
    void resizeEvent(QResizeEvent *event) override;  // capture mouse clicks

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();


private:
    QWidget *lineNumberArea;
    QSet<int> breakpoints;
    QMap<int, QSet<QString>> pipelineLabels;
    void paintPipelineStages(QPainter &painter, const QRect &rect);


};

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;

};

#endif // CODEEDITOR_H

