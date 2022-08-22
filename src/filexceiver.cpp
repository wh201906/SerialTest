#include "filexceiver.h"

#include <QTimer>
#include <QElapsedTimer>

FileXceiver::FileXceiver(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<qsizetype>("qsizetype"); // qsizetype is an alias, so the typeName is compulsory
    qRegisterMetaType<FileXceiver::Protocol>();
    qRegisterMetaType<FileXceiver::ThrottleArgument>();
    m_file.setParent(this); // for moveToThread()
}

FileXceiver::~FileXceiver()
{
    stop();
}

bool FileXceiver::startTransmit(const QString& filename)
{
    m_file.close();
    m_file.setFileName(filename);
    if(!m_file.open(QFile::ReadOnly))
    {
        emit startResult(false);
        return false;
    }

    m_isRunning = true;
    m_handledNum = 0;
    if(m_protocol == RawProtocol)
    {
        if(m_throttleArgument.waitTime != -1)
        {
            m_batchSize = m_throttleArgument.batchByteNum;
            m_waitTime = m_throttleArgument.waitTime;
        }
        else
        {
            m_batchSize = 32;
            m_waitTime = 0;
        }
        m_speedAdjustTimer.start();
        QTimer::singleShot(0, this, &FileXceiver::RawTransmitProgress);
    }
    emit startResult(true);
    return true;
}

bool FileXceiver::startReceive(const QString &filename)
{
    m_file.close();
    m_file.setFileName(filename);
    if(!m_file.open(QFile::WriteOnly | QFile::Unbuffered))
    {
        emit startResult(false);
        return false;
    }

    m_isRunning = true;
    m_handledNum = 0;
    emit startResult(true);
    return true;
}

void FileXceiver::setProtocol(Protocol p)
{
    m_protocol = p;
}

void FileXceiver::setThrottleArgument(ThrottleArgument arg)
{
    m_throttleArgument = arg;
}

void FileXceiver::setAutostop(qsizetype num)
{
    m_expectedNum = num;
}

void FileXceiver::newData(const QByteArray &data)
{
    if(!m_isRunning)
    {
        // emit signal?
        return;
    }
    if(m_protocol == RawProtocol)
    {
        qsizetype num;
        qsizetype limit = -1, currNum = -1;
        if(m_expectedNum != -1)
        {
            limit = m_expectedNum - m_handledNum;
            currNum = limit < data.size() ? limit : data.size();
            num = m_file.write(data.constData(), currNum);
        }
        else
            num = m_file.write(data);
        m_file.flush();
        m_handledNum += num;
        emit dataReceived(num);
        if(m_expectedNum != -1 && currNum == limit)
            emit finished();
    }
}

void FileXceiver::RawTransmitProgress()
{
    if(!m_isRunning)
    {
        // emit signal?
        return;
    }
    QByteArray buf = m_file.read(m_batchSize);
    m_handledNum += buf.length();
    emit send(buf);
    emit dataTransmitted(buf.length());
    if(m_throttleArgument.waitTime == -1)
    {
        qsizetype tick = m_speedAdjustTimer.restart();
        // expected responce time is 200ms
        // low threshold = expected / 2
        // high threshold = expected * 2
        // maximum batch size is 8MB
        if(m_batchSize < 4 * 1024 * 1024)
        {
            if(tick < 100)
                m_batchSize <<= 1;
            else if(tick < 200)
                m_batchSize += 32;
        }
        else if(tick > 400)
        {
            m_batchSize *= 200.0 / tick;
            m_batchSize++; // m_batchSize != 0
        }
    }
    if(!m_file.atEnd())
        QTimer::singleShot(m_waitTime, this, &FileXceiver::RawTransmitProgress);
    else
    {
        m_file.close();
        emit finished();
    }
}

void FileXceiver::stop()
{
    m_isRunning = false;
    m_file.close(); // for receiving
}

