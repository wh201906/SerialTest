#ifndef ASYNCCRC_H
#define ASYNCCRC_H

#include <QObject>

class AsyncCRC : public QObject
{
    Q_OBJECT
public:
    explicit AsyncCRC(QObject *parent = nullptr);
    AsyncCRC(quint8 width, quint64 poly, quint64 init = 0ULL, bool refIn = false, bool refOut = false, quint64 xorOut = 0ULL);
    AsyncCRC(const AsyncCRC& obj);
    AsyncCRC &operator= (const AsyncCRC& obj);

    enum CRCFileError
    {
        OpenFileError = 0,
        ReadFileError
    };
    Q_ENUM(CRCFileError);

    Q_INVOKABLE void loadFile(const QString& path);
    Q_INVOKABLE void addData(const char* data, qsizetype length);
    Q_INVOKABLE void addData(const QByteArray& data);
    Q_INVOKABLE quint64 getResult();
    Q_INVOKABLE void setNotify(bool state);
    Q_INVOKABLE void setParam(quint8 width, quint64 poly, quint64 init = 0ULL, bool refIn = false, bool refOut = false, quint64 xorOut = 0ULL);
    Q_INVOKABLE void reset();
protected:
    bool m_notify = false;

    quint64 m_crc;
    quint64 m_table[8][256];
    bool m_isInitVal = true;

    quint8 m_width;
    quint64 m_poly, m_initVal, m_xorOut, m_mask;
    bool m_refIn, m_refOut;

    quint64 reflect(quint64 data, quint8 len);
signals:
    void result(quint64 crcResult);
    void fileError(AsyncCRC::CRCFileError error);
};

#endif // ASYNCCRC_H
