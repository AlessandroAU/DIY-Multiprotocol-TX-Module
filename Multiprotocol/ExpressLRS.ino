#if defined(ELRS_SX1276_INO)
#include "iface_sx1276.h"
#include "ExpressLRS.h"
#include "ExpressLRS_OTA.h"

uint8_t UID[6] = {10, 183, 12, 124, 56, 3}; // default UID
uint8_t CRCCaesarCipher = UID[4];
uint8_t DeviceAddr = 0b111111 & UID[5];

uint8_t ExpressLRS_TXdataBuffer[8] = {0};
uint8_t expresslrs_nonce_tx = 0;

void expresslrs_dio0_isr()
{
  expresslrs_nonce_tx++;
  SX1276_ClearIRQFlags();
  expresslrs_fhss_handle();
}

uint16_t initExpressLRS()
{
  SX1276_Reset();
  if (!SX1276_DetectChip())
  {
    return 0;
  }
  //ExpressLRS_FHSSrandomiseFHSSsequence();

  debugln("Setting ExpressLRS LoRa reg defaults");
  //SX1276 Init
  SX1276_SetMode(true, false, SX1276_OPMODE_SLEEP);
  SX1276_SetMode(true, false, SX1276_OPMODE_STDBY);
  SX1276_WriteReg(SX1276_40_DIOMAPPING1, 0b11000000); //undocumented "hack", looking at Table 18 from datasheet SX127X_REG_DIO_MAPPING_1 = 11 appears to be unspported by infact it generates an intterupt on both RXdone and TXdone, this saves switching modes.


  expresslrs_fhss_init_freq(sub_protocol);
  ExpressLRS_SetRFLinkRate(RATE_100HZ);

  SX1276_SetFrequency_expresslrs(expresslrs_fhss_inital_freq());

  attachInterrupt(SX1276_DIO0_pin, expresslrs_dio0_isr, RISING);
}

uint16_t ExpressLRS_callback()
{
  ExpressLRS_SendRCdataToRF();
  return ExpressLRS_currAirRate_Modparams->interval;
}

#endif