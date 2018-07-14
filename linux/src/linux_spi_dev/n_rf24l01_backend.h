/*
 * n_rf24l01_backend.h
 *
 *  Created on: Jul 4, 2018
 *      Author: sergs
 */

#ifndef N_RF24L01_BACKEND_H
#define N_RF24L01_BACKEND_H

#include "n_rf24l01_core.h"

int init_n_rf24l01_backend();
void deinit_n_rf24l01_backend();
int get_n_rf24l01_interrupt_line_fd();

/* cbs provided by this backend */
void set_up_ce_pin( u_char value );
void send_cmd( u_char cmd, u_char* status_reg, u_char* data, u_char num, u_char direction );
void usleep_( u_int delay_mks );

#endif /* N_RF24L01_BACKEND_H */
