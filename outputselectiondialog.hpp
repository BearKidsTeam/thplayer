#ifndef OUTPUTSELECTIONDIALOG_H
#define OUTPUTSELECTIONDIALOG_H

#include <QDialog>
#include <QAudioDeviceInfo>

namespace Ui {
class OutputSelectionDialog;
}

class OutputSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OutputSelectionDialog(QWidget *parent = 0);
    ~OutputSelectionDialog();
	void init(int curout);
	int selection();

private slots:
	void on_buttonBox_accepted();

private:
    Ui::OutputSelectionDialog *ui;
	int _selection;
};

#endif // OUTPUTSELECTIONDIALOG_H
