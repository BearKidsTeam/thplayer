#include "thdatwrapper.hpp"
#include <cstdlib>
#include <cstring>

thDatWrapper::thDatWrapper(const char *datpath,unsigned ver)
{
	thtk_error_t *e=NULL;
	datf=thtk_io_open_file(datpath,"rb",&e);
	thtk_error_free(&e);
	dat=thdat_open(ver,datf,&e);
	if(!dat)
	//just try the latest supported version instead
	{
		thtk_error_free(&e);
		dat=thdat_open(16,datf,&e);
	}
	thtk_error_free(&e);
}
ssize_t thDatWrapper::getFileSize(const char *path)
{
	thtk_error_t *e=NULL;
	int en=thdat_entry_by_name(dat,path,&e);
	thtk_error_free(&e);
	if(!~en)return -1;
	return thdat_entry_get_size(dat,en,&e);
}
int thDatWrapper::getFile(const char *path,char *dest)
{
	thtk_error_t *e=NULL;
	int en=thdat_entry_by_name(dat,path,&e);
	thtk_error_free(&e);
	if(!~en)return -1;
	ssize_t sz=thdat_entry_get_size(dat,en,&e);
	thtk_error_free(&e);
	void *m=malloc(sz);
	thtk_io_t* mf=thtk_io_open_memory(m,sz,&e);
	thtk_error_free(&e);
	ssize_t r=thdat_entry_read_data(dat,en,mf,&e);
	if(!~r)return -1;
	thtk_error_free(&e);
	memcpy(dest,m,sz);
	thtk_io_close(mf);
	return 0;
}
thDatWrapper::~thDatWrapper()
{
	thtk_error_t *e=NULL;
	thdat_close(dat,&e);
	thtk_error_free(&e);
	thtk_io_close(datf);
}
