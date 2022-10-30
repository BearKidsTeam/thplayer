#include "mainwindow.hpp"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("thplayer");
    a.setApplicationVersion("0.17");
    QCommandLineParser p;
    p.addOption({{"l", "list-devices"}, "Print the list of audio output devices and exit."});
    p.addOption({{"d", "device"}, "Set the desired audio output device.", "Device ID"});
    p.addPositionalArgument("path", "The path to open.", "[path]");
    p.addVersionOption();
    p.addHelpOption();
    p.process(a);

    MainWindow w;
    if (w.args(p))return 0;
    w.show();

    return a.exec();
}
