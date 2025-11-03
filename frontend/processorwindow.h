#ifndef PROCESSORWINDOW_H
#define PROCESSORWINDOW_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class ProcessorWindow : public QDialog {
    Q_OBJECT
public:
    explicit ProcessorWindow(QWidget *parent = nullptr);
    void setInitialSelection(const QString& stage, const QString& isa);
    QString selectedProcessorName() const;
    QString selectedISA() const;
private slots:
    void onISAChanged(int index);
    void onStageChanged(int index);
private:
    QComboBox *isaCombo;
    QComboBox *stageCombo;
    QLabel *isaLabel;
    QLabel *stageLabel;
    QPushButton *okButton;
    QPushButton *cancelButton;

    QString m_isa;
    QString m_stage;
};

#endif // PROCESSORWINDOW_H

