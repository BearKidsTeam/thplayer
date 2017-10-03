#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
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
    Ui::MainWindow *ui;
    FmtFile fmt;

private slots:
    // drag n drop
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent* event);
    void on_playButton_clicked();
};

#endif // MAINWINDOW_H
