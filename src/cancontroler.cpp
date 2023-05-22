#include "cancontroler.h"

CanControler::CanControler()
{

}
CanControler::~CanControler()
{

}
Backend& CanControler::backend()
{
    return Backend::instance();
}
int CanControler::SetCanInterfaceId(int interfaceId)
{
    return m_nCanInterfaceId = interfaceId;
}
void CanControler::SetConsoleHandle(Console* p_sConsole)
{
    m_sConsole =  p_sConsole;
}
void CanControler::SendMessageByCan(QString* str)
{

    CanMessage msg;

    bool en_extended = false;
    bool en_rtr = false;

    uint8_t data_int[64];
    int data_ctr = 0;

    //    data_int[data_ctr++] = ui->fieldByte0_0->text().toUpper().toInt(NULL, 16);

    uint32_t address = 0x80;

    uint8_t dlc = 6;


    // Set payload data
    for(int i = 0; i < dlc; i++)
    {
        data_int[data_ctr++] = i;
        msg.setDataAt(i, data_int[i]);
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
    //    backend().getInterfaceList();
    CanInterface* intf = backend().getInterfaceById((CanInterfaceId)m_nCanInterfaceId);
    intf->sendMessage(msg);


    //    char outmsg[256];
    //    snprintf(outmsg, 256, "Send [%s] to %d on port %s [ext=%u rtr=%u err=%u fd=%u brs=%u]",
    //             msg.getDataHexString().toLocal8Bit().constData(), msg.getId(), intf->getName().toLocal8Bit().constData(),
    //             msg.isExtended(), msg.isRTR(), msg.isErrorFrame(), msg.isFD(), msg.isBRS());
    //    log_info(outmsg);

}

