#ifndef FILETAB_H
#define FILETAB_H

#include <QWidget>
#include <QRadioButton>
#include <QThread>

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
    void onChecksumUpdated(quint32 checksum);
private slots:
    void on_fileBrowseButton_clicked();

    void on_Raw_throttleGrp_buttonClicked(QAbstractButton *button);

    void on_checksumButton_clicked();

private:
    Ui::FileTab *ui;

    QThread* m_checksumThread;
    AsyncCRC* m_checksumCalc;
};

#endif // FILETAB_H
