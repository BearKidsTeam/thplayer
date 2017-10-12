#ifndef THDATWRAPPER_H
#define THDATWRAPPER_H

#include <thtk-config/config.h>
#include <thtk/io.h>
#include <thtk/dat.h>

class thDatWrapper
{
	private:
		thdat_t* dat;
		thtk_io_t *datf,*memf;
	public:
		thDatWrapper(const char* datpath,unsigned ver);
		ssize_t getFileSize(const char* path);
		int getFile(const char* path,char* dest);
		~thDatWrapper();
};

#endif // THTKWRAPPER_H
