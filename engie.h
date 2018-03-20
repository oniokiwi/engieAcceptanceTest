/*
 * Copyright © kiwipower 2017
 *
 * Header file to simulate battery charge/discharge profile
 */
#ifndef NEC_DOT_H
#define NEC_DOT_H

#include <stdint.h>
#include <modbus/modbus.h>
#include "typedefs.h"

#define StateOfCharge      1
#define PowerToDeliver     2

void  init(init_param_t* );
void  dispose();
int   process_read_register(uint16_t address, uint16_t data);
int   process_write_register(uint16_t address, uint16_t data);


#endif