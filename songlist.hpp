#ifndef SONGLIST_H
#define SONGLIST_H

#include <QFile>
#include <QBuffer>
#include "thdatwrapper.hpp"

struct song_t
{
    QString filename, title, comment;
    unsigned start;
    unsigned loopStart;
    unsigned length;
    unsigned rate;
};

class SongList
{
public:
    song_t songs[50];
    int songCnt = 0;
    QString thbgmFilePath = nullptr;
    bool isTrial = false;
    SongList();
    bool LoadFile(QString filepath, bool ignoreAnUint = false);
    bool LoadFile(QBuffer *buf, bool ignoreAnUint = false);
    bool LoadFile(thDatWrapper *datw, bool ignoreAnUint = false);

private:

    bool fileLoaded = false;
    bool ignoreAnUint = false;
    bool SongListReadGroup(QBuffer *buf, song_t &song);
    static unsigned BEu32b(QBuffer *buf);
    void LoadComment(thDatWrapper *datw);
};

#endif // SONGLIST_H
