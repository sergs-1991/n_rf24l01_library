/*
 * n_rf24l01.cpp
 *
 *  Created on: Jun 26, 2018
 *      Author: sergs (ivan0ivanov0@mail.ru)
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "n_rf24l01_core.h"
#include "n_rf24l01_linux.h"
#include "n_rf24l01_backend.h"


typedef struct
{
  /* [0] is going to be used by a user
   * [1] is going to be used by a library (wrapper) */
  int sockets_pair[2];

  /* Linux SYSFS GPIO provides a way to work with GPIO lines
   * as with files */
  int interrupt_line_fd;

  pthread_t n_rf_thread;
} n_rf24l01_t;


static n_rf24l01_t n_rf24l01 = {{-1, -1}, -1};


static void _stop_n_rf24l01_library()
{
  close( n_rf24l01.sockets_pair[0] );
  close( n_rf24l01.sockets_pair[1] );

  pthread_cancel( n_rf24l01.n_rf_thread );

  deinit_n_rf24l01_backend();
}

static void _data_from_user()
{
  char buff[256];
  int ret;

  ret = read( n_rf24l01.sockets_pair[1], buff, sizeof(buff) );
  if( ret < 0 )
    return;

  printf( "some data from user.\n" );

  n_rf24l01_prepare_to_transmit();
  n_rf24l01_transmit_pkgs( buff, ret );
  n_rf24l01_prepare_to_receive();
}

/* gets called if the library's core (and the transceiver) received some data from a remote side */
static void _handle_received_data( const void* data, u_int num )
{
  int count_to_write, current_offset;
  int ret;

  count_to_write = num;
  current_offset = 0;

  while( 1 )
  {
    /* try to write up to count_to_write bytes  */
    ret = write( n_rf24l01.sockets_pair[1], data + current_offset, count_to_write );

    if( ret < 0 && errno == EINTR )
      continue;

    /* keep on till all data is written */
    if( ret < 0 && errno == EWOULDBLOCK )
      continue;

    if( ret < 0 )
    {
      char error_buf[256];
      int err = errno;

      strerror_r( err, error_buf, sizeof(error_buf) );

      printf( "_handle_received_data: fail to write to a socket: %s.\n", error_buf );
      return;
    }

    count_to_write -= ret;
    current_offset += ret;

    /* no more data to write */
    if( count_to_write == 0 )
      break;
  }
}

static void _interrupt_on_n_rf24l01_device()
{
  static int first_interrupt = 1;
  char buff[1]; /* "value" ... reads as either 0 (low) or 1 (high). */

  /* Linux SYSFS GPIO API requires such operations to be performed;
   * Note: actually we don't need to know the exact value on an
   *       interrupt line, only the fact that an interrupt happened */
  lseek( n_rf24l01.interrupt_line_fd, 0, SEEK_SET );
  read( n_rf24l01.interrupt_line_fd, buff, sizeof(buff) );

  printf( "got an interrupt on an n_rf24l01 device.\n" );

  /* for some reason there's a fake interrupt at the beginning,
   * so just skip it */
  if( first_interrupt )
  {
    first_interrupt = 0;
    return;
  }

  /* let library's core to do it work */
  n_rf24l01_upper_half_irq();
  n_rf24l01_bottom_half_irq();
}

static void* _n_rf_thread( void* data )
{
  struct pollfd events_fd[2];

  events_fd[0].events = POLLIN;
  events_fd[0].fd = n_rf24l01.sockets_pair[1];

  /* Linux SYSFS GPIO API requires to set POLLPRI and POLLERR
   * as events to wait for */
  events_fd[1].events = POLLPRI | POLLERR;
  events_fd[1].fd = n_rf24l01.interrupt_line_fd;

  printf( "_n_rf_thread: wait for events...\n" );

  while( 1 )
  {
    int ret;

    ret = poll( events_fd, sizeof(events_fd) / sizeof(struct pollfd), -1 );
    if( ret < 0 && errno == EINTR )
      continue;

    if( ret < 0 )
    {
      _stop_n_rf24l01_library();
      return NULL;  /* implicitly call ptread_exit( NULL ) */
    }

    if( events_fd[0].revents == POLLIN )
      _data_from_user();

    /* it's not enough clear what type of event Linux SYSFS GPIO provides in case of
     * an interrupt on a line, so handle only a POLLPRI | POLLERR combination */
    if( events_fd[1].revents == (POLLPRI | POLLERR) )
      _interrupt_on_n_rf24l01_device();
  }
}

static int _init_n_rf24l01_backend( void )
{
  n_rf24l01_backend_t backend;
  int ret;

  ret = init_n_rf24l01_backend();
  if( ret < 0 )
    return -1;

  backend.set_up_ce_pin = set_up_ce_pin;
  backend.send_cmd = send_cmd;
  backend.usleep = usleep_;
  backend.handle_received_data = _handle_received_data;

  ret = n_rf24l01_init( &backend );
  if( ret < 0 )
    return -1;

  /* by default a transceiver is in a receive mode,
   * waiting for incoming data */
  n_rf24l01_prepare_to_receive();

  return 0;
}


/* Public API */


int n_rf24l01_open( void )
{
  int ret;

  ret = _init_n_rf24l01_backend();
  if( ret < 0 )
    return -1;

  n_rf24l01.interrupt_line_fd = get_n_rf24l01_interrupt_line_fd();
  if( n_rf24l01.interrupt_line_fd < 0 )
  {
    _stop_n_rf24l01_library();
    return -1;
  }

  printf( "an n_rf24l01 backend was successfully prepared to use.\n" );

  ret = socketpair( AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, n_rf24l01.sockets_pair );
  if( ret < 0 )
  {
    _stop_n_rf24l01_library();
    return -1;
  }

  ret = pthread_create( &n_rf24l01.n_rf_thread, NULL, _n_rf_thread, NULL );
  if( ret < 0 )
  {
    _stop_n_rf24l01_library();
    return -1;
  }

  /* return NO duplicate to be able to somehow notice a user that we have some problem
   * (in case of a some insoluble error we just close the sockets) */
  return n_rf24l01.sockets_pair[0];
}

void n_rf24l01_close( int fd )
{
  _stop_n_rf24l01_library();
}

int n_rf24l01_open_dbg( void )
{
  n_rf24l01_backend_t backend;
  int ret;

  memset( &backend, 0, sizeof(backend) );

  ret = init_n_rf24l01_backend();
  if( ret < 0 )
    return -1;

  backend.send_cmd = send_cmd;

  return n_rf24l01_init_dbg( &backend );
}
