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
    Q_ENUM(Protocol)

    struct ThrottleArgument
    {
        // -1: no throttle
        qsizetype waitTime = -1;

        qsizetype batchByteNum = -1;
        // qsizetype  batchTime = -1;
    };

    explicit FileXceiver(QObject *parent = nullptr);
    ~FileXceiver();

    Q_INVOKABLE void stop();
    Q_INVOKABLE bool startTransmit(const QString &filename);
    Q_INVOKABLE bool startReceive(const QString &filename);
    Q_INVOKABLE void setProtocol(FileXceiver::Protocol p);
    Q_INVOKABLE void setThrottleArgument(FileXceiver::ThrottleArgument arg);
    Q_INVOKABLE void setAutostop(qsizetype num);

public slots:
    void newData(const QByteArray& data);
protected:
    qsizetype m_handledNum = 0;
    qsizetype m_batchSize = 0; // default batcheSize is set in startTransmit()
    qsizetype m_waitTime = 0;
    qsizetype m_expectedNum = -1; // -1: infinite
    QElapsedTimer m_speedAdjustTimer;

    QFile m_file;
    bool m_isRunning = false;
    Protocol m_protocol = RawProtocol;
    ThrottleArgument m_throttleArgument;
    AsyncCRC* m_protocolChecksum;
    QThread* m_protocolChecksumThread;

    void RawTransmitProgress();
    void RawReceiveProgress(const QByteArray& data);

signals:
    void dataTransmitted(qsizetype num);
    void dataReceived(qsizetype num);
    void send(const QByteArray& data);
    void finished();
    void startResult(bool result);
};

Q_DECLARE_METATYPE(qsizetype)
Q_DECLARE_METATYPE(FileXceiver::Protocol)
Q_DECLARE_METATYPE(FileXceiver::ThrottleArgument)

#endif // FILEXCEIVER_H
