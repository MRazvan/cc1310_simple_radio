/*
 * board.h
 *
 *  Created on: Jan 14, 2019
 *      Author: razvanm
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <driverlib/ioc.h>

#define F_CPU                       48000000


#define LED_RED                     IOID_6
#define LED_GREEN                   IOID_7

#define UART_DEV                    UART0_BASE

#define UART_RX                     IOID_2          /* RXD  */
#define UART_TX                     IOID_3          /* TXD  */
#define UART_CTS                    IOID_19         /* CTS  */
#define UART_RTS                    IOID_18         /* RTS */

#define UART_BAUD                   115200

#endif /* BOARD_H_ */
