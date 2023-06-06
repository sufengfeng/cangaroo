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
    SMotorUpdate* pMotor = &subBoardUpdate;//tmpMotorID=5
    m_stopFlag = true;
    pMotor->eSendSt = MUPDATE_SENDED_SUCC;
    //    this->wait();
}
void WorkerDownloadThread::run()
{
    //该线程管理类对应的线程实际运行代码位置
    while(1)
    {
        while(!m_stopFlag)
        {
            QTime time;
            time.start();
            UpdateSubBoardMain();
            emit Signal_progress(m_nProceValue, QString(tr("update total time:[%1]s")).arg(time.elapsed() / 1000.0));
            StopDownload();
        }
    }
}
void WorkerDownloadThread::HandleCanMessage(const CanMessage* RxMessage)
{
    u32 tmpIndex = 0;
    SMotorUpdate* pMotor = &subBoardUpdate;//tmpMotorID=5
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
    m_sQFileInfo = new QFileInfo(m_sFilePathName);
    QFile* file = new QFile;
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
    gBinSizeWord  = m_sQFileInfo->size() / 4;
    gBinCheckSum = CheckSumAdd08Anti((unsigned char*)m_sBinFileRawData.data(), m_sQFileInfo->size());
    emit Signal_progress(m_nProceValue,  QString("CRC=[%1]len=[%2]package=[%3]").arg(gBinCheckSum).arg(gBinSizeWord).arg(m_sQFileInfo->size()));
    return chkFlashState;
}

void WorkerDownloadThread::SubBoardUpdate(void)
{
    SMotorUpdate* pMotor = &subBoardUpdate;
    pMotor->sendMotorID = m_nCanID;
    //    int send_header_cnt = 0;
    int flag_MUPDATE_NULL = 1;
    int flag_MUPDATE_SEND_HEADER = 1;
    QTime time;
    time.start();
    while(1)
    {
        if(pMotor->eSendSt == MUPDATE_SENDED_SUCC || (pMotor->responseWordIndex > gBinSizeWord)) //MUPDATE_SENDED_SUCC:4
        {
            static uint8_t counter = 0;
            counter++;
            emit Signal_progress(m_nProceValue,  "pMotor->eSendSt == MUPDATE_SENDED_SUCC. Finish!");
            if(counter > 5)
            {
                emit Signal_progress(m_nProceValue,  "pMotor->eSendSt == MUPDATE_SENDED_SUCC. Finish Done!");
                msleep(1000);
                break;
            }
        }
        switch(pMotor->eSendSt)
        {
            case MUPDATE_NULL:
            {
                QString tmpStr = (flag_MUPDATE_NULL ? "-> Waiting for the device to be upgraded to be inserted..." : ".");
                if(flag_MUPDATE_NULL)
                {
                    flag_MUPDATE_NULL = 0;
                }
                emit Signal_progress(m_nProceValue,  tmpStr);
                pMotor->sendWordTotal      = gBinSizeWord ;
                pMotor->sendWordCheckSum   = gBinCheckSum;
                bsp_SubBoard_Update_Start(m_nCanID, m_nUpdateType);
                msleep(100);
                break;
            }
            case MUPDATE_SEND_HEADER:
            {
                QString tmpStr = (flag_MUPDATE_SEND_HEADER ? "-> Waiting for the Package header... " : ".");
                if(flag_MUPDATE_SEND_HEADER)
                {
                    flag_MUPDATE_SEND_HEADER = 0;
                }
                emit Signal_progress(m_nProceValue,  tmpStr);
                bsp_SubBoard_Update_InitCmd(m_nCanID, pMotor->sendWordTotal, pMotor->sendWordCheckSum);
                msleep(100);
                break;
            }
            case MUPDATE_SENDING_PACK://3
            {
                uint32_t tmpResponseIndex = pMotor->responseWordIndex;
                memcpy(&pMotor->sendData, m_sBinFileRawData.data() + ((tmpResponseIndex - 1) * 4), sizeof(int));
                SubBoardUpdateSendPackData(m_nCanID, tmpResponseIndex, pMotor->sendData);
                m_nProceValue = tmpResponseIndex * 100.0 / gBinSizeWord;
                emit Signal_progress(m_nProceValue,  "");
                if(!(tmpResponseIndex % 100) || tmpResponseIndex < 3)
                {

                    double completed_percentage = m_nProceValue / 100.0;
                    double time_elapsed = time.elapsed() / 1000.0;
                    // 计算总时间和剩余时间
                    double total_time = (time_elapsed * 100) / completed_percentage;
                    double remaining_time = total_time - time_elapsed;

                    emit Signal_progress(m_nProceValue,  QString("index=%1(%2) [%3][%4][%5]").arg(tmpResponseIndex).arg(gBinSizeWord).arg(total_time).arg(time_elapsed).arg(remaining_time));

                }
                if(tmpResponseIndex >= gBinSizeWord)
                {
                    SubBoardUpdateSendPackData(m_nCanID, tmpResponseIndex, pMotor->sendData);
                    emit Signal_progress(m_nProceValue,  QString("[%1] index=%2(%3)").arg(pMotor->sendMotorID).arg(tmpResponseIndex).arg(gBinSizeWord));
                    pMotor->eSendSt = MUPDATE_SENDED_SUCC ;
                }
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
}
