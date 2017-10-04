#include "fmtfile.h"
#include <QFileInfo>
#include <QDebug>

FmtFile::FmtFile()
{
    // nothing...
}

FmtFile::FmtFile(QString filepath, bool ignoreAnUint)
{
    LoadFile(filepath, ignoreAnUint);
}

bool FmtFile::LoadFile(QString filepath, bool ignoreAnUint)
{
    QFileInfo fmtFileInfo(filepath);
    if (fmtFileInfo.exists() && fmtFileInfo.isFile()) {
        songCnt = 0;
        QFile fmtFile(fmtFileInfo.absoluteFilePath());
        fmtFile.open(QIODevice::ReadOnly);
        this->ignoreAnUint = ignoreAnUint;
        while(FmtFileReadGroup(fmtFile, songs[songCnt++])) {
            //qDebug() << songCnt;
        };
        songCnt--;
        fmtFile.close();
        fileLoaded = true;
        return true;
    } else {
        return false;
    }
}

bool FmtFile::FmtFileReadGroup(QFile& file, song_t &song)
{
    for (int i = 0; i < 16; ++i) {
        if (!file.getChar(&song.name[i])) return false;
    }
    song.start = BEu32b(file);
    if(!ignoreAnUint) {
        song.length = BEu32b(file);
        song.loopStart = BEu32b(file);
        file.seek(file.pos() + 8);
    } else {
        file.seek(file.pos() + 4);
        song.loopStart = BEu32b(file);
        song.length = BEu32b(file);
        file.seek(file.pos() + 4);
    }
    song.rate = BEu32b(file);
    for(int i = 1;i <= 12; ++i) {
        if(!file.getChar(0)) return false;
    }
    return true;
}

unsigned FmtFile::BEu32b(QFile& file)
{
    unsigned char c[4];unsigned res=0;
    for(int i=0;i<4;++i) file.getChar((char*)&c[i]);
    for(int i=3;i>=0;--i) res*=256, res+=c[i];
    return res;
}
