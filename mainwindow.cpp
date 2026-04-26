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
#include <QMediaDevices>

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

class KeyEventFilter : public QObject
{
    Q_OBJECT
public:
    KeyEventFilter(QObject *parent = nullptr) : QObject(parent) {}

Q_SIGNALS:
    void c();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent *e = static_cast<QKeyEvent *>(event);
            if (e->key() == Qt::Key::Key_C) {
                Q_EMIT c();
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->altmixButton->hide();
    setPlayListTableHeader();
    ui->playlistTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->playlistTable->setSortingEnabled(false);
    datw = nullptr;
    devi = -1;
    timer = new QTimer();
    timer->setInterval(100);
    auto kef = new KeyEventFilter(this);
    this->installEventFilter(kef);
    connect(kef, &KeyEventFilter::c, this, &MainWindow::switch_mix);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateWidgets);
    connect(ui->progressslider, &QSlider::sliderReleased, this, &MainWindow::seek);
    connect(ui->altmixButton, &QPushButton::clicked, this, &MainWindow::switch_mix);
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
        for (auto &di : QMediaDevices::audioOutputs())
        {
            printf("%d        \t%s\n", id++, di.description().toStdString().c_str());
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
        if (t >= QMediaDevices::audioOutputs().size() || t < 0)
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

    tracklist.thbgmFilePath = bgmurl.url();
    tracklist.isTrial = isTrial;

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
        tracklist.thbgmFilePath = url.url();
        tracklist.LoadFile_th6(datw, fs::path(datf.toStdString()));
    }
    else
        tracklist.LoadFile(datw, ver < 13 ? true : false);
    if (ver == 13)
        ui->altmixButton->setText("Spirit World [C]");
    ui->thnameLabel->setText(url.url());
    SetupTrackList();
    return true;
}

/*!    \brief  Load song data from thbgm.songs file.
 *
 *    Call to load thbgm.songs file.
 *    Then load song data from SongList to playlist table.
 */
bool MainWindow::SetupTrackList()
{
    ui->playlistTable->clear();
    ui->playlistTable->setRowCount(0);
    setPlayListTableHeader();
    ui->playlistTable->setSortingEnabled(false);
    QFont fnt = QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont);
    for (int i = 0; i < tracklist.tracks.size(); i++)
    {
        track_t *trk = &tracklist.tracks[i];
        if (trk->is_altmix) continue;
        QString fileName(trk->filename);

        QTableWidgetItem *itemTitle = new QTableWidgetItem(trk->title);
        QTableWidgetItem *itemName = new QTableWidgetItem(fileName);
        QTableWidgetItem *itemStart = new QTableWidgetItem("0x" + QString::number(trk->start, 16));
        QTableWidgetItem *itemLpSt = new QTableWidgetItem("0x" + QString::number(trk->loopStart, 16));
        QTableWidgetItem *itemLen = new QTableWidgetItem("0x" + QString::number(trk->length, 16));
        QTableWidgetItem *itemRate = new QTableWidgetItem(QString::number(trk->rate));
        int r = ui->playlistTable->rowCount();
        ui->playlistTable->insertRow(r);
        ui->playlistTable->setItem(r, 0, itemTitle);
        ui->playlistTable->setItem(r, 1, itemName);
        ui->playlistTable->setItem(r, 2, itemStart);
        ui->playlistTable->setItem(r, 3, itemLpSt);
        ui->playlistTable->setItem(r, 4, itemLen);
        ui->playlistTable->setItem(r, 5, itemRate);
        for (int j = 0; j < 6; ++j) {
            auto itm = ui->playlistTable->item(r, j);
            if (j != 0) itm->setData(Qt::ItemDataRole::FontRole, fnt);
            itm->setData(Qt::UserRole + 1, i);
        }
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
    altmix_state = -1;
    if (audioOutput)
    {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
        delete st;
        st = nullptr;
    }
    if (audioOutput_alt) {
        audioOutput_alt->stop();
        delete audioOutput_alt;
        audioOutput_alt = nullptr;
        delete st_alt;
        st_alt = nullptr;
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
    auto *active_st = altmix_state == 1 ? st_alt : st;
    if (!active_st) return;
    if (!curtrk.length) return;
    ui->progressslider->setValue((int)100. * active_st->pos_sample() / active_st->length_sample());
}
void MainWindow::seek()
{
    auto *active_st = altmix_state == 1 ? st_alt : st;
    active_st->seek_sample(ui->progressslider->value() / 100. * active_st->length_sample());
}

QAudioFormat MainWindow::getAudioFormat(unsigned rate)
{
    QAudioFormat audioFormat;
    audioFormat.setChannelCount(2);
    audioFormat.setSampleRate(rate);
    audioFormat.setSampleFormat(QAudioFormat::SampleFormat::Int16);
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
    int trkIdx = -1;
    if (index != -1) trkIdx = index;
    if (trkIdx < 0 || trkIdx >= tracklist.tracks.size()) return;

    altmix_state = -1;

    ui->trknameLabel->setText(tracklist.tracks[trkIdx].title.length() ? tracklist.tracks[trkIdx].title : tracklist.tracks[trkIdx].filename);
    ui->commentTB->setText(tracklist.tracks[trkIdx].comment);
    curtrk = tracklist.tracks[trkIdx];

    // audio playback:
    QAudioFormat desiredFormat1 = getAudioFormat(tracklist.tracks[trkIdx].rate);

    QAudioDevice info1(
        ~devi ? QMediaDevices::audioOutputs()[devi]
        : QMediaDevices::defaultAudioOutput());
    if (!info1.isFormatSupported(desiredFormat1))
    {
        qWarning() << "Default format not supported, trying to use the nearest.";
        desiredFormat1 = info1.preferredFormat();
    }
    stop();
    audioOutput = new QAudioSink(info1, desiredFormat1, this);
    audioOutput->setVolume(1.0);
    fs::path srcfile = qstring_to_path(tracklist.thbgmFilePath);
    if (thver == 6)
        srcfile /= fs::path("bgm") / tracklist.tracks[trkIdx].filename.toStdString();
    st = new LoopedPCMStreamer(srcfile, tracklist.tracks[trkIdx]);

    audioOutput->start([this](QSpan<int16_t> s){ st->callback(s); });
    if (curtrk.altmix) {
        ui->altmixButton->show();
        ui->altmixButton->setChecked(false);
        altmix_state = 0;
        auto afaltmix = getAudioFormat(curtrk.altmix->rate);
        QAudioDevice adaltmix(
            ~devi ? QMediaDevices::audioOutputs()[devi]
            : QMediaDevices::defaultAudioOutput());
        if (!adaltmix.isFormatSupported(afaltmix))
        {
            qWarning() << "Default format not supported, trying to use the nearest.";
            afaltmix = adaltmix.preferredFormat();
        }
        audioOutput_alt = new QAudioSink(adaltmix, afaltmix, this);
        audioOutput_alt->setVolume(0.);
        st_alt = new LoopedPCMStreamer(srcfile, *curtrk.altmix);
        audioOutput_alt->start([this](QSpan<int16_t> s){ st_alt->callback(s); });
    } else {
        ui->altmixButton->hide();
    }
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
    play(index.data(Qt::UserRole + 1).toInt());
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

void MainWindow::switch_mix()
{
    if (altmix_state == -1 || ui->pauseButton->isChecked()) return;
    auto *old_st = altmix_state ? st_alt : st;
    auto *new_st = altmix_state ? st : st_alt;
    auto *old_ao = altmix_state ? audioOutput_alt : audioOutput;
    auto *new_ao = altmix_state ? audioOutput : audioOutput_alt;
    auto old_pos = old_st->pos_sample();
    auto new_pos = old_pos * (1. * new_ao->format().sampleRate() / old_ao->format().sampleRate());
    new_st->seek_sample(new_pos);
    old_ao->setVolume(0.);
    new_ao->setVolume(1.);
    altmix_state = 1 - altmix_state;
    ui->altmixButton->setChecked(altmix_state);
}

void MainWindow::on_pauseButton_clicked(bool checked)
{
    auto *ao = altmix_state == 1 ? audioOutput_alt : audioOutput;
    if (!ao) return;
    if (checked) ao->suspend();
    else ao->resume();
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
                       "TouHou BGM player for all platforms." + "\n\n" +
                       "https:://github.com/BearKidsTeam/thplayer");
}

#include "mainwindow.moc"
