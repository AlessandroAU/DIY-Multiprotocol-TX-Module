
#ifndef _ExpressLRS_OTA_H_
#define _ExpressLRS_OTA_H_

#include "ExpressLRS.h"
#include "iface_sx1276.h"

extern uint8_t expresslrs_nonce_tx;

uint32_t SyncPacketLastSent;

void ExpressLRS_SetRFLinkRate(expresslrs_RFrates_e rate) // Set speed of RF link (hz)
{
    expresslrs_mod_settings_s *const ModParams = get_elrs_airRateConfig(rate);
    expresslrs_rf_pref_params_s *const RFperf = get_elrs_RFperfParams(rate);

    SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);

    SX1276_ConfigModem1(ModParams->bw, ModParams->cr, true);
    SX1276_ConfigModem2(ModParams->sf, false, false);
    SX1276_ConfigModem3(false, true);
    SX1276_SetPreambleLength(ModParams->PreambleLen);

    if (ModParams->sf == 6)
    {
        SX1276_SetDetectionThreshold(SX1276_MODEM_DETECTION_THRESHOLD_SF6);
        SX1276_SetDetectOptimize(true, SX1276_DETECT_OPTIMIZE_SF6);
    }
    else
    {
        SX1276_SetDetectionThreshold(SX1276_MODEM_DETECTION_THRESHOLD_SF7_TO_SF12);
        SX1276_SetDetectOptimize(true, SX1276_DETECT_OPTIMIZE_SF7_TO_SF12);
    }

    SX1276_SetSyncWord(UID[3]);
    SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, 8);

    ExpressLRS_currAirRate_Modparams = ModParams;
    ExpressLRS_currAirRate_RFperfParams = RFperf;
    connectionState = disconnected;
}

static inline uint8_t UINT11_to_BIT(uint16_t Val)
{
    if (Val > 1000)
        return 1;
    else
        return 0;
};

void expresslrs_fhss_handle()
{
    uint8_t modresult = (expresslrs_nonce_tx) % ExpressLRS_currAirRate_Modparams->FHSShopInterval;

    if (modresult == 0) // if it time to hop, do so.
    {
        SX1276_SetFrequency_expresslrs(expresslrs_fhss_get_next_freq());
    }
}

void ExpressLRS_GenerateSyncPacketData()
{
    uint8_t PacketHeaderAddr;
    PacketHeaderAddr = (DeviceAddr << 2) + ELRS_SYNC_PACKET;
    ExpressLRS_TXdataBuffer[0] = PacketHeaderAddr;
    ExpressLRS_TXdataBuffer[1] = expresslrs_fhss_get_index();
    ExpressLRS_TXdataBuffer[2] = expresslrs_nonce_tx;
    ExpressLRS_TXdataBuffer[3] = ((ExpressLRS_currAirRate_Modparams->enum_rate & 0b111) << 5) + ((ExpressLRS_currAirRate_Modparams->TLMinterval & 0b111) << 2);
    ExpressLRS_TXdataBuffer[4] = UID[3];
    ExpressLRS_TXdataBuffer[5] = UID[4];
    ExpressLRS_TXdataBuffer[6] = UID[5];
}

//  16 Channels on 11 bits (0..2047)
// 	0		-125%
//  204		-100%
// 	1024	   0%
// 	1843	+100%
// 	2047	+125%

void ExpressLRS_Generate4ChannelData_11bit()
{
    uint8_t PacketHeaderAddr;
    PacketHeaderAddr = (DeviceAddr << 2) + ELRS_RC_DATA_PACKET;
    ExpressLRS_TXdataBuffer[0] = PacketHeaderAddr;
    ExpressLRS_TXdataBuffer[1] = ((Channel_data[0] - 32) >> 3); //- 32 is needed to make the scales correct
    ExpressLRS_TXdataBuffer[2] = ((Channel_data[1] - 32) >> 3);
    ExpressLRS_TXdataBuffer[3] = ((Channel_data[2] - 32) >> 3);
    ExpressLRS_TXdataBuffer[4] = ((Channel_data[3] - 32) >> 3);
    ExpressLRS_TXdataBuffer[5] = (((Channel_data[0] - 32) & 0b00000111) << 5) +
                                 (((Channel_data[1] - 32) & 0b111) << 2) +
                                 (((Channel_data[2] - 32) & 0b110) >> 1);
    ExpressLRS_TXdataBuffer[6] = (((Channel_data[2] - 32) & 0b001) << 7) +
                                 (((Channel_data[3] - 32) & 0b111) << 4); // 4 bits left over for something else?
#ifdef One_Bit_Switches
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[4] - 32) << 3;
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[5] - 32) << 2;
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[6] - 32) << 1;
    ExpressLRS_TXdataBuffer[6] += UINT11_to_BIT(Channel_data[7] - 32) << 0;
#endif
}

void ExpressLRS_SendRCdataToRF()
{
#ifdef MULTI_SYNC
    //JustSentRFpacket(); // tells the radio that we want to send data now - this allows opentx packet syncing
#endif

    uint32_t SyncInterval;
    SyncInterval = ExpressLRS_currAirRate_RFperfParams->SyncPktIntervalDisconnected;

    if ((millis() > (SyncPacketLastSent + SyncInterval)) && (expresslrs_fhss_get_curr_freq() == expresslrs_fhss_inital_freq()) && ((expresslrs_nonce_tx) % ExpressLRS_currAirRate_Modparams->FHSShopInterval == 1)) // sync just after we changed freqs (helps with hwTimer.init() being in sync from the get go)
    {
        ExpressLRS_GenerateSyncPacketData();
        SyncPacketLastSent = millis();
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
    uint8_t crc = expresslrs_crc(ExpressLRS_TXdataBuffer, 7) + CRCCaesarCipher;
    ExpressLRS_TXdataBuffer[7] = crc;
    SX1276_WritePayloadToFifo(ExpressLRS_TXdataBuffer, 8);
    SX1276_SetMode(true, false, SX1276_OPMODE_TX);
}

#endif