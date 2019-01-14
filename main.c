
#include <stdint.h>
#include <driverlib/setup.h>
#include <driverlib/prcm.h>

#define EXAMPLE_TX
#if defined(EXAMPLE_TX)
    #include "example_tx.h"
#else
    #include "example_rx.h"
#endif


/**
 * main.c
 */
int main(void)
{
    SetupTrimDevice();
    PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH | PRCM_DOMAIN_SERIAL);
    while(PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH | PRCM_DOMAIN_SERIAL) != PRCM_DOMAIN_POWER_ON){}
    // Enable clock for GPIO / UART
    PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);
    PRCMLoadSet();
    while(!PRCMLoadGet()){}

    PHY_open(); // Open radio in RX mode
    example_handler();
    PHY_close();// Close radio
    return 0;
}
