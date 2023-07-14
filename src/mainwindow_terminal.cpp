#include "mainwindow_terminal.h"
#include "ui_mainwindow_terminal.h"

#include "console.h"
#include "settingsdialog.h"
#include <core/MeasurementSetup.h>
#include <core/CanTrace.h>
#include <window/TraceWindow/TraceWindow.h>
#include <window/SetupDialog/SetupDialog.h>
#include <window/LogWindow/LogWindow.h>
#include <window/GraphWindow/GraphWindow.h>
#include <window/CanStatusWindow/CanStatusWindow.h>
#include <window/RawTxWindow/RawTxWindow.h>

#include <driver/SLCANDriver/SLCANDriver.h>
#include <driver/CANBlastDriver/CANBlasterDriver.h>
#include <driver/CanInterface.h>
#include "qmywidget.h"
#include "mainwindow_download.h"
#include "QSimpleUpdater.h"
#include "buildversion/version.h"
#include <iostream>
Q_LOGGING_CATEGORY(logTest, "test")
static const QString DEFS_URL = "https://gitee.com/sufengfeng/gcan-term-update/raw/master/definitions/updates.json";
//! [0]
MainWindow_terminal::MainWindow_terminal(QWidget* parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow_terminal),
    m_status(new QLabel),
    m_console(new Console),
    m_settings(new SettingsDialog),
    //! [1]
    m_serial(new QSerialPort(this))
    //! [1]
{
    //! [0]
    m_ui->setupUi(this);
    n_sListVeiw=nullptr;
    m_console = m_ui->widget;
    m_console->setEnabled(false);
    m_Qtimer_2s = new QTimer(this);     //2s计数器
    connect(m_Qtimer_2s, SIGNAL(timeout()), this, SLOT(Slot_HandleTimeout()));
    setWindowIcon(QIcon(":/assets/cagaroo.png"));

    //浮动窗口
    m_sDockRight = new QDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, m_sDockRight);

    QMyWidget* widge = new QMyWidget;
    widge->size = QSize(300, 250);
    m_sDockRight->setWidget(widge);
    m_sTraceDockLeft = new TraceWindow(widge, backend());

    m_sDockRight->hide();
    m_sMainWindow_Download = new MainWindow_Download;
    //左侧浮动窗口
    m_sDockLeft = new QDockWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_sDockLeft);

    QMyWidget* widgeDeviceList = new QMyWidget;
    widge->size = QSize(200, 400);
    m_sDockLeft->setWidget(widgeDeviceList);
    m_sQListWidget = new QListWidget(widgeDeviceList);
    //    QListWidgetItem* item = new QListWidgetItem;
    //    item->setData(Qt::DisplayRole, "1");
    //    item->setData(Qt::CheckStateRole, Qt::Checked);
    //    m_sQListWidget->addItem(item);

    //    QListWidgetItem* item1 = new QListWidgetItem;
    //    item1->setData(Qt::DisplayRole, "2");
    //    item1->setData(Qt::CheckStateRole, Qt::Checked);
    //    m_sQListWidget->addItem(item1);
    connect(m_sQListWidget, &QListWidget::itemClicked, this, &MainWindow_terminal::Slot_DeviceList_ItemClicked);
    m_sDockLeft->hide();

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    m_ui->actionCallDevices->setEnabled(false);
    m_ui->statusBar->addWidget(m_status);
    m_nCurrentIndexCommandList = 0;
    initActionsConnections();

    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow_terminal::handleError);

    //! [2]
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow_terminal::readData);
    //! [2]
    connect(m_console, &Console::getData, this, &MainWindow_terminal::writeData);
    //! [3]
    QIcon icon(":/assets/cangaroo.png");
    setWindowIcon(icon);

    m_updater = QSimpleUpdater::getInstance();
    connect(m_ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow_terminal::Slot_checkForUpdates);

    /* Check for updates when the "Check For Updates" button is clicked */
    connect(m_updater, &QSimpleUpdater::checkingFinished, this, &MainWindow_terminal::Slot_updateChangelog);
    connect(m_updater, &QSimpleUpdater::appcastDownloaded, this, &MainWindow_terminal::Slot_displayAppcast);
    Slot_checkForUpdates();//启动检测一下版本
//    qDebug()<<__func__<<__LINE__;
//    qCDebug(logTest) << QString("debug message");
}
MainWindow_terminal::~MainWindow_terminal()
{
    backend().stopMeasurement();
    delete m_sMainWindow_Download;
    delete m_settings;
    delete m_sDockRight;
    if(n_sListVeiw){
        delete n_sListVeiw;
    }
    m_sQListDevice.clear();
    m_sQListWidget->clear();    //清空
    delete m_sQListWidget;
    m_sDockLeft->close();
    delete m_sDockLeft ;
    delete m_Qtimer_2s;
    delete m_ui;
}

void MainWindow_terminal::CanConnectStatusChanged(int status)
{
    const SettingsDialog::Settings p = m_settings->settings();
    if(status == 1)     //connected
    {
        m_console->setEnabled(true);
        m_console->setLocalEchoEnabled(p.localEchoEnabled);
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        m_ui->actionCallDevices->setEnabled(true);
        //        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
        //                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
        //                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        showStatusMessage(tr("Connected to can device "));
        Slot_CallAllCanDevices();
    }
    else                //disconnected
    {
        m_console->setEnabled(false);
        m_ui->actionConnect->setEnabled(true);
        m_ui->actionDisconnect->setEnabled(false);
        m_ui->actionConfigure->setEnabled(true);
        m_ui->actionCallDevices->setEnabled(false);
        showStatusMessage(tr("Disconnected"));
    }
}
//! [4]
void MainWindow_terminal::OpenDevice()
{
    if(IsCanDevice())
    {
        emit EmitSignalOpenCanDevice();
    }
    else
    {
        openSerialPort();
    }
}

//! [4]
void MainWindow_terminal::openSerialPort()
{
    const SettingsDialog::Settings p = m_settings->settings();
    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if(m_serial->open(QIODevice::ReadWrite))
    {
        m_console->setEnabled(true);
        m_console->setLocalEchoEnabled(p.localEchoEnabled);
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());

        showStatusMessage(tr("Open error"));
    }
}
//! [4]
void MainWindow_terminal::CloseDevice()
{
    if(IsCanDevice())
    {
        emit EmitSignalCloseCanDevice();
    }
    else
    {
        closeSerialPort();
    }
}
//#include <QList>
QList<QString> g_lVersionList={
    "V1.0.0 将termial窗口加入到cangaroo中",
    "V1.1.0 发布基本命令版本,can读写已调通",
    "V1.2.0 增加浮动窗口，增加can设备OTA功能",
    "V1.3.0 优化升级功能",
    "V1.4.0 增加日志保存功能、适配42M频率Can设备、增加时间戳",
    "V1.5.0 修复backspace和delete的问题，增加发送can消息超时功能，避免界面卡死",
    "V1.6.1 增加自动远程升级功能",
    "V1.7.2 增加程序时开启terminal和日志记录功能",
    "V1.8.0 待发布",
};

//! [5]
void MainWindow_terminal::closeSerialPort()
{
    if(m_serial->isOpen())
    {
        m_serial->close();
    }
    m_console->setEnabled(false);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}
//! [5]

void MainWindow_terminal::about()
{
    int btnStatus = QMessageBox::information(this, tr("About GCAN-Term"),
                       QString("Version: <b>V%1</b><br>"
                               "The <b>GCAN-Term</b> is used for can device debugging, Factory testing, and firmware download for Geekplus<br>"
                               "build time:[%2 %3]<br>").arg(BUTIANYUN_VERSION).arg(__DATE__).arg(__TIME__),
                                             tr("版本历史详情" ),tr("OK" )

                      );
    if(0 == btnStatus)//点击了按钮1（按钮索引位置为0，后面的依次增加）
    {
        if(n_sListVeiw==nullptr){
            n_sListVeiw=new QListView();
            n_sListVeiw->setWindowTitle(tr("版本历史详情"));
            QStandardItemModel *ItemModel = new QStandardItemModel(n_sListVeiw);
            foreach (QString qVersion, g_lVersionList) {
                QStandardItem *item = new QStandardItem(qVersion);
                ItemModel->appendRow(item);
                item->setEnabled(false);
            }
            n_sListVeiw->setModel(ItemModel);
            n_sListVeiw->setWindowModality(Qt::ApplicationModal);
            n_sListVeiw->resize(500,300);
        }
        n_sListVeiw->show();
    }

}
int MainWindow_terminal::GetCanId(void)
{
    return m_ui->spinBox->value();
}
void MainWindow_terminal::SendQByteArrayByCan(const QByteArray& data)
{
    CanMessage msg;
    uint32_t address = 0x170 + GetCanId();
    int sendIndex = 0;
    int dlc = 8;
    while(sendIndex < data.length())
    {
        // Set payload data
        for(uint8_t i = 0; (i < 8 && sendIndex < data.length()); i++)
        {
            msg.setDataAt(i, data[sendIndex]);
            sendIndex++;
        }
        int vel = sendIndex % 8;
        if(vel == 0)
        {
            dlc = 8;
        }
        else
        {
            dlc = vel;
        }
        msg.setId(address);
        msg.setLength(dlc);
        msg.setExtended(false);
        msg.setRTR(false);
        msg.setErrorFrame(false);
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        msg.setTimestamp(tv);
        msg.setTx(true);
        CanInterfaceIdList canInterfaceIdList = backend().getInterfaceList();
        if(canInterfaceIdList.isEmpty() == false)
        {
            CanInterface* intf = backend().getInterfaceById(canInterfaceIdList.at(0));
            try
            {
                intf->sendMessage(msg);
                backend().getTrace()->InsertCanMessageTrace(msg);
            }
            catch(...)
            {
                backend().stopMeasurement();    //关闭通道
                QMessageBox::warning(this, tr("warning"), QString("The response times out. Please reinsert the device and try again!"));
            }

        }
        else
        {
            qDebug() << "no device";
        }
    }
}
int MainWindow_terminal::SetCanInterfaceId(int interfaceId)
{
    return m_nCanInterfaceId = interfaceId;
}
bool MainWindow_terminal::IsCanDevice(void)
{
    return m_ui->actionCanDevice->isChecked();
}
//! [6]
void MainWindow_terminal::writeData(const QByteArray& data)
{
    if(data.length() == 1 && data[0] == 0x08)       //backspace
    {
        if(tmpByteArray.length() >= 1)
        {
            tmpByteArray.resize(tmpByteArray.length() - 1);
        }
    }
    else
    {
        tmpByteArray.append(data);
    }
    if(data.contains('\r'))
    {
        if(tmpByteArray.length() > 1)
        {
            tmpByteArray.resize(tmpByteArray.length() - 1);
            if(m_sLastCommandList.contains(QString(tmpByteArray)))  //包含则移到最后位置
            {
                m_sLastCommandList.removeOne(QString(tmpByteArray));
            }
            m_sLastCommandList.append(QString(tmpByteArray));
            if(m_sLastCommandList.length() > 10)
            {
                m_sLastCommandList.removeFirst();
            }
            m_ui->lineEdit->setText(m_sLastCommandList.last());
            m_nCurrentIndexCommandList = m_sLastCommandList.length() - 1;
            if(m_sLastCommandList.last().indexOf("upgrade") >= 0)
            {
                //                qDebug() << __func__ << __LINE__;
                Slot_StartDownLoad();   //开始升级
            }
        }
        m_console->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);   //将光标移动到文本末尾
        tmpByteArray.clear();
    }
    if(IsCanDevice())
    {
        SendQByteArrayByCan(data);
    }
    else
    {
        m_serial->write(data);
    }

}
//! [6]

//! [7]
void MainWindow_terminal::readData()
{
    const QByteArray data = m_serial->readAll();
    m_console->putData(data);
}
Backend& MainWindow_terminal::backend()
{
    return Backend::instance();
}
int  MainWindow_terminal::CallBackReveiveCanData(CanMessage message)
{
    //    CanInterfaceIdList canInterfaceIdList = backend().getInterfaceList();
    //    CanInterface* intf = backend().getInterfaceById(canInterfaceIdList.at(0));
    //    intf->sendMessage(message);
    //    m_sMainWindow_Download->HandleCanMessage(&message);
    //    qDebug() << QString("Value[%1]").arg(message.getId());
    return 0;
}

//! [7]
void MainWindow_terminal::SlotReveiveCanData(int idx)
{
    CanTrace* p_sTrace = backend().getTrace();

    const CanMessage* p_sCanMessage = p_sTrace->getMessage(idx);
    //    qDebug() << "test" << idx << p_sCanMessage->getLength() << p_sCanMessage->getByte(0);
    if(p_sCanMessage->getId() == (0x170 + GetCanId()))
    {
        QByteArray data ;
        for(int i = 0; i < p_sCanMessage->getLength(); i++)
        {
            data.append(p_sCanMessage->getByte(i));
        }
        int flagSkipInsertDataToConsole = 0;             //标识不更新console
        if(data.length() == 1)                          //屏蔽delet反馈
        {
            if(data[0] == (char)0x7F)
            {
                flagSkipInsertDataToConsole = 1;
            }
        }
        if(data.length() == 3)                          //屏蔽backspace反馈
        {
            if((data[0] == (char)0x08) && (data[1] == (char)0x20) && (data[2] == (char)0x08))
            {
                flagSkipInsertDataToConsole = 1;
            }
        }
        if(flagSkipInsertDataToConsole == 0)
        {
            m_console->putData(data);
        }
        if(m_nFlagSaveLog)       //追加日志
        {
            static int firstLine = 1;       //针对首次打开插入时间戳问题
            if(firstLine)
            {
                firstLine = 0;
                data.insert(0, '\n');
            }
            if(data.length() >= 2)
            {
                int dataLen = data.length() - 1;
                int index = 0;
                for(int i = 0; i < dataLen;)
                {
                    index = data.indexOf('\n', i);
                    if(index >= 0) //行首增加时间
                    {
                        QDateTime current_date_time = QDateTime::currentDateTime();
                        QString current_date = current_date_time.toString("[yyyy.MM.dd-hh:mm:ss.zzz] ");
                        data.insert(index + 1, current_date);
                        data.remove(index, 1);      //去除回车 \n
                        i += index + 1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            m_QStLogData.append(data);
        }
        data.clear();
    }
    else if(p_sCanMessage->getId() == 0x160)
    {
        AddDeviceList(p_sCanMessage->getByte(0));
    }
    m_sMainWindow_Download->HandleCanMessage(p_sCanMessage);          //使用回调函数
}
//! [8]
void MainWindow_terminal::handleError(QSerialPort::SerialPortError error)
{
    if(error == QSerialPort::ResourceError)
    {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}
//! [8]

void MainWindow_terminal::initActionsConnections()
{
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow_terminal::OpenDevice);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow_terminal::CloseDevice);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow_terminal::close);
    //    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionConfigure, &QAction::triggered, this, &MainWindow_terminal::showConfig);
    connect(m_ui->actionClear, &QAction::triggered, m_console, &Console::clear);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow_terminal::about);
    //    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(m_ui->actionCallDevices, &QAction::triggered, this, &MainWindow_terminal::Slot_CallAllCanDevices);

    connect(m_ui->actioncangaroo, &QAction::triggered, this, &MainWindow_terminal::SlotShowCangaroo);
    connect(m_ui->actionCanAlyst, &QAction::triggered, this, &MainWindow_terminal::SlotShowCanalyst);
    connect(m_ui->actionDownload, &QAction::triggered, this, &MainWindow_terminal::Slot_StartDownLoad);
    connect(m_ui->actionSave, &QAction::triggered, this, &MainWindow_terminal::Slot_SaveLog);
}
void MainWindow_terminal::Slot_HandleTimeout(void)
{
    if(m_Qtimer_2s->isActive())
    {
        if(m_nFlagSaveLog && m_QStLogData.isEmpty() == false)
        {
            QFile file(m_QStrFileName);//内容保存到路径文件
            if(file.open(QIODevice::WriteOnly | QIODevice::Text))//以文本方式打开
            {
                QTextStream out(&file); //IO设备对象的地址对其进行初始化
                out << m_QStLogData << Qt::endl   ; //输出
                //                QMessageBox::warning(this, tr("Finish"), tr("Successfully save the file!"));
                file.close();
            }
            else
            {
                //                QMessageBox::warning(this, tr("Error"), tr("File to open file!"));
                qDebug() << QString("File to open file!");
            }
        }
    }
}

void MainWindow_terminal::Slot_SaveLog(void)
{
    //    m_ui->actionConfigure->setEnabled(false);

    if(m_nFlagSaveLog)      //关闭日志记录
    {
        m_nFlagSaveLog = 0;
        m_Qtimer_2s->stop();
        m_ui->actionSave->setToolTip("save log to ...");
        m_ui->actionSave->setIcon(QIcon(":/images/save.png"));
        QMessageBox::warning(this, tr("Finish"), tr("Successfully save log <br>%1!").arg(m_QStrFileName));
    }
    else                    //打开日志记录
    {
        QFileDialog dlg(this);
        //获取内容的保存路径
        m_QStrFileName = dlg.getSaveFileName(this, tr("Save As"), "./", tr("Text File(*.log)"));
        if(m_QStrFileName == "")        //未选中
        {
            return;
        }
        m_nFlagSaveLog = 1;
        m_Qtimer_2s->start(3000);           //3s记录一次
        m_ui->actionSave->setToolTip("saving log ...");
        m_ui->actionSave->setIcon(QIcon(":/images/saved.png"));
    }
}

void MainWindow_terminal::Slot_StartDownLoad(void)
{
    CanInterfaceIdList canInterfaceIdList = backend().getInterfaceList();
    if(canInterfaceIdList.isEmpty() == false)
    {
        CanInterface* intf = backend().getInterfaceById(canInterfaceIdList.at(0));
        if(intf->isOpen())
        {
            m_sMainWindow_Download->show();
        }
        else
        {
            showConfig();
        }
    }
    else
    {
        showConfig();
    }
}


void MainWindow_terminal::showConfig(void)
{
    if(IsCanDevice())
    {
        emit EmitSignalOpenCanDevice();
    }
    else
    {
        m_settings->show();
    }

}

void MainWindow_terminal::SlotShowCangaroo(void)
{
    emit EmitSignalShowCangaroo();
}

void MainWindow_terminal::SlotShowCanalyst(void)
{
    if(m_ui->actionCanAlyst->isChecked())
    {
        this->m_sDockRight->show();
    }
    else
    {
        this->m_sDockRight->hide();
    }

}

void MainWindow_terminal::Slot_DeviceList_ItemClicked(QListWidgetItem* item)
{
    int canId = item->text().toInt();
    //    qDebug() << __func__ << __LINE__ << item->text() << item->checkState();
    if(item->checkState() == Qt::Checked)
    {
        if(m_sQListDevice.contains(canId))
        {
            ;
        }
        else
        {
            m_sQListDevice.append(canId);
            m_sMainWindow_Download->SetCanId(canId);
            m_ui->spinBox->setValue(canId);
        }
    }
    else
    {
        if(m_sQListDevice.contains(canId))
        {
            m_sQListDevice.removeOne(canId);
            if(m_sQListDevice.isEmpty() != true)
            {
                canId = m_sQListDevice.last();
                m_sMainWindow_Download->SetCanId(canId);
                m_ui->spinBox->setValue(canId);
            }
        }
    }
}
void MainWindow_terminal::AddDeviceList(int canId)
{
    int i = 0, nCnt = m_sQListWidget->count();
    for(i = 0; i < nCnt; ++i)
    {
        QListWidgetItem* pItem = m_sQListWidget->item(i);
        if(pItem->text().indexOf(QString("%1").arg(canId)) >= 0)
        {
            break;
        }
    }
    if(i >= nCnt)   //不存在则新增
    {
        QListWidgetItem* item = new QListWidgetItem;
        item->setData(Qt::DisplayRole, QString("%1").arg(canId));
        item->setData(Qt::CheckStateRole, Qt::Unchecked);

        m_sQListWidget->addItem(item);
    }

    m_sDockLeft->show();
}
#include <stdio.h>
#include <sys/time.h>


inline long getCurrentTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;

}
void MainWindow_terminal::Slot_CallAllCanDevices(void)
{
    m_sQListWidget->clear();
    CanMessage msg;
    msg.setDataAt(0, 0x01);
    msg.setId(0x160);
    msg.setLength(8);
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    msg.setTimestamp(tv);
    msg.setTx(true);
    CanInterfaceIdList canInterfaceIdList = backend().getInterfaceList();
    if(canInterfaceIdList.isEmpty() == false)
    {
        CanInterface* intf = backend().getInterfaceById(canInterfaceIdList.at(0));
        try
        {
            intf->sendMessage(msg);
            backend().getTrace()->InsertCanMessageTrace(msg);
        }
        catch(...)
        {
            backend().stopMeasurement();    //关闭通道
            QMessageBox::warning(this, tr("warning"), QString("The response times out. Please reinsert the device and try again!"));
        }

    }
    else
    {
        qDebug() << "no device";
    }
}

void MainWindow_terminal::showStatusMessage(const QString& message)
{
    m_status->setText(message);
}
void MainWindow_terminal::keyPressEvent(QKeyEvent* e)
{
    switch(e->key())
    {
        case Qt::Key_Return:
            writeData(m_ui->lineEdit->text().toLatin1().append('\r'));
            break;
        case Qt::Key_Up:
            if(m_sLastCommandList.isEmpty() == false)
            {
                m_nCurrentIndexCommandList--;
                if(m_nCurrentIndexCommandList < 0)
                {
                    m_nCurrentIndexCommandList = 0;
                }
                QString strTmp = m_sLastCommandList.at(m_nCurrentIndexCommandList);
                m_ui->lineEdit->setText(strTmp);
            }
            break;
        case Qt::Key_Down:
            if(m_sLastCommandList.isEmpty() == false)
            {
                m_nCurrentIndexCommandList++;
                if(m_nCurrentIndexCommandList >= m_sLastCommandList.length())
                {
                    m_nCurrentIndexCommandList = m_sLastCommandList.length() - 1;
                }
                QString strTmp = m_sLastCommandList.at(m_nCurrentIndexCommandList);
                m_ui->lineEdit->setText(strTmp);
            }
            break;
        default:
            ;
    }
}

// Window::checkForUpdates
void MainWindow_terminal::Slot_checkForUpdates(void)
{
    /* Get settings from the UI */
    QString version = BUTIANYUN_VERSION;
    //    bool customAppcast =false;
    //    bool downloaderEnabled = true;
    //    bool notifyOnFinish = false;
    //    bool notifyOnUpdate = false;
    //    bool mandatoryUpdate = false;

    /* Apply the settings */
        m_updater->setModuleVersion(DEFS_URL, version);
    //    m_updater->setNotifyOnFinish(DEFS_URL, notifyOnFinish);
    //    m_updater->setNotifyOnUpdate(DEFS_URL, notifyOnUpdate);
    //    m_updater->setUseCustomAppcast(DEFS_URL, customAppcast);
    //    m_updater->setDownloaderEnabled(DEFS_URL, downloaderEnabled);
    //    m_updater->setMandatoryUpdate(DEFS_URL, mandatoryUpdate);

    /* Check for updates */
    m_updater->checkForUpdates(DEFS_URL);
}
/**
 * Compares the two version strings (\a x and \a y).
 *     - If \a x is greater than \y, this function returns \c true.
 *     - If \a y is greater than \x, this function returns \c false.
 *     - If both versions are the same, this function returns \c false.
 */
static bool compare(const QString &x, const QString &y)
{
    QStringList versionsX = x.split(".");
    QStringList versionsY = y.split(".");

    int count = qMin(versionsX.count(), versionsY.count());

    for (int i = 0; i < count; ++i)
    {
            int a = QString(versionsX.at(i)).toInt();
            int b = QString(versionsY.at(i)).toInt();

            if (a > b)
                return true;

            else if (b > a)
                return false;
    }

    return versionsY.count() < versionsX.count();
}
// Window::updateChangelog
void MainWindow_terminal::Slot_updateChangelog(const QString &url)
{
    if (url == DEFS_URL){
            QString latestVersion=m_updater->getLatestVersion(DEFS_URL);
            QString    ModuleVersion=m_updater->getModuleVersion(DEFS_URL);
            if(compare(latestVersion,ModuleVersion)){
//               QMessageBox::information(this, tr("Congratulations"), "There are a versions available");
            }else{
                QMessageBox::information(this, tr("Congratulations"), "Now you have the latest version!!!");
            }
            qDebug()<<__func__<<__LINE__<<(m_updater->getChangelog(url))<<latestVersion<<  ModuleVersion ;
//            this->close();
//            qApp->quit();
    }
}
// Window::displayAppcast
void MainWindow_terminal::Slot_displayAppcast(const QString &url, const QByteArray &reply)
{
    if (url == DEFS_URL)
    {
            QString text = "This is the downloaded appcast: <p><pre>" + QString::fromUtf8(reply)
                           + "</pre></p><p> If you need to store more information on the "
                             "appcast (or use another format), just use the "
                             "<b>QSimpleUpdater::setCustomAppcast()</b> function. "
                             "It allows your application to interpret the appcast "
                             "using your code and not QSU's code.</p>";

            qDebug()<<__func__<<__LINE__<<(text);
    }
}
