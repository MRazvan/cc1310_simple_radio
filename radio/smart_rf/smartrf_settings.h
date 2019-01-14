/*
 * settings.h
 *
 *  Created on: Jan 7, 2019
 *      Author: razvanm
 */

#ifndef SMART_RF_SETTINGS_H_
#define SMART_RF_SETTINGS_H_

#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_prop_cmd.h>

typedef struct {
    uint8_t rfMode;             ///< Specifies which PHY modes should be activated. Must be set to RF_MODE_MULTIPLE for dual-mode operation.
    void (*cpePatchFxn)(void);  ///< Pointer to CPE patch function
    void (*mcePatchFxn)(void);  ///< Pointer to MCE patch function
    void (*rfePatchFxn)(void);  ///< Pointer to RFE patch function
} RF_Mode;

extern RF_Mode RF_prop;

// RF Core API commands
extern rfc_CMD_PROP_RADIO_DIV_SETUP_t RF_cmdPropRadioDivSetup;
extern rfc_CMD_FS_t RF_cmdFs;
extern rfc_CMD_PROP_TX_t RF_cmdPropTx;
extern rfc_CMD_PROP_RX_t RF_cmdPropRx;

// RF Core API Overrides
extern uint32_t pOverrides[];


#endif /* SMART_RF_SETTINGS_H_ */
