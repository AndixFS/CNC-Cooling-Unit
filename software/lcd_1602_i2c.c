/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

/* Example code to drive a LCD panel via a I2C bridge chip (e.g. PCF8574)

   GPIO 4 (pin 6)-> SDA on LCD bridge board
   GPIO 5 (pin 7)-> SCL on LCD bridge board
   3.3v (pin 36) -> VCC on LCD bridge board
   GND (pin 38)  -> GND on LCD bridge board
*/
// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0


#define MAX_LINES      4 
#define MAX_CHARS      20

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    //int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    //int row_offs[]={0x00, 0x40, 0x14, 0x54};
    //int dramaddr = 0x80;
    int val;
    switch(line) {
	case 0:	val = 0x80 + position;
	case 1: val = 0xC0 + position;
	case 2: val = 0x94 + position;
	case 3: val = 0xD4 + position;
    
	default: val = 0x94 + position; 
	}

    lcd_send_byte(val, LCD_COMMAND);
}

static void inline lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
    while (*s) {
        lcd_char(*s++);
    }
}

void lcd_init() {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);

    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
}

/*----------PIN Definition--------------------*/
#define SW0 	 19	// Toggle Air only on/off
#define SW1	 18	// Manual Boost
#define SW2	 20	// Toggle Auto Boost on/off

#define OK0	 12	// Start MIST Cooling Program
#define OK1	 13	// 

#define ADC0_PIN 26
#define ADC1_PIN 27
#define ADC0	 0	//define liquid flow
#define ADC1	 1	//define boost time

#define STEP_DIR 11
#define STEP_S	 10

#define REL0	 6	// Relais for Air
#define REL1	 7	// Relais for Liquid

/*---------Stepper Struct----------------------*/
struct stepper{
	uint f_pwm;
	uint f_sys;
	uint duty;
	uint slice_num;
	float divider;
	uint top;
	uint level;
};

/*---------Switch Struct-------------------------*/
struct sw{
	uint pin;
	uint previous;
	uint current;
	bool toggle;
};

void switch_init(struct sw *drv, uint SW){
	gpio_init(SW);
	gpio_set_dir(SW, GPIO_IN);
	gpio_set_pulls(SW,1,0); //Pullup

	drv->pin      = SW;
	drv->previous = 0;
	drv->current  = 0;
	drv->toggle   = false;
}

void switch_update(struct sw *drv){
	//get switch status
	drv->current = !gpio_get(drv->pin); //True when pressed
	
	//rising edge
       	if(drv->current & (!drv->previous)){
		drv->previous = drv->current;
		drv->toggle = !drv->toggle;
	}

	if((!drv->current) & drv->previous){
		drv->previous = drv->current;
	}
}


void stepper_init(struct stepper *drv){
	// init DIR = GPIO 11
	gpio_init(STEP_DIR);
	gpio_set_dir(STEP_DIR, GPIO_OUT); 

	//setup PWM module variables
	drv->f_pwm = 0;
	drv->duty  = 50; //50% Duty cycle
	
	gpio_set_function(STEP_S, GPIO_FUNC_PWM);
	drv->slice_num = pwm_gpio_to_slice_num(STEP_S);
	drv->f_sys = clock_get_hz(clk_sys);
	drv->divider = drv->f_sys / 1000000UL;
	pwm_set_clkdiv(drv->slice_num, drv->divider);


}

void stepper_set_speed(struct stepper *drv, uint speed, bool dir){
	
	//set direction
	gpio_put(STEP_DIR,dir);
	
	//set speed
	drv->top = 1000000UL/speed -1;
	pwm_set_wrap(drv->slice_num, drv->top);
	drv->level = (drv->top+1) * drv->duty / 100 -1; 
	pwm_set_chan_level(drv->slice_num, 0,drv->level);
	pwm_set_enabled(drv->slice_num, true);
}

void stepper_stop(struct stepper *drv){
	
	pwm_set_enabled(drv->slice_num,false);

}

uint steps2ml(uint steps){
	return ((steps*16)/4096);
}


int main() {
	stdio_init_all();
	adc_init();
	adc_gpio_init(ADC0_PIN);
	adc_gpio_init(ADC1_PIN);

#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
    #warning i2c/lcd_1602_i2c example requires a board with I2C pins
#else
    	// This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    	i2c_init(i2c_default, 100 * 1000);
    	gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    	gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    	gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    	gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    	// Make the I2C pins available to picotool
    	bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    	lcd_init();

	struct stepper *pump_ptr, pump;
	pump_ptr = &pump;

	struct sw *sw0_ptr, *sw1_ptr, *sw2_ptr, sw0, sw1, sw2;
	sw0_ptr = &sw0;
	sw1_ptr = &sw1;
	sw2_ptr = &sw2;

	uint adc0_speed;
	uint adc1_time;

	uint boost_time  = 0;
	uint boost_speed = 6500;
	uint lcd_timer   = 30;	
	uint lcd_refresh = 60;  // lcd refresh time (x*prog_time)
	uint prog_time   = 5; //programm cycle time in ms
	uint speed_ml    = 0;

	char lcd0_str[21];
	char lcd1_str[21];
	char lcd2_str[21];
	char lcd3_str[21];

	switch_init(sw0_ptr,SW0);
	sw0.toggle = 1;
	switch_init(sw1_ptr,SW1);
	switch_init(sw2_ptr,SW2);

	//Init optocouplers
	 
	gpio_init(OK0);
	gpio_set_dir(OK0, GPIO_IN);
	gpio_set_pulls(OK0,1,0); //Pullup

	gpio_init(OK1);
	gpio_set_dir(OK1, GPIO_IN);
	gpio_set_pulls(OK1,1,0); //Pullup

	//Init Relais
	gpio_init(REL0);
	gpio_set_dir(REL0, GPIO_OUT); 
	gpio_put(REL0,1); //turn relais off

	gpio_init(REL1);
	gpio_set_dir(REL1, GPIO_OUT); 
	gpio_put(REL1,1); //turn relais off

	stepper_init(pump_ptr);
	stepper_set_speed(pump_ptr,1,0);
	sleep_ms(10);
	stepper_set_speed(pump_ptr,1,1);
	sleep_ms(10);
	stepper_stop(pump_ptr);
	

    while (1) {
	//update switches    
	switch_update(sw0_ptr);
	switch_update(sw1_ptr);
	switch_update(sw2_ptr);
	//measure ADC
	adc_select_input(ADC0);    
	adc0_speed = adc_read()*2;
	speed_ml = steps2ml(adc0_speed);
	adc_select_input(ADC1);
	adc1_time = adc_read()/4;
	
	//Programm start
	// Set Relais
	if(!gpio_get(OK0)){
		gpio_put(REL0,0);
	}
	else{
		gpio_put(REL0,1);
	}	
	
	if((!gpio_get(OK0)) & (sw0.toggle)){
		gpio_put(REL1,0);
	}
	else{
		gpio_put(REL1,1);
	}

	//manual boost
	if(sw1.current){
		adc0_speed = boost_speed;
		speed_ml = steps2ml(boost_speed);
	}

	if((!gpio_get(OK0)) & (sw0.toggle)){
		//is boost activated
		if((boost_time > 0) & (sw2.toggle)){
			stepper_set_speed(pump_ptr, boost_speed, false);
			speed_ml = steps2ml(boost_speed);

			boost_time--;
		}
		else{
			stepper_set_speed(pump_ptr, adc0_speed,false);
		}



	}
	else{
		//set boost time if boost is activeded
		if(sw2.toggle){
			boost_time = adc1_time;
		}
		
		stepper_stop(pump_ptr);

	}
	

//speed_ml
	uint sec  = boost_time*prog_time/500;

	sprintf(lcd0_str,"Flow:   %2d ml/min   ", speed_ml);
	if(sw2.toggle & (gpio_get(OK0)))
		sprintf(lcd1_str,"Boost:  %2d s   activ", sec);
	
	if(sw2.toggle & !(gpio_get(OK0)) & sw0.toggle)
		sprintf(lcd1_str,"Boost:  %2d s left   ", sec);
	

	if(!(sw2.toggle) & (gpio_get(OK0)))
		sprintf(lcd1_str,"Boost:   off        ");



	//sprintf(lcd2_str,"--------------------");
	if (sw0.toggle){
		sprintf(lcd2_str,"Cooling: Mist       ");
	}
	else{
		sprintf(lcd2_str,"Cooling: Air only  ");
	}

	sprintf(lcd3_str,"|Man|Air|Boost|ml|s|");


	//LCD update cycle
	if(lcd_timer <= 0){
		//update display
		//LCD line 0
		lcd_send_byte(0x80,LCD_COMMAND);
		lcd_string(lcd0_str);

		//LCD line 1
		lcd_send_byte(0xC0,LCD_COMMAND);
		lcd_string(lcd1_str);

		//LCD line 2
		lcd_send_byte(0x94,LCD_COMMAND);
		lcd_string(lcd2_str);

		//LCD line 3
		lcd_send_byte(0xD4,LCD_COMMAND);
		lcd_string(lcd3_str);
		
		lcd_timer=lcd_refresh;
	}
	else{
		//decrease timer
		lcd_timer--;
	}

	sleep_ms(prog_time);
        //lcd_clear();


	}
    return 0;
#endif
}
