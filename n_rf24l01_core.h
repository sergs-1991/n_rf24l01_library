#ifndef N_RF24L01_PORT_H
#define N_RF24L01_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* This is API of n_rf24l01 library's core. Generally this API isn't
 * used directly by a user. The user instead has to use some wrapper/backend
 * to work this the n_rf24l01, like linux,...  */

/* you must set up spi and gpio peripherals before start use the library
 *
 * spi on n_rf24l01 works with next settings:
 *  CPOL = 0, CPHA = 0
 *  msbit first
 *  8 bits per word
 *  spi speed up to 10MHz
 *  CSN - is active low
 *  Note: CSN pin must be hold unchanged during command transaction
 *        (from sending cmd to sending/receiving cmd's data)
 *
 * CE - (output pin)
 * IRQ - is active low (input pin) */

typedef unsigned char u_char;
typedef unsigned int u_int;

typedef void (*set_up_ce_pin_ptr)( u_char value );
typedef void (*send_cmd_ptr)( u_char cmd, u_char* status_reg, u_char* data, u_char num, u_char direction );
typedef void (*usleep_ptr)( u_int delay_mks );
typedef void (*handle_received_data_ptr)( void* data, u_int num );


/**
 * @brief This structure describes library's callbacks
 *
 * Note: you must implement these callback functions to proper work of the library
 */
typedef struct n_rf24l01_backend_t
{
  /**
   * @brief control CE pin state
   *
   * void (*set_up_ce_pin_ptr)( u_char value );
   *
   * @param[in] value - a new state of CE pin (0 - '0' logical level, 1 - '1' logical level)
   */
  set_up_ce_pin_ptr set_up_ce_pin;

  /**
   * @brief send a command to n_rf24l01 over SPI peripheral
   *
   * void (*send_cmd_ptr)( u_char cmd, u_char* status_reg, u_char* data, u_char num, u_char direction );
   *
   * @param[in]     cmd        - a command to send
   * @param[out]    status_reg - a pointer an n_rf24l01 status register will be written to
   * @param[in,out] data       - a pointer to data to be written to or to be read from n_rf24l01, depends on @type,
   *                             a real amount of data to read from/write to is depends on @num parameter
   * @param[in]     num        - an amount of bytes to read or write (except a command byte)
   * @param[in]     direction  - a type of an operation: 1 - write, 0 - read
   * @return -1 if failed, 0 otherwise
   *
   * Note: this function must implement sending command and sending/receiving command's data over spi.
   *       First you must send a @cmd, retrieve an answer and store it to a @status_reg, and then send @num bytes from @data,
   *       or receive @num bytes and store to @data depend of a @direction value.
   *       @status_reg may be NULL - just throw away a respond of first spi transaction (@cmd).
   *       @num may be 0 - in this case just send a @cmd and store a respond to @status_reg, if @status_reg isn't NULL.
   *       Memory allocation occurred on calling side.
   */
  send_cmd_ptr send_cmd;

  /**
   * @brief put to sleep a library execution flow, max sleep interval ~1500 ms
   *
   * void (*usleep_ptr)( u_int delay_mks );
   *
   * @param[in] delay_mks - time in microseconds to sleep
   *
   * Note: you can on you own decide how to put to sleep a library execution flow,
   *       for example: short sleep interval - just a 'for/while' loop,
   *                    long - something else.
   *       library execution flow mustn't obtain a control due @delay_mks :-) K.O.
   */
  usleep_ptr usleep;

  /**
   * @brief handle received data
   *
   * void (*handle_received_data_ptr)( void* data, u_int num );
   *
   * @param[in] data - data received from n_rf24l01
   * @param[in] num  - an amount of received data, in bytes
   *
   */
  handle_received_data_ptr handle_received_data;

} n_rf24l01_backend_t;

// -------------------------------------- IRQ handler --------------------------------------------------

/**
 * @brief an upper half of the n_rf24l01 irq handler
 *
 * Note: it's desirable to call this function from the hardware interrupt context
 */
//======================================================================================================
void n_rf24l01_upper_half_irq( void );

/**
 * @brief a bottom half of the n_rf24l01 irq handler
 *
 * Note: bottom half mustn't be executed in hardware interrupt context, due to a big execution time
 */
//======================================================================================================
void n_rf24l01_bottom_half_irq( void );


// ----------------------------------------- API -------------------------------------------------------


/**
 * @brief configure the library and the transceiver
 *
 * @param[in] - a pointer to a structure with callbacks
 * @return -1, if failed
 *
 * Note: call this function before start the work with the library
 */
//======================================================================================================
int n_rf24l01_init( const n_rf24l01_backend_t* n_rf24l01_backend );

/**
 * @brief configure the n_rf24l01 to be a transmitter
 */
//======================================================================================================
void n_rf24l01_prepare_to_transmit( void );

/**
 * @brief configure the n_rf24l01 to be a receiver
 */
//======================================================================================================
void n_rf24l01_prepare_to_receive( void );

/**
 * @brief transmit packages through the n_rf24l01 transceiver
 *
 * @param[in]  data - a package's data to transmit
 * @param[num] num  - an amount of data to transmit, in bytes
 *
 * Note: this is block call (until all data are transmitted)
 */
//======================================================================================================
void n_rf24l01_transmit_pkgs( const void* data, u_int num );

#ifdef __cplusplus
}
#endif

#endif // N_RF24L01_PORT_H
