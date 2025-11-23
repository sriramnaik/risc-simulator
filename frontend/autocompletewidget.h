
// autocompletewidget.h
#ifndef AUTOCOMPLETEWIDGET_H
#define AUTOCOMPLETEWIDGET_H

#include <QListWidget>
#include <QKeyEvent>

class AutoCompleteWidget : public QListWidget {
    Q_OBJECT

public:
    explicit AutoCompleteWidget(QWidget* parent = nullptr);

    void showCompletions(const QStringList& completions, const QPoint& position);
    QString getSelectedCompletion() const;

signals:
    void completionSelected(const QString& completion);
    void cancelled();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onItemClicked(QListWidgetItem* item);
};

#endif // AUTOCOMPLETEWIDGET_H
