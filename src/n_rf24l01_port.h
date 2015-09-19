#ifndef N_RF24L01_PORT_H
#define N_RF24L01_PORT_H

// you must set up spi and gpio peripherals before start use library

// spi on n_rf24l01 works with next settings:
//  CPOL = 0, CPHA = 0
//  msbit first
//  8 bits per word
//  spi speed up to 8MHz
//  CSN - is active low
//  Note: CSN pin must be hold unchanged during command transaction
//        (from sending cmd to sending/receiving cmd's data)

// CE - (output pin)
// IRQ - is active low (input pin)

typedef unsigned char u_char;
typedef unsigned int u_int;

// this function sets/clears CE pin
// value - value to set onto pin
typedef void (*set_up_ce_pin_ptr)( u_char value );

// send a command to n_rf24l01
// cmd - command to send
// status_reg - pointer n_rf24l01 status register will be written to
// data - pointer to data to be written to or to be read from n_rf24l01, depends on @type,
//		 real amount of data to read from/write to is depends on @num parameter
// num - amount of bytes to read or write (except command byte) (max amount is COMMAND_DATA_SIZE)
// type - type of operation: 1 - write, 0 - read
// return -1 if failed, 0 otherwise
typedef void (*send_cmd_ptr)( u_char cmd, u_char* status_reg, u_char* data, u_char num, u_char type );

// this function put to sleep library execution flow, max sleep interval ~1500 ms
typedef void (*usleep_ptr)( u_int ms );

// you must provide these callback functions to proper work of library
typedef struct n_rf24l01_backend_t
{
  // this function controls CE pin behavior, 0 - set logical '0'
  set_up_ce_pin_ptr set_up_ce_pin;

  // this function must implements sending command and sending/receiving command's data over spi
  // first you must send @cmd, retrieve answer and store it to @status_reg, and then send/receive @num bytes from
  // @data depend of @type value.
  send_cmd_ptr send_cmd;

  // you can on you own decide how to put to sleep library execution flow,
  // for example: short sleep interval - just 'for/while' loop,
  //              long - something else
  usleep_ptr usleep;

} n_rf24l01_backend_t;

// upper and bottom halfs of n_rf24l01 irq handler
// upper half can be executed in hardware interrupt context
// bottom half mustn't be executed in hardware interrupt context, due to a big execution time
extern int n_rf24l01_upper_half_irq( void );
extern int n_rf24l01_bottom_half_irq( void );

// API:

// call this function before start work with library
// returns -1, if failed
extern int n_rf24l01_init( const n_rf24l01_backend_t* n_rf24l01_backend );

// call these functions before transmit/receive operations
extern void prepare_to_transmit( void );
extern void prepare_to_receive( void );

// transmit, over n_rf24l01 transceiver one byte @byte to space
extern void n_rf24l01_transmit_byte( u_char byte );

#endif // N_RF24L01_PORT_H
