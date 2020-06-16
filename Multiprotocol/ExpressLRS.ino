#if defined(EXPRESSLRS_SX1276_INO)
#include "iface_sx1276.h"
#include "ExpressLRS_common.h"
#include "ExpressLRS_FHSS.h"

uint8_t ExpressLRS_TXdataBuffer[8] = {0};
uint8_t ExpressLRS_NonceTX = 0;

uint16_t initExpressLRS()
{
  SX1276_Reset();
  if (!SX1276_DetectChip())
  {
    return;
  }
}

void ExpressLRS_SetRFLinkRate(expresslrs_RFrates_e rate) // Set speed of RF link (hz)
{
  expresslrs_mod_settings_s *const ModParams = get_elrs_airRateConfig(rate);
  expresslrs_rf_pref_params_s *const RFperf = get_elrs_RFperfParams(rate);

  Radio.Config(ModParams->bw, ModParams->sf, ModParams->cr, ModParams->PreambleLen);
  hwTimer.updateInterval(ModParams->interval);

  ExpressLRS_currAirRate_Modparams = ModParams;
  ExpressLRS_currAirRate_RFperfParams = RFperf;

  crsf.RequestedRCpacketInterval = ModParams->interval;
  isRXconnected = false;
}

void ExpressLRS_GenerateSyncPacketData()
{
  uint8_t PacketHeaderAddr;
  PacketHeaderAddr = (DeviceAddr << 2) + ELRS_SYNC_PACKET;
  ExpressLRS_TXdataBuffer[0] = PacketHeaderAddr;
  ExpressLRS_TXdataBuffer[1] = FHSSgetCurrIndex();
  ExpressLRS_TXdataBuffer[2] = NonceTX;
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
  ExpressLRS_TXdataBuffer[1] = ((crsf.ChannelDataIn[0]) >> 3);
  ExpressLRS_TXdataBuffer[2] = ((crsf.ChannelDataIn[1]) >> 3);
  ExpressLRS_TXdataBuffer[3] = ((crsf.ChannelDataIn[2]) >> 3);
  ExpressLRS_TXdataBuffer[4] = ((crsf.ChannelDataIn[3]) >> 3);
  ExpressLRS_TXdataBuffer[5] = ((crsf.ChannelDataIn[0] & 0b00000111) << 5) +
                               ((crsf.ChannelDataIn[1] & 0b111) << 2) +
                               ((crsf.ChannelDataIn[2] & 0b110) >> 1);
  ExpressLRS_TXdataBuffer[6] = ((crsf.ChannelDataIn[2] & 0b001) << 7) +
                               ((crsf.ChannelDataIn[3] & 0b111) << 4); // 4 bits left over for something else?
#ifdef One_Bit_Switches
  ExpressLRS_TXdataBuffer[6] += CRSF_to_BIT(crsf.ChannelDataIn[4]) << 3;
  ExpressLRS_TXdataBuffer[6] += CRSF_to_BIT(crsf.ChannelDataIn[5]) << 2;
  ExpressLRS_TXdataBuffer[6] += CRSF_to_BIT(crsf.ChannelDataIn[6]) << 1;
  ExpressLRS_TXdataBuffer[6] += CRSF_to_BIT(crsf.ChannelDataIn[7]) << 0;
#endif
}

void ExpressLRS_SendRCdataToRF()
{
#ifdef FEATURE_OPENTX_SYNC
  //JustSentRFpacket(); // tells the radio that we want to send data now - this allows opentx packet syncing
#endif

  uint32_t SyncInterval;
  SyncInterval = ExpressLRS_currAirRate_RFperfParams->SyncPktIntervalDisconnected;

  if (((millis() > (SyncPacketLastSent + SyncInterval)) && (Radio.currFreq == GetInitialFreq()))) //only send sync when its time and only on channel 0;
  {

    GenerateSyncPacketData();
    SyncPacketLastSent = millis();
    ChangeAirRateSentUpdate = true;
    //Serial.println("sync");
    //Serial.println(Radio.currFreq);
  }
  else
  {
    if ((millis() > (MSP_PACKET_SEND_INTERVAL + MSPPacketLastSent)) && MSPPacketSendCount)
    {
      GenerateMSPData();
      MSPPacketLastSent = millis();
      MSPPacketSendCount--;
    }
    else
    {
#if defined HYBRID_SWITCHES_8
      GenerateChannelDataHybridSwitch8(&Radio, &crsf, DeviceAddr);
#elif defined SEQ_SWITCHES
      GenerateChannelDataSeqSwitch(&Radio, &crsf, DeviceAddr);
#else
      Generate4ChannelData_11bit();
#endif
    }
  }

  ///// Next, Calculate the CRC and put it into the buffer /////
  uint8_t crc = CalcCRC(Radio.TXdataBuffer, 7) + CRCCaesarCipher;
  Radio.TXdataBuffer[7] = crc;
  Radio.TXnb(Radio.TXdataBuffer, 8);

  if (ChangeAirRateRequested)
  {
    ChangeAirRateSentUpdate = true;
  }
}

uint16_t ExpressLRS_callback()
{
  debug("callback");
  return 5000;
}

#endif