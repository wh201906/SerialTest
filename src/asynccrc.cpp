#include "asynccrc.h"

#include <QDebug>
#include <QFile>

AsyncCRC::AsyncCRC(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<AsyncCRC::CRCFileError>();
}

void AsyncCRC::loadFile(const QString &path)
{
    // call reset() outside for new file
    bool notifyState = m_notify;
    m_notify = false;
    qint64 threshold = 1024 * 1024 * 128;

    QFile file(path);
    if(!file.open(QFile::ReadOnly))
    {
        emit fileError(OpenFileError);
        return;
    }
    threshold = file.size() < threshold ? file.size() : threshold;
    char* dataBuf = new char[threshold + 16];
    qint64 size = 1;
    while(size != 0)
    {
        size = file.read(dataBuf, threshold);
        if(size == -1 || size == 0)
            break;
        else
            addData(dataBuf, size);
    }
    if(size == 0)
    {
        m_notify = notifyState;
        if(m_notify)
            emit result(m_crc);
    }
    else if(size == -1)
        emit fileError(ReadFileError);

    delete []dataBuf;
}

void AsyncCRC::addData(const char *data, qsizetype length)
{
    if(!m_isInitVal)
    {
        // revert
        m_crc ^= m_xorOut;
        if(m_refIn != m_refOut)
            m_crc = reflect(m_crc, m_width);
    }
    m_isInitVal = false;
    if(!m_refIn)
    {
        quint8 offset = m_width - 8;
        quint8 tmp;
        while(length--)
        {
            tmp = m_crc >> offset; // & 0xFF
            m_crc = (m_crc << 8) ^ m_table[0][tmp ^ (quint8)(*data++)];
        }
        m_crc &= (2ULL << (m_width - 1)) - 1ULL; // mask
    }
    else
    {
        quint64* slice = (quint64*)data;
        while(length >= 8)
        {
            m_crc ^= *slice++;

            m_crc = m_table[7][ m_crc        & 0xFF] ^
                    m_table[6][(m_crc >> 8)  & 0xFF] ^
                    m_table[5][(m_crc >> 16) & 0xFF] ^
                    m_table[4][(m_crc >> 24) & 0xFF] ^
                    m_table[3][(m_crc >> 32) & 0xFF] ^
                    m_table[2][(m_crc >> 40) & 0xFF] ^
                    m_table[1][(m_crc >> 48) & 0xFF] ^
                    m_table[0][ m_crc >> 56];
            length -= 8;
        }
        data = (const char*)slice;
        while(length--)
            m_crc = (m_crc >> 8) ^ m_table[0][(m_crc & 0xFF) ^ (quint8)(*data++)];

        m_crc &= (2ULL << (m_width - 1)) - 1ULL; // mask, not necessary
    }

    if(m_refIn != m_refOut)
        m_crc = reflect(m_crc, m_width);
    m_crc ^= m_xorOut;

    if(m_notify)
        emit result(m_crc);
}

void AsyncCRC::addData(const QByteArray & data)
{
    addData(data.constData(), data.length());
}

void AsyncCRC::reset()
{
    m_crc = m_refIn ? reflect(m_initVal, m_width) : m_initVal;
    m_isInitVal = true;
}

quint64 AsyncCRC::reflect(quint64 data, quint8 len)
{
    quint64 result = 0;
    while(len--)
    {
        result <<= 1;
        result |= data & 1;
        data >>= 1;
    }
    return result;
}

quint64 AsyncCRC::getResult()
{
    emit result(m_crc);
    return m_crc;
}

void AsyncCRC::setNotify(bool state)
{
    m_notify = state;
}

void AsyncCRC::setParam(quint8 width, quint64 poly, quint64 init, bool refIn, bool refOut, quint64 xorOut)
{
    m_width = width;
    m_initVal = init;
    m_refIn = refIn;
    m_refOut = refOut;
    m_xorOut = xorOut;

    quint64 tmp;
    // the result of (1ULL << 64) - 1ULL is undefined.
    // (2ULL << 63) - 1ULL will be fine.
    quint64 mask = (2ULL << (m_width - 1)) - 1ULL;

    if(!m_refIn)
    {
        quint8 offset = m_width - 8;
        quint64 MSB = 1ULL << (m_width - 1);
        for(quint64 i = 0; i < 256; i++)
        {
            tmp = i << offset;
            for(quint8 j = 0; j < 8; j++)
                tmp = (tmp & MSB) ? ((tmp << 1) ^ poly) : (tmp << 1);
            m_table[0][i] = tmp & mask;
        }
    }
    else
    {
        poly = reflect(poly, m_width);
        for(quint16 i = 0; i < 256; i++)
        {
            tmp = i;
            for(quint8 j = 0; j < 8; j++)
                tmp = (tmp & 1) ? ((tmp >> 1) ^ poly) : (tmp >> 1);
            m_table[0][i] = tmp & mask;
        }
        for(quint8 i = 1; i < 8; i++)
        {
            for(quint16 j = 0; j < 256; j++)
                m_table[i][j] = (m_table[i - 1][j] >> 8) ^ m_table[0][m_table[i - 1][j] & 0xFF];
        }
    }
    reset();
}


