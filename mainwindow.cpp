#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <cstdio>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	playerThreadHandler = nullptr;
}

/*!    \brief  Load file.
 *
 *    Load any loadable file, thbgm.fmt and thbgm.dat for most of case.
 *    should try to auto detect touhou version to avoid pop-up.
 */
bool MainWindow::LoadFile(QString filepath)
{
	QFileInfo fileInfo(filepath);
	if (!fileInfo.exists() || !fileInfo.isFile()) {
		return false;
	}

	bool fmtIgnoreAnUint = true;
	int thVersion = thVersionDetect(filepath);
	enum ProcState { NOT_LOADED, PROCESSING, FILE_LOADED };
	ProcState fmtFileState = NOT_LOADED;
	ProcState datFileState = NOT_LOADED;
	QFileInfo fmtFileInfo, datFileInfo;
	QDir fileDir = fileInfo.absoluteDir();

	if (fileInfo.suffix() == tr("fmt")) {
		fmtFileInfo = fileInfo;
		fmtFileState = PROCESSING;
	} else if (fileInfo.suffix() == tr("dat")) {
		bool tryLoad = true;
		if (fileInfo.fileName() != tr("thbgm.dat")) {
			QMessageBox::StandardButton userClicked;
			userClicked = QMessageBox::information(this, "Info!", tr("The file you choosen seems not `thbgm.dat`, Still load this file?\n") +
												   tr("If you choose No, we will try to load `thbgm.dat` if file exist."),
												   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if (userClicked == QMessageBox::No) {
				tryLoad = false;
			}
		}
		if (tryLoad) {
			datFileInfo = fileInfo;
			datFileState = PROCESSING;
		}
	}

	do {
		switch (fmtFileState) {
			case NOT_LOADED:
				fmtFileInfo.setFile(fileDir, tr("thbgm.fmt")); // try load
				if (!fmtFileInfo.exists() || !fmtFileInfo.isFile()) {
					fmtFileInfo.setFile(QFileDialog::getOpenFileName(this,"Open fmt file"));
				}
				// fall through
			case PROCESSING:
				// TODO: if detect failed, ask for ignoreAnUint
				if (thVersion == -1) thVersion = thVersionDetect(fmtFileInfo.absoluteFilePath());
				if (thVersion == -1) {
					QMessageBox::StandardButton userClicked;
					userClicked = QMessageBox::information(this, "Info!", tr("Did the `thbgm.fmt` use the newer format?\n") +
														   tr("If you don't know what did this means, click Yes if Touhou version >= 13."),
														   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
					fmtIgnoreAnUint = (userClicked == QMessageBox::Yes);
				} else {
					fmtIgnoreAnUint = thVersion >= 13 ? true : false;
				}
				if (!LoadFmtFile(fmtFileInfo.absoluteFilePath(), fmtIgnoreAnUint)) {
					fmtFileState = NOT_LOADED;
					break;
				}
				fmtFileState = FILE_LOADED;
				// fall through
			case FILE_LOADED:
				ui->thnameLabel->setText(fmtFileInfo.fileName());
		}

		switch (datFileState) {
			case NOT_LOADED:
				datFileInfo.setFile(fileDir, tr("thbgm.dat")); // try load
				if (!datFileInfo.exists() || !datFileInfo.isFile()) {
					datFileInfo.setFile(QFileDialog::getOpenFileName(this,"Open dat file"));
				}
				// fall through
			case PROCESSING:
				// TODO: if detect failed, ask for ignoreAnUint
				fmt.thbgmFilePath = datFileInfo.absoluteFilePath();
				datFileState = FILE_LOADED;
				// fall through
			case FILE_LOADED:
				bgmdat=fopen(fmt.thbgmFilePath.toStdString().c_str(),"rb");
		}
	} while (fmtFileState != FILE_LOADED || datFileState != FILE_LOADED);

	return true;
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
			QTableWidgetItem *itemStart = new QTableWidgetItem("0x"+QString::number(song->start,16));
			QTableWidgetItem *itemLpSt = new QTableWidgetItem("0x"+QString::number(song->loopStart,16));
			QTableWidgetItem *itemLen = new QTableWidgetItem("0x"+QString::number(song->length,16));
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
	stop();
	delete ui;
	if(bgmdat)fclose(bgmdat);
}

void MainWindow::stop()
{
	loopEnabled = false;
	if(audio_buffer)audio_buffer->close();
	if(playerThreadHandler){
		playerThreadHandler->join();
		delete playerThreadHandler;
		playerThreadHandler = nullptr;
	}
	if (audioOutput) {
		audioOutput->stop();
		delete audioOutput;
		delete audio_buffer;
		audioOutput = nullptr;
		audio_buffer = nullptr;
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
	QList<QUrl> urls = event->mimeData()->urls();
	if(urls.isEmpty()) return;
	QString fileName = urls.first().toLocalFile();
	LoadFile(fileName);
}

void MainWindow::on_playButton_clicked()
{
	LoadFile(QFileDialog::getOpenFileName(this,"Open fmt or dat file"));
}

// Audio format which thbgm.dat used: S16_LE 44100 stereo
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

int MainWindow::thVersionDetect(QString str)
{
	QRegExp rx("th(\\d{2})");
	int pos = rx.indexIn(str);
	if (pos == -1) return -1;
	QStringList list = rx.capturedTexts();
	QString ret = rx.cap(1);
	return ret.toInt();
}

void MainWindow::setPlayListTableHeader()
{
	ui->playlistTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
	ui->playlistTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Start"));
	ui->playlistTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Loop"));
	ui->playlistTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Length"));
	ui->playlistTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Rate"));
}

void MainWindow::play(int index)
{
	if (index != -1) songIdx = index;
	if (songIdx < 0 || songIdx >= fmt.songCnt) return;

	if (audioOutput != nullptr) {
		audioOutput->stop();
		delete audioOutput;
		audioOutput = nullptr;
	}

	ui->songnameLabel->setText(fmt.songs[songIdx].name);

	// audio playback:
	QAudioFormat desiredFormat1 = getAudioFormat(fmt.songs[songIdx].rate);

	// TODO:
	// output devices from QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)
	QAudioDeviceInfo info1(QAudioDeviceInfo::defaultOutputDevice());
	if (!info1.isFormatSupported(desiredFormat1)) {
		qWarning() << "Default format not supported, trying to use the nearest.";
		desiredFormat1 = info1.preferredFormat();
	}
	stop();
	audioOutput = new QAudioOutput(desiredFormat1, this);
	audioOutput->setVolume(1.0);
	audio_buffer=new BoundedBuffer(262144);
	audio_buffer->open(QIODevice::ReadWrite);

	audioOutput->start(audio_buffer);
	loopEnabled = true;
	playerThreadHandler = new std::thread(&MainWindow::playerThread,this);
}

void MainWindow::on_playlistTable_doubleClicked(const QModelIndex &index)
{
	play(index.row());
}

void MainWindow::playerThread()
{
	static const size_t buf_size=2048;
	char* buf=new char[buf_size];
	fseek(bgmdat,fmt.songs[songIdx].start,SEEK_SET);
	for(int ll=fmt.songs[songIdx].loopStart;loopEnabled&&ll>0;ll-=buf_size)
	{
		size_t sz=fread(buf,1,std::min(ll,(int)buf_size),bgmdat);
		if(!loopEnabled)break;
		audio_buffer->write(buf,sz);
	}
	while(loopEnabled)
	{
		fseek(bgmdat,fmt.songs[songIdx].start+fmt.songs[songIdx].loopStart,SEEK_SET);
		for(int ll=fmt.songs[songIdx].length-fmt.songs[songIdx].loopStart;loopEnabled&&ll>0;ll-=buf_size)
		{
			size_t sz=fread(buf,1,std::min(ll,(int)buf_size),bgmdat);
			if(!loopEnabled)break;
			audio_buffer->write(buf,sz);
		}
	}
}

void MainWindow::on_loopButton_clicked()
{
	// TODO: Lazy work, NOT thread safe!
	loopEnabled = !loopEnabled;
	ui->loopButton->setText(loopEnabled ? tr("Loop: On") : tr("Loop: Off"));
}
