#ifndef WINRTBLUETOOTH_H
#define WINRTBLUETOOTH_H

#include <QObject>
#include <QBluetoothHostInfo>

class WinRTBluetooth : public QObject
{
    Q_OBJECT
public:
    explicit WinRTBluetooth(QObject *parent = nullptr);
    static QList<QBluetoothHostInfo> allLocalDevices(bool PoweredOnOnly = false);
signals:

};

#endif // WINRTBLUETOOTH_H
