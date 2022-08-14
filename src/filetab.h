#ifndef FILETAB_H
#define FILETAB_H

#include <QWidget>
#include <QRadioButton>
#include <QThread>

#ifdef Q_OS_ANDROID
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#endif

#include "asynccrc.h"
#include "filexceiver.h"

namespace Ui
{
class FileTab;
}

class FileTab : public QWidget
{
    Q_OBJECT

public:
    explicit FileTab(QWidget *parent = nullptr);
    ~FileTab();

    FileXceiver* fileXceiver();
public slots:
    void onChecksumUpdated(quint64 checksum);
    void onChecksumError(AsyncCRC::CRCFileError error);
    void onDataTransmitted(qsizetype num);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private slots:
    void on_fileBrowseButton_clicked();

    void on_Raw_throttleGrp_buttonClicked(QAbstractButton *button);

    void on_checksumButton_clicked();

    void on_clearButton_clicked();

    void on_startButton_clicked();

    void on_stopButton_clicked();

private:
    Ui::FileTab *ui;

    qsizetype m_fileSize = -1;
    qsizetype m_handledSize = -1;
    QThread* m_checksumThread = nullptr;
    AsyncCRC* m_checksumCalc = nullptr;
    FileXceiver* m_fileXceiver = nullptr;
    static FileTab* m_currInstance;

    void showUpTabHelper(int id);
    void onFilePathSet(const QString &path);

#ifdef Q_OS_ANDROID
    static void onSharedFileReceived(JNIEnv *env, jobject thiz, jstring text);
#endif

signals:
    void showUpTab(int id);
};

#endif // FILETAB_H
