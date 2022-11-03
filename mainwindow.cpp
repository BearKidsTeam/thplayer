#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QFontDatabase>

#include "outputselectiondialog.hpp"
#include "loopedpcmstreamer.hpp"

#ifdef _WIN32
#define NOMINMAX //Windows API breaks STL, shit.
#include <Windows.h>
#endif

QString fsstr_to_qstring(const fs::path::string_type &s)
{
#if PATH_VALSIZE == 2 //the degenerate platform
    return QString::fromStdWString(s);
#else
    return QString::fromStdString(s);
#endif
}

fs::path qstring_to_path(const QString &s)
{
#if PATH_VALSIZE == 2 //the degenerate platform
    return fs::path(s.toStdWString());
#else
    return fs::path(s.toStdString());
#endif
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setPlayListTableHeader();
    ui->playlistTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->playlistTable->setSortingEnabled(false);
    datw = nullptr;
    devi = -1;
    timer = new QTimer();
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateWidgets);
    connect(ui->progressslider, &QSlider::sliderReleased, this, &MainWindow::seek);
    timer->start();
}
bool MainWindow::args(QCommandLineParser &p)
{
    argp = &p;
    if (argp->isSet("list-devices"))
    {
        printf("List of available output devices:\n");
        printf("Device ID\tDevice Name\n");
        int id = 0;
        for (auto &di : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        {
            printf("%d        \t%s\n", id++, di.deviceName().toStdString().c_str());
        }
        return true;
    }
    if (argp->isSet("device"))
    {
        bool ok = false;
        int t = argp->value("device").toInt(&ok);
        if (!ok)
        {
            printf("--device: Number expected.\n");
            return true;
        }
        if (t >= QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).size() || t < 0)
        {
            printf("--device: device ID out of range.\n");
            return true;
        }
        devi = t;
    }
    if (argp->positionalArguments().size())
        this->LoadFile(argp->positionalArguments()[0]);
    return false;
}

/*!    \brief  Load file.
 *
 *    Loads the game folder or any file in the folder.
 *    Determine game version.
 *    Returns true on success.
 */
bool MainWindow::LoadFile(QString filepath)
{
    QUrl url(filepath), bgmurl;
    bool isTrial = false;
    if (QFileInfo(filepath).isFile())
    {
        url = url.adjusted(QUrl::RemoveFilename);
    }

    if (QFile::exists(url.url() + "/thbgm.dat"))
    {
        bgmurl = QUrl(url.url() + "/thbgm.dat");
    }
    else if (QFile::exists(url.url() + "/thbgm_tr.dat"))
    {
        isTrial = true;
        bgmurl = QUrl(url.url() + "/thbgm_tr.dat");
    }
    else if (QFile::exists(url.url() + "/albgm.dat"))
    {
        bgmurl = QUrl(url.url() + "/albgm.dat");
    }

    songs.thbgmFilePath = bgmurl.url();
    songs.isTrial = isTrial;

    QDir gamedir = QDir(url.url());
    QStringList sl;
    sl << "*.dat";
    QFileInfoList fil = gamedir.entryInfoList(sl, QDir::NoFilter, QDir::Name);
    QString datf = "";
    int ver = -1;
    for (auto &i : fil)
    {
        if (~(ver = thVersionDetect(i)))
        {
            datf = i.absoluteFilePath();
            break;
        }
    }
    if (!datf.length())return false;
    stop();
    if (datw) delete datw;
    datw = new thDatWrapper(qstring_to_path(datf), ver);
    if (ver > 50) ver /= 10; //95,125,128,143 etc
    thver = ver;
    if (ver == 6)
    {
        songs.thbgmFilePath = url.url();
        songs.LoadFile_th6(datw, fs::path(datf.toStdString()));
    }
    else
        songs.LoadFile(datw, ver < 13 ? true : false);
    ui->thnameLabel->setText(url.url());
    SetupSongList();
    return true;
}

/*!    \brief  Load song data from thbgm.songs file.
 *
 *    Call to load thbgm.songs file.
 *    Then load song data from SongList to playlist table.
 */
bool MainWindow::SetupSongList()
{
    ui->playlistTable->clear();
    setPlayListTableHeader();
    ui->playlistTable->setRowCount(songs.songCnt);
    ui->playlistTable->setSortingEnabled(false);
    QFont fnt = QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont);
    for (int i = 0; i < songs.songCnt; i++)
    {
        song_t *song = &songs.songs[i];
        QString fileName(song->filename);

        QTableWidgetItem *itemTitle = new QTableWidgetItem(song->title);
        QTableWidgetItem *itemName = new QTableWidgetItem(fileName);
        QTableWidgetItem *itemStart = new QTableWidgetItem("0x" + QString::number(song->start, 16));
        QTableWidgetItem *itemLpSt = new QTableWidgetItem("0x" + QString::number(song->loopStart, 16));
        QTableWidgetItem *itemLen = new QTableWidgetItem("0x" + QString::number(song->length, 16));
        QTableWidgetItem *itemRate = new QTableWidgetItem(QString::number(song->rate));
        ui->playlistTable->setItem(i, 0, itemTitle);
        ui->playlistTable->setItem(i, 1, itemName);
        ui->playlistTable->setItem(i, 2, itemStart);
        ui->playlistTable->setItem(i, 3, itemLpSt);
        ui->playlistTable->setItem(i, 4, itemLen);
        ui->playlistTable->setItem(i, 5, itemRate);
        for (int j = 1; j < 6; ++j)
            ui->playlistTable->item(i, j)->setData(Qt::ItemDataRole::FontRole, fnt);
    }
    //ui->playlistTable->setSortingEnabled(true);
    return true;
}

MainWindow::~MainWindow()
{
    stop();
    delete ui;
    if (datw)delete datw;
}

void MainWindow::stop()
{
    ui->pauseButton->setEnabled(false);
    ui->pauseButton->setChecked(false);
    if (audioOutput)
    {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
        st->close();
        delete st;
        st = nullptr;
    }
}

// drag n drop
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    //check droped file
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    QString fileName = urls.first().toLocalFile();
    LoadFile(fileName);
}

void MainWindow::updateWidgets()
{
    if (!st) return;
    if (!cursong.length) return;
    ui->progressslider->setValue((int)100.*st->pos_sample() / (cursong.length / 4)); //TODO: don't hardcode the 4 here
}
void MainWindow::seek()
{
    st->seek_sample(ui->progressslider->value() / 100. * (cursong.length / 4.)); //TODO: don't hardcode the 4 here
}

QAudioFormat MainWindow::getAudioFormat(unsigned rate)
{
    QAudioFormat audioFormat;
    audioFormat.setCodec("audio/pcm");
    audioFormat.setChannelCount(2);
    audioFormat.setSampleRate(rate);
    audioFormat.setSampleSize(16);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    return audioFormat;
}

int MainWindow::thVersionDetect(QFileInfo i)
{
    auto str = i.fileName();
    QRegularExpression re06("[Mm][Dd]\\.[Dd][Aa][Tt]");
    auto mch = re06.match(str);
    if (mch.hasMatch())
    {
        thDatWrapper mdw(qstring_to_path(i.filePath()), 6);
        if (~mdw.getFileSize("musiccmt.txt")) return 6;
        return -1;
    }
    if (str.startsWith("alcostg"))
        return 103;
    QRegularExpression re("^[Tt][Hh](\\d{2,3})");
    mch = re.match(str);
    if (!mch.hasMatch()) return -1;
    QString ret = mch.captured(1);
    return ret.toInt();
}

void MainWindow::setPlayListTableHeader()
{
    ui->playlistTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Title"));
    ui->playlistTable->setHorizontalHeaderItem(1, new QTableWidgetItem("File"));
    ui->playlistTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Start"));
    ui->playlistTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Loop"));
    ui->playlistTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Length"));
    ui->playlistTable->setHorizontalHeaderItem(5, new QTableWidgetItem("Rate"));
}

void MainWindow::play(int index)
{
    int songIdx = -1;
    if (index != -1) songIdx = index;
    if (songIdx < 0 || songIdx >= songs.songCnt) return;

    ui->songnameLabel->setText(songs.songs[songIdx].title.length() ? songs.songs[songIdx].title : songs.songs[songIdx].filename);
    ui->commentTB->setText(songs.songs[songIdx].comment);
    cursong = songs.songs[songIdx];

    // audio playback:
    QAudioFormat desiredFormat1 = getAudioFormat(songs.songs[songIdx].rate);

    QAudioDeviceInfo info1(
        ~devi ? QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)[devi]
        : QAudioDeviceInfo::defaultOutputDevice());
    if (!info1.isFormatSupported(desiredFormat1))
    {
        qWarning() << "Default format not supported, trying to use the nearest.";
        desiredFormat1 = info1.preferredFormat();
    }
    stop();
    audioOutput = new QAudioOutput(info1, desiredFormat1, this);
    audioOutput->setVolume(1.0);
    fs::path srcfile = qstring_to_path(songs.thbgmFilePath);
    if (thver == 6)
        srcfile /= fs::path("bgm") / songs.songs[songIdx].filename.toStdString();
    st = new LoopedPCMStreamer(srcfile,
        songs.songs[songIdx].start,
        songs.songs[songIdx].length,
        songs.songs[songIdx].loopStart);
    st->open(QIODevice::OpenModeFlag::ReadOnly);

    audioOutput->start(st);
    if (audioOutput->error())
    {
        OutputSelectionDialog d;
        d.init(devi);
        d.exec();
        devi = d.selection();
        return play(index);
    }
    ui->pauseButton->setEnabled(true);
    ui->pauseButton->setChecked(false);
}

void MainWindow::on_playlistTable_doubleClicked(const QModelIndex &index)
{
    play(index.row());
}

void MainWindow::on_loopButton_clicked()
{
    // TODO: ???
    // ui->loopButton->setText(loopEnabled ? tr("Loop: On") : tr("Loop: Off"));
}

void MainWindow::on_prevButton_clicked()
{
    int r = ui->playlistTable->currentRow();
    int c = ui->playlistTable->currentColumn();
    int rc = ui->playlistTable->rowCount();
    r = (r + rc - 1) % rc;
    ui->playlistTable->setCurrentCell(r, c);
    play(r);
}

void MainWindow::on_nextButton_clicked()
{
    int r = ui->playlistTable->currentRow();
    int c = ui->playlistTable->currentColumn();
    int rc = ui->playlistTable->rowCount();
    r = (r + 1) % rc;
    ui->playlistTable->setCurrentCell(r, c);
    play(r);
}

void MainWindow::on_pauseButton_clicked(bool checked)
{
    if (!audioOutput) return;
    if (checked) audioOutput->suspend();
    else audioOutput->resume();
}

void MainWindow::on_action_Open_triggered()
{
    LoadFile(QFileDialog::getExistingDirectory(this, "Select game directory"));
}


void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}


void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(this, "About TouHou Player",
                       QString("TouHou Player") + "\n"
                       "TouHou BGM player for all platform." + "\n\n" +
                       "https:://github.com/BearKidsTeam/thplayer");
}

