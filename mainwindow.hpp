#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QAudioSink>
#include <QBuffer>
#include <QTimer>
#include <QSlider>
#include <QCommandLineParser>
#include <QFileInfo>
#include <thread>
#include "songlist.hpp"
#include "thdatwrapper.hpp"

namespace Ui
{
class MainWindow;
}

class QClickableSlider: public QSlider
{
    Q_OBJECT
public:
    explicit QClickableSlider(QWidget *parent = 0): QSlider(parent) {}
protected:
    void mouseReleaseEvent(QMouseEvent *e)
    {
        QSlider::mouseReleaseEvent(e);
        if (e->buttons()^Qt::LeftButton)
        {
            double p = e->pos().x() / (double)width();
            setValue(p * (maximum() - minimum()) + minimum());
            emit sliderReleased();
        }
    }
};

class LoopedPCMStreamer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    bool LoadFile(QString filepath);
    bool SetupSongList();
    bool args(QCommandLineParser &p);
    ~MainWindow();

private:
    unsigned loopStart;
    Ui::MainWindow *ui;
    SongList songs;
    song_t cursong;
    LoopedPCMStreamer *st = nullptr;
    QAudioSink *audioOutput = nullptr;
    QAudioFormat getAudioFormat(unsigned rate);
    QTimer *timer;
    thDatWrapper *datw;
    QCommandLineParser *argp;
    int devi;
    int thver;
    int thVersionDetect(QFileInfo fi);
    void setPlayListTableHeader();
    void play(int index = -1);
    void stop();

private slots:
    // drag n drop
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void updateWidgets();
    void seek();
    void on_playlistTable_doubleClicked(const QModelIndex &index);
    void on_loopButton_clicked();
    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void on_pauseButton_clicked(bool checked);
    void on_action_Open_triggered();
    void on_actionAbout_Qt_triggered();
    void on_action_About_triggered();
};

#endif // MAINWINDOW_H
