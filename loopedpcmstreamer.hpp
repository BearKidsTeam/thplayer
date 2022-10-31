#ifndef LOOPEDPCMSTREAMER_HPP
#define LOOPEDPCMSTREAMER_HPP

#include <filesystem>

#include <QIODevice>

namespace fs = std::filesystem;

class LoopedPCMStreamer : public QIODevice
{
    Q_OBJECT

public:
    LoopedPCMStreamer(const fs::path &source, uint64_t offset, uint32_t length, uint32_t lppnt, QObject *parent = nullptr);
    ~LoopedPCMStreamer();

    bool open(QIODevice::OpenMode mode);
    void close();
    bool seek(qint64 pos);

    bool atEnd() const;
    qint64 bytesAvailable() const;
    bool isSequential() const;
    qint64 pos() const;
    qint64 size() const;

    void seek_sample(uint64_t pos);
    uint64_t pos_sample() const;
protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);
private:
    void *mapped;
    intptr_t fd;
#ifdef _WIN32
    intptr_t maph;
#endif
    fs::path srcpath;
    uint64_t mapped_offset;
    uint64_t mapped_length;
    uint64_t offset;
    uint64_t position;
    uint32_t nsamples;
    uint32_t loopsample;
};

#endif
