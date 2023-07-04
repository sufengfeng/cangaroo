
/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAINWINDOW_TERMINAL_H
#define MAINWINDOW_TERMINAL_H

#include <QMainWindow>
#include <QSerialPort>
#include <QListWidget>
#include <QListWidgetItem>
//#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <core/Backend.h>
QT_BEGIN_NAMESPACE


namespace Ui
{
class MainWindow_terminal;
}

QT_END_NAMESPACE

class Console;
class SettingsDialog;
class MainWindow_Download;
class TraceWindow;

class MainWindow_terminal : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow_terminal(QWidget* parent = nullptr);
    ~MainWindow_terminal();

    Backend& backend();
public slots:
    void SlotReveiveCanData(int idx);
    void CanConnectStatusChanged(int status);
    void Slot_StartDownLoad(void);
    void Slot_SaveLog(void);
    void Slot_HandleTimeout(void);
signals:
    void EmitSignalShowCangaroo(void);
    void EmitSignalCloseCanDevice(void);
    void EmitSignalOpenCanDevice();
protected:
    void keyPressEvent(QKeyEvent* e);
private slots:
    void openSerialPort();
    void closeSerialPort();
    void about();
    void writeData(const QByteArray& data);
    void readData();
    void SendQByteArrayByCan(const QByteArray& data);
    int SetCanInterfaceId(int interfaceId);
    void handleError(QSerialPort::SerialPortError error);
    void showConfig();
    bool IsCanDevice(void);
    void Slot_CallAllCanDevices(void);
    void Slot_DeviceList_ItemClicked(QListWidgetItem* item);
    int GetCanId(void);
    void SlotShowCangaroo(void);
    void SlotShowCanalyst(void);
private:
    void AddDeviceList(int canId);
    void initActionsConnections();
    void OpenDevice();
    void CloseDevice();
private:
    void showStatusMessage(const QString& message);

    Ui::MainWindow_terminal* m_ui = nullptr;
    QLabel* m_status = nullptr;
    Console* m_console = nullptr;
    SettingsDialog* m_settings = nullptr;
    QSerialPort* m_serial = nullptr;
    int m_nCanInterfaceId;
    QList<QString>  m_sLastCommandList;
    int m_nCurrentIndexCommandList;
    QDockWidget* m_sDockRight;
    QListWidget* m_sQListWidget;
    QDockWidget* m_sDockLeft ;
    TraceWindow* m_sTraceDockLeft;
    MainWindow_Download* m_sMainWindow_Download;
    QList<int> m_sQListDevice;
    int m_nFlagSaveLog = 0;
    QTimer* m_Qtimer_2s;
    //获取内容的保存路径
    QString m_QStrFileName;
    QString m_QStLogData;
};

#endif // MAINWINDOW_H
