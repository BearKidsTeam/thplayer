#ifndef LOOPEDPCMSTREAMER_HPP
#define LOOPEDPCMSTREAMER_HPP

#include <QObject>
#include <QSpan>
// https://bugreports.qt.io/browse/QTBUG-73263
#include <atomic>
#include <filesystem>
#include "tracklist.hpp"

namespace fs = std::filesystem;

class LoopedPCMStreamer : public QObject
{
    Q_OBJECT

public:
    LoopedPCMStreamer(const fs::path &source, const track_t &trk, QObject *parent = nullptr);
    ~LoopedPCMStreamer();

    void seek_sample(uint64_t pos);
    uint64_t pos_sample() const;
    uint64_t length_sample() const;

    void callback(QSpan<int16_t> samples);

Q_SIGNALS:
    void warped();

private:
    void load();
    void unload();

    void *mapped;
    intptr_t fd;
#ifdef _WIN32
    intptr_t maph;
#endif
    fs::path srcpath;
    uint64_t mapped_offset;
    uint64_t mapped_length;
    uint64_t offset;
    // position in sample #
    std::atomic<uint64_t> position;
    uint32_t nsamples;
    uint32_t loopsample;
};

#endif
