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
	unsigned char led_order;
} led_manager_config_t;

int led_manager_argb8888_to_leds(const unsigned char *buffer, unsigned char *data);
int led_manager_init(led_manager_config_t * config, led_manager_profile_t * profile);
int led_manager_set_profile(const led_manager_profile_t* profile);
int led_manager_get_profile(led_manager_profile_t * profile);
int led_manager_print_area(char * buffer);
unsigned int led_manager_get_profile_index();
int led_manager_set_state(unsigned char state);
int led_manager_get_state();
int led_manager_set_intensity(unsigned int intensity);
int led_manager_get_intensity();
unsigned char led_manager_get_borders(const unsigned char* buffer, unsigned short* h_border, unsigned short* v_border);
#endif
