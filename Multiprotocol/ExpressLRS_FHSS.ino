#include "ExpressLRS_FHSS.h"

uint8_t volatile ExpressLRS_FHSSptr = 0;
uint8_t ExpressLRS_FHSSsequence[256] = {0};

void ExpressLRS_FHSSsetCurrIndex(uint8_t value)
{ // set the current index of the FHSS pointer
    ExpressLRS_FHSSptr = value;
}

uint8_t ExpressLRS_FHSSgetCurrIndex()
{ // get the current index of the FHSS pointer
    return ExpressLRS_FHSSptr;
}

uint32_t ExpressLRS_GetInitialFreq()
{
    return ExpressLRS_FHSSfreqs[0];
}

uint32_t ExpressLRS_FHSSgetCurrFreq()
{
    return ExpressLRS_FHSSfreqs[FHSSsequence[FHSSptr]] - FreqCorrection;
}

uint32_t ExpressLRS_FHSSgetNextFreq()
{
    ExpressLRS_FHSSptr++;
    return ExpressLRS_FHSSgetCurrFreq();
}

// Set all of the flags in the array to true, except for the first one
// which corresponds to the sync channel and is never available for normal
// allocation.
void ExpressLRS_resetIsAvailable(uint8_t *array)
{
    // channel 0 is the sync channel and is never considered available
    array[0] = 0;

    // all other entires to 1
    for (unsigned int i = 1; i < NR_FHSS_ENTRIES; i++)
        array[i] = 1;
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
void ExpressLRS_FHSSrandomiseFHSSsequence()
{

#ifdef Regulatory_Domain_AU_915
    Serial.println("Setting 915MHz Mode");
#elif defined Regulatory_Domain_FCC_915
    Serial.println("Setting 915MHz Mode");
#elif defined Regulatory_Domain_EU_868
    Serial.println("Setting 868MHz Mode");
#elif defined Regulatory_Domain_AU_433 || defined Regulatory_Domain_EU_433
    Serial.println("Setting 433MHz Mode");
#else
#error No regulatory domain defined, please define one in common.h
#endif

    Serial.print("Number of FHSS frequencies =");
    Serial.println(NR_FHSS_ENTRIES);

    long macSeed = ((long)UID[2] << 24) + ((long)UID[3] << 16) + ((long)UID[4] << 8) + UID[5];
    rngSeed(macSeed);

    uint8_t ExpressLRS_isAvailable[NR_FHSS_ENTRIES];

    resetIsAvailable(isAvailable);

    // Fill the FHSSsequence with channel indices
    // The 0 index is special - the 'sync' channel. The sync channel appears every
    // syncInterval hops. The other channels are randomly distributed between the
    // sync channels
    const int SYNC_INTERVAL = 20;

    int nLeft = NR_FHSS_ENTRIES - 1; // how many channels are left to be allocated. Does not include the sync channel
    unsigned int prev = 0;           // needed to prevent repeats of the same index

    // for each slot in the sequence table
    for (int i = 0; i < NR_SEQUENCE_ENTRIES; i++)
    {
        if (i % SYNC_INTERVAL == 0)
        {
            // assign sync channel 0
            ExpressLRS_FHSSsequence[i] = 0;
            prev = 0;
        }
        else
        {
            // pick one of the available channels. May need to loop to avoid repeats
            unsigned int index;
            do
            {
                int c = rngN(nLeft); // returnc 0<c<nLeft
                // find the c'th entry in the isAvailable array
                // can skip 0 as that's the sync channel and is never available for normal allocation
                index = 1;
                int found = 0;
                while (index < NR_FHSS_ENTRIES)
                {
                    if (isAvailable[index])
                    {
                        if (found == c)
                            break;
                        found++;
                    }
                    index++;
                }
                if (index == NR_FHSS_ENTRIES)
                {
                    // This should never happen
                    Serial.print("FAILED to find the available entry!\n");
                    // What to do? We don't want to hang as that will stop us getting to the wifi hotspot
                    // Use the sync channel
                    index = 0;
                    break;
                }
            } while (index == prev); // can't use index if it repeats the previous value

            ExpressLRS_FHSSsequence[i] = index; // assign the value to the sequence array
            ExpressLRS_isAvailable[index] = 0;  // clear the flag
            prev = index;            // remember for next iteration
            nLeft--;                 // reduce the count of available channels
            if (nLeft == 0)
            {
                // we've assigned all of the channels, so reset for next cycle
                resetIsAvailable(isAvailable);
                nLeft = NR_FHSS_ENTRIES - 1;
            }
        }

        Serial.print(ExpressLRS_FHSSsequence[i]);
        if ((i + 1) % 10 == 0)
        {
            Serial.println();
        }
        else
        {
            Serial.print(" ");
        }
    } // for each element in FHSSsequence

    Serial.println();
}
