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
#include "workerdownloadthread.h"
typedef void (*Fun_t)(bool checked);
//void clicked(bool checked = false);
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
}
//! [3]

MainWindow_terminal::~MainWindow_terminal()
{
    backend().stopMeasurement();
    delete m_sMainWindow_Download;
    delete m_settings;
    delete m_sDockRight;
    delete m_sTraceDockLeft;

    m_sQListDevice.clear();
    m_sQListWidget->clear();    //清空
    delete m_sQListWidget;
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
    QMessageBox::about(this, tr("About GCAN-Term"),
                       QString("Version: <b>V1.4</b><br>"
                               "The <b>GCAN-Term</b> is used for can device debugging, factory testing, and firmware download for Geekplus<br>"
                               "build time:[%1 %2]<br>").arg(__DATE__).arg(__TIME__)
                      );

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
    int dlc = data.length();
    if(dlc > 8)
    {
        dlc = 8;
    }
    while(sendIndex < data.length())
    {
        // Set payload data
        for(uint8_t i = 0; (i < 8 && sendIndex < data.length()); i++)
        {
            msg.setDataAt(i, data[sendIndex]);
            sendIndex++;
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
            catch(char* ch)
            {
                QMessageBox::warning(this, tr("warning"), QString(ch));
            }
            catch(...)
            {
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
    static QByteArray tmpByteArray;

    if(data.contains('\r'))
    {
        tmpByteArray.append(data);
        if(tmpByteArray.length() > 1)
        {
            if(m_sLastCommandList.contains(QString(tmpByteArray)))  //包含则移到最后位置
            {
                m_sLastCommandList.removeOne(QString(tmpByteArray));
            }
            m_sLastCommandList.append(QString(tmpByteArray));
            if(m_sLastCommandList.length() > 10)
            {
                m_sLastCommandList.removeFirst();
            }
            m_ui->lineEdit->setText(QString(tmpByteArray));
            m_nCurrentIndexCommandList = m_sLastCommandList.length() - 1;
            if(QString(tmpByteArray).indexOf("upgrade") >= 0)
            {
                //                qDebug() << __func__ << __LINE__;
                Slot_StartDownLoad();   //开始升级
            }
        }
        m_console->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);   //将光标移动到文本末尾
        tmpByteArray.clear();
    }
    else
    {
        tmpByteArray.append(data);
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
int  MainWindow_terminal::CallBackReveiveCanData(int idx)
{
    //        SlotReveiveCanData(idx);
    //    qDebug() << QString("Value[%1]").arg(idx);
    //    CanTrace* p_sTrace = backend().getTrace();
    //    const CanMessage* p_sCanMessage = p_sTrace->getMessage(idx);
    //    m_sMainWindow_Download->HandleCanMessage(p_sCanMessage);
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
            int dataLen = data.length() - 2;
            for(int i = 0; i < dataLen;)
            {
                int index = data.indexOf('\n', i);
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

            m_QStLogData.append(data);
        }
        data.clear();
    }
    else if(p_sCanMessage->getId() == 0x160)
    {
        AddDeviceList(p_sCanMessage->getByte(0));
    }
    m_sMainWindow_Download->HandleCanMessage(p_sCanMessage);
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
    //    return;
    if(m_Qtimer_2s->isActive())
    {
        if(m_nFlagSaveLog)
        {
            QFile file(m_QStrFileName);//内容保存到路径文件
            if(file.open(QIODevice::WriteOnly | QIODevice::Text))//以文本方式打开
            {
                QTextStream out(&file); //IO设备对象的地址对其进行初始化
                out << m_QStLogData << endl   ; //输出
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
        QMessageBox::warning(this, tr("Finish"), tr("Successfully save the file!"));
    }
    else                    //打开日志记录
    {
        QFileDialog dlg(this);
        //获取内容的保存路径
        m_QStrFileName = dlg.getSaveFileName(this, tr("Save As"), "./", tr("Text File(*.txt)"));
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

        catch(char* ch)
        {
            QMessageBox::warning(this, tr("warning"), QString(ch));
        }
        catch(...)
        {
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
            m_sLastCommandList.append(m_ui->lineEdit->text());
            if(m_sLastCommandList.length() > 10)
            {
                m_sLastCommandList.removeFirst();
            }
            m_ui->lineEdit->clear();
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
                QByteArray arrayTmp = strTmp.toLatin1();

                m_ui->lineEdit->setText(QString(arrayTmp.left(arrayTmp.length() - 1)));
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
                QByteArray arrayTmp = strTmp.toLatin1();

                m_ui->lineEdit->setText(QString(arrayTmp.left(arrayTmp.length() - 1)));
            }
            break;
        default:
            ;
    }

}
