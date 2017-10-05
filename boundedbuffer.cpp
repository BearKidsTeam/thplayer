#include "boundedbuffer.hpp"
#include <cstring>

BoundedBuffer::BoundedBuffer(unsigned _cap)
{
	capacity=_cap;cnt=l=r=0;
	buf=new char[capacity];
}
BoundedBuffer::~BoundedBuffer()
{
	delete[] buf;
}

qint64 BoundedBuffer::readData(char *data,qint64 maxlen)
{
	if(!bufopen)return 0;
	std::unique_lock<std::mutex> lck(lock);
	read_ready.wait(lck,[this]{return cnt!=0||!bufopen;});
	if(!bufopen){lck.unlock();return 0;}
	qint64 ret=maxlen;if(cnt<ret)ret=cnt;
	if(l+ret<capacity)
		memcpy(data,buf+l,ret);
	else
	{
		memcpy(data,buf+l,capacity-l);
		memcpy(data+capacity-l,buf,ret-capacity+l);
	}
	l+=ret;l%=capacity;
	cnt-=ret;
	lck.unlock();
	write_ready.notify_one();
	return ret;
}

qint64 BoundedBuffer::writeData(const char *data,qint64 len)
{
	if(!bufopen)return 0;
	std::unique_lock<std::mutex> lck(lock);
	write_ready.wait(lck,[this,len]{return cnt+len<capacity||!bufopen;});
	if(!bufopen){lck.unlock();return 0;}
	qint64 ret=len;if(cnt+ret>capacity)ret=capacity-cnt;
	if(r+ret<capacity)
		memcpy(buf+r,data,len);
	else
	{
		memcpy(buf+r,data,capacity-r);
		memcpy(buf,data+capacity-r,ret-capacity+r);
	}
	r+=ret;r%=capacity;
	cnt+=ret;
	lck.unlock();
	read_ready.notify_one();
	return ret;
}

bool BoundedBuffer::open(OpenMode mode)
{
	bufopen=true;
	return QIODevice::open(mode);
}
void BoundedBuffer::close()
{
	bufopen=false;
	read_ready.notify_all();
	write_ready.notify_all();
	QIODevice::close();
}

qint64 BoundedBuffer::size()const
{
	return cnt;
}

bool BoundedBuffer::isSequential()const
{
	return true;
}
