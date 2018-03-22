#include <stdio.h>
#include <stdlib.h>
#include "engie.h"
#include "typedefs.h"
#include <unistd.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#define BATTERY_POWER_RATING            230           // kW
#define TIME_CHARGE_FROM_0_TO_100       3000          // seconds
#define TIME_DISCHARGE_FROM_100_TO_0    2800          // seconds

// Private data
static modbus_t* ctx;
static modbus_mapping_t *mb_mapping;
static uint16_t debug = 0;
static init_param_t *param;

static pthread_t thread1;
static uint8_t terminate1;

static float battery_discharge_decrement = 0.0;
static float battery_charge_increment = 0.0;
static bool battery_charging = false;
static bool battery_discharging = false;
static float state_of_charge;
static uint16_t power_to_deliver;

static const uint16_t sign_bit_mask             = 0x8000;
static const float state_of_charge_default      = 50.00;
static const float battery_charge_resolution    = 100.00 / (BATTERY_POWER_RATING * TIME_CHARGE_FROM_0_TO_100);     // % increase in charge per sec
static const float battery_discharge_resolution = 100.00 / (BATTERY_POWER_RATING * TIME_DISCHARGE_FROM_100_TO_0);  // % decrease in charge per sec
static const float battery_fully_charged        = 100.00;
static const float battery_fully_discharged     = 0.0;

// private functions
static int _enableDebugTrace(uint16_t);
static int _stateOfCharge ();
static int _readPowerToDeliver();
static int _writePowerToDeliver(uint16_t);
static void* _thread_handler( void *ptr );


int process_write_register(uint16_t address, uint16_t data)
{
    int retval;

    switch ( address )
    {
        case enableDebugTrace: // Not a modbus register
            retval = _enableDebugTrace(data);
            break;

        case PowerToDeliver:
            retval = _writePowerToDeliver(data);
            break;

        default:
            retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
            break;

    }
    return retval;
}

int process_read_register(uint16_t address, uint16_t data)
{
    int retval;

    switch ( address )
    {
        case StateOfCharge:
            retval = _stateOfCharge();
            break;

        case PowerToDeliver:
            retval = _readPowerToDeliver();
            break;

        default:
            retval = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
            break;

    }
    return retval;
}

int _enableDebugTrace(uint16_t value)
{
    debug =  value & 0x0001;
    uint16_t *address;
    uint16_t address_offset;

    printf("%s - %s\n", __PRETTY_FUNCTION__, debug?"TRUE":"FALSE");
    address_offset = mb_mapping->start_registers + enableDebugTrace;
    address = mb_mapping->tab_registers + address_offset;
    if ( address < (mb_mapping->tab_registers + mb_mapping-> nb_registers) )
    {
       *address = debug;
    }
    return MODBUS_SUCCESS;
}

int _stateOfCharge ()
{
    uint16_t *address;
    uint16_t address_offset;

    if (debug) printf("%s\n", __PRETTY_FUNCTION__);
    address_offset = mb_mapping->start_registers + StateOfCharge;
    address = mb_mapping->tab_registers + address_offset;
    if ( address < (mb_mapping->tab_registers + mb_mapping-> nb_registers) )
    {
       *address = (state_of_charge * 10);
    }
    return MODBUS_SUCCESS;
}

int _readPowerToDeliver()
{
    uint16_t *address;
    uint16_t address_offset;

    if (debug) printf("%s\n", __PRETTY_FUNCTION__);
    address_offset = mb_mapping->start_registers + PowerToDeliver;
    address = mb_mapping->tab_registers + address_offset;
    if ( address < (mb_mapping->tab_registers + mb_mapping-> nb_registers) )
    {
       *address = power_to_deliver;
    }
    return MODBUS_SUCCESS;
}

int _writePowerToDeliver(uint16_t value)
{
	uint16_t val = value;

    if (debug) printf("%s\n", __PRETTY_FUNCTION__);

    power_to_deliver = val;
    if ( val & sign_bit_mask )
    {
        val = ((~val) + 1);                            // get 2nd complement value
        if (debug) printf("%s - battery charging val(-%d)\n", __PRETTY_FUNCTION__, val);
        battery_charging = true;
        battery_discharging = false;
        battery_charge_increment = ( val * battery_charge_resolution);  ;
    }
    else if (val > 0)
    {
        if (debug) printf("%s - battery discharging val(%d)\n", __PRETTY_FUNCTION__, val);
        battery_discharging = true;
        battery_charging = false;
        battery_discharge_decrement = (val * battery_discharge_resolution);
    }
    else
    {
        if (debug) printf("%s - not charging val(%d)\n", __PRETTY_FUNCTION__, val);
        battery_discharging = false;
        battery_charging = false;
    }

    return MODBUS_SUCCESS;
}


void init(init_param_t* init_param)
{
    thread_param_t* thread_param;
    setvbuf(stdout, NULL, _IONBF, 0);                          // disable stdout buffering
    mb_mapping = init_param->modbus_mapping;
    terminate1 = FALSE;
    thread_param = (thread_param_t*) malloc(sizeof (thread_param_t));
    thread_param -> terminate = &terminate1;
    pthread_create( &thread1, NULL, _thread_handler, thread_param);
}

void dispose()
{
    terminate1 = true;
    pthread_join(thread1, NULL);
}


//
// Thread handler
//
void *_thread_handler( void *ptr )
{
    uint8_t *terminate;
    thread_param_t* param = (thread_param_t*) ptr;
    ctx = param->ctx;
    terminate = param->terminate;
    free(param);
    state_of_charge = state_of_charge_default; // set to default state of charge value

    while ( *terminate == false )
    {
        sleep(1);
		if (battery_charging)
		{
			if ( (state_of_charge + battery_charge_increment) <= battery_fully_charged )
			{
				state_of_charge += battery_charge_increment;
			}
			else
			{
				state_of_charge = battery_fully_charged;
				battery_charging = false;
			}
		}
		else if (battery_discharging)
		{
			if ( (state_of_charge - battery_discharge_decrement) >= battery_fully_discharged )
			{
				state_of_charge -= battery_discharge_decrement;
			}
			else
			{
				state_of_charge = battery_fully_discharged;
				battery_discharging = false;
			}
		}
    }
}

