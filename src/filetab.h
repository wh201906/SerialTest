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

public slots:
    void onChecksumUpdated(quint64 checksum);
private slots:
    void on_fileBrowseButton_clicked();

    void on_Raw_throttleGrp_buttonClicked(QAbstractButton *button);

    void on_checksumButton_clicked();

private:
    Ui::FileTab *ui;

    QThread* m_checksumThread;
    AsyncCRC* m_checksumCalc;

    void showUpTabHelper(int id);

#ifdef Q_OS_ANDROID
    static FileTab* m_currInstance;
    static void onSharedFileReceived(JNIEnv *env, jobject thiz, jstring text);
#endif

signals:
    void showUpTab(int id);
};

#endif // FILETAB_H
