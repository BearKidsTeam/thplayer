#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDir>
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
    QFileInfo fileInfo(filepath);
    if (fileInfo.exists() && fileInfo.isFile()) {
        qDebug() << fileInfo.absoluteDir();
        qDebug() << fileInfo.absoluteFilePath();
        qDebug() << fileInfo.absolutePath();
        qDebug() << fileInfo.absoluteDir().dirName();
        if (fileInfo.suffix() == tr("fmt")) {
            // TODO: ask for ignoreAnUint
            LoadFmtFile(filepath, true);
            return true;
        }
    }
    return false;
}

bool MainWindow::LoadFmtFile(QString filepath, bool ignoreAnUint)
{
    if (fmt.LoadFile(filepath, ignoreAnUint)) {
        ui->playlistTable->clear();
        ui->playlistTable->setRowCount(fmt.songCnt);
        ui->playlistTable->setSortingEnabled(false);
        for (int i = 0; i < fmt.songCnt; i++) {
            song_t* song = &fmt.songs[i];
            QString fileName(song->name);

            QTableWidgetItem *itemName = new QTableWidgetItem(fileName);
            QTableWidgetItem *itemStart = new QTableWidgetItem(QString::number(song->start));
            QTableWidgetItem *itemLpSt = new QTableWidgetItem(QString::number(song->loopStart));
            QTableWidgetItem *itemLpLen = new QTableWidgetItem(QString::number(song->loopLen));
            QTableWidgetItem *itemRate = new QTableWidgetItem(QString::number(song->rate));
            //qDebug() << fileName;
            ui->playlistTable->setItem(i, 0, itemName);
            ui->playlistTable->setItem(i, 1, itemStart);
            ui->playlistTable->setItem(i, 2, itemLpSt);
            ui->playlistTable->setItem(i, 3, itemLpLen);
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
}

// drag n drop
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    //check droped file
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    // fmt file drop???
}

void MainWindow::on_playButton_clicked()
{
    LoadFile(QString("/home/blumia/Programs/WinePrograms/th16/thbgm.fmt"));
}
