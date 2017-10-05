#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cstdio>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	playerth=NULL;
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
	cont=0;
	if(audio_buffer)audio_buffer->close();
	if(playerth){playerth->join();delete playerth;}
	if (audioOutput) {
		audioOutput->stop();
		delete audioOutput;
		delete audio_buffer;
		audioOutput=NULL;
		audio_buffer=NULL;
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
	LoadFile(QFileDialog::getOpenFileName(this,"Open fmt file"));
	bgmdat=fopen(fmt.thbgmFilePath.toStdString().c_str(),"rb");
}

// Audio format which thbgm.dat used: S16_LE 44100 stereo
QAudioFormat MainWindow::getAudioFormat()
{
	QAudioFormat audioFormat;
	audioFormat.setCodec("audio/pcm");
	audioFormat.setChannelCount(2);
	audioFormat.setSampleRate(fmt.songs[songIdx].rate);
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
	songIdx = index.row();

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
	stop();
	audioOutput = new QAudioOutput(desiredFormat1, this);
	audioOutput->setVolume(1.0);
	audio_buffer=new BoundedBuffer(262144);
	audio_buffer->open(QIODevice::ReadWrite);

	audioOutput->start(audio_buffer);
	cont=1;
	playerth=new std::thread(&MainWindow::playerThread,this);
}

void MainWindow::playerThread()
{
	static const size_t buf_size=2048;
	char* buf=new char[buf_size];
	fseek(bgmdat,fmt.songs[songIdx].start,SEEK_SET);
	for(int ll=fmt.songs[songIdx].loopStart;cont&&ll>0;ll-=buf_size)
	{
		size_t sz=fread(buf,1,std::min(ll,(int)buf_size),bgmdat);
		if(!cont)break;
		audio_buffer->write(buf,sz);
	}
	while(cont)
	{
		fseek(bgmdat,fmt.songs[songIdx].start+fmt.songs[songIdx].loopStart,SEEK_SET);
		for(int ll=fmt.songs[songIdx].length-fmt.songs[songIdx].loopStart;cont&&ll>0;ll-=buf_size)
		{
			size_t sz=fread(buf,1,std::min(ll,(int)buf_size),bgmdat);
			if(!cont)break;
			audio_buffer->write(buf,sz);
		}
	}
}
