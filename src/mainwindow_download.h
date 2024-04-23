#ifndef MAINWINDOW_DOWNLOAD_H
#define MAINWINDOW_DOWNLOAD_H

#include <QMainWindow>

#include <QTextBrowser>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <core/Backend.h>
namespace Ui
{
class MainWindow_Download;
}
class QMyWidget;
class WorkerDownloadThread;
class MainWindow_Download : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow_Download(QWidget* parent = nullptr);
    ~MainWindow_Download();
    void OpenFile(void);
    void DownloadFile(void);
    void HandleCanMessage(const CanMessage* p_sCanMessage);
    void SetCanId(int canId);
public slots:
    bool Slot_UpdateProcess(const int counter, QString str);
    void closeEvent(QCloseEvent* event);

public:
    WorkerDownloadThread* m_sWorkerDownloadThread;
private slots:


private:
    Ui::MainWindow_Download* ui;

    bool isUntitled;                                //true表示文件没被保存过,false表示有
    QString m_sFilePathName;                               //当前文件的路径

};

#endif // MAINWINDOW_DOWNLOAD_H
