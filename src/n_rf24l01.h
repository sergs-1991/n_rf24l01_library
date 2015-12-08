#ifndef N_RF24L01_H
#define N_RF24L01_H

#include "n_rf24l01_port.h"

// commands set
#define R_REGISTER 		0x00
#define W_REGISTER 		0x20
#define R_RX_PAYLOAD    0x61
#define W_TX_PAYLOAD	0xa0
#define NOP 			0xff

// registers set
#define CONFIG_RG 		0x00
#define EN_AA_RG		0x01
#define RF_SETUP_RG		0x06
#define STATUS_RG		0x07

//--------- 5-bytes registers ---------
#define RX_ADDR_P0_RG 	0x0A
#define RX_ADDR_P1_RG 	0x0B
#define TX_ADDR_RG 		0x10
//-------------------------------------

#define RX_PW_P0_RG		0x11

// bits definition:

//  CONFIG register
#define PRIM_RX	0x01
#define PWR_UP 	0x02

// each register has 5 bits address in registers map
// used for R_REGISTER and W_REGISTER commands
#define REG_ADDR_BITS 0x1f

// max amount of command's data, in bytes
#define COMMAND_DATA_SIZE 32

static void write_register( u_char reg_addr, u_char reg_val );
static void read_register( u_char reg_addr, u_char* reg_val );
static void clear_bits( u_char reg_addr, u_char bits );
static void set_bits( u_char reg_addr, u_char bits );
static void clear_pending_interrupts( void );

#endif // N_RF24L01_H
