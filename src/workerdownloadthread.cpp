#include "workerdownloadthread.h"
#include <QDebug>


//WorkerThread.cpp
WorkerDownloadThread::WorkerDownloadThread()
    : QThread()
    , m_stopFlag(false)
{

}

WorkerDownloadThread::~WorkerDownloadThread()
{
}

void WorkerDownloadThread::StartUpdateHardWare(QString filePathName)
{
    m_stopFlag = false;
    m_sFilePathName = filePathName;
    start();    //启动即开始运行
}

void WorkerDownloadThread::stop()
{
    m_stopFlag = true;
    this->quit();
    this->wait();
}

void WorkerDownloadThread::run()
{
    //该线程管理类对应的线程实际运行代码位置
    while(1)
    {
        int value = 0;
        while(!m_stopFlag)
        {
            //do something
            value++;
            emit Signal_progress(value, QString("value:") + value);
            //usleep(100);
            msleep(100);

            if(value >= 100)
            {
                stop();
            }
        }
    }
}
void WorkerDownloadThread::HandleCanMessage(const CanMessage* RxMessage)
{
    u32 tmpStdID =  GET_C2STD_SRC(RxMessage->getId());
    u32 tmptype =  GET_C2STD_TYPE(RxMessage->getId());
    u32 tmpIndex = 0;

    SMotorUpdate* pMotor = &subBoardUpdate[0];//tmpMotorID=5
    uint8_t tmpData[32];
    for(int i = 0; i < RxMessage->getLength(); i++)
    {
        tmpData[i] = RxMessage->getByte(i);
    }
    uint8_t* pData = tmpData;

    if(tmptype == 3)        //(tmpStdID == CAN2_RECV_PACK_TOTAL)//7
    {
        pMotor->responseWordTotal = UI32_MAKE(pData[0], pData[1], pData[2], pData[3]);
        pMotor->responseWordCheckSum = UI32_MAKE(pData[4], pData[5], pData[6], pData[7]);
        pMotor->eSendSt = MUPDATE_SENDED_SUCC;
    }
    else if(tmptype == 2)   //(tmpStdID == CAN2_RECV_EXPET_PACK_NO)//6
    {
        tmpIndex = UI32_MAKE(pData[0], pData[1], pData[2], pData[3]);
        if(tmpIndex <= pMotor->responseWordIndex + 1 && tmpIndex >= pMotor->responseWordIndex)
        {
            pMotor->responseWordIndex = tmpIndex;
        }
        pMotor->eSendSt = MUPDATE_SENDING_PACK;
    }

    if(tmptype == 1)        //((tmpStdID == CAN2_RECV_EXPET_PACK_NO)&&(tmptype==1))//bjf_version
    {
        uint32_t version  =  UI32_MAKE(pData[0], pData[1], pData[2], pData[3]);
        //        SetSoftwareVersion(VER_GAC_SUB, version);                   //设置电机子板版本
        qDebug() << "version:" << version;
        uint32_t buildtime  =  UI32_MAKE(pData[4], pData[5], pData[6], pData[7]);
        qDebug() << "version:" << buildtime;
        //SetSoftwareVersion(VER_GAC_SUB_DATA, buildtime);          //设置电机子板编译时间
    }
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
    stop();
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
        #if 0
        if(GetControlWifiCount() > LOG_UPLOAD_TIMES)
        {
            SetControlWifiCount(0);
            UploadRobotInfo();
        }
        #endif
        if(pMotor->eSendSt == MUPDATE_SENDED_SUCC || (pMotor->responseWordIndex == gBinSizeWord)) //MUPDATE_SENDED_SUCC:4
        {
            static uint8_t counter = 0;
            counter++;
            INF("pMotor->eSendSt == MUPDATE_SENDED_SUCC. Finish!\r\n");
            if(counter > 20)
            {
                INF("pMotor->eSendSt == MUPDATE_SENDED_SUCC. Finish Done!\r\n");
                msleep(1000);
                break;
            }
        }
        switch(pMotor->eSendSt)
        {
            case MUPDATE_NULL:
                INF("-> MUPDATE_NULL\r\n");
                pMotor->readFlashStartAddr = gBinFlashAddr;
                pMotor->sendWordTotal      = gBinSizeWord ;
                pMotor->sendWordCheckSum   = gBinCheckSum;
                pMotor->eSendSt = MUPDATE_SEND_HEADER;

                //SetSingleMotor(0x01, false);
                bsp_SubBoard_Update_InitCmd(C2_OBST_UP_M, 0, 0, 0); //(CAN2_SEND_UPDATE_INIT,0,0,1);//CAN2_SEND_UPDATE_INIT:4
                msleep(100);
                break;
            case MUPDATE_SEND_HEADER:
                //INF("-> MUPDATE_SEND_HEADER (%d)\r\n", send_header_cnt++);
                bsp_SubBoard_Update_InitCmd(C2_OBST_UP_M, pMotor->sendWordTotal, pMotor->sendWordCheckSum, 1); //(CAN2_SEND_UPDATE_INIT, pMotor->sendWordTotal, pMotor->sendWordCheckSum,0);
                msleep(100);
                break;
            case MUPDATE_SENDING_PACK://3
                //                pMotor->sendData = READ_WORD(pMotor->readFlashStartAddr + ((pMotor->responseWordIndex - 1) * 4));
                memcpy(&pMotor->sendData, m_sBinFileRawData.data() + ((pMotor->responseWordIndex - 1) * 4), sizeof(int));
                //BJFUpdateSendPackData(CAN2_SEND_UPDATE_PACKET, pMotor->responseWordIndex, pMotor->sendData);
                SubBoardUpdateSendPackData(C2_OBST_UP_M, pMotor->responseWordIndex, pMotor->sendData, 2);
                msleep(1);
                if(!(pMotor->responseWordIndex % 100) || pMotor->responseWordIndex < 10)
                {
                    INF("[%d] index=%d(%d)\r\n", pMotor->sendMotorID, pMotor->responseWordIndex, gBinSizeWord);
                }
                SubBoard_watch_flag = true;
                SubBoard_watch_pack = pMotor->responseWordIndex;
                SubBoard_watch_total = gBinSizeWord;
                break;
            default:
                ERR("-> MUPDATE_(ERR)\r\n");
                break;
        }
    }
}


void WorkerDownloadThread::bsp_SubBoard_Update_InitCmd(u32 stdID, u32 binSize, u32 binCheckSum, u32 type)
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
    canMessage.setId(SET_C2STD_OBS(stdID, C2ID_BATT, type, (0 + 1)));

    Signal_SendCanMessage(&canMessage);


    //    CAN1_Send_Msg_SubBoard(&tmpCandata[0], type, stdID);
}

void WorkerDownloadThread::SubBoardUpdateSendPackData(u32 stdID, unint32 dataIndex, unint32 dataSend, u32 type)
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
    canMessage.setId(SET_C2STD_OBS(stdID, C2ID_BATT, type, (0 + 1)));

    Signal_SendCanMessage(&canMessage);

    //    bsp_SubBoard_Update_Packet(stdID, &tempData08[0], type);
}
