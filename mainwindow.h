#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>
#include <QTimer>
#include <thread>
#include "fmtfile.h"
#include "boundedbuffer.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	bool LoadFile(QString filepath);
	bool LoadFmtFile(QString filepath, bool ignoreAnUint);
	void playerThread();
	~MainWindow();

private:
	unsigned loopStart;
	Ui::MainWindow *ui;
	FmtFile fmt;
	FILE* bgmdat;
	std::thread *playerth;
	bool cont;
	int songIdx;
	QAudioOutput* audioOutput=nullptr;
	BoundedBuffer *audio_buffer=NULL;
	QAudioFormat getAudioFormat();
	void setPlayListTableHeader();
	void updateWidgets();
	void stop();

private slots:
	// drag n drop
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent* event);
	void on_playButton_clicked();
	void on_playlistTable_doubleClicked(const QModelIndex &index);
};

#endif // MAINWINDOW_H
