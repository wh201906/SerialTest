#include "winrtbluetooth.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Radios.h>

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Radios;

template <typename T>
static bool await(IAsyncOperation<T> &&asyncInfo, T &result, uint timeout = 0)
{
    using WinRtAsyncStatus = winrt::Windows::Foundation::AsyncStatus;
    WinRtAsyncStatus status;
    QElapsedTimer timer;
    if(timeout)
        timer.start();
    do
    {
        QCoreApplication::processEvents();
        status = asyncInfo.Status();
    }
    while(status == WinRtAsyncStatus::Started && (!timeout || !timer.hasExpired(timeout)));
    if(status == WinRtAsyncStatus::Completed)
    {
        result = asyncInfo.GetResults();
        return true;
    }
    else
    {
        return false;
    }
}

static DeviceInformationCollection getAvailableAdapters()
{
    const auto btSelector = BluetoothAdapter::GetDeviceSelector();
    DeviceInformationCollection deviceInfoCollection(nullptr);
    await(DeviceInformation::FindAllAsync(btSelector), deviceInfoCollection);
    return deviceInfoCollection;
}

static Radio getRadioFromAdapterId(winrt::hstring id)
{
    BluetoothAdapter a(nullptr);
    bool res = await(BluetoothAdapter::FromIdAsync(id), a);
    if(res && a)
    {
        Radio r(nullptr);
        res = await(a.GetRadioAsync(), r);
        if(res && r)
            return r;
    }
    return nullptr;
}

WinRTBluetooth::WinRTBluetooth(QObject *parent)
    : QObject{parent}
{

}

QList<QBluetoothHostInfo> WinRTBluetooth::allLocalDevices(bool PoweredOnOnly)
{
    QList<QBluetoothHostInfo> devices;
    const auto deviceInfoCollection = getAvailableAdapters();
    if(deviceInfoCollection)
    {
        for(const auto &devInfo : deviceInfoCollection)
        {
            BluetoothAdapter adapter(nullptr);
            Radio radio(nullptr);
            const bool res = await(BluetoothAdapter::FromIdAsync(devInfo.Id()), adapter);
            if(!res || !adapter)
            {
                continue;
            }
            if(!PoweredOnOnly || ((radio = getRadioFromAdapterId(devInfo.Id())) && radio.State() == RadioState::On))
            {
                QBluetoothHostInfo info;
                info.setName(QString::fromStdString(winrt::to_string(devInfo.Name())));
                info.setAddress(QBluetoothAddress(adapter.BluetoothAddress()));
                devices.push_back(std::move(info));
            }
        }
    }
    return devices;
}
