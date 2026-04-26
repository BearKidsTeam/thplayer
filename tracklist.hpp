#ifndef TRACKLIST_H
#define TRACKLIST_H

#include <filesystem>
#include <vector>
#include <optional>
#include <QFile>
#include <QBuffer>
#include "thdatwrapper.hpp"

namespace fs = std::filesystem;

struct track_t
{
    QString filename, title, comment;
    //all offsets here are in bytes, not samples
    unsigned start;
    unsigned loopStart;
    unsigned length;
    unsigned rate;
    track_t *altmix;
    bool is_altmix;
};

class TrackList
{
public:
    std::vector<track_t> tracks;
    QString thbgmFilePath;
    bool isTrial = false;
    TrackList();
    bool LoadFile(QString filepath, bool ignoreAnUint = false);
    bool LoadFile(QBuffer *buf, bool ignoreAnUint = false);
    bool LoadFile(thDatWrapper *datw, bool ignoreAnUint = false);
    bool LoadFile_th6(thDatWrapper *mdw, const fs::path &bgmdir);

private:

    bool fileLoaded = false;
    bool ignoreAnUint = false;
    std::optional<track_t> TrackListReadGroup(QBuffer *buf);
    uint32_t waveGetDataChunk(const fs::path &path);
    uint32_t waveGetSamplingRate(const fs::path &path);
    static uint32_t LEu32b(QBuffer *buf);
    static uint16_t LEu16b(QBuffer *buf);
    void LoadComment(thDatWrapper *datw);
};

#endif // TRACKLIST_H
