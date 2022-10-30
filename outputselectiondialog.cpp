#include "outputselectiondialog.hpp"
#include "ui_outputselectiondialog.h"

OutputSelectionDialog::OutputSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OutputSelectionDialog)
{
    ui->setupUi(this);
}

OutputSelectionDialog::~OutputSelectionDialog()
{
    delete ui;
}

void OutputSelectionDialog::init(int curout)
{
    QList<QAudioDeviceInfo> dl = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    if (curout < 0 || curout >= dl.size())curout = 0;
    ui->label->setText(QString("The current audio output '%1' didn't work, please choose another one below.").arg(dl[curout].deviceName()));
    _selection = -1;
    ui->comboBox->clear();
    for (auto di : dl)ui->comboBox->addItem(di.deviceName());
}
void OutputSelectionDialog::on_buttonBox_accepted()
{
    _selection = ui->comboBox->currentIndex();
}
int OutputSelectionDialog::selection()
{
    return _selection;
}
