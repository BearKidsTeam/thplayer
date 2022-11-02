#include "songlist.hpp"
#include <QFileInfo>
#include <QDebug>
#include <QByteArray>
#include <QTextCodec>
#include <QRegularExpression>
#include <map>
#include <fstream>

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
    while (SongListReadGroup(buf, songs[songCnt++]))
    {
        //qDebug() << songCnt;
    };
    songCnt--;
    fileLoaded = true;
    return true;
}
bool SongList::LoadFile(thDatWrapper *datw, bool ignoreAnUint)
{
    ssize_t sfmt = datw->getFileSize(isTrial ? "thbgm_tr.fmt" : "thbgm.fmt");
    bool is_al = false;
    if (!~sfmt)
    {
        sfmt = datw->getFileSize("albgm.fmt");
        if (~sfmt) is_al = true;
    }
    if (!~sfmt) return false;
    QByteArray *arr = new QByteArray((int)(sfmt + 1), '\0');
    char *dat = arr->data();
    if (is_al)
        datw->getFile("albgm.fmt", dat);
    else
        datw->getFile(isTrial ? "thbgm_tr.fmt" : "thbgm.fmt", dat);
    QBuffer *buf = new QBuffer(arr, nullptr);
    buf->open(QIODevice::ReadOnly);
    LoadFile(buf, ignoreAnUint);
    delete buf;
    delete arr;
    LoadComment(datw);
    return true;
}
bool SongList::LoadFile_th6(thDatWrapper *mdw, const fs::path &bgmdir)
{
    songCnt = 17;
    for (int i = 0; i < songCnt; ++i)
    {
        QString posf = QString("th06_%1.pos").arg(i + 1, 2, 10, QLatin1Char('0'));
        QString wavf = QString("th06_%1.wav").arg(i + 1, 2, 10, QLatin1Char('0'));
        fs::path wavp = (bgmdir / "../bgm").lexically_normal() / wavf.toStdString();
        ssize_t psz = mdw->getFileSize(posf.toStdString().c_str());
        if (!~psz) return false;
        QByteArray arr = QByteArray((int)(psz + 1), '\0');
        mdw->getFile(posf.toStdString().c_str(), arr.data());
        uint32_t *lppt = (uint32_t*) arr.data();
        songs[i].filename = wavf;
        songs[i].start = waveGetDataChunk(wavp);
        songs[i].rate = waveGetSamplingRate(wavp);
        songs[i].loopStart = songs[i].start + lppt[0] * 4;
        songs[i].length = songs[i].start + lppt[1] * 4;
    }
    LoadComment(mdw);
    return true;
}
void SongList::LoadComment(thDatWrapper *datw)
{
    ssize_t scmt = datw->getFileSize(isTrial ? "musiccmt_tr.txt" : "musiccmt.txt");
    if (!~scmt)return;
    QByteArray *arr = new QByteArray((int)(scmt + 1), '\0');
    char *dat = arr->data();
    datw->getFile(isTrial ? "musiccmt_tr.txt" : "musiccmt.txt", dat);
    QString s = QTextCodec::codecForName("Shift-JIS")->toUnicode(*arr);
    std::map<QString, std::pair<QString, QString>> map;
    QStringList sl = s.split('\n');
    for (auto &i : sl)i = i.trimmed();
    std::pair<QString, QString> *pcur = nullptr;
    for (auto &i : sl)
    {
        if (!i.length()) continue;
        if (i[0] == '@')
        {
            QString fn = i.mid(i.indexOf('/') + 1);
            if (fn.endsWith(".mid") || fn.endsWith(".wav"))
                fn = fn.left(fn.length() - 4);
            fn += ".wav";
            pcur = &map[fn];
        }
        else if (i[0] != '#' && i[0] != '\0')
        {
            if (!pcur)continue;
            if (!pcur->first.length())
            {
                QRegularExpression re("^No\\.\\s*\\d*\\s*");
                auto rem = re.match(i);
                if (rem.hasMatch())
                {
                    pcur->first = i.right(i.length() - rem.capturedLength(0));
                    continue;
                }
            }
            if (pcur->second.length())pcur->second += '\n';
            pcur->second += i;
        }
    }
    for (int i = 0; i < songCnt; ++i)
        if (map.find(songs[i].filename) != map.end())
        {
            songs[i].title = map[songs[i].filename].first;
            songs[i].comment = map[songs[i].filename].second;
        }
    delete arr;
}

bool SongList::SongListReadGroup(QBuffer *buf, song_t &song)
{
    if (buf->size() - buf->pos() < 52)return false;
    char name[16];
    for (int i = 0; i < 16; ++i)
    {
        if (!buf->getChar(&name[i])) return false;
    }
    song.filename = QString(name);
    song.start = BEu32b(buf);
    if (!ignoreAnUint)
    {
        song.length = BEu32b(buf);
        song.loopStart = BEu32b(buf);
        buf->seek(buf->pos() + 8);
    }
    else
    {
        buf->seek(buf->pos() + 4);
        song.loopStart = BEu32b(buf);
        song.length = BEu32b(buf);
        buf->seek(buf->pos() + 4);
    }
    song.rate = BEu32b(buf);
    song.title = song.comment = "";
    buf->seek(buf->pos() + 12);
    return true;
}

unsigned SongList::BEu32b(QBuffer *buf)
{
    unsigned char c[4];
    unsigned res = 0;
    for (int i = 0; i < 4; ++i) buf->getChar((char *)&c[i]);
    for (int i = 3; i >= 0; --i) res *= 256, res += c[i];
    return res;
}

uint32_t SongList::waveGetDataChunk(const fs::path &path)
{
    std::fstream wavef(path);
    wavef.ignore(12);
    uint32_t ret = 12;
    char fourcc[4];
    while (wavef.good())
    {
        uint32_t chnklen = 0;
        wavef.read(fourcc, 4);
        wavef.read((char*)&chnklen, 4);
        ret += 8;
        if (!memcmp(fourcc, "data", 4)) return ret;
        ret += chnklen;
        wavef.ignore(chnklen);
    }
    return ~0U;
}
uint32_t SongList::waveGetSamplingRate(const fs::path &path)
{
    std::fstream wavef(path);
    wavef.ignore(12);
    uint32_t ret = 0;
    char fourcc[4];
    while (wavef.good())
    {
        uint32_t chnklen = 0;
        wavef.read(fourcc, 4);
        wavef.read((char*)&chnklen, 4);
        if (!memcmp(fourcc, "fmt ", 4))
        {
            wavef.ignore(4);
            wavef.read((char*)&ret, 4);
            return ret;
        }
        wavef.ignore(chnklen);
    }
    return ~0U;
}
