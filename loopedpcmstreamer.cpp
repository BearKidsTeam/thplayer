#include "loopedpcmstreamer.hpp"

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#endif

#include <QDebug>

const int BPS = 2; //s16le
const int NCHAN = 2; //stereo
const int BPCS = BPS * NCHAN;

LoopedPCMStreamer::LoopedPCMStreamer(const fs::path &source, const track_t &trk, QObject *parent) :
    QObject(parent),
    mapped(nullptr),
    fd(-1),
    srcpath(source),
    offset(trk.start),
    nsamples(trk.length / BPCS),
    loopsample(trk.loopStart / BPCS)
{
    load();
}

LoopedPCMStreamer::~LoopedPCMStreamer()
{
    unload();
}

void LoopedPCMStreamer::load()
{
    //TODO: check file has sufficient data remaining for mapped_length
#ifndef _WIN32
    fd = open(srcpath.c_str(), 0);
    if (!~fd) return;

    const long pagesize = sysconf(_SC_PAGESIZE);
    uint64_t offset_aligned = offset / pagesize * pagesize;
    mapped_offset = offset - offset_aligned;

    const uint64_t blength = 1ULL * nsamples * BPCS;
    mapped_length = blength + mapped_offset;

    mapped = mmap(mapped, mapped_length, PROT_READ, MAP_PRIVATE, fd, offset_aligned);
    if (mapped == (void *) -1)
    {
        mapped = nullptr;
        unload();
        return;
    }
#else
    fd = (intptr_t)CreateFileW(srcpath.make_preferred().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if ((void*)fd == INVALID_HANDLE_VALUE)
    {
        fd = -1;
        unload();
        return;
    }

    maph = (intptr_t)CreateFileMappingA((void*)fd, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!maph)
    {
        unload();
        return;
    }

    SYSTEM_INFO snfo;
    GetSystemInfo(&snfo);
    const long pagesize = snfo.dwAllocationGranularity;
    const uint64_t blength = 1ULL * nsamples * BPCS;
    uint64_t offset_aligned = offset / pagesize * pagesize;
    mapped_offset = offset - offset_aligned;

    mapped_length = blength + mapped_offset;
    mapped = MapViewOfFile((void*)maph, FILE_MAP_READ, offset_aligned >> 32, offset_aligned & ((1LL << 33) - 1), mapped_length);
    if (!mapped)
    {
        unload();
        return;
    }
#endif

    // pre-load the entire file
    volatile uint8_t dummy = 0;
    for (uint64_t i = 0; i < mapped_length; ++i)
	dummy ^= ((uint8_t*)mapped)[i];

    position = 0;
}

void LoopedPCMStreamer::unload()
{
#ifndef _WIN32
    if (mapped)
        munmap(mapped, mapped_length);
    if (~fd)
        close(fd);
#else
    if (mapped) UnmapViewOfFile(mapped);
    if (maph) CloseHandle((void*)maph);
    if (fd) CloseHandle((void*)fd);
#endif
}

void LoopedPCMStreamer::callback(QSpan<int16_t> samples) {
    size_t maxSize = samples.size_bytes();
    uint8_t *data = reinterpret_cast<uint8_t*>(samples.data());
    uint64_t samplesread = maxSize / BPCS;
    uint64_t bytesread = samplesread * BPCS;
    uint64_t p = position.load();
    if (p + samplesread < nsamples)
    {
        memcpy(data, (char *)mapped + p * BPCS + mapped_offset, bytesread);
        p += samplesread;
    }
    else
    {
        uint64_t samplesremaining = nsamples - p;
        uint64_t sampleswarpped = samplesread - samplesremaining;
        memcpy(data, (char *)mapped + p * BPCS + mapped_offset, samplesremaining * BPCS);
        memcpy(data + samplesremaining * BPCS, (char *) mapped + mapped_offset + loopsample * BPCS, sampleswarpped * BPCS);
        p = loopsample + sampleswarpped;
        Q_EMIT warped();
    }
    position.store(p);
}

const void* LoopedPCMStreamer::get_data() const {
    return (void*)((char*)mapped + mapped_offset);
}

void LoopedPCMStreamer::seek_sample(uint64_t pos)
{
    position.store(pos);
}

uint64_t LoopedPCMStreamer::pos_sample() const
{
    return position.load();
}

uint64_t LoopedPCMStreamer::length_sample() const { return nsamples; }
