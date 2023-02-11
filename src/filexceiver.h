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
        qint64 waitTime = -1;

        qint64 batchByteNum = -1;
        // qint64  batchTime = -1;
    };

    explicit FileXceiver(QObject *parent = nullptr);
    ~FileXceiver();

    Q_INVOKABLE void stop();
    Q_INVOKABLE bool startTransmit(const QString &filename);
    Q_INVOKABLE bool startReceive(const QString &filename);
    Q_INVOKABLE void setProtocol(FileXceiver::Protocol p);
    Q_INVOKABLE void setThrottleArgument(FileXceiver::ThrottleArgument arg);
    Q_INVOKABLE void setAutostop(qint64 num);

public slots:
    void newData(const QByteArray& data);
protected:
    qint64 m_handledNum = 0;
    qint64 m_batchSize = 0; // default batcheSize is set in startTransmit()
    qint64 m_waitTime = 0;
    qint64 m_expectedNum = -1; // -1: infinite
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
    void dataTransmitted(qint64 num);
    void dataReceived(qint64 num);
    void send(const QByteArray& data);
    void finished();
    void startResult(bool result);
};

Q_DECLARE_METATYPE(FileXceiver::Protocol)
Q_DECLARE_METATYPE(FileXceiver::ThrottleArgument)

#endif // FILEXCEIVER_H
