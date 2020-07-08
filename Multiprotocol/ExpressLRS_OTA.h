
#ifndef _ExpressLRS_OTA_H_
#define _ExpressLRS_OTA_H_

#include "ExpressLRS.h"
#include "iface_sx1276.h"

uint32_t SyncPacketLastSent;

void ExpressLRS_SetRFLinkRate(expresslrs_RFrates_e rate) // Set speed of RF link (hz)
{
    expresslrs_mod_settings_s *const ModParams = get_elrs_airRateConfig(rate);
    expresslrs_rf_pref_params_s *const RFperf = get_elrs_RFperfParams(rate);

    //Radio.Config(ModParams->bw, ModParams->sf, ModParams->cr, ModParams->PreambleLen);
    //hwTimer.updateInterval(ModParams->interval);

    ExpressLRS_currAirRate_Modparams = ModParams;
    ExpressLRS_currAirRate_RFperfParams = RFperf;

    //crsf.RequestedRCpacketInterval = ModParams->interval;
    connectionState = disconnected;
}

//    16 Channels on 11 bits (0..2047)
// 	0		-125%
//     204		-100%
// 	1024	   0%
// 	1843	+100%
// 	2047	+125%

static inline uint8_t UINT11_to_BIT(uint16_t Val)
{
    if (Val > 1000)
        return 1;
    else
        return 0;
};

void ExpressLRS_GenerateSyncPacketData()
{
    uint8_t PacketHeaderAddr;
    PacketHeaderAddr = (DeviceAddr << 2) + ELRS_SYNC_PACKET;
    ExpressLRS_TXdataBuffer[0] = PacketHeaderAddr;
    ExpressLRS_TXdataBuffer[1] = ExpressLRS_FHSSgetCurrIndex();
    ExpressLRS_TXdataBuffer[2] = ExpressLRS_NonceTX;
    ExpressLRS_TXdataBuffer[3] = ((ExpressLRS_currAirRate_Modparams->enum_rate & 0b111) << 5) + ((ExpressLRS_currAirRate_Modparams->TLMinterval & 0b111) << 2);
    ExpressLRS_TXdataBuffer[4] = UID[3];
    ExpressLRS_TXdataBuffer[5] = UID[4];
    ExpressLRS_TXdataBuffer[6] = UID[5];
}

void ExpressLRS_Generate4ChannelData_11bit()
{
    uint8_t PacketHeaderAddr;
    PacketHeaderAddr = (DeviceAddr << 2) + ELRS_RC_DATA_PACKET;
    ExpressLRS_TXdataBuffer[0] = PacketHeaderAddr;
    ExpressLRS_TXdataBuffer[1] = ((Channel_data[0]) >> 3);
    ExpressLRS_TXdataBuffer[2] = ((Channel_data[1]) >> 3);
    ExpressLRS_TXdataBuffer[3] = ((Channel_data[2]) >> 3);
    ExpressLRS_TXdataBuffer[4] = ((Channel_data[3]) >> 3);
    ExpressLRS_TXdataBuffer[5] = ((Channel_data[0] & 0b00000111) << 5) +
                                 ((Channel_data[1] & 0b111) << 2) +
                                 ((Channel_data[2] & 0b110) >> 1);
    ExpressLRS_TXdataBuffer[6] = ((Channel_data[2] & 0b001) << 7) +
                                 ((Channel_data[3] & 0b111) << 4); // 4 bits left over for something else?
#ifdef One_Bit_Switches
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[4]) << 3;
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[5]) << 2;
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[6]) << 1;
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[7]) << 0;
#endif
}

void ExpressLRS_SendRCdataToRF()
{
#ifdef FEATURE_OPENTX_SYNC
    //JustSentRFpacket(); // tells the radio that we want to send data now - this allows opentx packet syncing
#endif

    uint32_t SyncInterval;
    SyncInterval = ExpressLRS_currAirRate_RFperfParams->SyncPktIntervalDisconnected;

    if (((millis() > (SyncPacketLastSent + SyncInterval)) && (ExpressLRS_FHSSgetCurrFreq() == ExpressLRS_GetInitialFreq()))) //only send sync when its time and only on channel 0;
    {

        ExpressLRS_GenerateSyncPacketData();
        SyncPacketLastSent = millis();
        //Serial.println("sync");
        //Serial.println(Radio.currFreq);
    }
    else
    {
#if defined HYBRID_SWITCHES_8
        GenerateChannelDataHybridSwitch8(&Radio, &crsf, DeviceAddr);
#elif defined SEQ_SWITCHES
        GenerateChannelDataSeqSwitch(&Radio, &crsf, DeviceAddr);
#else
        ExpressLRS_Generate4ChannelData_11bit();
#endif
    }
    ///// Next, Calculate the CRC and put it into the buffer /////
    uint8_t crc = CalcCRC(ExpressLRS_TXdataBuffer, 7) + CRCCaesarCipher;
    ExpressLRS_TXdataBuffer[7] = crc;
    SX1276_WritePayloadToFifo(ExpressLRS_TXdataBuffer, 8);
    SX1276_SetMode(true, false, SX1276_OPMODE_TX);
    //Radio.TXnb(ExpressLRS_TXdataBuffer, 8); TODO
}

#endif