/*

  Copyright (c) 2015, 2016 Hubert Denkmair <hubert@denkmair.de>

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"
#include <QApplication>
#include <QtDebug>
#include "buildversion/version.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include "windows.h"
#define ENABLE_TERMINAL "ENABLE_TERMINAL"
#define ENABLE_SAVE_LOG "ENABLE_SAVE_LOG"
bool l_bIsStdOutput=0;   //是否通过terminal标准输出
bool l_bIsSaveLog=0;   //是否记录日志
QString l_sFileName;     //保存名称
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
void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if(l_bIsStdOutput){
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type) {
        case QtDebugMsg:
            fprintf(stdout, "[DEBUG] %s\n", localMsg.constData());
            break;
        case QtWarningMsg:
            fprintf(stderr, "[WARNING] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtCriticalMsg:
            fprintf(stderr, "[CRITICAL] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            fprintf(stderr, "[FATAL] %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            abort();
        case QtInfoMsg:
            break;
        }
    }
    if(l_bIsSaveLog){
        QString dir_str = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)+"/log";
        if(isDirExist(dir_str))
        {
            QFile file(dir_str+"/"+l_sFileName+".txt");
            if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                return;
            }
            QTextStream out(&file);
            out.setCodec("UTF-8");
            switch (type) {
            case QtDebugMsg:
                out << QString("[DEBUG] %1\n").arg(msg);
                break;
            case QtWarningMsg:
                out << QString("[WARNING] %1 (%2:%3,%4)\n").arg(msg).arg(context.file).arg(context.line).arg(context.function);
                break;
            case QtCriticalMsg:
                out << QString("[CRITICAL] %1 (%2:%3,%4)\n").arg(msg).arg(context.file).arg(context.line).arg(context.function);
                break;
            case QtFatalMsg:
                out << QString("[FATAL] %1 (%2:%3,%4)\n").arg(msg).arg(context.file).arg(context.line).arg(context.function);
                abort();
            case QtInfoMsg:
                break;
            }
        }
    }
}
int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    QStringList list_parm=QCoreApplication::arguments();
    if(list_parm.contains(ENABLE_TERMINAL)){
        qInstallMessageHandler(customMessageHandler);
        AllocConsole();//打开控制台
        freopen("CONOUT$", "w", stdout);//将输出定向到控制台
        l_bIsStdOutput=1;
    }
    if(list_parm.contains(ENABLE_SAVE_LOG)){
        l_bIsSaveLog=1;
        QDateTime current_date_time =QDateTime::currentDateTime();
        l_sFileName =current_date_time.toString("yyyyMMdd_hhmmss_zzz");
    }
    a.setApplicationVersion(BUTIANYUN_VERSION);
    a.setApplicationName("cangaroo");
    MainWindow w;
    //    w.show();
    w.ShowTerminal();
    return a.exec();
}
