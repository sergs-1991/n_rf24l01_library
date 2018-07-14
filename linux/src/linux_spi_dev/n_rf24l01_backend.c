/*
 * n_rf24l01_backend.cpp
 *
 *  Created on: Jun 26, 2018
 *      Author: sergs (ivan0ivanov0@mail.ru)
 */

/*
 * This file contains callback functions to work with n_rf24l01 transceiver via /dev/spidevA.B device file

 * n_rf24l01 commands have 8 bits.
 * Every new command must be started by a high to low transition on CSN.
 * In parallel to the SPI command word applied on the MOSI pin, the STATUS register is shifted serially out on
 * the MISO pin.

 * The serial shifting SPI commands is in the following format:
 *  <Command word: MSBit to LSBit (one byte)>
 *  <Data bytes: LSByte to MSByte, MSBit in each byte first>

 * spi on n_rf24l01 works with next settings:
 *  CPOL = 0, CPHA = 0
 *  msbit first
 *  8 bits per word
 *  spi speed up to 8MHz (but now we use only 50kHz)
 *  CSN - is active low
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "n_rf24l01_backend.h"


#define stringizer_(NAME) #NAME
#define _(NAME) stringizer_(NAME)

typedef struct
{
  /* an spidev device file fd */
  int spi_fd;

  /* Linux SYSFS GPIO provides a way to work with GPIO lines
   * as with files */
  int interrupt_line_fd;
  int ce_line_fd;
} n_rf24l01__backend_t;


static n_rf24l01__backend_t n_rf24l01_backend = {-1, -1, -1};

static const char* interrupt_line_path = "/sys/class/gpio/gpio"_(INTERRUPT_LINE_PIN_NUM)"/value";
static const char* ce_line_path = "/sys/class/gpio/gpio"_(CE_LINE_PIN_NUM)"/value";


static int _init_pins( void )
{
  n_rf24l01_backend.interrupt_line_fd = open( interrupt_line_path, O_RDONLY | O_CLOEXEC );
  if( n_rf24l01_backend.interrupt_line_fd < 0 )
    return -1;

  n_rf24l01_backend.ce_line_fd = open( ce_line_path, O_WRONLY | O_CLOEXEC );
  if( n_rf24l01_backend.ce_line_fd < 0 )
    return -1;

	return 0;
}

/* set up the spi master to correct settings
 * the spi on n_rf24l01 works with next settings:
 *  CPOL = 0, CPHA = 0
 *  msbit first
 *  8 bits per word
 *  spi speed up to 8MHz (but now we use only 50kHz) */
static int _setup_master_spi( void )
{
	int ret;
	__u8 mode;
	__u8 bits_order;
	__u8 bits_per_word;
	__u32 speed_hz;

	mode = SPI_MODE_0; /* CPOL = 0, CPHA = 0 */
	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_WR_MODE, &mode );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_WR_MODE ioctl call" );
		return -1;
	}

	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_RD_MODE, &mode );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_RD_MODE ioctl call" );
		return -1;
	}

	bits_order = 0; /* msbit first */
	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_WR_LSB_FIRST, &bits_order );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_WR_LSB_FIRST ioctl call" );
		return -1;
	}

	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_RD_LSB_FIRST, &bits_order );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_RD_LSB_FIRST ioctl call" );
		return -1;
	}

	bits_per_word = 0; /* 8 bit per word */
	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_WR_BITS_PER_WORD ioctl call" );
		return -1;
	}

	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_RD_BITS_PER_WORD ioctl call" );
		return -1;
	}

	speed_hz = 500000; /* 500 kHz */
	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_WR_MAX_SPEED_HZ ioctl call" );
		return -1;
	}

	ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz );
	if( ret < 0 )
	{
		perror( "error while SPI_IOC_RD_MAX_SPEED_HZ ioctl call" );
		return -1;
	}

	printf( "spi mode: SPI_MODE_%hhu.\n", mode );
	printf( "spi bits order: %s.\n", bits_order ? "lsbit first" : "msbit first" );
	if (!bits_per_word)
		printf( "spi bits per word: 8.\n" );
	else
		printf( "spi bits per word: %hhu.\n", bits_per_word );
	printf( "spi speed: %uHz.\n\n", speed_hz );

	return 0;
}

static void _stop_n_rf24l01_backend()
{
  close( n_rf24l01_backend.spi_fd );
  close( n_rf24l01_backend.interrupt_line_fd );
  close( n_rf24l01_backend.ce_line_fd );
}


/* Public API */


/* make all initialization steps to prepare an n_rf24l01 device to work */
int init_n_rf24l01_backend()
{
  int ret;

  ret = _init_pins();
  if( ret < 0 )
  {
    _stop_n_rf24l01_backend();
    return -1;
  }

  printf( "n_rf24l01_backend: pins were successfully prepared to use.\n" );

  n_rf24l01_backend.spi_fd = open( SPI_DEVICE_FILE, O_RDWR );
  if( n_rf24l01_backend.spi_fd < 0 )
  {
    char temp[128];

    snprintf( temp, sizeof temp, "error while open spidev device file: %s", SPI_DEVICE_FILE );
    perror( temp );

    return -1;
  }

  ret = _setup_master_spi();
  if( ret < 0 )
  {
    _stop_n_rf24l01_backend();
    return -1;
  }

  printf( "n_rf24l01_backend: spi master was successfully prepared to use.\n" );
  return 0;
}

void deinit_n_rf24l01_backend()
{
  _stop_n_rf24l01_backend();
}

int get_n_rf24l01_interrupt_line_fd()
{
  /* no duplication, 'cause a backend and a wrapper are part of one thing - the library */
  return n_rf24l01_backend.interrupt_line_fd;
}


/* Backend's call-backs */


void set_up_ce_pin( u_char value )
{
  char str[2];  /* snprintf appends the string by a '\0' symbol */
  int ret;

  /* write either 1 or 0 to construst either '0' or '1' */
  snprintf( str, sizeof str, "%u", !!value );

  ret = write( n_rf24l01_backend.ce_line_fd, str, 1 );
  if( ret < 0 || ret != 1 )
    printf( "error while set_up_pin call.\n" );

  return;
}

void send_cmd( u_char cmd, u_char* status_reg, u_char* data, u_char num, u_char direction )
{
  struct spi_ioc_transfer transfers[2];
  int ret;

  /* @data can be passed as NULL if only an n_rf24l01 status register is going to be read */
  if( num && !data ) return;

  memset( transfers, 0, sizeof(transfers) );

  /* a transaction to send command */
  transfers[0].tx_buf = (uintptr_t)&cmd;
  transfers[0].rx_buf = (uintptr_t)status_reg;
  transfers[0].len = 1;

  /* a transaction to write/read command's data */
  if( direction ) /* if a client wants to write some data */
  {
      transfers[1].tx_buf = (uintptr_t)data;
      transfers[1].rx_buf = (uintptr_t)NULL;
  }
  else
  {
      transfers[1].tx_buf = (uintptr_t)NULL; /* zeroes will be shifted */
      transfers[1].rx_buf = (uintptr_t)data;
  }

  transfers[1].len = num;

  /* ask to do actually spi fullduplex transactions */
  if( !num )
    ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_MESSAGE(1), transfers );
  else
    ret = ioctl( n_rf24l01_backend.spi_fd, SPI_IOC_MESSAGE(2), transfers );

  if( ret < 0 )
      perror( "error while SPI_IOC_MESSAGE ioctl" );
}

void usleep_( u_int delay_mks )
{
  usleep( delay_mks );
}

