#include "ExpressLRS_common.h"
#include "iface_SX1276.h"

expresslrs_mod_settings_s *ExpressLRS_currAirRate_Modparams;
expresslrs_rf_pref_params_s *ExpressLRS_currAirRate_RFperfParams;

expresslrs_mod_settings_s ExpressLRS_AirRateConfig[RATE_MAX] = {
    {RATE_200HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 6, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 5000, 200, TLM_RATIO_1_64, 2, 8},
    {RATE_100HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 7, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 10000, 100, TLM_RATIO_1_64, 2, 8},
    {RATE_50HZ, SX1276_MODEM_CONFIG1_BW_500KHZ, 8, SX1276_MODEM_CONFIG1_CODING_RATE_4_7, 20000, 50, TLM_RATIO_NO_TLM, 2, 8}}; // for model recovery 

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
        return &ExpressLRS_AirRateConfig[RATE_MAX - 1];  // Set to last usable entry in the array (currently 50HZ)
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

uint8_t TLMratioEnumToValue(expresslrs_tlm_ratio_e enumval)
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