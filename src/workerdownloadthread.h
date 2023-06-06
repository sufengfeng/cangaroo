#ifndef WORKERDOWNLOADTHREAD_H
#define WORKERDOWNLOADTHREAD_H
#include <QThread>
#include <QObject>
#include <QTextBrowser>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QString>
#include <core/Backend.h>
#include <QByteArray>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define unint8 uint8_t
#define unint16 uint16_t
#define unint32 uint32_t

#define unint08 uint8_t

/* 32位中的低8位 */
#define MASK_LO08                       0x000000FF

#define C2ID_BATT   0x3u    /* 电池编号 */
#define SET_C2STD_OBS(src, dst, type, seq)  ((src) << 8) | ((dst) << 5) |  ((type) << 3)  | ((seq))
/* 提取32位无符号整型数的四个字节数,big indian */
#define UI32_HIHI8(ui32)                ((unint08)(((ui32) >> 24) & MASK_LO08))
#define UI32_HILO8(ui32)                ((unint08)(((ui32) >> 16) & MASK_LO08))
#define UI32_LOHI8(ui32)                ((unint08)(((ui32) >>  8) & MASK_LO08))
#define UI32_LOLO8(ui32)                ((unint08)(((ui32)      ) & MASK_LO08))
#define INF printf
typedef enum  _TAG_ENUM_UPDATE_MOTORS_STATE
{
    MUPDATE_NULL = 0,

    MUPDATE_SEND_TYPE,
    MUPDATE_SEND_HEADER,
    MUPDATE_SEND_AUDIO_NAME,
    MUPDATE_SENDING_PACK,
    MUPDATE_WAIT_FINISH,
    MUPDATE_SENDED_SUCC,

    MUPDATE_ERROR,
} EUpdateStatus;

typedef enum STsSUB_SYS_STDID_
{

    //前面板升级
    SUB_FRONT_UPDATE_START = 0xA0U,//升级启动
    SUB_FRONT_UPDATE_SEND = 0xC0U,//升级发送固件
    SUB_FRONT_UPDATE_RES = 0xB0U,//升级命令反馈
    SUB_FRONT_UPDATE_SUCC_RES = 0xD0U,//升级完成反馈

    //后面板升级
    SUB_BACK_UPDATE_START = 0xA2U,//升级启动
    SUB_BACK_UPDATE_SEND = 0xC2U,//升级发送固件
    SUB_BACK_UPDATE_RES = 0xB2U,//升级命令反馈
    SUB_BACK_UPDATE_SUCC_RES = 0xD2U,//升级完成反馈

    SUB_GET_VERSION = 0x01U,//升级工具获取版本号
} SUB_SYS_STDID; //与MCU通信的CAN消息节点定义
typedef struct _TAG_MOTOR_UPDATA_CTRL
{
    unint32  sendMotorID;
    //    unint32  readFlashStartAddr;

    /* target file info. */
    unint32  sendWordTotal;
    unint32  sendWordCheckSum;
    unint16  sendAddSum;

    /* send info. for every single */
    unint32  sendWordIndex;
    unint32  sendData;
    unint32  responseWordIndex;
    EUpdateStatus eSendSt;

    /* response info. from motor */
    unint32  responseWordTotal;
    unint32  responseWordCheckSum;
    unint32  versionInfo;
} SMotorUpdate;
/* CAN2总线,从标识符中得到相应信息,电池避障通用 */
#define GET_C2STD_SRC(std)    (((std) >> 8) & 0x7)      /* 源设备编号 */
#define GET_C2STD_DEST(std)   (((std) >> 5) & 0x7)      /* 目的设备编号 */
#define GET_C2STD_TYPE(std)   (((std) >> 3) & 0x3)      /* 报文类型 */
#define GET_C2STD_SEQ(std)    (((std)     ) & 0x7)      /* 同一类型报文的序列编号 */
/* 将四个字节数拼成32位无符号整型数,big indian */
#define UI32_MAKE(hh8, hl8, lh8, ll8)   ((((unint32)(hh8)) << 24) | (((unint32)(hl8)) << 16) | (((unint32)(lh8)) << 8) | ((ll8)))

#define ERR printf

#define C2_OBST_UP_M    0x4    /*主控和避障盒远程升级时,主控消息ID,*/
#define C2_OBST_UP_O    0x5    /*主控和避障盒远程升级时，避障盒消息ID*/

class WorkerDownloadThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerDownloadThread();
    ~WorkerDownloadThread();

    void StartUpdateHardWare(QString filePathName, int canid, int updateType);
    void HandleCanMessage(const CanMessage* p_sCanMessage);
    void StopDownload(void);
signals:
    void Signal_progress(const int value, QString str);
    int Signal_SendCanMessage(CanMessage* psCanMessage);
protected:
    void    run();    //虚函数
private:
    void UpdateSubBoardMain(void);
    unint32 SubBoardUpdateStateReady(void);
    unint32 SubBoardUpdateInit(void);
    void SubBoardUpdate(void);
    unint32 SubBoardUpdateEnd(void);
    void bsp_SubBoard_Update_Start(int canId, int updateType);
    void bsp_SubBoard_Update_InitCmd(u32 stdID, u32 binSize, u32 binCheckSum);
    void SubBoardUpdateSendPackData(u32 stdID, unint32 dataIndex, unint32 dataSend);
private:
    bool    m_stopFlag;
    uint32_t m_nCanID;
    uint32_t m_nUpdateType;
    QString m_sFilePathName;
    uint32_t m_nProceValue;
    //    QFileInfo* m_sQFileInfo ;
    QByteArray m_sBinFileRawData;
    unint32 gBinSizeWord = 0;
    unint32 gBinCheckSum = 0;
    SMotorUpdate subBoardUpdate;//yk


    //    bool SubBoard_watch_flag = false;
    //    int SubBoard_watch_pack = 0;
    //    int SubBoard_watch_total = 0;

};

#endif // WORKERDOWNLOADTHREAD_H
