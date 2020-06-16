
#ifndef _ExpressLRS_common_H_
#define _ExpressLRS_common_H_

#define One_Bit_Switches

extern uint8_t UID[6];
extern uint8_t CRCCaesarCipher;
extern uint8_t DeviceAddr;

#define ELRS_RC_DATA_PACKET 0b00
#define ELRS_MSP_DATA_PACKET 0b01
#define ELRS_TLM_PACKET 0b11
#define ELRS_SYNC_PACKET 0b10

typedef enum
{
    TLM_RATIO_NO_TLM = 0,
    TLM_RATIO_1_128 = 1,
    TLM_RATIO_1_64 = 2,
    TLM_RATIO_1_32 = 3,
    TLM_RATIO_1_16 = 4,
    TLM_RATIO_1_8 = 5,
    TLM_RATIO_1_4 = 6,
    TLM_RATIO_1_2 = 7

} expresslrs_tlm_ratio_e;

typedef enum
{
    bad_sync_retry = 4,
    bad_sync = 3,
    connected = 2,
    tentative = 1,
    disconnected = 0
} connectionState_e;

typedef enum
{
    RATE_200HZ = 0,
    RATE_100HZ = 1,
    RATE_50HZ = 2,
    RATE_25HZ = 3,
    RATE_4HZ = 4
} expresslrs_RFrates_e; // Max value of 16 since only 4 bits have been assigned in the sync package.

#define RATE_MAX 5

typedef struct expresslrs_mod_settings_s
{
    expresslrs_RFrates_e enum_rate; // Max value of 16 since only 4 bits have been assigned in the sync package.
    uint8_t bw;
    uint8_t sf;
    uint8_t cr;
    uint32_t interval;                  //interval in us seconds that corresponds to that frequnecy
    uint8_t rate;                       // rate in hz
    expresslrs_tlm_ratio_e TLMinterval; // every X packets is a response TLM packet, should be a power of 2
    uint8_t FHSShopInterval;            // every X packets we hope to a new frequnecy. Max value of 16 since only 4 bits have been assigned in the sync package.
    uint8_t PreambleLen;

} expresslrs_mod_settings_t;

typedef struct expresslrs_rf_pref_params_s
{
    expresslrs_RFrates_e enum_rate; // Max value of 16 since only 4 bits have been assigned in the sync package.
    int32_t RXsensitivity;          //expected RF sensitivity based on
    uint32_t TOA;                   //time on air in microseconds
    uint32_t RFmodeCycleInterval;
    uint32_t RFmodeCycleAddtionalTime; 
    uint32_t SyncPktIntervalDisconnected;
    uint32_t SyncPktIntervalConnected;

} expresslrs_rf_pref_params_s;

expresslrs_mod_settings_s *get_elrs_airRateConfig(expresslrs_RFrates_e rate);
expresslrs_rf_pref_params_s *get_elrs_RFperfParams(expresslrs_RFrates_e rate);

extern expresslrs_mod_settings_s *ExpressLRS_currAirRate_Modparams;
extern expresslrs_rf_pref_params_s *ExpressLRS_currAirRate_RFperfParams;
uint8_t TLMratioEnumToValue(expresslrs_tlm_ratio_e enumval);

#endif