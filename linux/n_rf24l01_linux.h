#ifndef N_RF24L01_H
#define N_RF24L01_H

#ifdef __cplusplus
extern "C" {
#endif

int n_rf24l01_open( void );
// int n_rf24l01_setup( int fd, ... );

/* for internal reasons, a close() system call may be not
 * enough to deinitialize the library */
void n_rf24l01_close( int fd );

/* this function may return -1, if a debug support isn't stipulated,
 * otherwise both
 *   uint64_t n_rf24l01_read_register_dbg( u_char reg_addr );
 *   void n_rf24l01_write_register_dbg( u_char reg_addr, uint64_t value );
 * functions can be used to read/write n_rf24l01 registers
 * */
int n_rf24l01_open_dbg( void );

#ifdef __cplusplus
}
#endif

#endif // N_RF24L01_H
