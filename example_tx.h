/*
 * tx_example.h
 *
 *  Created on: Jan 14, 2019
 *      Author: razvanm
 */

#ifndef EXAMPLE_TX_H_
#define EXAMPLE_TX_H_

#include <stdint.h>
#include <stdarg.h>

#include <driverlib/ioc.h>
#include <driverlib/systick.h>
#include <driverlib/gpio.h>
#include <driverlib/uart.h>
#include <driverlib/aon_batmon.h>
#include <radio/radio.h>

#include "board.h"

uint8_t g_printf_buffer[128] = {0};
void simple_itoa(int32_t nr, char* buff, uint8_t base){
    static char values[] = "0123456789ABCDEF";
    char *tmp_buff = buff;
    if (base < 2 || base > 16)
        return;
    if (nr < 0){
        *tmp_buff++ = '-';
        nr *= -1;
    }else if (nr == 0){
        *tmp_buff++ = '0';
        return;
    }

    int8_t digits = 0;
    int32_t magnitude = 1;
    while(magnitude < nr){
        digits++;
        magnitude *= base;
    }
    if (magnitude == nr){
        digits++;
    }
    tmp_buff += digits;
    *tmp_buff-- = 0;// End the number here

    while(digits-- > 0){
        *tmp_buff-- = values[(uint8_t)(nr % base)];
        nr /= base;
    }
}

int32_t simple_sprintf(char* buff, char* fmt, ...){
    char*   buffIdx = buff;
    char    ch;
    va_list args;


    va_start(args, fmt);

    while ((ch = (*fmt++))){
        if (ch != '%'){
            *buffIdx++ = ch;
            continue;
        }

        ch = *fmt++;
        switch(ch){
            case 'c':{
                int32_t tmpChar = va_arg(args, int);
                *buffIdx++ = (char)tmpChar;
                break;
            }
            case 's':{
                char* tmpStr = va_arg(args, char *);
                while(*tmpStr){
                    *buffIdx++ = *tmpStr++;
                }
                break;
            }
            case 'd':{
                int32_t tmpInt = va_arg(args, int);
                simple_itoa(tmpInt, buffIdx, 10);
                buffIdx += strlen(buffIdx);
                break;
            }
            case 'x':
            case 'X':{
                int32_t tmpHexInt = va_arg(args, int);
                simple_itoa(tmpHexInt, buffIdx, 16);
                buffIdx += strlen(buffIdx);
                break;
            }
            default:{
                *buffIdx++ = ch;
                break;
            }
        }
    }
    *buffIdx = '\0';

    va_end(args);
    return strlen(buff);
}

volatile uint32_t g_total_ticks = 0;
void SysTickIntHandler(){
    g_total_ticks++;
}

void example_handler(){
    AONBatMonEnable();

    SysTickEnable();
    SysTickPeriodSet(48000);
    SysTickIntEnable();

    IOCPinTypeGpioOutput(LED_GREEN);

    uint32_t lastTick = g_total_ticks;
    while(true){
        if (g_total_ticks - lastTick > 999){
            uint8_t totalChars = simple_sprintf((char*)&g_printf_buffer[0], "Total Ticks : %d. Temp : %d Deg C.", g_total_ticks, AONBatMonTemperatureGetDegC());
            PHY_send(&g_printf_buffer[0], totalChars);
            GPIO_toggleDio(LED_GREEN);
            lastTick = g_total_ticks;
        }
    }
}

#endif /* EXAMPLE_TX_H_ */
