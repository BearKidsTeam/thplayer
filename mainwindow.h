#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QBuffer>
#include "fmtfile.h"

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
    ~MainWindow();

private:
    unsigned loopStart;
    Ui::MainWindow *ui;
    FmtFile fmt;
    QAudioOutput* audioOutput = nullptr;
    QBuffer songBuffer;
    QAudioFormat getAudioFormat();
    void setPlayListTableHeader();

private slots:
    // drag n drop
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);
    void on_playButton_clicked();
    void when_audioOutput_stateChanged(QAudio::State newState);
    void on_playlistTable_doubleClicked(const QModelIndex &index);
};

#endif // MAINWINDOW_H
