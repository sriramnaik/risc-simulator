#ifndef ERRORCONSOLE_H
#define ERRORCONSOLE_H

#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>
#include <QString>
#include <QRegularExpression>

// #include "../../backend/assembler/parser.h"//

// struct ErrorMessage {
//     int line;     // "Error", "Warning", "Info"
//     QString message;
// };

class ErrorConsole : public QWidget
{
    Q_OBJECT

public:
    explicit ErrorConsole(QWidget *parent = nullptr);


signals:
    void errorClicked(int line); // emitted when user clicks on a message

public slots:
    void clearConsole();
    void addMessages(const QVector<std::string> &messages);


private slots:
    void handleAnchorClicked(const QUrl &link);

private:
    QTextBrowser *textBrowser;
    QPushButton *clearButton;
    QString colorizeAnsi(const QString &input) {
        QString text = input;

        // 1. Convert ANSI color codes to HTML spans (common compiler colors)
        // text.replace(QRegularExpression("\033[31m"), "<span style='color:red;'>");     // Red (errors)  // Green (success)
        // text.replace(QRegularExpression("\033[0m"), "</span>");                       // Reset
        text.replace(QRegularExpression("\x1B\\[[0-9;]*m"), "");                        // Remove any others

        // 2. Escape HTML reserved characters from assembly/paths
        text = text.toHtmlEscaped();

        // Restore preserved color tags after escaping
        text.replace("&lt;span", "<span");
        text.replace("&lt;/span&gt;", "</span>");

        // 3. Remove consecutive blank lines to match terminal grouping
        text.replace(QRegularExpression("(\n){3,}"), "\n\n");

        // 4. Preserve indentation — replace leading spaces with non-breaking spaces
        QStringList lines = text.split("\n");
        for (QString &line : lines) {
            int count = 0;
            for (QChar c : line) {
                if (c == ' ') count++; else break;
            }
            if (count > 0)
                line.replace(0, count, QString("&nbsp;").repeated(count));
        }

        // Join with <br> to preserve line breaks within a <pre>
        // text = lines.join("<br>");

        // 5. Wrap everything in a <pre> block with monospace font
        text = "<pre style='font-family: Consolas, Courier, monospace; "
               "white-space: pre-wrap; margin:0; padding:0; line-height: 1.15; "
               "font-size: 12.8px;'>"
               + text +
               "</pre>";

        return text;
    }


};

#endif // ERRORCONSOLE_H
