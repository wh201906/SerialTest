#include "asynccrc.h"

#include <QDebug>
#include <QFile>

AsyncCRC::AsyncCRC(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<AsyncCRC::CRCFileError>();
}

AsyncCRC::AsyncCRC(quint8 width, quint64 poly, quint64 init, bool refIn, bool refOut, quint64 xorOut) : AsyncCRC(nullptr)
{
    setParam(width, poly, init, refIn, refOut, xorOut);
}

AsyncCRC::AsyncCRC(const AsyncCRC &obj) : AsyncCRC(nullptr)
{
    operator=(obj);
}

AsyncCRC &AsyncCRC::operator=(const AsyncCRC &obj)
{
    if(this != &obj)
    {
        setParam(obj.m_width, obj.m_poly, obj.m_initVal, obj.m_refIn, obj.m_refOut, obj.m_xorOut);
        m_crc = obj.m_crc;
        m_isInitVal = obj.m_isInitVal;
        setNotify(obj.m_notify);
    }
    return *this;
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
    quint64* slice;
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
        quint8 offset;
        quint8 tmp;
        quint64 shiftedCRC;

        offset = m_width - 8;
        while(length && ((quintptr)data & 7) != 0)
        {
            tmp = m_crc >> offset; // & 0xFF
            m_crc = (m_crc << 8) ^ m_table[0][tmp ^ (quint8)(*data++)];
            length--;
        }

        offset = 64 - m_width;
        slice = (quint64*)data;
        while(length >= 8)
        {
            shiftedCRC = m_crc << offset;
            m_crc = m_table[7][(quint8)(*slice)       ^ (quint8)(shiftedCRC >> 56)] ^
                    m_table[6][(quint8)(*slice >> 8)  ^ (quint8)(shiftedCRC >> 48)] ^
                    m_table[5][(quint8)(*slice >> 16) ^ (quint8)(shiftedCRC >> 40)] ^
                    m_table[4][(quint8)(*slice >> 24) ^ (quint8)(shiftedCRC >> 32)] ^
                    m_table[3][(quint8)(*slice >> 32) ^ (quint8)(shiftedCRC >> 24)] ^
                    m_table[2][(quint8)(*slice >> 40) ^ (quint8)(shiftedCRC >> 16)] ^
                    m_table[1][(quint8)(*slice >> 48) ^ (quint8)(shiftedCRC >> 8) ] ^
                    m_table[0][(quint8)(*slice >> 56) ^ (quint8)(shiftedCRC)      ];
            slice++;
            length -= 8;
        }

        data = (const char*)slice;
        offset = m_width - 8;
        while(length--)
        {
            tmp = m_crc >> offset; // & 0xFF
            m_crc = (m_crc << 8) ^ m_table[0][tmp ^ (quint8)(*data++)];
        }
        m_crc &= m_mask;
    }
    else
    {

        while(length && ((quintptr)data & 7) != 0)
        {
            m_crc = (m_crc >> 8) ^ m_table[0][(quint8)m_crc ^ (quint8)(*data++)];
            length--;
        }

        slice = (quint64*)data;
        while(length >= 8)
        {
            m_crc ^= *slice++;

            m_crc = m_table[7][(quint8)(m_crc)      ] ^
                    m_table[6][(quint8)(m_crc >> 8) ] ^
                    m_table[5][(quint8)(m_crc >> 16)] ^
                    m_table[4][(quint8)(m_crc >> 24)] ^
                    m_table[3][(quint8)(m_crc >> 32)] ^
                    m_table[2][(quint8)(m_crc >> 40)] ^
                    m_table[1][(quint8)(m_crc >> 48)] ^
                    m_table[0][(quint8)(m_crc >> 56)];
            length -= 8;
        }

        data = (const char*)slice;
        while(length--)
        {
            m_crc = (m_crc >> 8) ^ m_table[0][(quint8)m_crc ^ (quint8)(*data++)];
        }

        m_crc &= m_mask; // not necessary
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
    m_poly = poly; // for copy only
    m_initVal = init;
    m_refIn = refIn;
    m_refOut = refOut;
    m_xorOut = xorOut;
    // the result of (1ULL << 64) - 1ULL is undefined.
    // (2ULL << 63) - 1ULL will be fine.
    m_mask = (2ULL << (m_width - 1)) - 1ULL;

    quint64 tmp;

    if(!m_refIn)
    {
        quint8 offset = m_width - 8;
        quint64 MSB = 1ULL << (m_width - 1);
        for(quint64 i = 0; i < 256; i++)
        {
            tmp = i << offset;
            for(quint8 j = 0; j < 8; j++)
                tmp = (tmp & MSB) ? ((tmp << 1) ^ poly) : (tmp << 1);
            m_table[0][i] = tmp & m_mask;
        }
        for(quint8 i = 1; i < 8; i++)
        {
            for(quint16 j = 0; j < 256; j++)
                m_table[i][j] = (m_table[i - 1][j] << 8) ^ m_table[0][(quint8)(m_table[i - 1][j] >> offset)];
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
            m_table[0][i] = tmp & m_mask;
        }
        for(quint8 i = 1; i < 8; i++)
        {
            for(quint16 j = 0; j < 256; j++)
                m_table[i][j] = (m_table[i - 1][j] >> 8) ^ m_table[0][(quint8)m_table[i - 1][j]];
        }
    }
    reset();
}
