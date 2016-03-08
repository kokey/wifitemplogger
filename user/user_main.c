/* user_main.c

wifi temperature logger
Kriek Jooste/kokeycode@speakopen.org/17.02.2016

early experiment with the ESP8266

*/
 

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "wifi.h"
#include "httpclient.h"
#include "user_config.h"
#include "ds18b20.h"

/* stuff to keep ssl library happy */
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
#define DELAY 60000 /* milliseconds */

os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;
static volatile os_timer_t WiFiLinker;
static volatile os_timer_t ds18b20_timer;

LOCAL void ICACHE_FLASH_ATTR ds18b20_cb(void *arg) {
	/* temperature reading callback */
	ds18b20();
}


int ICACHE_FLASH_ATTR ds18b20()
{
	int r, i;
	uint8_t addr[8], data[12];
	
	ds_init();

	r = ds_search(addr);
	if(r)
	{
		os_printf("Found Device @ %02x %02x %02x %02x %02x %02x %02x %02x\r\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
		if(crc8(addr, 7) != addr[7])
			os_printf( "CRC mismatch, crc=%xd, addr[7]=%xd\r\n", crc8(addr, 7), addr[7]);

		switch(addr[0])
		{
		case 0x10:
			os_printf("Device is DS18S20 family.\r\n");
			break;

		case 0x28:
			os_printf("Device is DS18B20 family.\r\n");
			break;

		default:
			os_printf("Device is unknown family.\r\n");
			return 1;
		}
	}
	else { 
		os_printf("No DS18B20 detected, sorry.\r\n");
		return 1;
	}
	// perform the conversion
	reset();
	select(addr);

	write(DS1820_CONVERT_T, 1); // perform temperature conversion

	os_delay_us(100000);

	os_printf("Scratchpad: ");
	reset();
	select(addr);
	write(DS1820_READ_SCRATCHPAD, 0); // read scratchpad
	
	for(i = 0; i < 9; i++)
	{
		data[i] = read();
		os_printf("%2x ", data[i]);
	}
	os_printf("\r\n");

	int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
		TReading = (TReading ^ 0xffff) + 1; // 2's comp
	
	Whole = TReading >> 4;  // separate off the whole and fractional portions
	Fract = (TReading & 0xf) * 100 / 16;

	char temperaturestr[80];

	os_sprintf(temperaturestr, "%c%d.%d", SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract);
	os_printf("Temperature: %s Celsius\r\n", temperaturestr);

	char submiturl[64] = SUBMITURL; /* e.g. http://blah.com/temperature?level= */
	char webgetstr[80];
	os_sprintf(webgetstr, "%s%s", submiturl, temperaturestr);

	if (wifi_station_get_connect_status() == 5) {
		// 5 = STATION_GOT_IP I think

		if (Whole != 85) {
			os_printf("sending temperature to web server\n\r");
			http_get(webgetstr, "", http_callback_example);
		}
	}

	return r;
}
void some_timerfunc(void *arg) {
    // blink LED on GPIO2
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
    {
        //Set GPIO2 to LOW
	 os_printf("setting low\n\r");
        gpio_output_set(0, BIT2, BIT2, 0);
    }
    else
    {
        //Set GPIO2 to HIGH
	 os_printf("setting high\n\r");
        gpio_output_set(BIT2, 0, BIT2, 0);
    }
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
    os_delay_us(10);
}

void wifi_wait_and_send(void *arg) {

    os_timer_disarm(&WiFiLinker);

    if (wifi_station_get_connect_status() == 5) {
		// 5 = STATION_GOT_IP I think

		os_printf("SUCCESS: got IP address\n\r");
	} else {
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_wait_and_send, NULL);
		os_timer_arm(&WiFiLinker, 1000, 0);
		os_printf("waiting for wifi connection\n\r");
	}
}

//Init function 
void ICACHE_FLASH_ATTR user_init() {

	uart_div_modify(0, UART_CLK_FREQ / 115200);

	os_delay_us(100000);
	os_printf("\n\r\n\rstartup\n\r");

	// Initialize the GPIO subsystem.
	gpio_init();

	//Set GPIO2 to output mode
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	//Set GPIO2 low
	gpio_output_set(0, BIT2, BIT2, 0);

	 os_printf("gpio setup done\n\r");


	// setup the SSID and SSID Password
	char ssid[32] = SSID;
	char password[64] = SSID_PASS;  
	connect_to_wifi(SSID, SSID_PASS);


	 os_printf("wifi connection initiated\n\r");


    os_timer_disarm(&WiFiLinker);
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_wait_and_send, NULL);
    os_timer_arm(&WiFiLinker, 1000, 0);

/* this was LED flashing code, but the LED was on the gpio
   the temperature sensor is now connected to

	os_printf("now try to make the LED blink....\n\r");

	//Disarm timer
	os_timer_disarm(&some_timer);

	//Setup timer
	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

	//Arm the timer
	//&some_timer is the pointer
	//1000 is the fire time in ms
	//0 for once and 1 for repeating
	os_timer_arm(&some_timer, 1000, 1);

*/
	
	os_printf("now try to kick off reading the temperature sensor....\n\r");
	os_timer_disarm(&ds18b20_timer);
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)ds18b20_cb, (void *)0);
	os_timer_arm(&ds18b20_timer, DELAY, 1);

	//Start os task
	system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

}

void http_callback_stuff(char * response, int http_status, char * full_response)
{
    os_printf("http_status=%d\n", http_status);
    if (http_status != HTTP_STATUS_GENERIC_ERROR) {
        os_printf("strlen(full_response)=%d\n", strlen(full_response));
        os_printf("response=%s<EOF>\n", response);
    }
}
