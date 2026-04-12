#include <string.h>
#include "protocol_ftm.h"
#include "max30102.h"

void build_payload(uint8_t *payload)
{
    memset(payload, 0, 50);

    // --- FTM (distância)
    uint16_t dist_int = (uint16_t)s_dist_est;

    payload[0] = (dist_int >> 8) & 0xFF;
    payload[1] = dist_int & 0xFF;

    // --- MAX30102 (frequência cardíaca)
    uint16_t hr_int = (uint16_t)heartrate;

    payload[2] = (hr_int >> 8) & 0xFF;
    payload[3] = hr_int & 0xFF;
}