#include "errorconsole.h"
#include <QScrollBar>
#include <QVBoxLayout>

ErrorConsole::ErrorConsole(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(2);

    // --- Clear button on left ---
    clearButton = new QPushButton("Clear", this);
    clearButton->setFixedWidth(80); // button width
    clearButton->setStyleSheet(
        "QPushButton { background-color: #3a3d41; color: white; border: 1px solid #555; border-radius: 4px; }"
        "QPushButton:hover { background-color: #505356; }"
        );
    mainLayout->addWidget(clearButton);

    // --- Container for QTextBrowser with border ---
    QWidget *textContainer = new QWidget(this);
    textContainer->setStyleSheet(
        "QWidget { "
        "border: 2px solid #606060; "
        "border-radius: 4px; "
        "background-color: #1e1e1e; "
        "}"
        );

    QVBoxLayout *containerLayout = new QVBoxLayout(textContainer);
    containerLayout->setContentsMargins(2,2,2,2); // small inner margin for border visibility
    containerLayout->setSpacing(0);

    // --- QTextBrowser ---
    textBrowser = new QTextBrowser(this);
    textBrowser->setStyleSheet(
        "QTextBrowser { "
        "background-color: #1e1e1e; "
        "color: #dcdcdc; "
        "border: none; "
        "padding: 4px; "
        "}"
        );
    textBrowser->setOpenExternalLinks(false);
    textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    containerLayout->addWidget(textBrowser);
    mainLayout->addWidget(textContainer, 1); // stretch factor 1

    connect(clearButton, &QPushButton::clicked, this, &ErrorConsole::clearConsole);
    connect(textBrowser, &QTextBrowser::anchorClicked, this, &ErrorConsole::handleAnchorClicked);
}

void ErrorConsole::addMessages(const QVector<std::string> &errors)
{

    for (auto &err : errors) {
        // Display in your QTextBrowser etc
        // e.g.
        QString msg = QString::fromStdString(err);
        textBrowser->append(colorizeAnsi(msg));

        // textBrowser->append(QString("%1").arg(err));
    }
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
}


void ErrorConsole::handleAnchorClicked(const QUrl &link)
{
    bool ok;
    int line = link.toString().toInt(&ok);
    if (ok) emit errorClicked(line);
}

void ErrorConsole::clearConsole()
{
    textBrowser->clear();
}


