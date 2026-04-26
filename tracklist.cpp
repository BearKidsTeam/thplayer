#include "tracklist.hpp"
#include <QFileInfo>
#include <QDebug>
#include <QByteArray>
#include <QStringConverter>
#include <QStringDecoder>
#include <QRegularExpression>
#include <map>
#include <fstream>

TrackList::TrackList()
{
    // nothing...
}

bool TrackList::LoadFile(QString filepath, bool ignoreAnUint)
{
    //left to be implemented by BLBLB
    return false;
}
bool TrackList::LoadFile(QBuffer *buf, bool ignoreAnUint)
{
    tracks.clear();
    this->ignoreAnUint = ignoreAnUint;
    while (auto tr = TrackListReadGroup(buf)) tracks.push_back(tr.value());
    if (tracks.size() > 0 && tracks[0].filename.startsWith("th13_")) //th13 spirit world shenanigans
        for (size_t i = 0; i < tracks.size() - 1; ++i) {
            if (tracks[i].filename.size() < 4) continue;
            QString n = tracks[i].filename.chopped(4);
            if (tracks[i + 1].filename.startsWith(n)) {
                tracks[i].altmix = &tracks[i + 1];
                tracks[i + 1].is_altmix = true;
            }
        }
    fileLoaded = true;
    return true;
}
bool TrackList::LoadFile(thDatWrapper *datw, bool ignoreAnUint)
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
bool TrackList::LoadFile_th6(thDatWrapper *mdw, const fs::path &bgmdir)
{
    tracks.clear();
    for (int i = 0; i < 17; ++i)
    {
        track_t tr;
        QString posf = QString("th06_%1.pos").arg(i + 1, 2, 10, QLatin1Char('0'));
        QString wavf = QString("th06_%1.wav").arg(i + 1, 2, 10, QLatin1Char('0'));
        fs::path wavp = (bgmdir / "../bgm").lexically_normal() / wavf.toStdString();
        ssize_t psz = mdw->getFileSize(posf.toStdString().c_str());
        if (!~psz) return false;
        QByteArray arr = QByteArray((int)(psz + 1), '\0');
        mdw->getFile(posf.toStdString().c_str(), arr.data());
        uint32_t *lppt = (uint32_t*) arr.data();
        tr.filename = wavf;
        tr.start = waveGetDataChunk(wavp);
        tr.rate = waveGetSamplingRate(wavp);
        tr.loopStart = tr.start + lppt[0] * 4;
        tr.length = tr.start + lppt[1] * 4;
        tr.altmix = nullptr;
        tr.is_altmix = false;
        tracks.push_back(tr);
    }
    LoadComment(mdw);
    return true;
}
void TrackList::LoadComment(thDatWrapper *datw)
{
    ssize_t scmt = datw->getFileSize(isTrial ? "musiccmt_tr.txt" : "musiccmt.txt");
    if (!~scmt)return;
    QByteArray *arr = new QByteArray((int)(scmt + 1), '\0');
    char *dat = arr->data();
    datw->getFile(isTrial ? "musiccmt_tr.txt" : "musiccmt.txt", dat);
    QString s;
    if (QStringConverter::availableCodecs().contains("Shift_JIS"))
        s = QStringDecoder("Shift_JIS")(dat);
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
    for (size_t i = 0; i < tracks.size(); ++i)
        if (map.find(tracks[i].filename) != map.end())
        {
            tracks[i].title = map[tracks[i].filename].first;
            tracks[i].comment = map[tracks[i].filename].second;
        }
    delete arr;
}

std::optional<track_t> TrackList::TrackListReadGroup(QBuffer *buf)
{
    if (buf->size() - buf->pos() < 52) return {};
    char name[16];
    for (int i = 0; i < 16; ++i)
    {
        if (!buf->getChar(&name[i])) return {};
    }
    track_t track;
    track.altmix = nullptr;
    track.is_altmix = false;
    track.filename = QString(name);
    track.start = BEu32b(buf);
    if (!ignoreAnUint)
    {
        track.length = BEu32b(buf);
        track.loopStart = BEu32b(buf);
        buf->seek(buf->pos() + 8);
    }
    else
    {
        buf->seek(buf->pos() + 4);
        track.loopStart = BEu32b(buf);
        track.length = BEu32b(buf);
        buf->seek(buf->pos() + 4);
    }
    track.rate = BEu32b(buf);
    track.title = track.comment = "";
    buf->seek(buf->pos() + 12);
    return track;
}

unsigned TrackList::BEu32b(QBuffer *buf)
{
    unsigned char c[4];
    unsigned res = 0;
    for (int i = 0; i < 4; ++i) buf->getChar((char *)&c[i]);
    for (int i = 3; i >= 0; --i) res *= 256, res += c[i];
    return res;
}

uint32_t TrackList::waveGetDataChunk(const fs::path &path)
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
uint32_t TrackList::waveGetSamplingRate(const fs::path &path)
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
