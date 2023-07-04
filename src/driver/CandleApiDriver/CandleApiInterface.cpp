#include "CandleApiDriver.h"
#include "CandleApiInterface.h"

CandleApiInterface::CandleApiInterface(CandleApiDriver *driver, candle_handle handle)
  : CanInterface(driver),
    _hostOffsetStart(0),
    _deviceTicksStart(0),
    _handle(handle),
    _backend(driver->backend()),
    _numRx(0),
    _numTx(0),
    _numTxErr(0)
{
    _settings.setBitrate(500000);
    _settings.setSamplePoint(875);



    // Timings for 170MHz processors (CANable 2.0)
    // Tseg1: 2..256 Tseg2: 2..128 sjw: 1..128 brp: 1..512
    // Note: as expressed below, Tseg1 does not include 1 count for prop phase
    _timings
        << CandleApiTiming(170000000,   10000, 875, 68, 217, 31)
        << CandleApiTiming(170000000,   20000, 875, 34, 217, 31)
        << CandleApiTiming(170000000,   50000, 875, 17, 173, 25)
        << CandleApiTiming(170000000,   83333, 875,  8, 221, 32)
        << CandleApiTiming(170000000,  100000, 875, 10, 147, 21)
        << CandleApiTiming(170000000,  125000, 875, 8,  147, 21)
        << CandleApiTiming(170000000,  250000, 875, 4,  147, 21)
        << CandleApiTiming(170000000,  500000, 875, 2,  147, 21)
        << CandleApiTiming(170000000, 1000000, 875, 1,  147, 21);


    // Timings for 48MHz processors (CANable 0.X)
    _timings
        // sample point: 50.0%
        << CandleApiTiming(48000000,   10000, 500, 300, 6, 8)
        << CandleApiTiming(48000000,   20000, 500, 150, 6, 8)
        << CandleApiTiming(48000000,   50000, 500,  60, 6, 8)
        << CandleApiTiming(48000000,   83333, 500,  36, 6, 8)
        << CandleApiTiming(48000000,  100000, 500,  30, 6, 8)
        << CandleApiTiming(48000000,  125000, 500,  24, 6, 8)
        << CandleApiTiming(48000000,  250000, 500,  12, 6, 8)
        << CandleApiTiming(48000000,  500000, 500,   6, 6, 8)
        << CandleApiTiming(48000000,  800000, 500,   3, 8, 9)
        << CandleApiTiming(48000000, 1000000, 500,   3, 6, 8)

        // sample point: 62.5%
        << CandleApiTiming(48000000,   10000, 625, 300, 8, 6)
        << CandleApiTiming(48000000,   20000, 625, 150, 8, 6)
        << CandleApiTiming(48000000,   50000, 625,  60, 8, 6)
        << CandleApiTiming(48000000,   83333, 625,  36, 8, 6)
        << CandleApiTiming(48000000,  100000, 625,  30, 8, 6)
        << CandleApiTiming(48000000,  125000, 625,  24, 8, 6)
        << CandleApiTiming(48000000,  250000, 625,  12, 8, 6)
        << CandleApiTiming(48000000,  500000, 625,   6, 8, 6)
        << CandleApiTiming(48000000,  800000, 600,   4, 7, 6)
        << CandleApiTiming(48000000, 1000000, 625,   3, 8, 6)

        // sample point: 75.0%
        << CandleApiTiming(48000000,   10000, 750, 300, 10, 4)
        << CandleApiTiming(48000000,   20000, 750, 150, 10, 4)
        << CandleApiTiming(48000000,   50000, 750,  60, 10, 4)
        << CandleApiTiming(48000000,   83333, 750,  36, 10, 4)
        << CandleApiTiming(48000000,  100000, 750,  30, 10, 4)
        << CandleApiTiming(48000000,  125000, 750,  24, 10, 4)
        << CandleApiTiming(48000000,  250000, 750,  12, 10, 4)
        << CandleApiTiming(48000000,  500000, 750,   6, 10, 4)
        << CandleApiTiming(48000000,  800000, 750,   3, 13, 5)
        << CandleApiTiming(48000000, 1000000, 750,   3, 10, 4)

        // sample point: 87.5%
        << CandleApiTiming(48000000,   10000, 875, 300, 12, 2)
        << CandleApiTiming(48000000,   20000, 875, 150, 12, 2)
        << CandleApiTiming(48000000,   50000, 875,  60, 12, 2)
        << CandleApiTiming(48000000,   83333, 875,  36, 12, 2)
        << CandleApiTiming(48000000,  100000, 875,  30, 12, 2)
        << CandleApiTiming(48000000,  125000, 875,  24, 12, 2)
        << CandleApiTiming(48000000,  250000, 875,  12, 12, 2)
        << CandleApiTiming(48000000,  500000, 875,   6, 12, 2)
        << CandleApiTiming(48000000,  800000, 867,   4, 11, 2)
        << CandleApiTiming(48000000, 1000000, 875,   3, 12, 2);


    _timings
    //sample point: 50.0%
            << CandleApiTiming(42000000, 10000,     500, 300,   5, 7)
            << CandleApiTiming(42000000, 20000,     500, 150,   5, 7)
            << CandleApiTiming(42000000, 50000,     500, 60,    5, 7)
            << CandleApiTiming(42000000, 100000,    500, 30,    5, 7)
            << CandleApiTiming(42000000, 125000,    500, 24,    5, 7)
            << CandleApiTiming(42000000, 250000,    500, 12,    5, 7)
            << CandleApiTiming(42000000, 500000,    500, 6,     5, 7)
            << CandleApiTiming(42000000, 1000000,   500, 3,     5, 7)

            // sample point: 62.6%
            << CandleApiTiming(42000000, 10000, 623, 300, 7, 5)
            << CandleApiTiming(42000000, 20000, 623, 150, 7, 5)
            << CandleApiTiming(42000000, 50000, 625, 105, 3, 3)
            << CandleApiTiming(42000000, 100000, 623, 30, 7, 5)
            << CandleApiTiming(42000000, 125000, 625, 42, 3, 3)
            << CandleApiTiming(42000000, 250000, 625, 21, 3, 3)
            << CandleApiTiming(42000000, 500000, 623, 6, 7, 5)
            << CandleApiTiming(42000000, 1000000, 623, 3, 7, 5)

            // sample point: 75.0%
            << CandleApiTiming(42000000, 10000, 750, 350, 7, 3)
            << CandleApiTiming(42000000, 20000, 750, 175, 7, 3)
            << CandleApiTiming(42000000, 50000, 750, 70, 7, 3)
            << CandleApiTiming(42000000, 100000, 750, 35, 7, 3)
            << CandleApiTiming(42000000, 125000, 750, 28, 7, 3)
            << CandleApiTiming(42000000, 250000, 750, 14, 7, 3)
            << CandleApiTiming(42000000, 500000, 750, 7, 7, 3)
            << CandleApiTiming(42000000, 1000000, 762, 2, 14, 5)

            // sample point: 87.5%
            << CandleApiTiming(42000000, 10000, 867, 280, 11, 2)
            << CandleApiTiming(42000000, 20000, 867, 140, 11, 2)
            << CandleApiTiming(42000000, 50000, 875, 105, 5, 1)
            << CandleApiTiming(42000000, 100000, 867, 28, 11, 2)
            << CandleApiTiming(42000000, 125000, 875, 42, 5, 1)
            << CandleApiTiming(42000000, 250000, 875, 21, 5, 1)
            << CandleApiTiming(42000000, 500000, 875, 12, 4, 1) //857
            << CandleApiTiming(42000000, 1000000, 857, 6, 4, 1);

    _timings
	
    // sample point: 50.0%
            << CandleApiTiming(16000000,   10000, 520, 64, 11, 12)
            << CandleApiTiming(16000000,   20000, 500, 50,  6,  8)
            << CandleApiTiming(16000000,   50000, 500, 20,  6,  8)
            << CandleApiTiming(16000000,   83333, 500, 12,  6,  8)
            << CandleApiTiming(16000000,  100000, 500, 10,  6,  8)
            << CandleApiTiming(16000000,  125000, 500,  8,  6,  8)
            << CandleApiTiming(16000000,  250000, 500,  4,  6,  8)
            << CandleApiTiming(16000000,  500000, 500,  2,  6,  8)
            << CandleApiTiming(16000000,  800000, 500,  1,  8, 10)
            << CandleApiTiming(16000000, 1000000, 500,  1,  6,  8)

        // sample point: 62.5%
        << CandleApiTiming(16000000,   10000, 625, 64, 14,  9)
        << CandleApiTiming(16000000,   20000, 625, 50,  8,  6)
        << CandleApiTiming(16000000,   50000, 625, 20,  8,  6)
        << CandleApiTiming(16000000,   83333, 625, 12,  8,  6)
        << CandleApiTiming(16000000,  100000, 625, 10,  8,  6)
        << CandleApiTiming(16000000,  125000, 625,  8,  8,  6)
        << CandleApiTiming(16000000,  250000, 625,  4,  8,  6)
        << CandleApiTiming(16000000,  500000, 625,  2,  8,  6)
        << CandleApiTiming(16000000,  800000, 625,  1, 11,  7)
        << CandleApiTiming(16000000, 1000000, 625,  1,  8,  6)

        // sample point: 75.0%
        << CandleApiTiming(16000000,   20000, 750, 50, 10,  4)
        << CandleApiTiming(16000000,   50000, 750, 20, 10,  4)
        << CandleApiTiming(16000000,   83333, 750, 12, 10,  4)
        << CandleApiTiming(16000000,  100000, 750, 10, 10,  4)
        << CandleApiTiming(16000000,  125000, 750,  8, 10,  4)
        << CandleApiTiming(16000000,  250000, 750,  4, 10,  4)
        << CandleApiTiming(16000000,  500000, 750,  2, 10,  4)
        << CandleApiTiming(16000000,  800000, 750,  1, 13,  5)
        << CandleApiTiming(16000000, 1000000, 750,  1, 10,  4)

        // sample point: 87.5%
        << CandleApiTiming(16000000,   20000, 875, 50, 12,  2)
        << CandleApiTiming(16000000,   50000, 875, 20, 12,  2)
        << CandleApiTiming(16000000,   83333, 875, 12, 12,  2)
        << CandleApiTiming(16000000,  100000, 875, 10, 12,  2)
        << CandleApiTiming(16000000,  125000, 875,  8, 12,  2)
        << CandleApiTiming(16000000,  250000, 875,  4, 12,  2)
        << CandleApiTiming(16000000,  500000, 875,  2, 12,  2)
        << CandleApiTiming(16000000,  800000, 900,  2,  7,  1)
        << CandleApiTiming(16000000, 1000000, 875,  1, 12,  2);
}

CandleApiInterface::~CandleApiInterface()
{

}

QString CandleApiInterface::getName() const
{
    return "candle" + QString::number(getId() & 0xFF);
}

QString CandleApiInterface::getDetailsStr() const
{
    return QString::fromStdWString(getPath());
}

void CandleApiInterface::applyConfig(const MeasurementInterface &mi)
{
    _settings = mi;
}

unsigned CandleApiInterface::getBitrate()
{
    return _settings.bitrate();
}

uint32_t CandleApiInterface::getCapabilities()
{
    candle_capability_t caps;

    if (candle_channel_get_capabilities(_handle, 0, &caps)) {

        uint32_t retval = 0;

        if (caps.feature & CANDLE_MODE_LISTEN_ONLY) {
            retval |= CanInterface::capability_listen_only;
        }

        if (caps.feature & CANDLE_MODE_ONE_SHOT) {
            retval |= CanInterface::capability_one_shot;
        }

        if (caps.feature & CANDLE_MODE_TRIPLE_SAMPLE) {
            retval |= CanInterface::capability_triple_sampling;
        }

        return retval;

    } else {
        return 0;
    }
}

QList<CanTiming> CandleApiInterface::getAvailableBitrates()
{
    QList<CanTiming> retval;

    candle_capability_t caps;
    if (candle_channel_get_capabilities(_handle, 0, &caps)) {
        int i = 0;
        foreach (const CandleApiTiming t, _timings) {
            if (t.getBaseClk() == caps.fclk_can) {
                retval << CanTiming(i++, t.getBitrate(), 0, t.getSamplePoint());
            }
        }
    }

    return retval;
}

bool CandleApiInterface::setBitTiming(uint32_t bitrate, uint32_t samplePoint)
{
    candle_capability_t caps;
    if (!candle_channel_get_capabilities(_handle, 0, &caps)) {
        return false;
    }

    foreach (const CandleApiTiming t, _timings) {
        if ( (t.getBaseClk() == caps.fclk_can)
          && (t.getBitrate()==bitrate)
          && (t.getSamplePoint()==samplePoint) )
        {
            candle_bittiming_t timing = t.getTiming();
            return candle_channel_set_timing(_handle, 0, &timing);
        }
    }

    // no valid timing found
    return false;
}

void CandleApiInterface::open()
{
    if (!candle_dev_open(_handle)) {
        // TODO what?
        _isOpen = false;
        return;
    }

    if (!setBitTiming(_settings.bitrate(), _settings.samplePoint())) {
        // TODO what?
        _isOpen = false;
        return;
    }

    uint32_t flags = 0;
    if (_settings.isListenOnlyMode()) {
        flags |= CANDLE_MODE_LISTEN_ONLY;
    }
    if (_settings.isOneShotMode()) {
        flags |= CANDLE_MODE_ONE_SHOT;
    }
    if (_settings.isTripleSampling()) {
        flags |= CANDLE_MODE_TRIPLE_SAMPLE;
    }

    _numRx = 0;
    _numTx = 0;
    _numTxErr = 0;

    uint32_t t_dev;
    if (candle_dev_get_timestamp_us(_handle, &t_dev)) {
        _hostOffsetStart =
                _backend.getUsecsAtMeasurementStart() +
                _backend.getUsecsSinceMeasurementStart();
        _deviceTicksStart = t_dev;
    }

    candle_channel_start(_handle, 0, flags);
    _isOpen = true;
}

bool CandleApiInterface::isOpen()
{
    return _isOpen;
}

void CandleApiInterface::close()
{
    candle_channel_stop(_handle, 0);
    candle_dev_close(_handle);
    _isOpen = false;
}

void CandleApiInterface::sendMessage(const CanMessage &msg)
{
    candle_frame_t frame;

    frame.can_id = msg.getId();
    if (msg.isExtended()) {
        frame.can_id |= CANDLE_ID_EXTENDED;
    }
    if (msg.isRTR()) {
        frame.can_id |= CANDLE_ID_RTR;
    }

    frame.can_dlc = msg.getLength();
    for (int i=0; i<8; i++) {
        frame.data[i] = msg.getByte(i);
    }
    try     //捕获异常
    {
        if(candle_frame_send(_handle, 0, &frame))
        {
            _numTx++;
        }
        else
        {
            _numTxErr++;
            throw "Can sand Frame error";
        }
    }
    catch(...)
    {
        close();//关闭通道
        throw(string("SendCanFramTimeout"));
    }
}

bool CandleApiInterface::readMessage(QList<CanMessage> &msglist, unsigned int timeout_ms)
{
    candle_frame_t frame;
    CanMessage msg;

    if (candle_frame_read(_handle, &frame, timeout_ms)) {

        if (candle_frame_type(&frame)==CANDLE_FRAMETYPE_RECEIVE) {
            _numRx++;

            msg.setInterfaceId(getId());
            msg.setErrorFrame(false);
            msg.setId(candle_frame_id(&frame));
            msg.setExtended(candle_frame_is_extended_id(&frame));
            msg.setRTR(candle_frame_is_rtr(&frame));

            uint8_t dlc = candle_frame_dlc(&frame);
            uint8_t *data = candle_frame_data(&frame);
            msg.setLength(dlc);
            for (int i=0; i<dlc; i++) {
                msg.setByte(i, data[i]);
            }

            //            uint32_t dev_ts = candle_frame_timestamp_us(&frame) - _deviceTicksStart;
            //            uint64_t ts_us = _hostOffsetStart + dev_ts;

            //            uint64_t us_since_start = _backend.getUsecsSinceMeasurementStart();
            //            if(us_since_start > 0x180000000)    // device timestamp overflow must have happend at least once
            //            {
            //                ts_us += us_since_start & 0xFFFFFFFF00000000;
            //            }
            //            msg.setTimestamp(ts_us / 1000000, ts_us % 1000000);
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            msg.setTimestamp(tv);
            msglist.append(msg);
            return true;
        }

    }

    return false;
}

bool CandleApiInterface::updateStatistics()
{
    return true;
}

uint32_t CandleApiInterface::getState()
{
    return CanInterface::state_ok;
}

int CandleApiInterface::getNumRxFrames()
{
    return _numRx;
}

int CandleApiInterface::getNumRxErrors()
{
    return 0;
}

int CandleApiInterface::getNumTxFrames()
{
    return _numTx;
}

int CandleApiInterface::getNumTxErrors()
{
    return _numTxErr;
}

int CandleApiInterface::getNumRxOverruns()
{
    return 0;
}

int CandleApiInterface::getNumTxDropped()
{
    return 0;
}

wstring CandleApiInterface::getPath() const
{
    return wstring(candle_dev_get_path(_handle));
}

void CandleApiInterface::update(candle_handle dev)
{
    candle_dev_free(_handle);
    _handle = dev;
}

#include <math.h>
#include <list>
int GetRegisterVal(int argc, char* argv[])
{
    double clk = 42000000;
    int brp, Tseg1, Tseg2;
    list<int>    baudrateList   =
    {
        10000,
        20000,
        50000,
        100000,
        125000,
        250000,
        500000,
        1000000,
    };
    //    list<int>    ratioList   = {500, 625, 750, 875};
    list<int>    ratioList   = {500};
    foreach(int ratio, ratioList)
    {
        foreach(int baudrate, baudrateList)
        {
            for(brp = 1; brp < 512; brp++)
            {
                for(Tseg2 = 1; Tseg2 < 8; Tseg2++)
                {
                    for(Tseg1 = 1; Tseg1 < 16; Tseg1++)
                    {
                        double realVeal = 1.0 * (Tseg1 + 2) / (Tseg1 + Tseg2 + 2);
                        double err =  realVeal - ratio / 1000.0;
                        if(clk / brp / (Tseg1 + Tseg2 + 2) == baudrate &&  fabs(err) < 0.02)
                        {
                            //<< CandleApiTiming(48000000,  500000, 875,   6, 12, 2)
                            printf("<< CandleApiTiming(%d, %d,%d,%d,%d,%d,%f,%f,\n", (int)clk, baudrate, ratio, brp, Tseg1, Tseg2,  err, realVeal);
                        }
                    }


                }
            }
        }
    }
    return 0;
}
