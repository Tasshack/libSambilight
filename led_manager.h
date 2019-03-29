/*
 *  tasshack
 *  (c) 2019
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
	unsigned long image_width;
	unsigned long image_height;
	unsigned char color_order[3];
	unsigned char led_order;
} led_manager_config_t;

int led_manager_argb8888_to_leds(const unsigned char *buffer, unsigned char *data);

int led_manager_init(led_manager_config_t * config, led_manager_profile_t * profile);

int led_manager_set_profile(led_manager_profile_t * profile);

int led_manager_get_profile(led_manager_profile_t * profile);

unsigned int led_manager_get_profile_index();

int led_manager_set_state(unsigned char state);

int led_manager_get_state();

int led_manager_set_brightness(unsigned int brightness);

int led_manager_get_brightness();
#endif
