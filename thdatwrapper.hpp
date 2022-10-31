#ifndef THDATWRAPPER_H
#define THDATWRAPPER_H

#include <filesystem>

#include <thtk/thdat.h>
#include <thtk/io.h>
#include <thtk/dat.h>

namespace fs = std::filesystem;

class thDatWrapper
{
private:
    thdat_t *dat;
    thtk_io_t *datf;
public:
    thDatWrapper(const fs::path &datpath, unsigned ver);
    ssize_t getFileSize(const char *path);
    int getFile(const char *path, char *dest);
    ~thDatWrapper();
};

#endif // THTKWRAPPER_H
