/*
 *  tasshack
 *  (c) 2019 - 2021
 *
 *  License: GPLv3
 *
 */

#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "led_profile.h"

typedef struct {
	unsigned short h_leds_count;
	unsigned short v_leds_count;
	unsigned short bottom_gap;
	unsigned short start_offset;
	unsigned short image_width;
	unsigned short image_height;
	char color_order[3];
	unsigned short capture_pos;
	unsigned char led_order;
} led_manager_config_t;

typedef struct {
	int x1;
	int x2;
	int y1;
	int y2;
} led_manager_led_t;

int led_manager_argb8888_to_leds(const unsigned char *buffer, unsigned char *data);
int led_manager_init(const led_manager_config_t * config, const led_manager_profile_t * profile);
int led_manager_set_profile(const led_manager_profile_t* profile);
int led_manager_get_profile(led_manager_profile_t * profile);
int led_manager_print_area(char * buffer);
unsigned int led_manager_get_profile_index();
int led_manager_set_state(unsigned char state);
int led_manager_get_state();
int led_manager_set_intensity(unsigned short intensity, unsigned int current);
int led_manager_get_intensity();
unsigned char led_manager_get_borders(const unsigned char* buffer, short* h_border, short* v_border);
void led_manager_deinit();
led_manager_led_t* led_manager_get_leds();
#endif
