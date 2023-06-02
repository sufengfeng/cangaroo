#include "mainwindow_download.h"
#include "ui_mainwindow_download.h"
#include "workerdownloadthread.h"
MainWindow_Download::MainWindow_Download(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow_Download)
{
    ui->setupUi(this);
    m_sWorkerDownloadThread = new WorkerDownloadThread;
    //浮动窗口
    //    ui->tabWidget->setVisible(false);
    connect(ui->pushButton_Open, &QPushButton::clicked, this, &MainWindow_Download::OpenFile);
    connect(ui->pushButton_Download, &QPushButton::clicked, this, &MainWindow_Download::DownloadFile);
    //    connect(m_sWorkerDownloadThread, &WorkerDownloadThread::Signal_progress, this, &MainWindow_Download::Slot_UpdateProcess);
}

MainWindow_Download::~MainWindow_Download()
{
    delete m_sWorkerDownloadThread;
    delete ui;
}

bool MainWindow_Download::Slot_UpdateProcess(const int counter, QString str)
{
    ui->progressBar->setValue(counter);
    ui->textBrowser_LogVeiw->append(str);
    return true;
}
void MainWindow_Download::OpenFile(void)
{
    //    1、退出界面

    QString curPath = QDir::currentPath(); //获取系统当前目录
    QString filter = "固件文件(*.bin *.gbin);;所有文件(*.*)"; //文件过滤器
    m_sFilePathName = QFileDialog::getOpenFileName(this, tr("请选择需要打开的文件"), curPath, filter);
    if(!m_sFilePathName.isEmpty())
    {
        //        loadFile(FileName);
        QFileInfo* fileInfo = new QFileInfo(m_sFilePathName);

        ui->textEdit_HexVeiw->setText("Bin Size:" + QString::number(fileInfo->size()) + "Bit");


        QFile* file = new QFile;
        /*
        * 读取Bin文件
        */
        file->setFileName(fileInfo->filePath());
        if(file->open(QIODevice::ReadOnly))
        {
            QDataStream BinFileData(file);
            char* pBuff = new char[fileInfo->size()];
            BinFileData.readRawData(pBuff, static_cast<int>(fileInfo->size()));
            QByteArray BinFileRawData = QByteArray(pBuff, static_cast<int>(fileInfo->size()));
            qDebug() << BinFileRawData.toHex(' ').toUpper();
            qDebug() << BinFileRawData;

            ui->textEdit_HexVeiw->append(BinFileRawData.toHex(' ').toUpper());
            delete []pBuff ;
            file->close();
        }
        else
        {
            QMessageBox mesg;
            mesg.critical(this, tr("Error"), tr("无法读取,请检查BIN文件路径!"));
        }
        delete file;
        delete  fileInfo;


        QApplication::restoreOverrideCursor();      //

        ui->lineEdit->setText(m_sFilePathName);
        ui->tabWidget->setCurrentIndex(1);
        //        ui->tabWidget->setVisible(true);
    }
}
void MainWindow_Download::DownloadFile(void)
{
    ui->tabWidget->setCurrentIndex(0);
    m_sWorkerDownloadThread->StartUpdateHardWare(m_sFilePathName);
}


void MainWindow_Download::HandleCanMessage(const CanMessage* p_sCanMessage)
{
    m_sWorkerDownloadThread->HandleCanMessage(p_sCanMessage);
}
