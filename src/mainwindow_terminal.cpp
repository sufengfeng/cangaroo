#include "mainwindow_terminal.h"
#include "ui_mainwindow_terminal.h"

#include "console.h"
#include "settingsdialog.h"

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

//! [6]
void MainWindow_terminal::writeData(const QByteArray& data)
{
    m_serial->write(data);
}
//! [6]

//! [7]
void MainWindow_terminal::readData()
{
    const QByteArray data = m_serial->readAll();
    m_console->putData(data);
}
//! [7]

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
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    //    connect(m_ui->actionConfigure, &QAction::triggered, this, &MainWindow_terminal::showConfig);
    connect(m_ui->actionClear, &QAction::triggered, m_console, &Console::clear);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow_terminal::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}
void MainWindow_terminal::showConfig(void)
{
    if(m_ui->actionCanDevice->isChecked())
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
