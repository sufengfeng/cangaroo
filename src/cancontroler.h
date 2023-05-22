#ifndef CANCONTROLER_H
#define CANCONTROLER_H

#include <QString>
#include <QtDebug>
#include <QDomDocument>
#include <QTimer>
#include <core/Backend.h>
#include <driver/CanInterface.h>
#include "core/CanTrace.h"
class Console;
class CanControler: public QObject
{
    Q_OBJECT
public:
    CanControler();
    ~CanControler();
    void SendMessageByCan(QString* str);
    Backend& backend();
    int SetCanInterfaceId(int interfaceId);
    void SetConsoleHandle(Console* p_sConsole);
    //public slots:
    //    void SlotReveiveCanData(int idx);
private:
    Console* m_sConsole = nullptr;
    int m_nCanInterfaceId;
};

#endif // CANCONTROLER_H
