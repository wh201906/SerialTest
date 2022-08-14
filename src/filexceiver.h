#ifndef FILEXCEIVER_H
#define FILEXCEIVER_H

#include <QObject>
#include <QFile>
#include <QElapsedTimer>
#include <QThread>

#include "asynccrc.h"

class FileXceiver : public QObject
{
    Q_OBJECT
public:

    enum Protocol
    {
        RawProtocol = 0,
    };
    Q_ENUM(Protocol);

    explicit FileXceiver(QObject *parent = nullptr);

    void stop();
    bool startTransmit(const QString &filename);
    bool startReceive(const QString &filename);
    void setProtocol(Protocol p);
public slots:
    void newData(const QByteArray& data);
protected:
    qsizetype m_handledNum = 0;

    QFile m_file;
    bool m_isRunning = false;
    Protocol m_protocol = RawProtocol;
    AsyncCRC* m_protocolChecksum;
    QThread* m_protocolChecksumThread;

    void RawTransmitProgress();
    void RawReceiveProgress(const QByteArray& data);

signals:
    void dataTransmitted(qsizetype num);
    void dataReceived(qsizetype num);
    void send(const QByteArray& data);
    void finished();
};

#endif // FILEXCEIVER_H
