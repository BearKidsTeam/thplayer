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
	streamerThread = nullptr;
	timer=new QTimer();
	timer->setInterval(100);
	connect(timer,SIGNAL(timeout()),this,SLOT(updateWidgets()));
	connect(ui->progressslider,SIGNAL(sliderReleased()),this,SLOT(seek()));
	timer->start();
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
#ifndef _WIN32
				bgmdat=fopen(fmt.thbgmFilePath.toStdString().c_str(),"rb");
#else

				bgmdat=_wfopen(fmt.thbgmFilePath.toStdWString().c_str(),L"rb");
#endif
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
	stopStreamer = true;
	if(audioBuffer)audioBuffer->close();
	if(streamerThread){
		streamerThread->join();
		delete streamerThread;
		streamerThread = nullptr;
	}
	if (audioOutput) {
		audioOutput->stop();
		delete audioOutput;
		delete audioBuffer;
		audioOutput = nullptr;
		audioBuffer = nullptr;
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

void MainWindow::updateWidgets()
{
	if(!bgmdat)return;
	if(!cursong.length)return;
	long pos=ftell(bgmdat)-(long)audioBuffer->size();
	if(pos<0)pos+=(-1*pos/(cursong.length-cursong.loopStart)+1)*(cursong.length-cursong.loopStart);
	long spos=pos-cursong.start;
	ui->progressslider->setValue((int)100.*spos/cursong.length);
}
void MainWindow::seek()
{
	if(!bgmdat)return;
	long spos=(long)(ui->progressslider->value()/100.*cursong.length);
	if(spos%4)spos-=spos%4;
	fseek(bgmdat,spos+cursong.start,SEEK_SET);
	audioBuffer->clear();
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
	int songIdx;
	if (index != -1) songIdx = index;
	if (songIdx < 0 || songIdx >= fmt.songCnt) return;

	ui->songnameLabel->setText(fmt.songs[songIdx].name);
	cursong=fmt.songs[songIdx];

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
	audioBuffer=new BoundedBuffer(32768);
	audioBuffer->open(QIODevice::ReadWrite);

	audioOutput->start(audioBuffer);
	stopStreamer=false;
	streamerThread = new std::thread(&MainWindow::audioStreamer,this);
}

void MainWindow::on_playlistTable_doubleClicked(const QModelIndex &index)
{
	play(index.row());
}

void MainWindow::audioStreamer()
{
	static const size_t buf_size=2048;
	char* buf=new char[buf_size];
	fseek(bgmdat,cursong.start,SEEK_SET);
	while(!stopStreamer)
	{
		long curpos=ftell(bgmdat);
		long songpos=curpos-cursong.start;
		size_t byte_expected=std::min((long)buf_size,cursong.length-songpos);
		size_t sz=fread(buf,1,byte_expected,bgmdat);
		audioBuffer->write(buf,sz);
		if(ftell(bgmdat)>=cursong.start+cursong.length)//also increment loop counter here
		fseek(bgmdat,cursong.start+cursong.loopStart,SEEK_SET);
	}
	delete[] buf;
}

void MainWindow::on_loopButton_clicked()
{
	// TODO: ???
	// ui->loopButton->setText(loopEnabled ? tr("Loop: On") : tr("Loop: Off"));
}

void MainWindow::on_prevButton_clicked()
{
	int r=ui->playlistTable->currentRow();
	int c=ui->playlistTable->currentColumn();
	int rc=ui->playlistTable->rowCount();
	r=(r+rc-1)%rc;
	ui->playlistTable->setCurrentCell(r,c);
	play(r);
}

void MainWindow::on_nextButton_clicked()
{
	int r=ui->playlistTable->currentRow();
	int c=ui->playlistTable->currentColumn();
	int rc=ui->playlistTable->rowCount();
	r=(r+1)%rc;
	ui->playlistTable->setCurrentCell(r,c);
	play(r);
}
