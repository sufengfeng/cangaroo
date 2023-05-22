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
    m_console->setEnabled(false);
    setCentralWidget(m_console);

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);

    m_ui->statusBar->addWidget(m_status);

    initActionsConnections();

    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow_terminal::handleError);

    //! [2]
    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow_terminal::readData);
    //! [2]
    connect(m_console, &Console::getData, this, &MainWindow_terminal::writeData);
    //! [3]

}
//! [3]

MainWindow_terminal::~MainWindow_terminal()
{
    backend().stopMeasurement();

    delete m_settings;
    delete m_ui;
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
    QMessageBox::about(this, tr("About Simple Terminal"),
                       tr("The <b>Simple Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}
void MainWindow_terminal::SendMessageByCan(const QByteArray& data)
{
    //    qDebug() << data;
    CanMessage msg;

    bool en_extended = false;
    bool en_rtr = false;

    //    uint8_t data_int[64];
    int data_ctr = 0;

    //    data_int[data_ctr++] = ui->fieldByte0_0->text().toUpper().toInt(NULL, 16);

    uint32_t address = 0x80;

    uint8_t dlc = data.length();
    if(dlc > 8)
    {
        dlc = 8;
    }
    // Set payload data
    for(int i = 0; i < dlc; i++)
    {
        //        data_int[data_ctr++] = data[i];
        msg.setDataAt(i, data[i]);
    }

    msg.setId(address);
    msg.setLength(dlc);

    msg.setExtended(en_extended);
    msg.setRTR(en_rtr);
    msg.setErrorFrame(false);

    //    if(ui->checkbox_BRS->isChecked())
    //    {
    //        msg.setBRS(true);
    //    }
    //    if(ui->checkbox_FD->isChecked())
    //    {
    //        msg.setFD(true);
    //    }

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

    //    char outmsg[256];
    //    snprintf(outmsg, 256, "Send [%s] to %d on port %s [ext=%u rtr=%u err=%u fd=%u brs=%u]",
    //             msg.getDataHexString().toLocal8Bit().constData(), msg.getId(), intf->getName().toLocal8Bit().constData(),
    //             msg.isExtended(), msg.isRTR(), msg.isErrorFrame(), msg.isFD(), msg.isBRS());
    //    log_info(outmsg);

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
    QByteArray data ;
    for(int i = 0; i < p_sCanMessage->getLength(); i++)
    {
        data.append(p_sCanMessage->getByte(i));
    }
    m_console->putData(data);
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
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow_terminal::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow_terminal::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow_terminal::close);
    //    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionConfigure, &QAction::triggered, this, &MainWindow_terminal::showConfig);
    connect(m_ui->actionClear, &QAction::triggered, m_console, &Console::clear);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow_terminal::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}
#include "QDebug"
void MainWindow_terminal::showConfig(void)
{
    if(IsCanDevice())
    {
        emit showCangaroo();
    }
    else
    {
        m_settings->show();
    }

}

void MainWindow_terminal::showStatusMessage(const QString& message)
{
    m_status->setText(message);
}
