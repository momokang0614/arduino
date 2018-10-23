/**
*
* Thermostat
*
*/


//#define DEBUG

/** *****************************************
 *  Pins
 *
 */

// Pins
#define ONE_WIRE_PIN 0
#define COLD_PIN 3 // Analog
#define HOT_PIN 2 // Analog
#define ETHERNET_CS_PIN 10
#define BEAT_PIN A1 // Digital connected to Analog 1

// Relay
#define PUMP_PIN 3
#define ROOM_1_PIN 4
#define ROOM_2_PIN 5
#define ROOM_3_PIN 6
#define ROOM_4_PIN 7
#define ROOM_5_PIN 8

#ifdef DEBUG
#define LOADER_ADDRESS "http://localhost/~ale/thermoduino/loader.js"
#else
#define LOADER_ADDRESS "https://raw.github.com/elpaso/arduino-ethercard-thermostat/save-ram/loader.js"
#endif

#include "EtherCard.h"


#include "Timer.h"
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <OneWire.h>
#include <DallasTemperature.h>

/** ****************************************************
 * EEPROM map
 *
 * 0    ID  0x99
 * 1    T0  16 bit
 * 2    T1  16 bit
 * 4    T2  16 bit
 * 6    T3  16 bit
 * 8    slot 7 * 16 bit
 * 22   10 * 3 * 8 bit weekly_program
 * 52   8 * 4 * 8 bit daily_program
 * 84  rooms program 8 bit * 5
 *
 *
 */

#define EEPROM_ID = 153
#define EEPROM_T_BASE = 1
#define EEPROM_SLOT_BASE = 8
#define EEPROM_W_BASE = 22
#define EEPROM_D_BASE = 52
#define EEPROM_ROOMS_BASE = 84

#define HTTP_PORT 80
#define BUFFER_SIZE 500
#define STR_BUFFER_SIZE 32


//please modify the following two lines. mac and ip have to be unique
//in your local area network. You can not have the same numbers in
//two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
static uint8_t myip[4] = {192,168,99,123};


// Used here and there...
static char strbuf[STR_BUFFER_SIZE+1];



/** ****************************************************
*
* Main constants, all times in millis
*/

// Number of rooms


#define ROOMS 5

#define WEEKLY_PROGRAM_NUMBER 10
#define DAILY_PROGRAM_NUMBER 8
#define SLOT_NUMBER 8

// TODO: move to config

#ifdef DEBUG

#include <MemoryFree.h>

// Run FAST!!!!
#define TEMP_READ_INTERVAL 10000 // millis
#define VALVE_OPENING_TIME_S 10UL // 10 sec
#define BLOCKED_TIME_S 60UL // 1 minute
#define RISE_TEMP_TIME_S 30UL // 30 seconds

#else

#define TEMP_READ_INTERVAL 10000 // millis
#define VALVE_OPENING_TIME_S 120UL // 2 minutes
#define BLOCKED_TIME_S 3600UL // 1 hour
#define RISE_TEMP_TIME_S 300UL // 5 minutes

#endif

#define HYSTERESIS 50
#define MAX_ALLOWED_T 2500 // in cents
#define RISE_TEMP_DELTA 200 // Minimum difference in cents


// Room status
#define OPENING 'V' // valves are opening for VALVE_OPENING_TIME
#define CLOSED 'C' // Closed
#define OPEN 'O' // Open (main pump is also open)
#define BLOCKED 'B' // Blocked until BLOCKED_TIME is elapsed

// Error codes
#define ERR_NO 0
#define ERR_WRONG_COMMAND 1
#define ERR_WRONG_ROOM 2
#define ERR_WRONG_PROGRAM 3
#define ERR_WRONG_PARM 4 // Generic parameter error

// Commands
#define CMD_ROOM_SET_PGM 1
#define CMD_WRITE_EEPROM 2
#define CMD_TIME_SET 3
#define CMD_TEMPERATURE_SET 4
#define CMD_RESET 5
#define CMD_W_PGM_SET_D_PGM 6
#define CMD_D_PGM_SET_T_PGM 7
#define CMD_SLOT_SET_UPPER_BOUND 8
#define CMD_CLEAR_EEPROM 9
#define CMD_SET_RISE_TEMP_TIME_S 10
#define CMD_SET_RISE_TEMP_DELTA 11
#define CMD_SET_BLOCKED_TIME_S 12
#define CMD_SET_HYSTERESIS 13
#define CMD_UNBLOCK 14

//#include "Timer.h"
Timer t;

/** *****************************************************
*
* RTC part
*
*/


RTC_DS1307 RTC;
DateTime now;


/** ************************************************
*
* DS18B20 part
*
*/


// Data wire is plugged into pin 6 on the Arduino
#define ONE_WIRE_BUS ONE_WIRE_PIN

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);



/** *************************************
 *
 * Programs
 *
 */

// Global time (minutes from 0)
uint32_t pump_blocked_time = 0;
uint32_t pump_open_time = 0;
uint32_t blocked_time_s = BLOCKED_TIME_S;
uint32_t rise_temp_time_s = RISE_TEMP_TIME_S;
uint16_t rise_temp_delta = RISE_TEMP_DELTA;
uint16_t hysteresis = HYSTERESIS;
byte this_weekday;
byte last_error_code = ERR_NO;
byte pump_open = 0;
byte this_slot = 0;
const uint8_t eeprom_id = 0x99;

// Temperatures
// TODO: configurable
uint16_t T[] = {500, 1500, 1800, 2000};
uint16_t hot_temp, cold_temp;

// Programs
// 8 slots                    0:0 6:30  8:00 12:00 13:00 16:00 20:00 22:00  24:00
uint16_t slot[SLOT_NUMBER - 1] = { 390,  480,  720,  780,  960, 1200, 1320 };
// 6 programs, T level for each slot/pgm tuple
/*
static byte daily_program[DAILY_PROGRAM_NUMBER][SLOT_NUMBER] = {
    //0:00 6:30  8:00 12:00 13:00 16:00 20:00 22:00
    {    0,   0,    0,    0,    0,    0,    0,    0 }, // all T0
    {    1,   1,    1,    1,    1,    1,    1,    1 }, // all T1
    {    2,   2,    2,    2,    2,    2,    2,    2 }, // all T2
    {    3,   3,    3,    3,    3,    3,    3,    3 }, // all T3
    {    1,   3,    1,    1,    1,    3,    2,    1 }, // awakening supper and evening 4
    {    1,   3,    1,    3,    1,    3,    2,    1 },  // awakening, meals and evening 5
    {    1,   3,    1,    3,    3,    3,    2,    1 },  // awakening, meals, afternoon and evening 6
    {    1,   3,    3,    3,    3,    3,    2,    1 },  // all day 7
};
*/


static byte daily_program[DAILY_PROGRAM_NUMBER][4] = {
    {
        B11111111, // 0
        B00000000,
        B00000000,
        B00000000
    },
    {
        B00000000, // 1
        B11111111,
        B00000000,
        B00000000
    },
    {
        B00000000, // 2
        B00000000,
        B11111111,
        B00000000
    },
    {
        B00000000, // 3
        B00000000,
        B00000000,
        B11111111
    },

    {
        B00000000, // 4
        B10111001,
        B00000100,
        B01000010
    },

    {
        B00000000, // 5
        B10101001,
        B00000100,
        B01010010
    },
    {
        B00000000, // 6
        B10100001,
        B00000010,
        B01011100
    },
    {
        B00000000, // 7
        B10000001,
        B00000010,
        B01111100
    }
};



// Weekly programs, 0 is monday
static byte weekly_program[WEEKLY_PROGRAM_NUMBER][3] = {
       //Mo-Fr Sa Su
        {0, 0, 0}, // always off
        {1, 1, 1}, // Always 1
        {2, 2, 2}, // Always 2
        {3, 3, 3}, // Always 3
        {4, 7, 7}, // 4 (5+2)
        {4, 4, 7}, // 4 (6+1)
        {5, 7, 7}, // 5 (5+2)
        {5, 5, 7}, // 5 (6+1)
        {6, 7, 7}, // 6 (5+2)
        {6, 6, 7}  // 6 (6+1)
};


/*/ Array of rooms
static struct room_t {
  DeviceAddress address;
  byte pin;
  byte program;
  char status;
  int temperature;
  uint32_t last_status_change;
} rooms[ROOMS] = {
    {{ 0x28, 0xAD, 0x4C, 0xC4, 0x03, 0x00, 0x00, 0x13}, ROOM_1_PIN, 3, CLOSED}, // 1 - Bagno
    {{ 0x28, 0x6C, 0x41, 0xC4, 0x03, 0x00, 0x00, 0x57}, ROOM_2_PIN, 8, CLOSED}, // 2 - Camera A
    {{ 0x28, 0x6C, 0x41, 0xC4, 0x03, 0x00, 0x00, 0x37}, ROOM_3_PIN, 8, CLOSED}, //   - Sala
    {{ 0x28, 0x6C, 0x41, 0xC4, 0x03, 0x00, 0x00, 0x27}, ROOM_4_PIN, 8, CLOSED}, //   - Camera O
    {{ 0x28, 0x6C, 0x41, 0xC4, 0x03, 0x00, 0x00, 0x67}, ROOM_5_PIN, 8, CLOSED}  //   - Camera P
};
*/

static struct room_t {
  byte pin;
  byte program;
  char status;
  int temperature;
  uint32_t last_status_change;
} rooms[ROOMS] = {
    { ROOM_1_PIN, 3, CLOSED}, // 1 - Bagno
    { ROOM_2_PIN, 4, CLOSED}, // 2 - Camera A
    { ROOM_3_PIN, 5, CLOSED}, //   - Sala
    { ROOM_4_PIN, 7, CLOSED}, //   - Camera O
    { ROOM_5_PIN, 8, CLOSED}  //   - Camera P
};


/**
 * Returns the desider temperature for a room and a time slot (in seconds)
 */
float get_desired_temperature(byte room){
    for(byte i=0; i<4; i++){
        if(daily_program[weekly_program[rooms[room].program][this_weekday]][i] & (B10000000 >> this_slot)){
            return T[i];
        }
    }
    return T[0];
}


/**
 * Check temperatures and perform actions
 *
 * Here is the core logic fo the heating system:
 *
 * a global var pump_open controls the pin, when it changes
 * a global pump_open_time is set and used to determine when
 * to check for a T-delta on the hot and cold pipes sensors.
 * If the T-delta is lower than the threshold, then the system
 * is blocked and the timestamp stored in pump_blocked_time.
 * As the time passes pump_blocked_time + BLOCKED_TIME_S
 * the system is unblocked and everything is reset to try
 * another cycle.
 */
void check_temperatures(){

    sensors.requestTemperatures();

    // Read pump temp
    hot_temp = (uint16_t) (48.828125 * analogRead(HOT_PIN));
    cold_temp = (uint16_t) (48.828125 * analogRead(COLD_PIN));

    // Get Time
    now = RTC.now();
    this_weekday = now.dayOfWeek(); // sunday is 0
    this_weekday = this_weekday ? this_weekday - 1 : 6;
    this_weekday = this_weekday <= 4 ? 0 : this_weekday - 4;
    // Get slot
    this_slot = 0;
    while(this_slot <= 6 && (uint16_t)now.hour() * 60 + (uint16_t)now.minute() > slot[this_slot]){
        this_slot++;
    }


    // Check if can unlock
    if(pump_blocked_time && (now.unixtime() > pump_blocked_time + blocked_time_s)){
        pump_blocked_time = 0;
    }


    // If the pump is not blocked and has been open more than RISE_TEMP_TIME_S,
    // check hot_temp and cold_temp
    if(!pump_blocked_time && pump_open_time && (now.unixtime() - pump_open_time > rise_temp_time_s)){
        // Block if lower
        if((hot_temp - cold_temp) < rise_temp_delta && hot_temp < T[3] + rise_temp_delta){
            pump_blocked_time = now.unixtime();
            pump_open = 0;
        }
    }

    // Local flags to check if any room needs_heating and
    // is ready to open the pump.
    byte needs_pump_open = 0;
    byte needs_heating = 0;

    for(int i=0; i<ROOMS; i++){
        // Get temperature
        float tempC = sensors.getTempCByIndex(i);
        if (tempC != -127.00) {
            rooms[i].temperature = (int)(tempC * 100);
        }
        char new_status = rooms[i].status;
        needs_heating = (new_status == OPENING);
        if(!needs_heating){
            needs_heating = rooms[i].temperature < (get_desired_temperature(i) + (new_status == OPEN ? hysteresis : 0));
        }
        if(!needs_heating){
            new_status = CLOSED;
        } else { // Needs heating!!!
            // Needs heating and target is T[0]: danger zone for pipe ice, unlock
            if(pump_blocked_time && get_desired_temperature(i) == T[0]){
                pump_blocked_time = 0;
            }
            if(pump_blocked_time){
               new_status = BLOCKED;
            } else {
                switch(rooms[i].status){
                    case OPENING:
                        if(VALVE_OPENING_TIME_S < now.unixtime() - rooms[i].last_status_change){
                            new_status = OPEN;
                        }
                        break;
                    case OPEN:
                        needs_pump_open = 1;
                        break;
                    case BLOCKED:
                        new_status = CLOSED;
                        break;
                    case CLOSED:
                        new_status = OPENING;
                        break;
                }
            }
        }
        if(new_status != rooms[i].status){
            rooms[i].last_status_change = now.unixtime();
            rooms[i].status = new_status;
        }
        digitalWrite(rooms[i].pin, (new_status == OPENING) || (new_status == OPEN));
    }
    // At least one room was ready to open the pump
    if(needs_pump_open){
        // Store the time if the pump wasn't already open
        if(!pump_open){
            pump_open = 1;
            pump_open_time = now.unixtime();
        }
    } else {
        pump_open = 0;
        pump_open_time = 0;
    }
    digitalWrite(PUMP_PIN, pump_open);
}


/**
 * Store config to EEPROM
 */
int store_config(){
    // Read EEPROM
    int write = 0;
    write += EEPROM_writeAnything(write, eeprom_id);
    write += EEPROM_writeAnything(write, T);
    write += EEPROM_writeAnything(write, slot);
    write += EEPROM_writeAnything(write, weekly_program);
    write += EEPROM_writeAnything(write, daily_program);
    for (int i=0; i<ROOMS; i++){
        write += EEPROM_writeAnything(write, rooms[i].program);
    }
    write += EEPROM_writeAnything(write, rise_temp_time_s);
    write += EEPROM_writeAnything(write, rise_temp_delta);
    write += EEPROM_writeAnything(write, blocked_time_s);
    write += EEPROM_writeAnything(write, hysteresis);
}


/**
 * Set up
 *
 */
void thermo_setup(){

    Wire.begin();
    RTC.begin();
    if (! RTC.isrunning()) {
        // following line sets the RTC to the date & time this sketch was compiled
        RTC.adjust(DateTime(__DATE__, __TIME__));
    }


    pinMode(PUMP_PIN, OUTPUT);

    // Sensors
    // set the resolution to 12 bit (maximum)
    sensors.begin();
    sensors.setResolution(12);

    for (int i=0; i<ROOMS; i++){
        pinMode(rooms[i].pin, OUTPUT);
    }

    t.every(TEMP_READ_INTERVAL, check_temperatures);

    pinMode(BEAT_PIN, OUTPUT);
    t.oscillate(BEAT_PIN, 1000, 1);

    // Read EEPROM
    if(EEPROM.read(0) == eeprom_id){
        int read = 1;
        read += EEPROM_readAnything(read, T);
        read += EEPROM_readAnything(read, slot);
        read += EEPROM_readAnything(read, weekly_program);
        read += EEPROM_readAnything(read, daily_program);
        for (int i=0; i<ROOMS; i++){
            read += EEPROM_readAnything(read, rooms[i].program);
        }
        read += EEPROM_readAnything(read, rise_temp_time_s);
        read += EEPROM_readAnything(read, rise_temp_delta);
        read += EEPROM_readAnything(read, blocked_time_s);
        read += EEPROM_readAnything(read, hysteresis);
    } else {
        store_config();
    }
}

void thermo_loop(){
    t.update();
}



/** ********************************************************
 *
 *  Ethernet part
 *
 */

byte Ethernet::buffer[BUFFER_SIZE];
BufferFiller bfill;


void setup(){

    if (ether.begin(sizeof Ethernet::buffer, mymac, ETHERNET_CS_PIN) == 0) {
        t.oscillate(BEAT_PIN, 500, 1);
        while(1){
            t.update();
        }
    }  else {
        ether.staticSetup(myip);
        // Thermo
        thermo_setup();
    }
}


/**
 * Returns the amount of free memory,
 * variables created by the build process when compiling the sketch
 */
int freeMemory () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}



/**
 * Find the value for a given key
 *
 * The returned value is stored in the global var strbuf
 */
uint8_t find_key_val(char *str,char *key){
  uint8_t found=0;
  uint8_t i=0;
  char *kp;
  kp=key;
  while(*str &&  *str!=' ' && found==0){
    if(*str == *kp){
      kp++;
      if(*kp == '\0'){
        str++;
        kp=key;
        if(*str == '='){
          found=1;
        }
      }
    }
    else{
      kp=key;
    }
    str++;
  }
  if(found==1){
    //copy the value to a buffer and terminate it with '\0'
    while(*str &&  *str!=' ' && *str!='&' && i<STR_BUFFER_SIZE){
      strbuf[i]=*str;
      i++;
      str++;
    }
    strbuf[i]='\0';
  }
  return(found);
}



/**
 * Get the numeric positive int parameter
 */
int analyse_cmd(char *str, char *key){
  int r=-1;
  char *buf_p;
  if(find_key_val(str, key)){
    buf_p = strbuf;
    while(0x2f < *buf_p && *buf_p < 0x3a){
      //is a ASCII number, return it
      r = (r >= 0)  ? r * 10 + (*buf_p - 0x30) : (*buf_p - 0x30);
      buf_p++;
    }
  }
  return r;
}



/**
 * Standard header
 */
void print_200ok(){
  bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"));
}


/**
 * Main home page
 */
void print_homepage(){
  print_200ok();
  bfill.emit_p(PSTR("<!DOCTYPE html><html><head></head><body>"));
  bfill.emit_p(PSTR("<h1>EtherShieldThermo</h1>"));
  bfill.emit_p(PSTR("<script src=\""));
  bfill.emit_p(PSTR(LOADER_ADDRESS));
  bfill.emit_p(PSTR("\"></script>"));
  bfill.emit_p(PSTR("</body></head></html>"));
}

/**
 * Int to float to string
 */
char* decimal_string(int num, char* _buf){
    char buf3[3];
    itoa(num/100, _buf, 10);
    strcat(_buf, ".");
    itoa(num%100, buf3, 10);
    strcat(_buf, buf3);
    return _buf;
}

/**
 * Json helpers
 */
void bracket_open(){
    return bfill.emit_p( PSTR("["));
}
void bracket_close(){
    return bfill.emit_p( PSTR("]"));
}

void json_array_wrap(uint16_t p[], int length){
    bracket_open();
    for(int i = 0; i < length; i++){
        decimal_string(p[i],  strbuf);
        bfill.emit_raw(strbuf, strlen(strbuf));
        if(i<length -1){
            bfill.emit_p(PSTR(","));
        }
    }
    bracket_close();
}

void json_array_wrap(byte p[], int length){
    bracket_open();
    for(int i = 0; i < length; i++){
        itoa((int)p[i], strbuf, 10);
        bfill.emit_raw(strbuf, strlen(strbuf));
        if(i<length -1){
            bfill.emit_p(PSTR(","));
        }
    }
    bracket_close();
}

void json_array_wrap(int p[], int length){
    bracket_open();
    for(int i = 0; i < length; i++){
        itoa(p[i], strbuf, 10);
        bfill.emit_raw(strbuf, strlen(strbuf));
        if(i<length -1){
            bfill.emit_p( PSTR(","));
        }
    }
    bracket_close();
}


/**
 * Print status
 */
void print_json_response(byte print_programs){
    // Update
    now = RTC.now();

    bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n"));


    if(print_programs){

        bfill.emit_p( PSTR("{\"T\":"));
        json_array_wrap(T, 4);

        bfill.emit_p(PSTR(",\"r\":$S"),
            decimal_string(rise_temp_delta, strbuf));

        bfill.emit_p(PSTR(",\"h\":$S"),
            decimal_string(hysteresis, strbuf));

        ultoa(rise_temp_time_s, strbuf, 10);
        bfill.emit_p(PSTR(",\"t\":$S"), strbuf);

        ultoa(blocked_time_s, strbuf, 10);
        bfill.emit_p(PSTR(",\"b\":$S"), strbuf);

        bfill.emit_p( PSTR(",\"s\":"));
        json_array_wrap(slot, SLOT_NUMBER - 1);

        bfill.emit_p( PSTR(",\"w\":["));

        for(int i=0; i<WEEKLY_PROGRAM_NUMBER; i++){
            json_array_wrap(weekly_program[i], 3);
            if(i<WEEKLY_PROGRAM_NUMBER-1){
                bfill.emit_p( PSTR(","));
            }
        }
        bfill.emit_p( PSTR("],\"d\":["));
        for(int i=0; i<DAILY_PROGRAM_NUMBER; i++){
            json_array_wrap(daily_program[i], 4);
            if(i<DAILY_PROGRAM_NUMBER-1){
                bfill.emit_p( PSTR(","));
            }
        }

    } else {
        ultoa(now.unixtime(), strbuf, 10);
        bfill.emit_p(PSTR("{\"P\":$D,\"u\":$S,\"s\":$D,"),
            pump_open,
            strbuf,
            (int)this_slot);

        ultoa((pump_blocked_time ? (pump_blocked_time + blocked_time_s) : 0UL), strbuf, 10);
        bfill.emit_p(PSTR("\"b\":$S,"),
            strbuf);

        bfill.emit_p(PSTR("\"c\":$S,"),
            decimal_string(cold_temp, strbuf));

        bfill.emit_p(PSTR("\"h\":$S,\"E\":$D,"),
            decimal_string(hot_temp, strbuf),
            (word)last_error_code);
        bfill.emit_p(PSTR("\"R\":["));

        for(int room=0; room<ROOMS; room++){
            bfill.emit_p( PSTR("{\"t\":$S,"),
                decimal_string(rooms[room].temperature, strbuf));

            bfill.emit_p( PSTR("\"T\":$S,\"p\":$D,\"d\":$D,"),
                decimal_string(get_desired_temperature(room), strbuf),
                rooms[room].program,
                weekly_program[rooms[room].program][this_weekday]);

            //ultoa(rooms[room].last_status_change, strbuf, 10);
            //bfill.emit_p( PSTR("\"l\":$S,\""),
            //    strbuf);

            strbuf[1] = '\0';
            strbuf[0] = rooms[room].status;
            bfill.emit_p( PSTR("\"s\":\"$S\""),
                strbuf);

            bfill.emit_p((room != ROOMS - 1 ? PSTR("},") : PSTR("}")));
        }
    }
    bfill.emit_p(PSTR("]}"));
}

byte in_range(int num, int low, int high){
    return (low <= num) && (num <= high);
}

/**
 * Main loop
 *
 */
void loop(){
    int cmd, parm1, parm2, parm3;
    // wait for an incoming TCP packet, but ignore its contents
    word len = ether.packetReceive();
    word pos = ether.packetLoop(len);
    if (pos) {

        delay(1);   // necessary for my system
        bfill = ether.tcpOffset();
        char *data = (char *) Ethernet::buffer + pos;
        if (strncmp("GET /", data, 5) != 0) {
            // head, post and other methods for possible status codes see:
            // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
            print_200ok();
            goto SENDTCP;
        }
        data += 5;
        if(strncmp(" ", data, 1)==0){
            print_homepage();
            goto SENDTCP;
        }
        if(strncmp("st", data, 2)==0){
            print_json_response(0);
            goto SENDTCP;
        }
        if(strncmp("pr", data, 2)==0){
            print_json_response(1);
            goto SENDTCP;
        }
        if(strncmp("db", data, 2)==0){
            itoa(freeMemory(), strbuf, 10);
            bfill.emit_p(PSTR("$S"), strbuf);
            goto SENDTCP;
        }
        cmd = analyse_cmd(data, "c");
        if(cmd > 0){
            last_error_code = ERR_NO;
            // Switch?
            switch(cmd){
                case CMD_UNBLOCK:
                    pump_blocked_time = 0;
                break;
                case CMD_ROOM_SET_PGM:
                    parm1 = analyse_cmd(data, "p");
                    if(in_range(parm1, 0, ROOMS - 1)){
                        parm2 = analyse_cmd(data, "v");
                        if(in_range(parm2, 0, WEEKLY_PROGRAM_NUMBER - 1 )){
                            rooms[parm1].program = parm2;
                        } else {
                            last_error_code = ERR_WRONG_PROGRAM;
                        }
                    } else {
                        last_error_code = ERR_WRONG_ROOM;
                    }
                break;
                case CMD_WRITE_EEPROM:
                    // Write to EEPROM
                    store_config();
                break;
                // Set daily_program for a given weekly_program
                case CMD_W_PGM_SET_D_PGM:
                    parm1 = analyse_cmd(data, "p");
                    if(in_range(parm1, 0, WEEKLY_PROGRAM_NUMBER - 1)){
                        parm2 = analyse_cmd(data, "v"); // dayOfWeek (0=mo-fr,1=sa,2=su)
                        if(in_range(parm2, 0, 2)){
                            parm3 = analyse_cmd(data, "w");
                            if(in_range(parm3, 0, DAILY_PROGRAM_NUMBER - 1)){
                                weekly_program[parm1][parm2] = parm3;
                            } else {
                                last_error_code = ERR_WRONG_PARM;
                            }
                        } else {
                            last_error_code = ERR_WRONG_PARM;
                        }
                    } else {
                        last_error_code = ERR_WRONG_PARM;
                    }
                break;
                // Set T level for a given slot on a given daily program
                case CMD_D_PGM_SET_T_PGM:
                    parm1 = analyse_cmd(data, "p"); // Program index
                    if(in_range(parm1, 0, DAILY_PROGRAM_NUMBER - 1)){
                        parm2 = analyse_cmd(data, "v"); // Slot (0 -7)
                        if(in_range(parm2, 0, 7)){
                            parm3 = analyse_cmd(data, "w");
                            if(in_range(parm3, 0, 3)){ // T level
                                daily_program[parm1][0] &= ~(B10000000 >> parm2);
                                daily_program[parm1][1] &= ~(B10000000 >> parm2);
                                daily_program[parm1][2] &= ~(B10000000 >> parm2);
                                daily_program[parm1][3] &= ~(B10000000 >> parm2);
                                daily_program[parm1][parm3] |= (B10000000 >> parm2);
                            } else {
                                last_error_code = ERR_WRONG_PARM;
                            }
                        } else {
                            last_error_code = ERR_WRONG_PARM;
                        }
                    } else {
                        last_error_code = ERR_WRONG_PARM;
                    }
                    // Write to EEPROM
                break;
                case CMD_SET_RISE_TEMP_TIME_S:
                    parm1 = analyse_cmd(data, "p");
                    if(in_range(parm1, 120, 1200)){ // Seconds
                        rise_temp_time_s = parm1;
                    } else {
                        last_error_code = ERR_WRONG_PARM;
                    }
                break;
                case CMD_SET_BLOCKED_TIME_S:
                    parm1 = analyse_cmd(data, "p");
                    if(in_range(parm1, 1800, 7200)){ // Seconds
                        blocked_time_s = parm1;
                    } else {
                        last_error_code = ERR_WRONG_PARM;
                    }

                break;
                case CMD_SET_HYSTERESIS:
                    parm1 = analyse_cmd(data, "p");
                    if(in_range(parm1, 0, 2000)){
                        hysteresis = parm1;
                    } else {
                        last_error_code = ERR_WRONG_PARM;
                    }
                break;
                case CMD_SET_RISE_TEMP_DELTA:
                    parm1 = analyse_cmd(data, "p");
                    if(in_range(parm1, 100, 2000)){
                        rise_temp_delta = parm1;
                    } else {
                        last_error_code = ERR_WRONG_PARM;
                    }
                break;
                case CMD_SLOT_SET_UPPER_BOUND: // parm1 is 1-based
                    parm1 = analyse_cmd(data, "p");
                    if(!in_range(parm1, 1, SLOT_NUMBER)){
                        last_error_code = ERR_WRONG_PARM;
                    } else {
                        parm2 = analyse_cmd(data, "v");
                        if((parm1 == 1 && parm2 < slot[1])
                            || (parm1 == SLOT_NUMBER && parm2 > slot[SLOT_NUMBER - 2])
                            || (in_range(parm2, slot[parm1 - 2], slot[parm1])))
                        {
                            slot[parm1 - 1] = parm2;
                        }
                    }
                break;
                case CMD_CLEAR_EEPROM:
                    EEPROM.write(0,0);
                break;
                case CMD_TEMPERATURE_SET:
                    // Set T1, T2 and T3
                    parm1 = analyse_cmd(data, "p");
                    if(!in_range(parm1, 1, 3)){
                        last_error_code = ERR_WRONG_PARM;
                    } else {
                        parm2 = analyse_cmd(data, "v");
                        switch(parm1){
                            case 1:
                                if(in_range(parm2, T[0] + 50, T[2] - 50)){
                                    T[1] = parm2;
                                } else {
                                    last_error_code = ERR_WRONG_PARM;
                                }
                            break;
                            case 2:
                                if(in_range(parm2, T[1] + 50,  T[3] - 50)){
                                    T[2] = parm2;
                                } else {
                                    last_error_code = ERR_WRONG_PARM;
                                }
                            break;
                            case 3:
                                if(in_range(parm2, T[2] + 50, MAX_ALLOWED_T )){
                                    T[3] = parm2;
                                } else {
                                    last_error_code = ERR_WRONG_PARM;
                                }
                            break;
                        }
                    }
                break;
                case CMD_TIME_SET:
                    parm1 = analyse_cmd(data, "p");
                    if(parm1 < 0 || parm1 > 5){ // 0 = hh, 1 = mm, 2 = ss, 3 = Y, 4 = m, 5 = d
                        last_error_code = ERR_WRONG_PARM;
                    } else {
                        now = RTC.now();
                        parm2 = analyse_cmd(data, "v");
                        switch(parm1){
                            case 0:
                                RTC.adjust(DateTime(now.year(), now.month(), now.day(), parm2, now.minute(), now.second()));
                            break;
                            case 1:
                                RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), parm2, now.second()));
                            break;
                            case 2:
                                RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), parm2));
                            break;
                            case 3:
                                RTC.adjust(DateTime(parm2, now.month(), now.day(), now.hour(), now.minute(), now.second()));
                            break;
                            case 4:
                                RTC.adjust(DateTime(now.year(), parm2, now.day(), now.hour(), now.minute(), now.second()));
                            break;
                            case 5:
                                RTC.adjust(DateTime(now.year(), now.month(), parm2, now.hour(), now.minute(), now.second()));
                            break;
                            default:
                                last_error_code = ERR_WRONG_PARM;
                        }
                    }
                break;
                default:
                    last_error_code = ERR_WRONG_COMMAND;

            }
        }
        print_json_response(0);

SENDTCP:ether.httpServerReply(bfill.position());
    }

    // Thermo
    thermo_loop();

}
