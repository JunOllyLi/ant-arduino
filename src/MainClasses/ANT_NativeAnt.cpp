#include <MainClasses/ANT_NativeAnt.h>
#include <ANT_defines.h>
#include <TX/Control/ANT_RequestMessage.h>
#ifdef NATIVE_API_AVAILABLE

#ifndef NRF_SDH_ANT_TOTAL_CHANNELS_ALLOCATED
#define NRF_SDH_ANT_TOTAL_CHANNELS_ALLOCATED 8
#endif

// <o> NRF_SDH_ANT_ENCRYPTED_CHANNELS - Encrypted ANT channels. 
#ifndef NRF_SDH_ANT_ENCRYPTED_CHANNELS
#define NRF_SDH_ANT_ENCRYPTED_CHANNELS 0
#endif

// </h> 
//==========================================================

// <h> ANT Queues 

//==========================================================
// <o> NRF_SDH_ANT_EVENT_QUEUE_SIZE - Event queue size. 
#ifndef NRF_SDH_ANT_EVENT_QUEUE_SIZE
#define NRF_SDH_ANT_EVENT_QUEUE_SIZE 32
#endif

// <o> NRF_SDH_ANT_BURST_QUEUE_SIZE - ANT burst queue size. 
#ifndef NRF_SDH_ANT_BURST_QUEUE_SIZE
#define NRF_SDH_ANT_BURST_QUEUE_SIZE 128
#endif

#define NRF_SDH_ANT_BUF_SIZE    ANT_ENABLE_GET_REQUIRED_SPACE(                                      \
                                    NRF_SDH_ANT_TOTAL_CHANNELS_ALLOCATED,                           \
                                    NRF_SDH_ANT_ENCRYPTED_CHANNELS,                                 \
                                    NRF_SDH_ANT_BURST_QUEUE_SIZE,                                   \
                                    NRF_SDH_ANT_EVENT_QUEUE_SIZE)
__ALIGN(4) static uint8_t m_ant_stack_buffer[NRF_SDH_ANT_BUF_SIZE];

NativeAnt::NativeAnt() : BaseAnt() {
    reqmsgid = 0;
    getResponse().setFrameData(_responseFrameData);
}

void NativeAnt::begin(){
    // Needs to be called after sd_device_enable
    ANT_ENABLE ant_enable_cfg =
    {
        .ucTotalNumberOfChannels     = NRF_SDH_ANT_TOTAL_CHANNELS_ALLOCATED,
        .ucNumberOfEncryptedChannels = NRF_SDH_ANT_ENCRYPTED_CHANNELS,
        .usNumberOfEvents            = NRF_SDH_ANT_EVENT_QUEUE_SIZE,
        .pucMemoryBlockStartLocation = m_ant_stack_buffer,
        .usMemoryBlockByteSize       = sizeof(m_ant_stack_buffer),
    };

    uint8_t ret_code = sd_ant_enable(&ant_enable_cfg);
#ifdef JLDEBUG
    if (NRF_SUCCESS != ret_code) {
        Serial.print("sd_ant_enable returned ");Serial.println(ret_code);
    }
#endif
}

void NativeAnt::read_softdevice_event() {
    ANT_MESSAGE antMsg;
    uint8_t ucChannel;
    uint8_t ucEvent;
    uint8_t retcode = sd_ant_event_get(&ucChannel, &ucEvent, (uint8_t *)&antMsg);
    if (NRF_SUCCESS == retcode) {
        switch (ucEvent) {
        case EVENT_TX:
        case EVENT_TRANSFER_TX_COMPLETED:
            _responseFrameData[0] = ucChannel;
            _responseFrameData[1] = 1; //ANTPLUS_CHANNELEVENT_MESSAGECODE
            _responseFrameData[2] = ucEvent;
            getResponse().setLength(3);
            getResponse().setMsgId(CHANNEL_EVENT);
            getResponse().setErrorCode(NO_ERROR);
            getResponse().setAvailable(true);
            break;
        case EVENT_RX:
            memcpy(_responseFrameData, &antMsg.stMessage.uFramedData.stFramedData.uMesgData.stMesgData,
                    sizeof(antMsg.stMessage.uFramedData.stFramedData.uMesgData.stMesgData));
            getResponse().setLength(antMsg.stMessage.ucSize);
            switch (antMsg.stMessage.uFramedData.stFramedData.ucMesgID) {
            case MESG_BROADCAST_DATA_ID:
                getResponse().setMsgId(BROADCAST_DATA);
                break;
            default:
                Serial.print("------unhandled ANT msg type ");
                Serial.println(antMsg.stMessage.uFramedData.stFramedData.ucMesgID, HEX);
            }

            getResponse().setErrorCode(NO_ERROR);
            getResponse().setAvailable(true);
            break;
        default:
            Serial.print("------unhandled ANT event ");Serial.print(ucChannel);
            Serial.print(" 0x");Serial.println(ucEvent, HEX);
            break;
        }
    }
}

void NativeAnt::native_api_event() {
    // requests handled by native API
    uint8_t ret = 0;
    switch (reqmsgid) {
    case CAPABILITIES:
        ret = sd_ant_capabilities_get(_responseFrameData);
        getResponse().setLength(8);
        getResponse().setMsgId(CAPABILITIES);
        getResponse().setErrorCode(NO_ERROR);
        getResponse().setAvailable(true);
#ifdef JLDEBUG
        if (NRF_SUCCESS != ret)
        {
            Serial.print("sd_ant_capabilities_get returned ");
            Serial.println(ret);
        }
        else
        {
            Serial.print("ANT Max Channel: ");
            Serial.println(_responseFrameData[0]);
            Serial.print("ANT Max Network: ");
            Serial.println(_responseFrameData[1]);
        }
#endif
        break;
    case ANT_VERSION:
        ret = sd_ant_version_get(_responseFrameData);
        getResponse().setMsgId(ANT_VERSION);
        getResponse().setErrorCode(NO_ERROR);
        getResponse().setAvailable(true);
#ifdef JLDEBUG
        Serial.print("ANT_VERSION ");Serial.println((const char *)_responseFrameData);
#endif
        break;
    case CHANNEL_STATUS:
        ret = sd_ant_channel_status_get(req_subid, _responseFrameData);
        getResponse().setLength(1);
        getResponse().setMsgId(CHANNEL_STATUS);
        getResponse().setErrorCode(NO_ERROR);
        getResponse().setAvailable(true);
        break;
    case CHANNEL_ID:
        ret = sd_ant_channel_id_get(req_subid, (uint16_t*)&_responseFrameData[1],
                &_responseFrameData[3], &_responseFrameData[4]);
        _responseFrameData[0] = req_subid;

        getResponse().setLength(5);
        getResponse().setMsgId(CHANNEL_ID);
        getResponse().setErrorCode(NO_ERROR);
        getResponse().setAvailable(true);
        break;
    default:
        Serial.print("=====unhandled request msg 0x");
        Serial.println(reqmsgid, HEX);
        break;
    }
}

void NativeAnt::readPacket(){
    // reset previous response
    if (getResponse().isAvailable() || getResponse().isError()) {
        // discard previous packet and start over
        getResponse().reset();
    }

    if (0 != reqmsgid) {
        native_api_event();
        reqmsgid = 0;
    } else {
        // read ANT events
        read_softdevice_event();
    }
}

void NativeAnt::send(AntRequest& request){
    uint8_t ret = 0;
    if (REQUEST_MESSAGE == request.getMsgId()) {
        reqmsgid = request.getData(1);
        req_subid = request.getData(0);
    } else {
        request.execute();
    }
}

#endif // NATIVE_API_AVAILABLE
