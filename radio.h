/*  radio.h
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/
#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdio.h>
#include <stdint.h>

// define
#define TX_BUFF_SIZE    1024

#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13

#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_SYMB_TIMEOUT_LSB  		0x1F
#define REG_PKT_SNR_VALUE		    0x19
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_MAX_PAYLOAD_LENGTH 		0x23
#define REG_HOP_PERIOD              0x24
#define REG_SYNC_WORD			    0x39
#define REG_VERSION	  		        0x42
#define REG_INV_IQ          0x33
#define REG_RSSI_VAL        0x1B
#define REG_PREAMBLE_LENGTH_LSB 0x21
#define RSSI_CORR           157
#define RSSI_CH_FREE        80

#define SX72_MODE_RX_CONTINUOS      0x8D    // 10001101 0x80=Lora, 0x08=HF???, 0x05=RX_CONT
#define SX72_MODE_TX                0x8B    // 10001011 Lora only
#define SX72_MODE_STDBY             0x89     // 10001001
#define SX72_MODE_SLEEP             0x88    // 10001000

#define SX1276_IRQ_TXDONE_MASK      0x08    // bit3
#define SX1276_IRQ_RXDONE_MASK      0x40    // bit6
//#define SX1276_MODE_MASK          0x07    // bit2-0
    
#define SX1276_MC3_AGCAUTO         0x04     
#define SX1276_INV_IQ_TX            0x01    // bit0
#define SX1276_INV_IQ_RX            0x40    // bit6

#define PAYLOAD_LENGTH              0x40

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN		    	0x20

// CONF REG
#define REG1                        0x0A
#define REG2                        0x84

#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE  0x01 // mandated for SF11 and SF12

// FRF
#define     REG_FRF_MSB              0x06
#define     REG_FRF_MID              0x07
#define     REG_FRF_LSB              0x08

// Tx Power Register
#define	    REG_PA_CFG	    0x09
#define     REG_PA_RAMP     0x0A

#define     FRF_MSB         0xD9 // 868.1 Mhz
#define     FRF_MID         0x06
#define     FRF_LSB         0x66

#define LORA_MAC_PREAMBLE   0x34

enum sf_t { SF7=7, SF8, SF9, SF10, SF11, SF12 };

// proto type declaration
void SetupwiringPiSPI();
void initpinmode(void);
void selectreceiver(void);
void unselectreceiver(void);
byte readRegister(byte );
void writeRegister(byte , byte );
void set_Opmode(byte);
void cfg_Modem(void);
boolean receivePkt(char *);
void set_Channel( uint32_t);
void set_Ch2(void);
void set_DataRate(enum sf_t);
void SetupLoRa(void);
int rf_receivepacket(void);
void transmitPkt(buf_t* );
void rf_transmitpacket(buf_t*);
boolean is_channel_free();

#endif // _RADIO_H_
