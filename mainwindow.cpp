#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <cstdio>
#include <cstring>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QMimeData>
#ifdef _WIN32
#define NOMINMAX //Windows API breaks STL, shit.
#include <Windows.h>
#endif

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	setPlayListTableHeader();
	ui->playlistTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->playlistTable->setSortingEnabled(false);
	datw=nullptr;
	streamerThread = nullptr;
	timer=new QTimer();
	timer->setInterval(100);
	connect(timer,SIGNAL(timeout()),this,SLOT(updateWidgets()));
	connect(ui->progressslider,SIGNAL(sliderReleased()),this,SLOT(seek()));
	timer->start();
}

/*!    \brief  Load file.
 *
 *    Loads the game folder or any file in the folder.
 *    Determine game version.
 *    Returns true on success.
 */
bool MainWindow::LoadFile(QString filepath)
{
	QUrl url(filepath);
	if(QFileInfo(filepath).isFile())
		url=url.adjusted(QUrl::RemoveFilename);
	QUrl bgmurl=QUrl(url.url()+"/thbgm.dat");
	songs.thbgmFilePath=bgmurl.url();
	if(!QFileInfo(bgmurl.url()).isFile())
		return false;
	QDir gamedir=QDir(url.url());
	QStringList sl;
	sl<<"*.dat";
	QFileInfoList fil=gamedir.entryInfoList(sl,QDir::NoFilter,QDir::Name);
	QString datf="";
	int ver=-1;
	for(auto&i:fil)
	{
		if(~(ver=thVersionDetect(i.fileName())))
		{datf=i.absoluteFilePath();break;}
	}
	if(!datf.length())return false;
#ifdef _WIN32
	std::wstring ws=datf.toStdWString();
	const wchar_t* s=ws.c_str();
	int size=WideCharToMultiByte(CP_OEMCP,WC_NO_BEST_FIT_CHARS,s,-1,0,0,0,0);
	char* c=(char*)calloc(size,sizeof(char));
	WideCharToMultiByte(CP_OEMCP,WC_NO_BEST_FIT_CHARS,s,-1,c,size,0,0);
	bgmdat=_wfopen(songs.thbgmFilePath.toStdWString().c_str(),L"rb");
#else
	char* c=(char*)calloc(datf.toStdString().length()+1,sizeof(char));
	strncpy(c,datf.toStdString().c_str(),datf.toStdString().length()+1);
	bgmdat=fopen(songs.thbgmFilePath.toStdString().c_str(),"rb");
#endif
	stop();
	if(datw)delete datw;
	datw=new thDatWrapper(c,ver);
	if(ver>50)ver/=10;//95,125,128,143 etc
	songs.LoadFile(datw,ver<13?true:false);
	free(c);
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
	for (int i = 0; i < songs.songCnt; i++) {
		song_t* song = &songs.songs[i];
		QString fileName(song->filename);

		QTableWidgetItem *itemTitle = new QTableWidgetItem(song->title);
		QTableWidgetItem *itemName = new QTableWidgetItem(fileName);
		QTableWidgetItem *itemStart = new QTableWidgetItem("0x"+QString::number(song->start,16));
		QTableWidgetItem *itemLpSt = new QTableWidgetItem("0x"+QString::number(song->loopStart,16));
		QTableWidgetItem *itemLen = new QTableWidgetItem("0x"+QString::number(song->length,16));
		QTableWidgetItem *itemRate = new QTableWidgetItem(QString::number(song->rate));
		ui->playlistTable->setItem(i, 0, itemTitle);
		ui->playlistTable->setItem(i, 1, itemName);
		ui->playlistTable->setItem(i, 2, itemStart);
		ui->playlistTable->setItem(i, 3, itemLpSt);
		ui->playlistTable->setItem(i, 4, itemLen);
		ui->playlistTable->setItem(i, 5, itemRate);
	}
	//ui->playlistTable->setSortingEnabled(true);
	return true;
}

MainWindow::~MainWindow()
{
	stop();
	delete ui;
	if(bgmdat)fclose(bgmdat);
	if(datw)delete datw;
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
	if(!audioBuffer)return;
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
	LoadFile(QFileDialog::getExistingDirectory(this,"Select game directory"));
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

int MainWindow::thVersionDetect(QString str)
{
	//QRegExp r06("[Mm][Dd].[Dd][Aa][Tt]");
	//if(r06.indexIn(str))return 6;
	QRegExp rx("^[Tt][Hh](\\d{2,3})");
	int pos = rx.indexIn(str);
	if (pos == -1) return -1;
	QStringList list = rx.capturedTexts();
	QString ret = rx.cap(1);
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
	int songIdx;
	if (index != -1) songIdx = index;
	if (songIdx < 0 || songIdx >= songs.songCnt) return;

	ui->songnameLabel->setText(songs.songs[songIdx].title.length()?songs.songs[songIdx].title:songs.songs[songIdx].filename);
	ui->commentTB->setText(songs.songs[songIdx].comment);
	cursong=songs.songs[songIdx];

	// audio playback:
	QAudioFormat desiredFormat1 = getAudioFormat(songs.songs[songIdx].rate);

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
		size_t byte_expected=std::min((long)buf_size,(long)cursong.length-songpos);
		size_t sz=fread(buf,1,byte_expected,bgmdat);
		audioBuffer->write(buf,sz);
		if((unsigned)ftell(bgmdat)>=cursong.start+cursong.length)//also increment loop counter here
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
