#if defined(ELRS_SX1276_INO)
#include "iface_sx1276.h"
#include "ExpressLRS.h"
#include "ExpressLRS_OTA.h"

uint8_t UID[6] = {180, 230, 45, 152, 126, 65}; //sandro unique ID
uint8_t CRCCaesarCipher = UID[4];
uint8_t DeviceAddr = 0b111111 & UID[5];

uint8_t ExpressLRS_TXdataBuffer[8] = {0};
uint8_t ExpressLRS_NonceTX = 0;

uint16_t initExpressLRS()
{
  SX1276_Reset();
  if (!SX1276_DetectChip())
  {
    return 0;
  }
  ExpressLRS_FHSSrandomiseFHSSsequence();

  debug("Setting ExpressLRS LoRa reg defaults");
  SX1276_WriteReg(SX1276_01_OPMODE, SX1276_OPMODE_SLEEP);
  SX1276_SetMode(true, false, SX1276_OPMODE_STDBY); //must be written in sleep mode

  SX1276_WriteReg(SX1276_22_PAYLOAD_LENGTH, 8);
  SX1276_WriteReg(SX1276_0E_FIFOTXBASEADDR, 0x00);
  //SX1276_WriteReg(SX1276_0E_FIFORXBASEADDR, 0x00);




  //SetSyncWord(currSyncWord);
  //hal.setRegValue(SX127X_REG_DIO_MAPPING_1, 0b11000000, 7, 6); //undocumented "hack", looking at Table 18 from datasheet SX127X_REG_DIO_MAPPING_1 = 11 appears to be unspported by infact it generates an intterupt on both RXdone and TXdone, this saves switching modes.
  //hal.writeRegister(SX127X_REG_LNA, SX127X_LNA_BOOST_ON);
  //hal.writeRegister(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_ON | SX1278_LOW_DATA_RATE_OPT_OFF);
  //hal.setRegValue(SX127X_REG_OCP, SX127X_OCP_ON | SX127X_OCP_150MA, 5, 0); //150ma max current
  //SetPreambleLength(SX127X_PREAMBLE_LENGTH_LSB);
}

uint16_t ExpressLRS_callback()
{
  debug("callback");
  ExpressLRS_SendRCdataToRF();
  ExpressLRS_NonceTX++;
  return 5000;
}

#endif