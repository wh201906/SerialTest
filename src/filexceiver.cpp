#include "filexceiver.h"

#include <QTimer>

FileXceiver::FileXceiver(QObject *parent)
    : QObject{parent}
{

}

bool FileXceiver::startTransmit(const QString& filename)
{
    m_file.close();
    m_file.setFileName(filename);
    if(!m_file.open(QFile::ReadOnly))
        return false;

    m_isRunning = true;
    m_handledNum = 0;
    if(m_protocol == RawProtocol)
        QTimer::singleShot(0, this, &FileXceiver::RawTransmitProgress);
    return true;
}

bool FileXceiver::startReceive(const QString &filename)
{
    m_file.close();
    m_file.setFileName(filename);
    if(!m_file.open(QFile::WriteOnly | QFile::Unbuffered))
        return false;

    m_isRunning = true;
    m_handledNum = 0;
    return true;
}

void FileXceiver::setProtocol(Protocol p)
{
    m_protocol = p;
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
        qsizetype num = m_file.write(data);
        m_handledNum += num;
        emit dataReceived(num);
    }
}

void FileXceiver::RawTransmitProgress()
{
    if(!m_isRunning)
    {
        // emit signal?
        return;
    }
    QByteArray buf = m_file.read(1024 * 512);
    m_handledNum += buf.length();
    emit send(buf);
    emit dataTransmitted(buf.length());
    if(!m_file.atEnd())
        QTimer::singleShot(0, this, &FileXceiver::RawTransmitProgress);
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

