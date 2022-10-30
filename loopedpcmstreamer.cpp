#include "loopedpcmstreamer.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <QDebug>

const int BPS = 2; //s16le
const int NCHAN = 2; //stereo
const int BPCS = BPS * NCHAN;

LoopedPCMStreamer::LoopedPCMStreamer(const fs::path &source, uint64_t offset, uint32_t length, uint32_t lppnt, QObject *parent) :
    QIODevice(parent),
    mapped(nullptr),
    fd(0),
    srcpath(source),
    offset(offset),
    nsamples(length / BPCS),
    loopsample(lppnt / BPCS)
{
}

LoopedPCMStreamer::~LoopedPCMStreamer()
{
    if (isOpen()) LoopedPCMStreamer::close();
}

bool LoopedPCMStreamer::open(QIODevice::OpenMode mode)
{
    if (mode != QIODevice::OpenModeFlag::ReadOnly)
        return false;
    QIODevice::open(mode);

    fd = ::open(srcpath.c_str(), 0);
    if (!~fd) return false;

    const long pagesize = sysconf(_SC_PAGESIZE);
    uint64_t offset_aligned = offset / pagesize * pagesize;
    mapped_offset = offset - offset_aligned;

    const uint64_t blength = 1ULL * nsamples * BPCS;
    mapped_length = blength + mapped_offset;

    mapped = mmap(mapped, mapped_length , PROT_READ, MAP_PRIVATE, fd, offset_aligned);
    if (mapped == (void*) -1)
    {
        mapped = nullptr;
        close();
        return false;
    }

    position = 0;
    
    return true;
}

void LoopedPCMStreamer::close()
{
    QIODevice::close();
    if (mapped)
        munmap(mapped, mapped_length);
    if (~fd)
        ::close(fd);
}

bool LoopedPCMStreamer::seek(qint64 pos)
{
    if (pos % BPCS != 0)
        return false;
    QIODevice::seek(pos);
    position = pos / BPCS % nsamples;
    return true;
}

bool LoopedPCMStreamer::atEnd() const
{
    return false; //it never ends!!!!
}

qint64 LoopedPCMStreamer::bytesAvailable() const
{
    return ~0ULL >> 1;
}

bool LoopedPCMStreamer::isSequential() const
{
    return false;
}

qint64 LoopedPCMStreamer::pos() const
{
    return position * BPCS;
}

qint64 LoopedPCMStreamer::size() const
{
    return ~0ULL >> 1;
}

qint64 LoopedPCMStreamer::readData(char *data, qint64 maxSize)
{
    uint64_t samplesread = maxSize / BPCS;
    uint64_t bytesread = samplesread * BPCS;
    if (position + samplesread < nsamples)
    {
        memcpy(data, (char*)mapped + position * BPCS + mapped_offset, bytesread);
        position += samplesread;
    }
    else
    {
        uint64_t samplesremaining = nsamples - position;
        uint64_t sampleswarpped = samplesread - samplesremaining;
        memcpy(data, (char*)mapped + position * BPCS + mapped_offset, samplesremaining * BPCS);
        memcpy(data + samplesremaining * BPCS, (char*) mapped + mapped_offset + loopsample * BPCS, sampleswarpped * BPCS);
        position = loopsample + sampleswarpped;
    }
    return bytesread;
}

qint64 LoopedPCMStreamer::writeData(const char *data, qint64 maxSize)
{
    return -1;

}
void LoopedPCMStreamer::seek_sample(uint64_t pos)
{
    position = pos;
    seek(position * BPCS);
}

uint64_t LoopedPCMStreamer::pos_sample() const
{
    return position;
}
