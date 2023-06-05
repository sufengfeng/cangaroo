#include "workerdownloadthread.h"
#include <QDebug>


//WorkerThread.cpp
WorkerDownloadThread::WorkerDownloadThread()
    : QThread()
    , m_stopFlag(false)
{
    m_nCanID = 12;
    m_nUpdateType = 0;
    m_nProceValue = 0;
}

WorkerDownloadThread::~WorkerDownloadThread()
{
}

void WorkerDownloadThread::StartUpdateHardWare(QString filePathName, int canid, int updateType)
{
    m_stopFlag = false;
    m_sFilePathName = filePathName;
    m_nCanID = canid;
    m_nUpdateType = updateType;
    start();    //启动即开始运行
}
void WorkerDownloadThread::StopDownload(void)
{
    SMotorUpdate* pMotor = &subBoardUpdate[0];//tmpMotorID=5
    m_stopFlag = true;
    pMotor->eSendSt = MUPDATE_SENDED_SUCC;

    //    this->wait();
}


void WorkerDownloadThread::run()
{
    //该线程管理类对应的线程实际运行代码位置
    while(1)
    {
        //        int value = 0;
        while(!m_stopFlag)
        {
            //            m_nUpdateType=1;
            //            bsp_SubBoard_Update_Start(m_nCanID, m_nUpdateType);
            //            m_stopFlag = true;
            QTime time;
            time.start();
            UpdateSubBoardMain();
            emit Signal_progress(m_nProceValue, QString(tr("update total time:[%1]s")).arg(time.elapsed() / 1000.0));
            StopDownload();
            //            //do something
            //            value++;
            //            emit Signal_progress(value, QString("value:") + value);
            //            //usleep(100);
            //            msleep(100);

            //            if(value >= 100)
            //            {
            //                stop();
            //            }
        }
    }
}
void WorkerDownloadThread::HandleCanMessage(const CanMessage* RxMessage)
{
    u32 tmpIndex = 0;
    SMotorUpdate* pMotor = &subBoardUpdate[0];//tmpMotorID=5
    uint8_t tmpData[32];
    for(int i = 0; i < RxMessage->getLength(); i++)
    {
        tmpData[i] = RxMessage->getByte(i);
    }
    uint8_t* pData = tmpData;
    uint32_t version;
    if(RxMessage->getId() == 0x01 && tmpData[0] == m_nCanID)
    {
        switch(tmpData[1])
        {
            case 0x09:
                version  =  UI32_MAKE(pData[2], pData[3], pData[4], pData[5]);
                emit Signal_progress(m_nProceValue, QString(tr("主MCU固件版本:0x%1")).arg(version, 0, 16));
                break;
            case 0x0a:
                version  =  UI32_MAKE(pData[2], pData[3], pData[4], pData[5]);
                emit Signal_progress(m_nProceValue, QString(tr("从MCU固件版本:0x%1")).arg(version, 0, 16));
                break;
            case 0x0b:
                version  =  UI32_MAKE(pData[2], pData[3], pData[4], pData[5]);
                emit Signal_progress(m_nProceValue, QString(tr("主MCU配置版本:0x%1")).arg(version, 0, 16));
                break;
            case 0x0c:
                version  =  UI32_MAKE(pData[2], pData[3], pData[4], pData[5]);
                emit Signal_progress(m_nProceValue, QString(tr("从MCU配置版本:0x%1")).arg(version, 0, 16));
                break;
            case 0x16:
                emit Signal_progress(m_nProceValue, QString(tr("升级开始...")).arg(version, 0, 16));
                break;
            case 0x7E:
                if(pData[3] == 2)
                {
                    emit Signal_progress(m_nProceValue, QString(tr("开始升级配置...")).arg(version, 0, 16));
                }
                else
                {
                    emit Signal_progress(m_nProceValue, QString(tr("开始升级固件...")).arg(version, 0, 16));
                }
                pMotor->eSendSt = MUPDATE_SEND_HEADER;      //确保升级配置
                break;
            default:
                break;
        }
    }
    else if(RxMessage->getId() == (SUB_FRONT_UPDATE_RES + m_nCanID))
    {
        tmpIndex = UI32_MAKE(pData[0], pData[1], pData[2], pData[3]);
        if(tmpIndex <= pMotor->responseWordIndex + 1 && tmpIndex >= pMotor->responseWordIndex)
        {
            pMotor->responseWordIndex = tmpIndex;
        }
        pMotor->eSendSt = MUPDATE_SENDING_PACK;
    }
    //    else if(RxMessage->getId() == (SUB_FRONT_UPDATE_SUCC_RES + m_nCanID))
    //    {
    //        pMotor->eSendSt = MUPDATE_SENDED_SUCC;
    //    }
    //    if(tmptype == 3)        //(tmpStdID == CAN2_RECV_PACK_TOTAL)//7
    //    {
    //        pMotor->responseWordTotal = UI32_MAKE(pData[0], pData[1], pData[2], pData[3]);
    //        pMotor->responseWordCheckSum = UI32_MAKE(pData[4], pData[5], pData[6], pData[7]);
    //        pMotor->eSendSt = MUPDATE_SENDED_SUCC;
    //    }
}
void WorkerDownloadThread::UpdateSubBoardMain(void)
{
    /* Check Ready and Init. */
    if(SubBoardUpdateStateReady())
    {
        emit Signal_progress(m_nProceValue, QString("State is not ready!\r\n"));
    }
    if(SubBoardUpdateInit())
    {
        emit Signal_progress(m_nProceValue,  "Failed init!\r\n");
    }

    SubBoardUpdate();//BJF ID

    SubBoardUpdateEnd();
}
//8位加法累加和取反
u16 CheckSumAdd08Anti(unsigned char* buffer, int length)
{
    unint32 addsum08 = 0;
    for(int uc = 0; uc < length; uc++)
    {
        addsum08 = addsum08 + buffer[uc];
    }
    addsum08 = ~addsum08;
    return (u16)addsum08;
}

//打开文件
unint32 WorkerDownloadThread::SubBoardUpdateStateReady(void)
{
    //        loadFile(FileName);
    m_sQFileInfo = new QFileInfo(m_sFilePathName);
    QFile* file = new QFile;
    /*
        * 读取Bin文件
        */
    file->setFileName(m_sQFileInfo->filePath());
    if(file->open(QIODevice::ReadOnly))
    {
        QDataStream BinFileData(file);
        char* pBuff = new char[file->size()];
        BinFileData.readRawData(pBuff, static_cast<int>(m_sQFileInfo->size()));
        m_sBinFileRawData = QByteArray(pBuff, static_cast<int>(m_sQFileInfo->size()));
        delete []pBuff ;
        file->close();
    }
    else
    {
        qDebug() << QString(tr("无法读取,请检查BIN文件路径!"));
    }
    delete file;

    return 0;
}


//打开文件
unint32 WorkerDownloadThread::SubBoardUpdateEnd(void)
{
    delete  m_sQFileInfo;
    return 0;
}
//计算checksum
unint32 WorkerDownloadThread::SubBoardUpdateInit(void)
{
    unint32 chkFlashState = 0;

    memset(&subBoardUpdate, 0x0, sizeof(subBoardUpdate));
    //    UPDATE_CTRL* pUTInfo;

    //    update_ctrl_flash = (UPDATE_CTRL*)FLASH_SECTOR_UPDATE_CTRL;
    //    update_ctrl_table =  *update_ctrl_flash;
    //    pUTInfo = &update_ctrl_table;

    gBinSizeWord  = m_sQFileInfo->size() / 4;
    //    gBinCheckSum  = pUTInfo->u32_robotconfig_checksum;
    //    gBinFlashAddr = pUTInfo->u32_robotconfig_addr;

    gBinCheckSum = CheckSumAdd08Anti((unsigned char*)m_sBinFileRawData.data(), m_sQFileInfo->size());
    emit Signal_progress(m_nProceValue,  QString("CRC=[%1]len=[%2]package=[%3]").arg(gBinCheckSum).arg(gBinSizeWord).arg(m_sQFileInfo->size()));
    return chkFlashState;
}

void WorkerDownloadThread::SubBoardUpdate(void)
{
    //INF("Update Motor[%d]:\r\n", upIDmotor);
    SMotorUpdate* pMotor = &subBoardUpdate[0];
    //pMotor->sendMotorID = upIDmotor;

    int send_header_cnt = 0;
    while(1)
    {
        if(pMotor->eSendSt == MUPDATE_SENDED_SUCC || (pMotor->responseWordIndex > gBinSizeWord)) //MUPDATE_SENDED_SUCC:4
        {
            static uint8_t counter = 0;
            counter++;
            emit Signal_progress(m_nProceValue,  "pMotor->eSendSt == MUPDATE_SENDED_SUCC. Finish!\r\n");
            if(counter > 20)
            {
                emit Signal_progress(m_nProceValue,  "pMotor->eSendSt == MUPDATE_SENDED_SUCC. Finish Done!\r\n");
                msleep(1000);
                break;
            }
        }
        switch(pMotor->eSendSt)
        {
            case MUPDATE_NULL:
                emit Signal_progress(m_nProceValue,  "-> MUPDATE_NULL\r\n");
                pMotor->readFlashStartAddr = gBinFlashAddr;
                pMotor->sendWordTotal      = gBinSizeWord ;
                pMotor->sendWordCheckSum   = gBinCheckSum;

                bsp_SubBoard_Update_Start(m_nCanID, m_nUpdateType); //(CAN2_SEND_UPDATE_INIT,0,0,1);//CAN2_SEND_UPDATE_INIT:4
                msleep(100);
                break;
            case MUPDATE_SEND_HEADER:
                emit Signal_progress(m_nProceValue,  "-> MUPDATE_SEND_HEADER ");
                bsp_SubBoard_Update_InitCmd(m_nCanID, pMotor->sendWordTotal, pMotor->sendWordCheckSum); //(CAN2_SEND_UPDATE_INIT, pMotor->sendWordTotal, pMotor->sendWordCheckSum,0);
                msleep(100);
                break;
            case MUPDATE_SENDING_PACK://3
            {
                uint32_t tmpResponseIndex = pMotor->responseWordIndex;
                memcpy(&pMotor->sendData, m_sBinFileRawData.data() + ((tmpResponseIndex - 1) * 4), sizeof(int));
                //BJFUpdateSendPackData(CAN2_SEND_UPDATE_PACKET, tmpResponseIndex, pMotor->sendData);
                SubBoardUpdateSendPackData(m_nCanID, tmpResponseIndex, pMotor->sendData);
                m_nProceValue = tmpResponseIndex * 100.0 / gBinSizeWord;
                emit Signal_progress(m_nProceValue,  "");
                if(!(tmpResponseIndex % 100) || tmpResponseIndex < 10)
                {
                    emit Signal_progress(m_nProceValue,  QString("[%1] index=%2(%3)").arg(pMotor->sendMotorID).arg(tmpResponseIndex).arg(gBinSizeWord));
                }
                if(tmpResponseIndex >= gBinSizeWord)
                {
                    SubBoardUpdateSendPackData(m_nCanID, tmpResponseIndex, pMotor->sendData);
                    emit Signal_progress(m_nProceValue,  QString("[%1] index=%2(%3)").arg(pMotor->sendMotorID).arg(tmpResponseIndex).arg(gBinSizeWord));
                    pMotor->eSendSt = MUPDATE_SENDED_SUCC ;
                }
                SubBoard_watch_flag = true;
                SubBoard_watch_pack = tmpResponseIndex;
                SubBoard_watch_total = gBinSizeWord;
            }
            break;
            default:
                ERR("-> MUPDATE_(ERR)\r\n");
                break;
        }
    }
}

//获取版本号
void WorkerDownloadThread::bsp_SubBoard_Update_Start(int canId, int updateType)
{
    CanMessage canMessage;

    uint8_t tmpCandata[8];
    memset(tmpCandata, 0, 8);
    tmpCandata[0] = canId;
    canMessage.setId(0x01);
    for(int i = 9; i < 0x0d; i++)       //获取版本号
    {
        tmpCandata[1] = i;
        canMessage.setData(tmpCandata[0], tmpCandata[1], tmpCandata[2], tmpCandata[3], tmpCandata[4], tmpCandata[5], tmpCandata[6], tmpCandata[7]);
        Signal_SendCanMessage(&canMessage);
        msleep(100);
    }
    //关闭电机
    tmpCandata[1] = 0x16;
    canMessage.setData(tmpCandata[0], tmpCandata[1], tmpCandata[2], tmpCandata[3], tmpCandata[4], tmpCandata[5], tmpCandata[6], tmpCandata[7]);
    Signal_SendCanMessage(&canMessage);
    msleep(100);

    //发送更新类型
    tmpCandata[1] = 0x7E;
    tmpCandata[3] = updateType;
    canMessage.setData(tmpCandata[0], tmpCandata[1], tmpCandata[2], tmpCandata[3], tmpCandata[4], tmpCandata[5], tmpCandata[6], tmpCandata[7]);
    Signal_SendCanMessage(&canMessage);
    msleep(100);
    //    CAN1_Send_Msg_SubBoard(&tmpCandata[0], type, stdID);
}

void WorkerDownloadThread::bsp_SubBoard_Update_InitCmd(u32 stdID, u32 binSize, u32 binCheckSum)
{
    CanMessage canMessage;

    uint8_t tmpCandata[8];
    tmpCandata[0] = UI32_HIHI8(binSize);
    tmpCandata[1] = UI32_HILO8(binSize);
    tmpCandata[2] = UI32_LOHI8(binSize);
    tmpCandata[3] = UI32_LOLO8(binSize);
    tmpCandata[4] = UI32_HIHI8(binCheckSum);
    tmpCandata[5] = UI32_HILO8(binCheckSum);
    tmpCandata[6] = UI32_LOHI8(binCheckSum);
    tmpCandata[7] = UI32_LOLO8(binCheckSum);
    canMessage.setData(tmpCandata[0], tmpCandata[1], tmpCandata[2], tmpCandata[3], tmpCandata[4], tmpCandata[5], tmpCandata[6], tmpCandata[7]);
    canMessage.setId(SUB_FRONT_UPDATE_START + stdID);

    Signal_SendCanMessage(&canMessage);
    msleep(100);

    //    CAN1_Send_Msg_SubBoard(&tmpCandata[0], type, stdID);
}

void WorkerDownloadThread::SubBoardUpdateSendPackData(u32 stdID, unint32 dataIndex, unint32 dataSend)
{
    CanMessage canMessage;
    unint08 tmpCandata[8];
    tmpCandata[0] = UI32_HIHI8(dataIndex);
    tmpCandata[1] = UI32_HILO8(dataIndex);
    tmpCandata[2] = UI32_LOHI8(dataIndex);
    tmpCandata[3] = UI32_LOLO8(dataIndex);
    tmpCandata[4] = UI32_HIHI8(dataSend);
    tmpCandata[5] = UI32_HILO8(dataSend);
    tmpCandata[6] = UI32_LOHI8(dataSend);
    tmpCandata[7] = UI32_LOLO8(dataSend);
    canMessage.setData(tmpCandata[0], tmpCandata[1], tmpCandata[2], tmpCandata[3], tmpCandata[4], tmpCandata[5], tmpCandata[6], tmpCandata[7]);
    canMessage.setId(SUB_FRONT_UPDATE_SEND + stdID);

    Signal_SendCanMessage(&canMessage);
    msleep(5);
    //    bsp_SubBoard_Update_Packet(stdID, &tempData08[0], type);
}
