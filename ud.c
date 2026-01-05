#include "ud.h"

#include <stdio.h>
#include <string.h>

#include "em_device.h"
#include "em_msc.h"
#include "uart.h"


#define UD_FW_MAJ 0
#define UD_FW_MIN 1

#define USER_PAGE_WORDS 16

typedef union {
    struct {
        __IM uint8_t FW_MAJ;
        __IM uint8_t FW_MIN;
        __IM uint32_t CAL1;
    };

    uint32_t WORDS[USER_PAGE_WORDS];
} USERDATA_Union;

const USERDATA_Union ud_default_values = {
    .FW_MAJ = 0,
    .FW_MIN = 1,
    .CAL1 = 2810
};

#define USERDATA  ((USERDATA_Union *) USERDATA_BASE)


static void write_user_data(const USERDATA_Union *new_data) {
    // Erase user data page
    MSC_ErasePage((uint32_t *) USERDATA_BASE);

    // Rewrite user page with new data
    if (MSC_WriteWord(USERDATA->WORDS, new_data->WORDS, USER_PAGE_WORDS) != mscReturnOk) {
        uart_tx("Writing to flash failed\r\n");
    } else {
        uart_tx("Writing to flash successful\r\n");
    }
}

void ud_check_version() {
    char buff[80];
    if (USERDATA->FW_MAJ != ud_default_values.FW_MAJ) {
        sprintf(buff, "User data version mismatch (%d != %d)\r\n", USERDATA->FW_MAJ, ud_default_values.FW_MAJ);
        uart_tx(buff);
        if (USERDATA->FW_MAJ == 0xff) {
            uart_tx("New device, ");
        } else {
            uart_tx("Incompatible version, ");
        }
        uart_tx("initialising user data with default values. Idq calibration recommended.\r\n");


        write_user_data(&ud_default_values);
    }

    sprintf(buff, "User data version %d.%d loaded. Idq calibration value = %lu\r\n",
        USERDATA->FW_MAJ, USERDATA->FW_MIN, USERDATA->CAL1);

    uart_tx(buff);
}


uint32_t ud_get_cal_value() {
    return USERDATA->CAL1;
}

void ud_update_cal_value(uint32_t new_cal_value) {
    USERDATA_Union user_page_copy;
    uint32_t i;

    // Make copy of user data page contents
    for (i = 0; i < USER_PAGE_WORDS; i++)
        user_page_copy.WORDS[i] = USERDATA->WORDS[i];

    *(uint8_t *) &user_page_copy.CAL1 = new_cal_value;

    write_user_data(&user_page_copy);
}
