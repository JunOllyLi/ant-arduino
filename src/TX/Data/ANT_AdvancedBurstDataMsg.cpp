#include <TX/Data/ANT_AdvancedBurstDataMsg.h>

#include <ANT_private_defines.h>

AdvancedBurstDataMsg::AdvancedBurstDataMsg() : AntTxDataRequest(ADVANCED_BURST_DATA) {

}

AdvancedBurstDataMsg::AdvancedBurstDataMsg(uint8_t channel, uint8_t length) : AntTxDataRequest(ADVANCED_BURST_DATA) {
    setChannelNumber(channel);
    setDataLength(length);
}

void AdvancedBurstDataMsg::setChannelNumber(uint8_t channel) {
    // TODO fix this to handle sequence
    _channel = channel;
}

uint8_t AdvancedBurstDataMsg::getChannelNumber() {
    return _channel;
}

uint8_t AdvancedBurstDataMsg::getData(uint8_t pos) {
    // TODO
    return 0;
}

void AdvancedBurstDataMsg::setDataLength(uint8_t length) {
    AntTxDataRequest::setDataLength(length);
}

uint8_t AdvancedBurstDataMsg::getDataLength() {
    // TODO
    return 0;
}

#ifdef NATIVE_API_AVAILABLE

uint32_t AdvancedBurstDataMsg::execute() {
    // TODO
    Serial.println(__FILE__);Serial.println(__FUNCTION__);
    return 0;
}

#endif // NATIVE_API_AVAILABLE
