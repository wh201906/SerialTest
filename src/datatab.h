#ifndef DATATAB_H
#define DATATAB_H

#include <QWidget>
#include <QScrollBar>
#include <QTextDecoder>

#ifdef Q_OS_ANDROID
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#endif

#include "mysettings.h"
#include "connection.h"

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

    void appendSendedData(const QByteArray &data);
    void appendReceivedData(const QByteArray &data);
    void syncReceivedEditWithData();
    void syncSendedEditWithData();
    void setConnection(Connection* conn);

    void setRepeat(bool state);
    bool getRxRealtimeState();
    void initSettings();

public slots:
    void onConnTypeChanged(Connection::Type type);
    void onConnEstablished();
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
    void on_data_flowDTRBox_clicked(bool checked);
    void on_data_flowRTSBox_clicked(bool checked);
    void on_data_unescapeBox_stateChanged(int arg1);

    void on_sendedEdit_selectionChanged();

    void on_receivedEdit_selectionChanged();

    void on_sendedEnableBox_stateChanged(int arg1);

private:
    Ui::DataTab *ui;

    QIODevice* IODevice;
    Connection* m_connection = nullptr;
    MySettings* settings;
    QTimer* repeatTimer;

    QScrollBar* RxSlider;
    int currRxSliderPos = 0;
    int userRequiredRxSliderPos = 0;

    bool isReceivedDataHex = false;
    bool isSendedDataHex = false;
    bool unescapeSendedData = false;

    QTextCodec* dataCodec = nullptr; // for Tx and generating Rx decoder
    QTextDecoder* RxDecoder = nullptr; // for Rx UI, a multi-byte character might be split.
    char lastReceivedByte = '\0';
    int RxHexCounter = 0, TxHexCounter = 0;
    QByteArray* rawReceivedData = nullptr;
    QByteArray* rawSendedData = nullptr;

    void loadPreference();
    void showUpTabHelper(int id);
#ifdef Q_OS_ANDROID
    static DataTab* m_currInstance;
    static void onSharedTextReceived(JNIEnv *env, jobject thiz, jstring text);
#endif
signals:
    void send(const QByteArray& data);
    void setDataCodec(QTextCodec* codec);
    void setPlotDecoder(QTextDecoder* decoder);
    void updateRxTxLen(bool updateRx, bool updateTx);
    void clearSendedData();
    void clearReceivedData();
    void setTxDataRecording(bool enabled);
    void showUpTab(int id);
};

#endif // DATATAB_H
