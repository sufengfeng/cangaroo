#include "mainwindow_download.h"
#include "ui_mainwindow_download.h"
#include "workerdownloadthread.h"
static bool isDirExist(QString fullPath)
{
    QDir dir(fullPath);
    if(dir.exists())
    {
        return true;
    }
    else
    {
        bool ok = dir.mkdir(fullPath);//只创建一级子目录，即必须保证上级目录存在
        return ok;
    }
}
QString GetConfigDowndFilePath(void)
{
    QString filePath ;
    QString dir_str = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    //    qDebug() << dir_str;
    if(isDirExist(dir_str))
    {
        QSettings* settings = new QSettings(dir_str + "/setting.ini", QSettings::IniFormat);
        settings->beginGroup("MAIN");
        filePath = settings->value("DOWNLOAD_FILE_PATH", "").toString();
        //        QString version = settings->value("VERSION", "x.y.z").toString();
        //        qDebug() << "LEARN_OBJECT=" << filePath << endl; //输出至控制台
        //        qDebug() << "QT_VERSION=" << version << endl; //输出至控制台
        settings->endGroup();
        delete settings;
    }
    return  filePath;
}
void SetConfigDowndFilePath(QString filepath)
{
    QString dir_str = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) ;
    if(isDirExist(dir_str))
    {
        QSettings* settings = new QSettings(dir_str + "/setting.ini", QSettings::IniFormat);

        settings->beginGroup("MAIN");
        settings->setValue("DOWNLOAD_FILE_PATH", filepath);
        settings->setValue("VERSION", "1.4.1");
        settings->endGroup();

        settings->beginGroup("OTHER");
        settings->setValue("CSDN_DESCRIPTION", "Share My Learning exprience");
        settings->endGroup();
        delete settings;
    }
}

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
    connect(m_sWorkerDownloadThread, &WorkerDownloadThread::Signal_progress, this, &MainWindow_Download::Slot_UpdateProcess);
    m_sFilePathName =    GetConfigDowndFilePath();
    if(!m_sFilePathName.isEmpty())
    {
        ui->lineEdit->setText(m_sFilePathName);
    }
}

MainWindow_Download::~MainWindow_Download()
{
    m_sWorkerDownloadThread->stopTread();
    m_sWorkerDownloadThread->exit(0);       //停止线程
    delete m_sWorkerDownloadThread;
    delete ui;
}
void MainWindow_Download::closeEvent(QCloseEvent* event)
{
    m_sWorkerDownloadThread->StopDownload();
    qDebug()<<__func__<<__LINE__;
//    event->accept();
}
bool MainWindow_Download::Slot_UpdateProcess(const int counter, QString str)
{
    ui->progressBar->setValue(counter);
    if(str.isEmpty())
    {
        ;
    }
    else
    {
        ui->textBrowser_LogVeiw->append(str);
    }

    //    qDebug() << __func__ << __LINE__ << counter << str;
    return true;
}
void MainWindow_Download::OpenFile(void)
{
    //    1、退出界面

    QString curPath = QDir::currentPath(); //获取系统当前目录
    QString filter = "固件文件(*.bin *.gbin);;所有文件(*.*)"; //文件过滤器
    if(!m_sFilePathName.isEmpty())
    {
        QFileInfo* fileInfo = new QFileInfo(m_sFilePathName);
        curPath = fileInfo->filePath();
        delete  fileInfo;
    }
    QString tmpFileName;
    tmpFileName = QFileDialog::getOpenFileName(this, tr("请选择需要打开的文件"), curPath, filter);
    if(!tmpFileName.isEmpty())
    {
        m_sFilePathName = tmpFileName;
        ui->tabWidget->setCurrentIndex(1);
        QFileInfo* fileInfo = new QFileInfo(m_sFilePathName);

        //        ui->textEdit_HexVeiw->setText("File:" + m_sFilePathName);
        ui->textEdit_HexVeiw->setHtml(QStringLiteral("<p style='margin:0px;padding:0px;'><strong>File:%1</strong>&nbsp;</p>").arg(m_sFilePathName));
        //        ui->textEdit_HexVeiw->append("Bin Size:" + QString::number(fileInfo->size()) + "Bit");
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); //等待旋转

        QFile* file = new QFile;
        file->setFileName(fileInfo->filePath());
        if(file->open(QIODevice::ReadOnly))
        {
            QDataStream BinFileData(file);
            char* pBuff = new char[fileInfo->size()];
            BinFileData.readRawData(pBuff, static_cast<int>(fileInfo->size()));
            QByteArray BinFileRawData = QByteArray(pBuff, static_cast<int>(fileInfo->size()));
            //            qDebug() << BinFileRawData.toHex(' ').toUpper();
            //            qDebug() << BinFileRawData;
            int forBitAlignment = BinFileRawData.size() % 4;    //字节对其
            if(forBitAlignment)
            {
                for(int i = forBitAlignment; i < 4; i++)
                {
                    BinFileRawData.append(0xff);
                }
            }
            ui->textEdit_HexVeiw->append("Bin Size:" + QString::number(BinFileRawData.size()) + "Bit");
            ui->textEdit_HexVeiw->append(BinFileRawData.toHex(' ').toUpper());
            delete []pBuff ;
            file->close();
            BinFileRawData.clear();
        }
        else
        {
            QMessageBox mesg;
            mesg.critical(this, tr("Error"), tr("无法读取,请检查BIN文件路径!"));
        }
        QApplication::restoreOverrideCursor();//恢复
        delete file;
        delete  fileInfo;


        QApplication::restoreOverrideCursor();      //

        ui->lineEdit->setText(m_sFilePathName);

        //        ui->tabWidget->setVisible(true);
    }
}
void MainWindow_Download::DownloadFile(void)
{
    m_sFilePathName = ui->lineEdit->text();
    ui->tabWidget->setCurrentIndex(0);
    int canid = ui->spinBox_CanID->value();
    int updateType = 0;
    SetConfigDowndFilePath(m_sFilePathName);            //保存下载文件位置
    m_sWorkerDownloadThread->StartUpdateHardWare(m_sFilePathName, canid, updateType);
}

void MainWindow_Download::SetCanId(int canId)
{
    ui->spinBox_CanID->setValue(canId);
}
void MainWindow_Download::HandleCanMessage(const CanMessage* p_sCanMessage)
{
    m_sWorkerDownloadThread->HandleCanMessage(p_sCanMessage);
}
