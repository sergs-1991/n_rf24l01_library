// This file contains functions to work with n_rf24l01 transceiver

// n_rf24l01 commands have 8 bits.
// Every new command must be started by a high to low transition on CSN.
// In parallel to the SPI command word applied on the MOSI pin, the STATUS register is shifted serially out on
// the MISO pin.

// The serial shifting SPI commands is in the following format:
//  <Command word: MSBit to LSBit (one byte)>
//  <Data bytes: LSByte to MSByte, MSBit in each byte first>

// spi on n_rf24l01 works with next settings:
//  CPOL = 0, CPHA = 0
//  msbit first
//  8 bits per word
//  spi speed up to 8MHz (but now we use only 50kHz)
//  CSN - is active low

#include <stdio.h>
#include <string.h>

#include "uart_service.h"

#include "n_rf24l01.h"

static n_rf24l01_backend_t n_rf24l01_backend;

// transmit one byte, n_rf24l01 must be in transmit mode
//======================================================================================================
void n_rf24l01_transmit_byte( u_char byte )
{
  n_rf24l01_backend.send_cmd( W_TX_PAYLOAD, NULL, &byte, 1, 1 );

  // CE up... sleep 10 us... CE down - to actual data transmit (in space)
  n_rf24l01_backend.set_up_ce_pin( 1 );
  n_rf24l01_backend.usleep( 10 );
  n_rf24l01_backend.set_up_ce_pin( 0 );
}

// turn n_rf24l01 into transmit mode
//======================================================================================================
void prepare_to_transmit( void )
{
  n_rf24l01_backend.set_up_ce_pin( 0 );

  clear_bits( CONFIG_RG, PRIM_RX );
  n_rf24l01_backend.usleep( 140 );
}

// turn n_rf24l01 into receive mode
//======================================================================================================
void prepare_to_receive( void )
{
  set_bits( CONFIG_RG, PRIM_RX );

  n_rf24l01_backend.set_up_ce_pin( 1 );
  n_rf24l01_backend.usleep( 140 );
}

// write register with @reg_addr from @reg_val
// reg_addr - address of register to be written to
// reg_val - variable register's content will be read from
// Note: only for 1-byte registers
//======================================================================================================
static void write_register( u_char reg_addr, u_char reg_val )
{
  // clear first command-specified bits (for R_REGISTER and W_REGISTER)
  reg_addr &= REG_ADDR_BITS;

  n_rf24l01_backend.send_cmd( W_REGISTER | reg_addr, NULL, &reg_val, 1, 1 );
}

// read register with @reg_addr in @reg_val
// reg_addr - address of register to be read from
// reg_val - pointer to memory register's content will be written into
// Note: only for 1-byte registers
//======================================================================================================
static void read_register( u_char reg_addr, u_char* reg_val )
{
  if( !reg_val )
    return;

  // clear first command-specified bits (for R_REGISTER and W_REGISTER)
  reg_addr &= REG_ADDR_BITS;

  n_rf24l01_backend.send_cmd( R_REGISTER | reg_addr, NULL, reg_val, 1, 0 );
}

// clear specified bits in register
// reg_addr - address of register to be modified
// bits - bits to be cleared
// only for 1-byte registers
//======================================================================================================
static void clear_bits( u_char reg_addr, u_char bits )
{
  u_char data = 0;

  // clear first command-specified bits (for R_REGISTER and W_REGISTER)
  reg_addr &= REG_ADDR_BITS;

  // firstly we must read register...
  n_rf24l01_backend.send_cmd( R_REGISTER | reg_addr, NULL, &data, 1, 0 );

  // apply new bits...
  data &= ~bits;

  // then set new bits
  n_rf24l01_backend.send_cmd( W_REGISTER | reg_addr, NULL, &data, 1, 1 );
}

// set specified bits in register
// reg_addr - address of register to be modified
// bits - bits to be set
// only for 1-byte registers
//======================================================================================================
static void set_bits( u_char reg_addr, u_char bits )
{
  u_char data = 0;

  // clear first command-specified bits (for R_REGISTER and W_REGISTER)
  reg_addr &= REG_ADDR_BITS;

  // firstly we must read register...
  n_rf24l01_backend.send_cmd( R_REGISTER | reg_addr, NULL, &data, 1, 0 );

  // apply new bits...
  data |= bits;

  // then set new bits
  n_rf24l01_backend.send_cmd( W_REGISTER | reg_addr, NULL, &data, 1, 1 );
}

// for clear interrupts pending bits
//======================================================================================================
static void clear_pending_interrupts( void )
{
  u_char status_reg;

  // NOP command: for read status register
  n_rf24l01_backend.send_cmd( NOP, &status_reg, NULL, 0, 0 );

  write_register( STATUS_RG, status_reg );
}

void read_payload( void )
{
  u_char buf = 0;
  char str[3] = { 0, };

  clear_pending_interrupts();
  n_rf24l01_backend.send_cmd( R_RX_PAYLOAD, NULL, &buf, 1, 0 );

  sprintf( str, "%c.", buf );
  uart_send_str( str );

  /*
  static char buf[25];
  static char* temp = buf;
  led_flash_irq( 450, 150 );
  n_rf24l01_backend.send_cmd( R_RX_PAYLOAD, NULL, temp, 1, 0 );

  if( ++temp >= (buf + sizeof(buf) - 1) )
  {
    *temp = 0;
    temp = buf;
  }

  if( strstr( buf, "Lena" ) )
    led_flash_irq( 450, 150 );

  clear_pending_interrupts();*/
}

// make all initialization step to prepare n_rf24l01 to work
// returns -1, if failed
//======================================================================================================
int n_rf24l01_init( const n_rf24l01_backend_t* n_rf24l01_backend_local )
{
  if( !n_rf24l01_backend_local )
    return -1;

  // get copy of callback set
  memcpy( &n_rf24l01_backend, n_rf24l01_backend_local, sizeof( n_rf24l01_backend ) );

  write_register( EN_AA_RG, 0x00 );

  // turn on n_rf24l01 transceiver
  set_bits( CONFIG_RG, PWR_UP );
  n_rf24l01_backend.usleep( 1500 );

  // set data field size to 32 byte (we will transmit 32 bytes for time)
  write_register( RX_PW_P0_RG, 0x01 );

  // set the lowermost transmit power
  clear_bits( RF_SETUP_RG, 0x06 );

  return 0;
}
