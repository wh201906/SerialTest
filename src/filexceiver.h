#ifndef FILEXCEIVER_H
#define FILEXCEIVER_H

#include <QObject>
#include <QFile>
#include <QElapsedTimer>

class FileXceiver : public QObject
{
    Q_OBJECT
public:

    enum Protocol
    {
        RawProtocol = 0,
    };
    explicit FileXceiver(QObject *parent = nullptr);

    void stop();

    bool startTransmit(const QString &filename);
    void setProtocol(Protocol p);
protected:
    qsizetype m_transmittedNum = 0;

    QFile m_file;
    bool m_isRunning = false;
    Protocol m_protocol = RawProtocol;

    void RawTransmitProgress();

signals:
    void dataTransmitted(qsizetype num);
    void send(const QByteArray& data);
};

#endif // FILEXCEIVER_H
