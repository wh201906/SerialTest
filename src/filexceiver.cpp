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
    m_transmittedNum = 0;
    if(m_protocol == RawProtocol)
        QTimer::singleShot(0, this, &FileXceiver::RawTransmitProgress);
    return true;
}

void FileXceiver::setProtocol(Protocol p)
{
    m_protocol = p;
}

void FileXceiver::RawTransmitProgress()
{
    if(!m_isRunning)
    {
        // emit signal?
        return;
    }
    QByteArray buf = m_file.read(1024 * 512);
    m_transmittedNum += buf.length();
    emit send(buf);
    emit dataTransmitted(buf.length());
    if(!m_file.atEnd())
        QTimer::singleShot(0, this, &FileXceiver::RawTransmitProgress);
}

void FileXceiver::stop()
{
    m_isRunning = false;
}

