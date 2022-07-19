#include "asynccrc.h"

#include <QDebug>

AsyncCRC::AsyncCRC(QObject *parent)
    : QObject{parent}
{
}

void AsyncCRC::addData(const char *data, qsizetype length)
{
    if(!m_refIn)
    {
        quint8 offset = m_width - 8;
        quint8 tmp;
        while(length--)
        {
            tmp = m_crc >> offset; // & 0xFF
            m_crc = (m_crc << 8) ^ m_table[tmp ^ *data++];
        }
        m_crc &= (1ULL << m_width) - 1ULL; // mask
    }
    else
    {
        while(length--)
            m_crc = (m_crc >> 8) ^ m_table[(m_crc & 0xFF) ^ *data++];
        m_crc &= (1ULL << m_width) - 1ULL; // mask, not necessary
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

void AsyncCRC::getResult()
{
    emit result(m_crc);
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
    quint64 mask = (1ULL << m_width) - 1ULL;

    if(!m_refIn)
    {
        quint8 offset = m_width - 8;
        quint64 MSB = 1ULL << (m_width - 1);
        for(quint16 i = 0; i < 256; i++)
        {
            tmp = i << offset;
            for(quint8 j = 0; j < 8; j++)
                tmp = (tmp & MSB) ? ((tmp << 1) ^ poly) : (tmp << 1);
            m_table[i] = tmp & mask;
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
            m_table[i] = tmp & mask;
        }
    }
    reset();
}


