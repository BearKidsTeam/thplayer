#ifndef BOUNDEDBUFFER_H
#define BOUNDEDBUFFER_H

#include <mutex>
#include <condition_variable>
#include <QObject>
#include <QIODevice>

class BoundedBuffer:public QIODevice
{
	private:
		char* buf;
		unsigned cap;
		unsigned l,r,cnt;
		bool bufopen;
		std::mutex lock;
		std::condition_variable write_ready,read_ready;
	public:
		BoundedBuffer(unsigned _cap=65536);
		virtual ~BoundedBuffer();
		qint64 readData(char *data,qint64 maxlen);
		qint64 writeData(const char *data,qint64 len);
		qint64 size()const;
		bool open(OpenMode mode);
		void close();
		bool isSequential()const;
};

#endif // BOUNDEDBUFFER_HPP
