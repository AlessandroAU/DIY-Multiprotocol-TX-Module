
#ifndef _ExpressLRS_H_
#define _ExpressLRS_H_

#include "iface_SX1276.h"

#define One_Bit_Switches

extern uint8_t UID[6];
extern uint8_t CRCCaesarCipher;
extern uint8_t DeviceAddr;

extern uint8_t ExpressLRS_TXdataBuffer[8];
extern uint8_t ExpressLRS_NonceTX;

#define ELRS_RC_DATA_PACKET 0b00
#define ELRS_MSP_DATA_PACKET 0b01
#define ELRS_TLM_PACKET 0b11
#define ELRS_SYNC_PACKET 0b10

void expresslrs_fhss_generate_sequence();

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
    connected = 2,
    tentative = 1,
    disconnected = 0
} connectionState_e;

typedef enum
{
    RATE_200HZ = 0,
    RATE_100HZ = 1,
    RATE_50HZ = 2,
} expresslrs_RFrates_e; // Max value of 16 since only 4 bits have been assigned in the sync package.

#define RATE_MAX 3

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

expresslrs_mod_settings_s *ExpressLRS_currAirRate_Modparams;
expresslrs_rf_pref_params_s *ExpressLRS_currAirRate_RFperfParams;

expresslrs_mod_settings_s ExpressLRS_AirRateConfig[RATE_MAX] = {
    {RATE_200HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 6, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 5000, 200, TLM_RATIO_NO_TLM, 2, 8},
    {RATE_100HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 7, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 10000, 100, TLM_RATIO_NO_TLM, 2, 8},
    {RATE_50HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 8, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 20000, 50, TLM_RATIO_NO_TLM, 2, 8}};

expresslrs_rf_pref_params_s ExpressLRS_AirRateRFperf[RATE_MAX] = {
    {RATE_200HZ, -112, 4380, 2500, 2000, 2000, 5000}, // ~ 3 sync packets
    {RATE_100HZ, -117, 8770, 2500, 3000, 2000, 5000},
    {RATE_50HZ, -120, 17540, 2500, 5000, 2000, 5000}}; // this means always send sync on ch[0] as soon as we can

expresslrs_mod_settings_s *get_elrs_airRateConfig(expresslrs_RFrates_e rate)
{
    if (rate < 0) // Protect against out of bounds rate
    {
        return &ExpressLRS_AirRateConfig[0]; // Set to first entry in the array (200HZ)
    }
    else if (rate > (RATE_MAX - 1))
    {
        return &ExpressLRS_AirRateConfig[RATE_MAX - 1]; // Set to last usable entry in the array (currently 50HZ)
    }
    return &ExpressLRS_AirRateConfig[rate];
}

expresslrs_rf_pref_params_s *get_elrs_RFperfParams(expresslrs_RFrates_e rate)
{

    if (rate < 0) // Protect against out of bounds rate
    {
        return &ExpressLRS_AirRateRFperf[0]; // Set to first entry in the array (200HZ)
    }
    else if (rate > (RATE_MAX - 1))
    {
        return &ExpressLRS_AirRateRFperf[RATE_MAX - 1]; // Set to last usable entry in the array (currently 50HZ)
    }
    return &ExpressLRS_AirRateRFperf[rate];
}

connectionState_e connectionState = disconnected;

uint8_t expresslrs_tlmratio_enum_to_val(expresslrs_tlm_ratio_e enumval)
{
    switch (enumval)
    {
    case TLM_RATIO_NO_TLM:
        return 0;
        break;
    case TLM_RATIO_1_2:
        return 2;
        break;
    case TLM_RATIO_1_4:
        return 4;
        break;
    case TLM_RATIO_1_8:
        return 8;
        break;
    case TLM_RATIO_1_16:
        return 16;
        break;
    case TLM_RATIO_1_32:
        return 32;
        break;
    case TLM_RATIO_1_64:
        return 64;
        break;
    case TLM_RATIO_1_128:
        return 128;
        break;
    default:
        return 0;
    }
}

//////////////////////////////////////////// RNG ROUTINES ///////////////////////////////////////////

#define RNG_MAX 0x7FFF
unsigned long ExpressLRS_seed = 0;
// returns values between 0 and 0x7FFF
// NB rngN depends on this output range, so if we change the
// behaviour rngN will need updating
int32_t expresslrs_rng(void)
{
    long m = 2147483648;
    long a = 214013;
    long c = 2531011;
    ExpressLRS_seed = (a * ExpressLRS_seed + c) % m;
    return ExpressLRS_seed >> 16;
}

void expresslrs_rngSeed(long newSeed)
{
    ExpressLRS_seed = newSeed;
}

// returns 0 <= x < max where max <= 256
// (actual upper limit is higher, but there is one and I haven't
//  thought carefully about what it is)
unsigned int expresslrs_rngN(unsigned int max)
{
    unsigned long x = expresslrs_rng();
    unsigned int result = (x * max) / RNG_MAX;
    return result;
}

long expresslrs_rng8Bit(void)
{
    return expresslrs_rng() & 0b11111111; // 0..255 returned
}

long expresslrs_rng5Bit(void)
{
    return expresslrs_rng() & 0b11111; // 0..31 returned
}

long expresslrs_rng0to2(void)
{
    int randomNumber = expresslrs_rng() & 0b11; // 0..2 returned

    while (randomNumber == 3)
    {
        randomNumber = expresslrs_rng() & 0b11;
    }
    return randomNumber;
}

//////////////////////////////////////////// FHSS ROUTINES ///////////////////////////////////////////

uint32_t expresslrs_fhss_start_freq;
uint32_t expresslrs_fhss_interval;
uint32_t expresslrs_fhss_num_freqs;

uint8_t expresslrs_fhss_ptr = 0; //value of current index in expresslrs_fhss_sequence array

#define NR_SEQUENCE_ENTRIES 256

uint8_t expresslrs_fhss_sequence[NR_SEQUENCE_ENTRIES] = {0};

void expresslrs_fhss_init_freq(uint8_t regulatory_domain)
{
    switch (regulatory_domain)
    {
    case 0: //915 AU
        expresslrs_fhss_start_freq = 915500000;
        expresslrs_fhss_interval = 600000;
        expresslrs_fhss_num_freqs = 20;
        debugln("915 AU");
        break;
    case 1: // 915 FCC
        expresslrs_fhss_start_freq = 903500000;
        expresslrs_fhss_interval = 600000;
        expresslrs_fhss_num_freqs = 40;
        debugln("915 FCC");
        break;
    case 2: //868 EU
        expresslrs_fhss_start_freq = 863275000;
        expresslrs_fhss_interval = 525000;
        expresslrs_fhss_num_freqs = 13;
        debugln("868 EU");
        break;
    }

    expresslrs_fhss_generate_sequence(); // generate the pseudo random hop seq
}

/* 868 EU Frequency bands taken from https://wetten.overheid.nl/BWBR0036378/2016-12-28#Bijlagen
 * Note: these frequencies fall in the license free H-band, but in combination with 500kHz 
 * LoRa modem bandwidth used by ExpressLRS (EU allows up to 125kHz modulation BW only) they
 * will never pass RED certification and they are ILLEGAL to use. 
 * 
 * Therefore we simply maximize the usage of available spectrum so laboratory testing of the software won't disturb existing
 * 868MHz ISM band traffic too much.

    863275000, // band H1, 863 - 865MHz, 0.1% duty cycle or CSMA techniques, 25mW EIRP
    863800000,
    864325000,
    864850000,
    865375000, // Band H2, 865 - 868.6MHz, 1.0% dutycycle or CSMA, 25mW EIRP
    865900000,
    866425000,
    866950000,
    867475000,
    868000000,
    868525000, // Band H3, 868.7-869.2MHz, 0.1% dutycycle or CSMA, 25mW EIRP
    869050000,
    869575000};
*/
// Very definitely not fully checked. An initial pass at increasing the hops

uint32_t expresslrs_fhss_get_array_val(uint8_t index)
{
    return ((index*expresslrs_fhss_interval)+expresslrs_fhss_start_freq);
}

uint8_t expresslrs_fhss_get_index()
{ // get the current index of the FHSS pointer
    return expresslrs_fhss_ptr;
}

uint32_t expresslrs_fhss_inital_freq()
{
    return expresslrs_fhss_start_freq;
}

uint32_t expresslrs_fhss_get_curr_freq()
{
    return expresslrs_fhss_get_array_val(expresslrs_fhss_sequence[expresslrs_fhss_ptr]);
}

uint32_t expresslrs_fhss_get_next_freq()
{
    expresslrs_fhss_ptr++;
    return expresslrs_fhss_get_curr_freq();
}

/**
Requirements:
1. 0 every n hops
2. No two repeated channels
3. Equal occurance of each (or as even as possible) of each channel
4. Pesudorandom

Approach:
  Initialise an array of flags indicating which channels have not yet been assigned and a counter of how many channels are available
  Iterate over the FHSSsequence array using index
    if index is a multiple of SYNC_INTERVAL assign the sync channel index (0)
    otherwise, generate a random number between 0 and the number of channels left to be assigned
    find the index of the nth remaining channel
    if the index is a repeat, generate a new random number
    if the index is not a repeat, assing it to the FHSSsequence array, clear the availability flag and decrement the available count
    if there are no available channels left, reset the flags array and the count
*/

// Set all of the flags in the array to true, except for the first one
// which corresponds to the sync channel and is never available for normal expresslrs_fhss_calc_reset_available
// allocation.
void expresslrs_fhss_calc_reset_available(uint8_t *array)
{
    // channel 0 is the sync channel and is never considered available
    array[0] = 0;

    // all other entires to 1
    for (unsigned int i = 1; i < expresslrs_fhss_num_freqs; i++)
        array[i] = 1;
}

void expresslrs_fhss_generate_sequence()
{
    debug("Number of FHSS frequencies =");
    debugln(" %d", expresslrs_fhss_num_freqs);

    long macSeed = ((long)UID[2] << 24) + ((long)UID[3] << 16) + ((long)UID[4] << 8) + UID[5];
    expresslrs_rngSeed(macSeed);

    uint8_t expresslrs_fhss_is_available[expresslrs_fhss_num_freqs];

    expresslrs_fhss_calc_reset_available(expresslrs_fhss_is_available);

    // Fill the FHSSsequence with channel indices
    // The 0 index is special - the 'sync' channel. The sync channel appears every
    // syncInterval hops. The other channels are randomly distributed between the
    // sync channels
    int SYNC_INTERVAL = expresslrs_fhss_num_freqs - 1;

    int nLeft = expresslrs_fhss_num_freqs - 1; // how many channels are left to be allocated. Does not include the sync channel
    unsigned int prev = 0;                     // needed to prevent repeats of the same index

    // for each slot in the sequence table
    for (int i = 0; i < NR_SEQUENCE_ENTRIES; i++)
    {
        if (i % SYNC_INTERVAL == 0)
        {
            // assign sync channel 0
            expresslrs_fhss_sequence[i] = 0;
            prev = 0;
        }
        else
        {
            // pick one of the available channels. May need to loop to avoid repeats
            unsigned int index;
            do
            {
                int c = expresslrs_rngN(nLeft); // returnc 0<c<nLeft
                // find the c'th entry in the isAvailable array
                // can skip 0 as that's the sync channel and is never available for normal allocation
                index = 1;
                int found = 0;
                while (index < expresslrs_fhss_num_freqs)
                {
                    if (expresslrs_fhss_is_available[index])
                    {
                        if (found == c)
                            break;
                        found++;
                    }
                    index++;
                }
                if (index == expresslrs_fhss_num_freqs)
                {
                    // This should never happen
                    debugln("FAILED to find the available entry!\n");
                    // What to do? We don't want to hang as that will stop us getting to the wifi hotspot
                    // Use the sync channel
                    index = 0;
                    break;
                }
            } while (index == prev); // can't use index if it repeats the previous value

            expresslrs_fhss_sequence[i] = index;     // assign the value to the sequence array
            expresslrs_fhss_is_available[index] = 0; // clear the flag
            prev = index;                            // remember for next iteration
            nLeft--;                                 // reduce the count of available channels
            if (nLeft == 0)
            {
                // we've assigned all of the channels, so reset for next cycle
                expresslrs_fhss_calc_reset_available(expresslrs_fhss_is_available);
                nLeft = expresslrs_fhss_num_freqs - 1;
            }
        }

        debug(" %02d", expresslrs_fhss_sequence[i]);
        if ((i + 1) % 10 == 0)
        {
            debugln(" ");
        }
        else
        {
            debug(" ");
        }
    } // for each element in FHSSsequence

    debugln("");

    debugln("List of Freqs:");

    for (int i = 0; i < expresslrs_fhss_num_freqs; i++)
    {
        debugln("%d", expresslrs_fhss_get_array_val(i));
    }

    debugln("");
}

/* CRC8 implementation with polynom = x​7​+ x​6​+ x​4​+ x​2​+ x​0 ​(0xD5) */ //from betaflight: https://github.com/betaflight/betaflight/blob/c8b5edb415c33916c91a7ccc8bd19c7276540cd1/src/test/unit/rx_crsf_unittest.cc#L65
static const unsigned char expresslrs_crc8tab[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

uint8_t expresslrs_crc(uint8_t *data, int length)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < length; i++)
    {
        crc = expresslrs_crc8tab[crc ^ *data++];
    }
    return crc;
}

#endif