/* 
*  config.h
*  Step1: sudo apt-get install wiringpi
*  Step2: install libgps from https://github.com/wdalmut/libgps
*  Step3. UPDATE THIS FILE ACCORDING TO YOUR ENVIRONMENT 
*  Step4. make
*  Step5. sudo ./enddevice
*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

// operation frequency, change if you need.
#define FREQUENCY   923200000
// Default RX2
#define FREQ_RX2    923200000

#define DEFAULT_KEEPALIVE   30  // default time interval for tx
#define DEFAULT_RX1     1       // default RX1 time in second
#define DEFAULT_RX2     1

// LoRaWAN NwkSKey, network session key 
// Change to your own key 
static const uint8_t DEVKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

// LoRaWAN AppSKey, application session key
// Change to your own key 
static const uint8_t ARTKEY[16] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
static const uint32_t DEVADDR = 0xffffffff ; // <-- Change this address for every node!

#endif // _CONFIG_H_
