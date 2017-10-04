#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

/*!    \brief  Load file.
 *
 *    Load any loadable file, thbgm.fmt and thbgm.dat for most of case.
 *    should try to auto detect touhou version to avoid pop-up.
 */
bool MainWindow::LoadFile(QString filepath)
{
    // TODO: should refactor this.
    QFileInfo fileInfo(filepath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        // TODO: try detect thgame version to avoid pop-up
        qDebug() << fileInfo.absoluteDir().dirName();
        if (fileInfo.suffix() == tr("fmt")) {
            // Load fmt file and try load dat file
            // TODO: if detect failed, ask for ignoreAnUint
            if (LoadFmtFile(filepath, true)) {
                QFileInfo thbgmdatFileInfo(fileInfo.absolutePath() + "/thbgm.dat");
                if (thbgmdatFileInfo.exists() && thbgmdatFileInfo.isFile()) {
                    fmt.thbgmFilePath = thbgmdatFileInfo.absoluteFilePath();
                } else {
                    // let user choose dat file.
                }
                ui->thnameLabel->setText(fileInfo.fileName());
                return true;
            } else {
                return false;
            }
        } else if (fileInfo.suffix() == tr("dat")) {
            // Load dat file and try load fmt file
            QFileInfo thbgmfmtFileInfo(fileInfo.absolutePath() + "/thbgm.fmt");
            if (!(thbgmfmtFileInfo.exists() && thbgmfmtFileInfo.isFile())) {
                // let user choose fmt file.
            }
            // TODO: if detect failed, ask for ignoreAnUint
            if (LoadFmtFile(thbgmfmtFileInfo.absoluteFilePath(), true)) {
                fmt.thbgmFilePath = fileInfo.absoluteFilePath();
                ui->thnameLabel->setText(thbgmfmtFileInfo.fileName());
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

/*!    \brief  Load song data from thbgm.fmt file.
 *
 *    Call to load thbgm.fmt file.
 *    Then load song data from FmtFile to playlist table.
 */
bool MainWindow::LoadFmtFile(QString filepath, bool ignoreAnUint)
{
    if (fmt.LoadFile(filepath, ignoreAnUint)) {
        ui->playlistTable->clear();
        setPlayListTableHeader();
        ui->playlistTable->setRowCount(fmt.songCnt);
        ui->playlistTable->setSortingEnabled(false);
        for (int i = 0; i < fmt.songCnt; i++) {
            song_t* song = &fmt.songs[i];
            QString fileName(song->name);

            QTableWidgetItem *itemName = new QTableWidgetItem(fileName);
            QTableWidgetItem *itemStart = new QTableWidgetItem(QString::number(song->start));
            QTableWidgetItem *itemLpSt = new QTableWidgetItem(QString::number(song->loopStart));
            QTableWidgetItem *itemLen = new QTableWidgetItem(QString::number(song->length));
            QTableWidgetItem *itemRate = new QTableWidgetItem(QString::number(song->rate));
            ui->playlistTable->setItem(i, 0, itemName);
            ui->playlistTable->setItem(i, 1, itemStart);
            ui->playlistTable->setItem(i, 2, itemLpSt);
            ui->playlistTable->setItem(i, 3, itemLen);
            ui->playlistTable->setItem(i, 4, itemRate);
        }
        ui->playlistTable->setSortingEnabled(true);
        return true;
    } else {
        return false;
    }
}

MainWindow::~MainWindow()
{
    delete ui;

    if (audioOutput != nullptr) {
        audioOutput->stop();
        delete audioOutput;
    }
}

// drag n drop
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    //check droped file
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    Q_UNUSED(event)
    // fmt file drop???
}

void MainWindow::on_playButton_clicked()
{
    LoadFile(QString("/home/blumia/Programs/WinePrograms/th16/thbgm.fmt"));

}

void MainWindow::when_audioOutput_stateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            songBuffer.seek(loopStart);
            audioOutput->start(&songBuffer);
            //audio->stop();
            //delete audio;
            break;

        case QAudio::StoppedState:
            // Stopped for other reasons
            if (audioOutput->error() != QAudio::NoError) {
                // Error handling
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}

// Audio format which thbgm.dat used: S16_LE 44100 stereo
QAudioFormat MainWindow::getAudioFormat()
{
    QAudioFormat audioFormat;
    audioFormat.setCodec("audio/pcm");
    audioFormat.setChannelCount(2);
    audioFormat.setSampleRate(44100);
    audioFormat.setSampleSize(16);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    return audioFormat;
}

void MainWindow::setPlayListTableHeader()
{
    ui->playlistTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
    ui->playlistTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Start"));
    ui->playlistTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Loop"));
    ui->playlistTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Length"));
    ui->playlistTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Rate"));
}

void MainWindow::on_playlistTable_doubleClicked(const QModelIndex &index)
{
    if (audioOutput != nullptr) {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
    }
    int songIdx = index.row();

    QFile sourceFile(fmt.thbgmFilePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        qDebug() << "open file failed";
    }
    sourceFile.seek(fmt.songs[songIdx].start);
    QByteArray ba = sourceFile.read(fmt.songs[songIdx].length);
    loopStart = fmt.songs[songIdx].loopStart;
    songBuffer.close();
    songBuffer.setData(ba);
    songBuffer.open(QIODevice::ReadOnly);
    sourceFile.close();
    ui->songnameLabel->setText(fmt.songs[songIdx].name);

    // audio playback:
    QAudioFormat desiredFormat1 = getAudioFormat();

    // TODO:
    // output devices from QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)
    QAudioDeviceInfo info1(QAudioDeviceInfo::defaultOutputDevice());
    if (!info1.isFormatSupported(desiredFormat1)) {
        qWarning() << "Default format not supported, trying to use the nearest.";
        desiredFormat1 = info1.preferredFormat();
    }

    audioOutput = new QAudioOutput(desiredFormat1, this);
    audioOutput->setVolume(1.0);

    connect(audioOutput, SIGNAL(stateChanged(QAudio::State)),
                   this, SLOT(when_audioOutput_stateChanged(QAudio::State)));

    audioOutput->start(&songBuffer);
}
