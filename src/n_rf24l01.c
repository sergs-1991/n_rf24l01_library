/**
 * @file n_rf24l01 transceiver library implementation
 */

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

#include "n_rf24l01.h"

static n_rf24l01_backend_t n_rf24l01_backend;


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
/*static void read_register( u_char reg_addr, u_char* reg_val )
{
  if( !reg_val )
    return;

  // clear first command-specified bits (for R_REGISTER and W_REGISTER)
  reg_addr &= REG_ADDR_BITS;

  n_rf24l01_backend.send_cmd( R_REGISTER | reg_addr, NULL, reg_val, 1, 0 );
}*/

/**
 * @brief read status register via NOP cmd
 *
 * @param[out] status_reg - pointer to write obtained status register to
 */
//======================================================================================================
static inline void read_status_reg( u_char* status_reg )
{
  // NOP command: for read status register
  n_rf24l01_backend.send_cmd( NOP, status_reg, NULL, 0, 0 );
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

  read_status_reg( &status_reg );
  write_register( STATUS_RG, status_reg );
}

/**
 * @brief transmit one package through n_rf24l01 transceiver
 *
 *  * @param[in] data - package's data to transmit
 *
 *  Note: this is block call (until all data are transmitted)
 *        number of byte to transmit depend on PKG_SIZE
 */
//======================================================================================================
static void transmit_pkg( u_char* data )
{
  n_rf24l01_backend.send_cmd( W_TX_PAYLOAD, NULL, data, PKG_SIZE, 1 );

  // CE up... sleep 10 us... CE down - to actual data transmit (in space)
  n_rf24l01_backend.set_up_ce_pin( 1 );
  n_rf24l01_backend.usleep( 10 );
  n_rf24l01_backend.set_up_ce_pin( 0 );
}


//======================================================================================================
//======================================================================================================


/**
 * @brief upper half of n_rf24l01 irq handler
 *
 * Note: it's desirable to call this function from in hardware interrupt context
 */
//======================================================================================================
void n_rf24l01_upper_half_irq( void )
{

}

/**
 * @brief bottom half of n_rf24l01 irq handler
 *
 * Note: bottom half mustn't be executed in hardware interrupt context, due to a big execution time
 */
//======================================================================================================
void n_rf24l01_bottom_half_irq( void )
{
  u_char buf[PKG_SIZE] = { 0, };
  u_char status_reg = 0;

  read_status_reg( &status_reg );

  if( status_reg & RX_DR )
    n_rf24l01_backend.send_cmd( R_RX_PAYLOAD, NULL, buf, PKG_SIZE, 0 );

  clear_pending_interrupts();

  if( status_reg & RX_DR )
    n_rf24l01_backend.handle_received_data( buf, PKG_SIZE );
}

/**
 * @brief transmit packages through n_rf24l01 transceiver
 *
 * @param[in]  data - package's data to transmit
 * @param[num] num  - amount of data to transmit, in bytes
 *
 * Note: this is block call (until all data are transmitted)
 */
//======================================================================================================
void n_rf24l01_transmit_pkgs( const void* data, u_int num )
{
  int i, pkgs_amount;
  u_char pkg[PKG_SIZE] = { 0, };
  u_char* frame = NULL;

  if( !data || !num )
    return;

  frame = (u_char*)data;

  // calculate amount of packages to transmit (n_rf24l01 allow to transmit up to 32 bytes for time)
  if( num <= PKG_SIZE )
    pkgs_amount = 1;
  else
  {
    pkgs_amount = num / PKG_SIZE;
    if( num % PKG_SIZE )
      pkgs_amount++;
  }

  if( pkgs_amount == 1 )
  {
    if( num != PKG_SIZE )
    {
      memcpy( pkg, frame, num );
      transmit_pkg( pkg );
    }
    else
      transmit_pkg( frame );

    // TODO: I think wait 300 mcs is quickly then wait TX irq, read STATUS reg and react on it
    n_rf24l01_backend.usleep( 300 );

    return;
  }

  for( i = 0; i < pkgs_amount; i++ )
  {
    // if it's last package to transmit
    if( i + 1 == pkgs_amount )
    {
      memcpy( pkg, frame, num % PKG_SIZE );
      transmit_pkg( pkg );
    }
    else
      transmit_pkg( frame );

    frame += PKG_SIZE;

    // TODO: I think wait 300 mcs is quickly then wait TX irq, read STATUS reg and react on it
    n_rf24l01_backend.usleep( 300 );
  }
}

/**
 * @brief configure n_rf24l01 to be a transmitter
 */
//======================================================================================================
void n_rf24l01_prepare_to_transmit( void )
{
  n_rf24l01_backend.set_up_ce_pin( 0 );

  clear_bits( CONFIG_RG, PRIM_RX );
  n_rf24l01_backend.usleep( 140 );
}

/**
 * @brief configure n_rf24l01 to be a receiver
 */
//======================================================================================================
void n_rf24l01_prepare_to_receive( void )
{
  set_bits( CONFIG_RG, PRIM_RX );

  n_rf24l01_backend.set_up_ce_pin( 1 );
  n_rf24l01_backend.usleep( 140 );
}

/**
 * @brief call this function before start work with library
 *
 * @param[in] - pointer to structure which is storage for callbacks
 * @return -1, if failed
 */
//======================================================================================================
int n_rf24l01_init( const n_rf24l01_backend_t* n_rf24l01_backend_local )
{
  if( !n_rf24l01_backend_local )
    return -1;

  // get copy of callback set
  memcpy( &n_rf24l01_backend, n_rf24l01_backend_local, sizeof( n_rf24l01_backend ) );

  // disable acknowledge for all channels
  write_register( EN_AA_RG, 0x00 );

  // turn on n_rf24l01 transceiver
  set_bits( CONFIG_RG, PWR_UP );
  n_rf24l01_backend.usleep( 1500 );

  // set data field size (we will transmit PKG_SIZE bytes for time)
  write_register( RX_PW_P0_RG, PKG_SIZE );

  // set the lowermost transmit power
  clear_bits( RF_SETUP_RG, 0x06 );

  return 0;
}
