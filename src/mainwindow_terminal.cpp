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
#include <QLabel>
#include <QMessageBox>
#include "qmywidget.h"
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
    //浮动窗口
    m_sDockRight = new QDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, m_sDockRight);

    QMyWidget* widge = new QMyWidget;
    widge->size = QSize(300, 250);
    m_sDockRight->setWidget(widge);
    TraceWindow* trace = new TraceWindow(widge, backend());
    m_sDockRight->hide();

    //左侧浮动窗口
    //    QDockWidget* m_sDockLeft = new QDockWidget(this);
    //    addDockWidget(Qt::RightDockWidgetArea, m_sDockLeft);

    //    QMyWidget* widgeDeviceList = new QMyWidget;
    //    widge->size = QSize(200, 400);
    //    m_sDockLeft->setWidget(widgeDeviceList);
    //    QListWidget* listWidget = new QListWidget(widgeDeviceList);
    //    listWidget->addItem("devie01");
    //    listWidget->addItem("devie02");
    //    listWidget->addItem("devie03");
    //    m_sDockLeft->hide();

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

    delete m_settings;
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
                       tr("Version: <b>V1.1</b><br>"
                          "The <b>GCAN-Term</b> is used for can device debugging, factory testing, and firmware download for Geekplus"
                         ));
}
int MainWindow_terminal::GetCanId(void)
{
    return m_ui->spinBox->value();
}
void MainWindow_terminal::SendMessageByCan(const QByteArray& data)
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
        CanInterfaceIdList canInterfaceIdList = backend().getInterfaceList();
        if(canInterfaceIdList.isEmpty() == false)
        {
            CanInterface* intf = backend().getInterfaceById(canInterfaceIdList.at(0));
            intf->sendMessage(msg);
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
        SendMessageByCan(data);
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
        m_console->putData(data);
    }
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
    connect(m_ui->actionCallDevices, &QAction::triggered, this, &MainWindow_terminal::CallAllCanDevices);

    connect(m_ui->actioncangaroo, &QAction::triggered, this, &MainWindow_terminal::SlotShowCangaroo);
    connect(m_ui->actionCanAlyst, &QAction::triggered, this, &MainWindow_terminal::SlotShowCanalyst);
}
#include "QDebug"
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
void MainWindow_terminal::CallAllCanDevices(void)
{

    CanMessage msg;
    uint32_t address = 0x160;
    int dlc = 8;
    if(dlc > 8)
    {
        dlc = 8;
    }
    // Set payload data
    //    for(uint8_t i = 0; i < dlc; i++)
    //    {

    //    }
    msg.setDataAt(0, 0x01);
    msg.setId(address);
    msg.setLength(dlc);
    msg.setExtended(false);
    msg.setRTR(false);
    msg.setErrorFrame(false);
    CanInterfaceIdList canInterfaceIdList = backend().getInterfaceList();
    if(canInterfaceIdList.isEmpty() == false)
    {
        CanInterface* intf = backend().getInterfaceById(canInterfaceIdList.at(0));
        intf->sendMessage(msg);
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
