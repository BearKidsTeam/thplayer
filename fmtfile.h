#ifndef FMTFILE_H
#define FMTFILE_H

#include <QFile>

typedef struct FmtSong {
    char name[16];
    unsigned start;
    unsigned loopStart;
    unsigned length;
    unsigned rate;
} song_t;

class FmtFile
{
public:
    song_t songs[50];
    int songCnt = 0;
    QString thbgmFilePath = nullptr;
    FmtFile();
    FmtFile(QString filepath, bool ignoreAnUint = false);
    bool LoadFile(QString filepath, bool ignoreAnUint = false);

private:
    bool fileLoaded = false;
    bool ignoreAnUint = false;
    bool FmtFileReadGroup(QFile& file, song_t& song);
    static unsigned BEu32b(QFile& file);
};

#endif // FMTFILE_H
