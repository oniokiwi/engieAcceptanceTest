/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <getopt.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include "typedefs.h"
#include "engie.h"

const uint16_t UT_REGISTERS_NB = 65535;
static uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

#define MODBUS_DEFAULT_PORT 1502

static init_param_t param;
static uint16_t *address;
static uint16_t address_offset;

static void usage(const char *app_name)
{
    printf("Usage:\n");
    printf("%s [option <value>] ...\n", app_name);
    printf("\nOptions:\n");
    printf(" -p \t\t # Set Modbus port to listen on for incoming requests (Default 1502)\n");
    printf(" -? \t\t # Print this help menu\n");
    printf("\nExamples:\n");
    printf("%s -p 1502  \t # Change the listen port to 1502\n", app_name);
    exit(1);
}

static void modbus_mem_init()
{
    param.modbus_mapping = modbus_mapping_new_start_address(
       0, 0,
       0, 0,
       0, UT_REGISTERS_NB,
       0, 0);

    if (param.modbus_mapping == NULL)
    {
        printf("Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        exit(1); // all bets are off
    }
    address_offset = param.modbus_mapping->start_registers + enableDebugTrace;
    address = param.modbus_mapping->tab_registers + address_offset;
    *address = FALSE;
}

static void scan_options(int argc, char* argv[])
{
    int opt;

    // set some default options
    param.port = MODBUS_DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            param.port = atoi(optarg);
            break;

            break;

        default:
            usage(*argv);
        }
    }
}

int main(int argc, char* argv[])
{
    void query_handler(modbus_pdu_t* mb);
    int rc, s = -1;
    bool done = FALSE;

    modbus_mem_init();
    scan_options(argc, argv);
    init(&param);

    printf("staring Engie acceptance test - listening on Port %d\n", param.port);

    for (;;)
    {
        param.ctx = modbus_new_tcp(NULL, param.port);
        if ( param.ctx == NULL )
        {
            printf("Failed creating modbus context\n");
            return -1;
        }
        address_offset = param.modbus_mapping->start_registers + enableDebugTrace;
        address = param.modbus_mapping->tab_registers + address_offset;
        modbus_set_debug(param.ctx, *address);
        s = modbus_tcp_listen(param.ctx, 1);
        modbus_tcp_accept(param.ctx, &s);
        done = FALSE;
        while (!done)
        {
            rc = modbus_receive(param.ctx, query);
            switch (rc)
            {
            case -1:
            	disconnect();
                close(s); // close the socket
                modbus_close(param.ctx);
                modbus_free(param.ctx);
                param.ctx = NULL;
                done = TRUE;
                break;

            case 0:
                // No data received
                break;

            default:
                query_handler((modbus_pdu_t*) query);
                break;
            }
        }
    } // for (;;)
    dispose();
    return 0;
}

/*
***************************************************************************************************************
 \fn      tesla_query_handler(modbus_pdu_t* mb)
 \brief   processess all incoming commands

 Process all input commands. The Modbus function code 0x17 which is not standard seems to exhibit non standaard
 data structure seen not belows.

 \note

      MODBUS_FC_READ_HOLDING_REGISTERS
      MODBUS_FC_WRITE_SINGLE_REGISTER - has the following data format
      ------------------------------------------------
      | TID | PID | LEN | UID | FC | [W|R]S | [W|R]Q |
      ------------------------------------------------
      0     1     3     5     7    8        11       13

      MODBUS_FC_WRITE_MULTIPLE_REGISTERS - has the following data format operation
      -------------------------------------------------------
      | TID | PID | LEN | UID | FC | WS | WQ | WC | WR x nn |
      -------------------------------------------------------
      0     1     3     5     7    8    11   13   14

      MODBUS_FC_WRITE_AND_READ_REGISTERS - has the following data format
      -----------------------------------------------------------------
      | TID | PID | LEN | UID | FC | RS | RQ | WS | WQ | WC | WR x nn |
      -----------------------------------------------------------------
      0     1     3     5     7    8    11   13   15   17   18

      TID = Transaction Id, PID = Protocol Id, LEN = Total length of message, UID = unit Id
      FC = Function Code, RS = Read start address, RQ = Read quantity, WS = Write start address,
      WQ = Write quantity, WC = Write count, WR = Write Register. nn => WQ x 2 bytes
**************************************************************************************************************
*/

void query_handler(modbus_pdu_t* mb)
{
    const int convert_bytes2word_value = 256;
    int i,j,retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    uint16_t address,value,count;
    int len = __bswap_16(mb->mbap.length) - 2; // len - fc - unit_id
    uint8_t fc;

   // for ( i = 0; i < len; i++ ) {
    fc = mb->fcode;
    switch ( fc ){
    case MODBUS_FC_READ_HOLDING_REGISTERS:
        //printf("%s MODBUS_FC_READ_HOLDING_REGISTERS\n", __PRETTY_FUNCTION__);
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        value   = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // data
        retval  = process_read_register(address, value);
        break;

    case MODBUS_FC_WRITE_SINGLE_REGISTER:
        //printf("%s MODBUS_FC_WRITE_SINGLE_REGISTER\n", __PRETTY_FUNCTION__);
        address = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // address
        value   = (mb->data[i++] * convert_bytes2word_value) + mb->data[i++]; // data
        retval  = process_write_register(address, value);
        break;

    default:
        retval = MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
        break;
        }
   // }
    if ( retval == MODBUS_SUCCESS)
    {
        modbus_reply(param.ctx, (uint8_t*)mb, sizeof(mbap_header_t) + sizeof(fc) + len, param.modbus_mapping); // subtract function code
    }
    else
    {
       modbus_reply_exception(param.ctx, (uint8_t*)mb, retval);
    }
}



