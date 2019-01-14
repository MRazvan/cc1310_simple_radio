/*
 * radio.c
 *
 *  Created on: Jan 7, 2019
 *      Author: razvanm
 */

#include <stdint.h>
#include <stdbool.h>

#include <driverlib/osc.h>
#include <driverlib/prcm.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_prop_mailbox.h>
#include <driverlib/rf_prop_cmd.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rfc.h>
#include <driverlib/cpu.h>
#include <inc/hw_rfc_pwr.h>
#include <radio/smart_rf/smartrf_settings.h>
#include "radio.h"

#define MHZ                        1000000
#define RF_CMD0                    0x0607
#define RF_CORE_CLOCKS_MASK (RFC_PWR_PWMCLKEN_RFC_M | RFC_PWR_PWMCLKEN_CPE_M \
                             | RFC_PWR_PWMCLKEN_CPERAM_M | RFC_PWR_PWMCLKEN_FSCA_M \
                             | RFC_PWR_PWMCLKEN_PHA_M | RFC_PWR_PWMCLKEN_RAT_M \
                             | RFC_PWR_PWMCLKEN_RFERAM_M | RFC_PWR_PWMCLKEN_RFE_M \
                             | RFC_PWR_PWMCLKEN_MDMRAM_M | RFC_PWR_PWMCLKEN_MDM_M)

#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define RX_MSG_SIZE            240 + (8 /* CRC, RSSI, STATE, TIMESTAMP */)
#define    RX_ENTRY_SIZE        (sizeof(rfc_dataEntryGeneral_t) + RX_MSG_SIZE)

uint8_t                g_rx_buffer[RX_MSG_COUNT * RX_ENTRY_SIZE];
rfc_propRxOutput_t     g_rx_stats;

volatile static phy_rx_handler g_rx_handler;
uint_fast8_t PHY_setRxHandler(phy_rx_handler handler){
    // Function for handling the receive of a packet
    g_rx_handler = handler;
    return PHY_OK;
}

// Function for sending of data
uint_fast8_t PHY_send(uint8_t* data, uint8_t size){
    RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_STOP)); // Stop current command (should be RX)
    RF_cmdPropTx.pPkt                        =    data;
    RF_cmdPropTx.pktLen                        =    MIN(size, RX_MSG_SIZE);

    RFCDoorbellSendTo((uint32_t)&RF_cmdPropTx);
    return PHY_OK;
}

uint_fast8_t PHY_open(){
    //***************************************************************************
    //***************************************************************************
    //                                Start the radio
    //***************************************************************************
    //***************************************************************************
    // Start the switch to HF oscillator is needed
    if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF){
        OSCClockSourceSet(OSC_SRC_CLK_MF | OSC_SRC_CLK_HF, OSC_XOSC_HF);
    }

    // Power on radio
    PRCMPowerDomainOn(PRCM_DOMAIN_RFCORE);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_ON);

    PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();
    while(!PRCMLoadGet());

    /* Let CPE boot */
    HWREG(RFC_PWR_NONBUF_BASE + RFC_PWR_O_PWMCLKEN) = RF_CORE_CLOCKS_MASK;

    if (RF_prop.cpePatchFxn != NULL || RF_prop.mcePatchFxn != NULL || RF_prop.rfePatchFxn != NULL){
        RFCDoorbellSendTo(CMDR_DIR_CMD_2BYTE(RF_CMD0,RFC_PWR_PWMCLKEN_MDMRAM | RFC_PWR_PWMCLKEN_RFERAM));

        if (RF_prop.cpePatchFxn != NULL)
            RF_prop.cpePatchFxn();
        if (RF_prop.mcePatchFxn != NULL)
            RF_prop.mcePatchFxn();
        if (RF_prop.rfePatchFxn != NULL)
            RF_prop.rfePatchFxn();

        RFCDoorbellSendTo(CMDR_DIR_CMD_1BYTE(CMD_BUS_REQUEST, 1));
    }
    RFCAdi3VcoLdoVoltageMode(true);

    // Switch to HF osc
    if (OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF){
        OSCHfSourceSwitch();
    }

    //***************************************************************************
    //***************************************************************************
    //                            Setup / Configure radio
    //***************************************************************************
    //***************************************************************************
    // Trim rf values
    RFCRTrim((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup);
    rfTrim_t rfTrim;
    RFCRfTrimRead((rfc_radioOp_t*)&RF_cmdPropRadioDivSetup, &rfTrim);
    RFCRfTrimSet(&rfTrim);

    // Finally start the Radio
    RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_START_RAT));
    CPUdelay(16000); // About 1ms delay
    RFCDoorbellSendTo((uint32_t)&RF_cmdPropRadioDivSetup);
    CPUdelay(16000); // About 1ms delay
    RFCDoorbellSendTo((uint32_t)&RF_cmdFs);
    CPUdelay(16000); // About 1ms delay


    //***************************************************************************
    //***************************************************************************
    //                            Setup interrupts
    //***************************************************************************
    //***************************************************************************
    // Clear interrupt flags
    IntMasterDisable();
    // All interrupts go to CPE0
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEISL) = 0x0;
    // Clear interrupt flags
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = 0x0;
    // Enable RX_ENTRY_DONE interrupt, ignore all other interrupts (and there are a lot of them)
    //  because we only look at RX_ENTRY_DONE we ignore all errors also
    HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIEN) = IRQ_RX_ENTRY_DONE;
    // Enable CPE0 interrupt

    IntPendClear(INT_RFC_CPE_0);
    IntEnable(INT_RFC_CPE_0);

    IntMasterEnable();


    //***************************************************************************
    //***************************************************************************
    //                            Setup rx command / structures
    //***************************************************************************
    //***************************************************************************
    // Setup the RX queue
    memset(g_rx_buffer, 0, sizeof(g_rx_buffer));
    rfc_dataEntry_t *entry;
    for(uint8_t idx = 0; idx < RX_MSG_COUNT; idx++){
        entry = (rfc_dataEntry_t *)&g_rx_buffer[idx * RX_ENTRY_SIZE];
        entry->status = DATA_ENTRY_PENDING;
        entry->config.type = DATA_ENTRY_TYPE_GEN;
        entry->config.lenSz = 1;
        entry->length = RX_MSG_SIZE - 8;
        entry->pNextEntry = &g_rx_buffer[(idx + 1) * RX_ENTRY_SIZE];
    }
    ((rfc_dataEntry_t *)&g_rx_buffer[(RX_MSG_COUNT - 1) * RX_ENTRY_SIZE])->pNextEntry = &g_rx_buffer[0];

    g_data_queue.pCurrEntry = &g_rx_buffer[0];
    g_data_queue.pLastEntry = NULL;    // Circular buffer
    g_rx_entry                = &g_rx_buffer[0];

    // Update the RX command
    RF_cmdPropRx.pQueue    = &g_data_queue;
    RF_cmdPropRx.pOutput= (uint8_t*)&g_rx_stats;
    RF_cmdPropRx.rxConf.bIncludeHdr = 0x0; // No header
    RF_cmdPropRx.rxConf.bIncludeCrc = 0x0; // No CRC
    RF_cmdPropRx.rxConf.bAppendRssi = 0x0; // No RSSI
    RF_cmdPropRx.rxConf.bAppendTimestamp = 0x0; // No Timestamp
    RF_cmdPropRx.rxConf.bAppendStatus = 0x0; // No status
    // Discard some packets (Invalid CRC, Ignored packets)
    RF_cmdPropRx.rxConf.bAutoFlushIgnored = 0x01;
    RF_cmdPropRx.rxConf.bAutoFlushCrcErr = 0x01;

    // Run RX continuously
    RF_cmdPropRx.pktConf.bRepeatOk = 0x1; // Start RX again after a successful receive
    RF_cmdPropRx.pktConf.bRepeatNok = 0x1; // Start RX again after a NOK packet

    // Set TX to continue with RX
    //  That way after we send something we immediately go to RX again
    RF_cmdPropTx.condition.rule = COND_ALWAYS;
    RF_cmdPropTx.pNextOp = (rfc_radioOp_t*)&RF_cmdPropRx;

    // Enter RX mode by default
    RFCDoorbellSendTo((uint32_t)&RF_cmdPropRx);
    CPUdelay(16000); // About 1ms delay

    return PHY_OK;
}

uint_fast8_t PHY_close(){
    // Stop the current command
    RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_STOP));
    CPUdelay(16000);

    // Unsubscribe from the interrupt
    IntMasterDisable();
    IntDisable(INT_RFC_CPE_0);
    IntPendClear(INT_RFC_CPE_0);
    IntMasterEnable();

    // Power down radio
    PRCMDomainDisable(PRCM_DOMAIN_RFCORE);
    PRCMLoadSet();
    while(!PRCMLoadGet());

    PRCMPowerDomainOff(PRCM_DOMAIN_RFCORE);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) == PRCM_DOMAIN_POWER_ON);

    return PHY_OK;
}

void RFCCPE0IntHandler(){
    // If the event is RX_ENTRY_DONE do something
    //      this should be the only event we have registered for
    if (HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) & IRQ_RX_ENTRY_DONE){
        rfc_dataEntryGeneral_t *entry = (rfc_dataEntryGeneral_t *)g_rx_entry;
        g_rx_entry = entry->pNextEntry;
        if (g_rx_handler != NULL){
            g_rx_handler(entry, &g_rx_stats);
        }
        entry->status = DATA_ENTRY_PENDING;
        HWREG(RFC_DBELL_NONBUF_BASE + RFC_DBELL_O_RFCPEIFG) = ~IRQ_RX_ENTRY_DONE;
    }
}
