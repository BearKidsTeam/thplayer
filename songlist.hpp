#ifndef SONGLIST_H
#define SONGLIST_H

#include <filesystem>
#include <QFile>
#include <QBuffer>
#include "thdatwrapper.hpp"

namespace fs = std::filesystem;

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
    bool LoadFile_th6(thDatWrapper *mdw, const fs::path &bgmdir);

private:

    bool fileLoaded = false;
    bool ignoreAnUint = false;
    bool SongListReadGroup(QBuffer *buf, song_t &song);
    uint32_t waveGetDataChunk(const fs::path &path);
    uint32_t waveGetSamplingRate(const fs::path &path);
    static unsigned BEu32b(QBuffer *buf);
    void LoadComment(thDatWrapper *datw);
};

#endif // SONGLIST_H
