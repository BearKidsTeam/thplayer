#include "songlist.hpp"
#include <QFileInfo>
#include <QDebug>
#include <QByteArray>
#include <QTextCodec>
#include <map>

SongList::SongList()
{
	// nothing...
}

bool SongList::LoadFile(QString filepath, bool ignoreAnUint)
{
	//left to be implemented by BLBLB
	return false;
}
bool SongList::LoadFile(QBuffer *buf, bool ignoreAnUint)
{
	songCnt = 0;
	this->ignoreAnUint = ignoreAnUint;
	while(SongListReadGroup(buf, songs[songCnt++])) {
		//qDebug() << songCnt;
	};
	songCnt--;
	fileLoaded = true;
	return true;
}
bool SongList::LoadFile(thDatWrapper *datw, bool ignoreAnUint)
{
	ssize_t sfmt=datw->getFileSize(isTrial ? "thbgm_tr.fmt" : "thbgm.fmt");
	bool is_al=false;
	if (!~sfmt)
	{
		sfmt=datw->getFileSize("albgm.fmt");
		if(~sfmt)is_al=true;
	}
	if(!~sfmt)return false;
	QByteArray *arr=new QByteArray((int)(sfmt+1),'\0');
	char* dat=arr->data();
	if (is_al)
		datw->getFile("albgm.fmt", dat);
	else
		datw->getFile(isTrial ? "thbgm_tr.fmt" : "thbgm.fmt", dat);
	QBuffer *buf=new QBuffer(arr,nullptr);
	buf->open(QIODevice::ReadOnly);
	LoadFile(buf,ignoreAnUint);
	delete buf;
	delete arr;
	LoadComment(datw);
	return true;
}
void SongList::LoadComment(thDatWrapper *datw)
{
	ssize_t scmt=datw->getFileSize(isTrial ? "musiccmt_tr.txt" : "musiccmt.txt");
	if(!~scmt)return;
	QByteArray *arr=new QByteArray((int)(scmt+1),'\0');
	char* dat=arr->data();
	datw->getFile(isTrial ? "musiccmt_tr.txt" : "musiccmt.txt", dat);
	QString s=QTextCodec::codecForName("Shift-JIS")->toUnicode(*arr);
	std::map<QString,std::pair<QString,QString>> map;
	QStringList sl=s.split('\n');
	for(auto&i:sl)i=i.trimmed();
	std::pair<QString,QString>* pcur=nullptr;
	for(auto&i:sl)
	{
		if(i[0]=='@')
		{
			QString fn=i.mid(i.indexOf('/')+1);
			if(fn.endsWith(".mid")||fn.endsWith(".wav"))
			fn=fn.left(fn.length()-4);
			fn+=".wav";
			pcur=&map[fn];
		}
		else if(i[0]!='#'&&i[0]!='\0')
		{
			if(!pcur)continue;
			if(!pcur->first.length())
			{
				if(!i.startsWith("No."))
				{
					if(i[0].unicode()!=0x266A)//'♪'
						pcur->first=i;
					else pcur->first=i.right(i.length()-1);
				}
			}
			if(pcur->second.length())pcur->second+='\n';
			pcur->second+=i;
		}
	}
	for(int i=0;i<songCnt;++i)
	if(map.find(songs[i].filename)!=map.end())
	{
		songs[i].title=map[songs[i].filename].first;
		songs[i].comment=map[songs[i].filename].second;
	}
	delete arr;
}

bool SongList::SongListReadGroup(QBuffer* buf, song_t &song)
{
	if(buf->size()-buf->pos()<52)return false;
	char name[16];
	for (int i = 0; i < 16; ++i) {
		if (!buf->getChar(&name[i])) return false;
	}
	song.filename=QString(name);
	song.start = BEu32b(buf);
	if(!ignoreAnUint) {
		song.length = BEu32b(buf);
		song.loopStart = BEu32b(buf);
		buf->seek(buf->pos() + 8);
	} else {
		buf->seek(buf->pos() + 4);
		song.loopStart = BEu32b(buf);
		song.length = BEu32b(buf);
		buf->seek(buf->pos() + 4);
	}
	song.rate = BEu32b(buf);
	song.title=song.comment="";
	buf->seek(buf->pos()+12);
	return true;
}

unsigned SongList::BEu32b(QBuffer *buf)
{
	unsigned char c[4];unsigned res=0;
	for(int i=0;i<4;++i) buf->getChar((char*)&c[i]);
	for(int i=3;i>=0;--i) res*=256, res+=c[i];
	return res;
}
