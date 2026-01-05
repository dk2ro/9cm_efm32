//
// Created by robert on 05.01.26.
//

#include "vdac.h"

#include "em_vdac.h"

void vdac_init() {
    VDAC_Init_TypeDef vdacInit = VDAC_INIT_DEFAULT;
    VDAC_InitChannel_TypeDef vdacChInit = VDAC_INITCHANNEL_DEFAULT;
    // Set prescaler to get 1 MHz VDAC clock frequency.
    vdacInit.prescaler = VDAC_PrescaleCalc(1000000, true, 0);
    vdacInit.reference = vdacRefAvdd;
    VDAC_Init(VDAC0, &vdacInit);
    vdacChInit.enable = true;
    VDAC_InitChannel(VDAC0, &vdacChInit, 0);

    VDAC_ChannelOutputSet(VDAC0, 0, 0);
}
