/*
 * Header files for typedefs
 *
*/
#ifndef TYPEDEFS_DOT_H
#define TYPEDEFS_DOT_H


#include <modbus/modbus.h>

//typedef enum {false, true} bool;

#define MODBUS_SUCCESS      0
#define enableDebugTrace    0

typedef struct init_param_struct
{
    uint8_t  *terminate;
    modbus_t *ctx;
    int   port;
    modbus_mapping_t *modbus_mapping;
    char powerToDeliverURL[128];                // powerToDeliverURL = ipaddress:port
    char submitReadingsURL[128];               // submitReadingsURL = ipaddress/endpoint
}init_param_t;

typedef struct thread_param_struct
{
    modbus_t *ctx;
    pthread_mutex_t* mutex;
    uint8_t *terminate;
}thread_param_t;


typedef struct mbap_header_struct
{
    uint16_t transport_id;
    uint16_t protocol_id;
    uint16_t length;
    uint8_t  unit_id;
}__attribute__((packed))mbap_header_t;

typedef struct modbus_pdu_struct
{
    mbap_header_t mbap;
    uint8_t  fcode;
    uint8_t  data[];
}__attribute__((packed))modbus_pdu_t;


#endif
