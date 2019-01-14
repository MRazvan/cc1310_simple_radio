# CC1310 Simple Radio

Basic project demonstrating the usage of the Radio on LAUNCHXL-CC1310 mcu without TI RTOS / SDK's.
The reason for the project was finding the simplest way to send / receive data using the Radio.

Obviously there are limitations.
* It does not treat error codes or error interrupts.
* There is no guard against overwhelming the radio with TX requests. At least for the default configuration.

The configuration in the project is using the 5kbs SimpleLink Long Range settings for the 868Mhz band. 
There is no limitation against using something else, just configure the settings in SmartRF Studio and import them in the project. The suggestion is to replace only the command definitions. There might be a need for some other patch files, that are not included depending on the configuration.

If you can't figure out how to switch between RX / TX then the project is not addressed to you :)

**Files** :
* radio/radio.c - The radio api implementation (open, close, send, setRxHandler)
* example_rx.h - RX implementation
* example_tx.h - TX implementation

Did I mention there is no error checking?