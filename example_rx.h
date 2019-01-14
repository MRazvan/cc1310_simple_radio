/*
 * rx_handler.h
 *
 *  Created on: Jan 14, 2019
 *      Author: razvanm
 */

#ifndef EXAMPLE_RX_H_
#define EXAMPLE_RX_H_

#include <driverlib/ioc.h>
#include <driverlib/gpio.h>
#include <driverlib/uart.h>
#include <driverlib/rf_prop_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <radio/radio.h>
#include "board.h"

typedef struct {
    rfc_dataEntry_t entry;
    uint8_t         length;
    uint8_t         payload[127];
} tData;

void UART_write(const char* msg){
    while(*msg)
        UARTCharPut(UART_DEV, *msg++);
}

void rx_handler(rfc_dataEntryGeneral_t* dataEntry, rfc_propRxOutput_t* stats){
    tData* data = (tData*)dataEntry;
    uint8_t idx = 0;
    while(idx < data->length)
        UARTCharPut(UART_DEV, data->payload[idx++]);
    UARTCharPut(UART_DEV, '\r');
    UARTCharPut(UART_DEV, '\n');
	GPIO_toggleDio(LED_RED);
}

void example_handler(){

    IOCPinTypeGpioOutput(LED_RED);


    //***************************************************************************
    //***************************************************************************
    //                         Setup / configure UART
    //***************************************************************************
    //***************************************************************************
    IntMasterDisable();
    PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL) != PRCM_DOMAIN_POWER_ON){}
    // Enable clock for GPIO / UART
    PRCMPeripheralRunEnable(PRCM_PERIPH_UART0);
    PRCMLoadSet();
    while(!PRCMLoadGet()){}

    // Configure UART
    IOCPinTypeUart(UART_DEV, UART_RX, UART_TX, UART_CTS, UART_RTS);
    UARTFIFODisable(UART_DEV);
    UARTIntDisable(UART_DEV, UART_INT_OE | UART_INT_BE | UART_INT_PE |
                                           UART_INT_FE | UART_INT_RT | UART_INT_TX |
                                           UART_INT_RX | UART_INT_CTS);
    UARTConfigSetExpClk(UART_DEV, F_CPU, UART_BAUD, UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE | UART_CONFIG_WLEN_8);
    UARTEnable(UART_DEV);
    IntMasterEnable();


    UART_write("Starting in RX mode.\r\n");
    //***************************************************************************
    //***************************************************************************
    //                          Register RX handler
    //***************************************************************************
    //***************************************************************************
    PHY_setRxHandler(&rx_handler);
    while(true);
}

#endif /* EXAMPLE_RX_H_ */
