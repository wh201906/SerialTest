#ifndef DATATAB_H
#define DATATAB_H

#include <QWidget>
#include <QScrollBar>
#include <QTextDecoder>

#include "mysettings.h"

namespace Ui
{
class DataTab;
}

class DataTab : public QWidget
{
    Q_OBJECT

public:
    explicit DataTab(QByteArray* RxBuf, QByteArray* TxBuf, QWidget *parent = nullptr);
    ~DataTab();

    void appendReceivedData(const QByteArray &data);
    void syncReceivedEditWithData();
    void syncSendedEditWithData();
    void setIODevice(QIODevice* dev);
    void setFlowCtrl(bool isRTSValid, bool rts, bool dtr);
    void setRepeat(bool state);
    bool getRxRealtimeState();
    void initSettings();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *ev) override;

private slots:
    void saveDataPreference();
    void on_data_encodingSetButton_clicked();
    void on_sendButton_clicked();
    void on_sendedHexBox_stateChanged(int arg1);
    void on_receivedHexBox_stateChanged(int arg1);
    void on_receivedClearButton_clicked();
    void on_sendedClearButton_clicked();
    void on_sendEdit_textChanged(const QString &arg1);
    void on_data_repeatCheckBox_stateChanged(int arg1);
    void on_receivedCopyButton_clicked();
    void on_sendedCopyButton_clicked();
    void on_receivedExportButton_clicked();
    void on_sendedExportButton_clicked();
    void on_data_suffixTypeBox_currentIndexChanged(int index);
    void onRxSliderValueChanged(int value);
    void onRxSliderMoved(int value);
    void on_receivedUpdateButton_clicked();
#ifndef Q_OS_ANDROID
    void on_data_flowDTRBox_clicked(bool checked);
    void on_data_flowRTSBox_clicked(bool checked);
#endif

private:
    Ui::DataTab *ui;

    QIODevice* IODevice;
    MySettings* settings;
    QTimer* repeatTimer;

    int dataEncodingId = 0;

    QScrollBar* RxSlider;
    int currRxSliderPos = 0;
    int userRequiredRxSliderPos = 0;

    bool isReceivedDataHex = false;
    bool isSendedDataHex = false;

    QTextCodec* dataCodec = nullptr; // for Tx and generating Rx decoder
    QTextDecoder* RxDecoder = nullptr; // for Rx UI, a multi-byte character might be split.
    char lastReceivedByte = '\0';
    int hexCounter = 0;
    QByteArray* rawReceivedData = nullptr;
    QByteArray* rawSendedData = nullptr;

    void loadPreference();
signals:
    void send(const QByteArray& data);
    void setDataCodec(QTextCodec* codec);
    void setPlotDecoder(QTextDecoder* decoder);
    void setRxLabelText(const QString& text);
    void setTxLabelText(const QString& text);
};

#endif // DATATAB_H
