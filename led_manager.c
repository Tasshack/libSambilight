/*
 *  tasshack
 *  (c) 2019
 *
 *  License: GPLv3
 *
 */

#include "led_manager.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	short x1;
	short x2;
	short y1;
	short y2;
} led_manager_area_t;

typedef struct {
	led_manager_config_t config;
	led_manager_area_t * led_area;
	led_manager_profile_t current_profile;
	led_manager_profile_t new_profile;

	unsigned short leds_count;

	unsigned short new_brightness;
	unsigned short current_brightness;

	unsigned char state;

	float fsaturation;
	float fvalue;
	float fbrightness;
} led_manager_t;

led_manager_t led_manager;

void led_manager_rgb2hsv(unsigned char red, unsigned char green, unsigned char blue, unsigned short * hue, unsigned char * saturation, unsigned char * value)
{
	unsigned char rgbMin, rgbMax;

	rgbMin = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
	rgbMax = red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);

	*value = rgbMax;
	if (*value == 0) {
		*hue = 0;
		*saturation = 0;
		return;
	}

	*saturation = 255 * (long)(rgbMax - rgbMin) / *value;
	if (*saturation == 0) {
		*hue = 0;
		return;
	}

	if (rgbMax == red) {
		// start from 360 to be sure that we won't assign a negative number to the unsigned hue value
		*hue = 360 + 60 * (green - blue) / (rgbMax - rgbMin);

		if (*hue > 359)
			*hue -= 360;
	}
	else if (rgbMax == green) {
		*hue = 120 + 60 * (blue - red) / (rgbMax - rgbMin);
	}
	else {
		*hue = 240 + 60 * (red - green) / (rgbMax - rgbMin);
	}
}

void led_manager_hsv2rgb(unsigned short hue, unsigned char saturation, unsigned char value, unsigned char * red, unsigned char * green, unsigned char * blue)
{
	unsigned char region, remainder, p, q, t;

	if (saturation == 0) {
		*red = value;
		*green = value;
		*blue = value;
		return;
	}

	region = hue / 60;
	remainder = (hue - (region * 60)) * 256 / 60;

	p = (value * (255 - saturation)) >> 8;
	q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
	t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

	switch (region) {
	case 0:
		*red = value; *green = t; *blue = p;
		break;
	case 1:
		*red = q; *green = value; *blue = p;
		break;
	case 2:
		*red = p; *green = value; *blue = t;
		break;
	case 3:
		*red = p; *green = q; *blue = value;
		break;
	case 4:
		*red = t; *green = p; *blue = value;
		break;
	default:
		*red = value; *green = p; *blue = q;
		break;
	}
}

void led_manager_transform(unsigned char * red, unsigned char * green, unsigned char * blue, float saturationGain, float valueGain)
{
	if (saturationGain != 1.0 || valueGain != 1.0) {
		unsigned short hue;
		unsigned char saturation, value;
		led_manager_rgb2hsv(*red, *green, *blue, &hue, &saturation, &value);

		int s = saturation * saturationGain;
		if (s > 255)
			saturation = 255;
		else
			saturation = s;

		int v = value * valueGain;
		if (v > 255)
			value = 255;
		else
			value = v;

		led_manager_hsv2rgb(hue, saturation, value, red, green, blue);
	}
}

int led_manager_increment_value(short * currentValue, short newValue, unsigned int step)
{
	if (*currentValue > newValue) {
		*currentValue -= step;
		if (*currentValue < newValue) {
			*currentValue = newValue;
		}
		return 1;
	}

	if (*currentValue < newValue) {
		*currentValue += step;
		if (*currentValue > newValue) {
			*currentValue = newValue;
		}
		return -1;
	}

	return 0;
}

int led_manager_calculate_area()
{
	unsigned short padded_width, padded_height, vertical_depth, horizontal_depth, index, i, j;

	padded_width = led_manager.config.image_width - (led_manager.current_profile.v_padding_px * 2);
	padded_height = led_manager.config.image_height - (led_manager.current_profile.h_padding_px * 2);

	vertical_depth = ((led_manager.current_profile.vertical_depth_percent / 100.0) * padded_width);
	horizontal_depth = ((led_manager.current_profile.horizontal_depth_percent / 100.0) * padded_height);

	memset(led_manager.led_area, 0, sizeof(led_manager_area_t) * led_manager.leds_count);

	for (i = 0; i < led_manager.config.h_leds_count; i++) {
		for (j = 0; j < 2; j++) {
			index = i + (j * (((led_manager.config.h_leds_count - i) * 2) + led_manager.config.v_leds_count - 1));
			if (index > (led_manager.config.h_leds_count + led_manager.config.v_leds_count + ((led_manager.config.h_leds_count - led_manager.config.bottom_gap) / 2) - 1)) {
				if (index < (led_manager.config.h_leds_count + led_manager.config.v_leds_count + ((led_manager.config.h_leds_count - led_manager.config.bottom_gap) / 2) + led_manager.config.bottom_gap - 1)) {
					break;
				}
				index -= led_manager.config.bottom_gap;
			}
			index += (led_manager.leds_count - led_manager.config.start_offset);
			if (index >= led_manager.leds_count) {
				index = index - led_manager.leds_count;
			}

			if (j == 0) {
				led_manager.led_area[index].y1 = led_manager.current_profile.h_padding_px;
			}
			else {
				led_manager.led_area[index].y1 = led_manager.config.image_height - vertical_depth - led_manager.current_profile.h_padding_px - 1;
			}
			led_manager.led_area[index].y2 = led_manager.led_area[index].y1 + vertical_depth;

			led_manager.led_area[index].x1 = (short)floorf(((float)padded_width / led_manager.config.h_leds_count) * i) + led_manager.current_profile.v_padding_px;
			led_manager.led_area[index].x2 = (short)ceilf(((float)padded_width / led_manager.config.h_leds_count) * (i + 1)) + led_manager.current_profile.v_padding_px;
		}
	}

	for (i = 0; i < led_manager.config.v_leds_count; i++) {
		for (j = 0; j < 2; j++) {
			index = i + led_manager.config.h_leds_count + (j * (((led_manager.config.v_leds_count - i) * 2) + led_manager.config.h_leds_count - led_manager.config.bottom_gap - 1)) + (led_manager.leds_count - led_manager.config.start_offset);
			if (index >= led_manager.leds_count) {
				index = index - led_manager.leds_count;
			}

			if (j == 0) {
				led_manager.led_area[index].x1 = led_manager.config.image_width - horizontal_depth - led_manager.current_profile.v_padding_px - 1;
			}
			else {
				led_manager.led_area[index].x1 = led_manager.current_profile.v_padding_px;
			}
			led_manager.led_area[index].x2 = led_manager.led_area[index].x1 + horizontal_depth;

			led_manager.led_area[index].y1 = (short)floorf(((float)padded_height / led_manager.config.v_leds_count) * i) + led_manager.current_profile.h_padding_px;
			led_manager.led_area[index].y2 = (short)ceilf(((float)padded_height / led_manager.config.v_leds_count) * (i + 1)) + led_manager.current_profile.h_padding_px;
		}
	}

	return 0;
}

int led_manager_recalculate_profile()
{
	if (memcmp(&led_manager.current_profile, &led_manager.new_profile, sizeof(led_manager_profile_t)) != 0) {
		unsigned int area_changed =
			led_manager_increment_value(&led_manager.current_profile.v_padding_px, led_manager.new_profile.v_padding_px, 1) != 0 ||
			led_manager_increment_value(&led_manager.current_profile.h_padding_px, led_manager.new_profile.h_padding_px, 1) != 0 ||
			led_manager_increment_value(&led_manager.current_profile.horizontal_depth_percent, led_manager.new_profile.horizontal_depth_percent, 1) != 0 ||
			led_manager_increment_value(&led_manager.current_profile.vertical_depth_percent, led_manager.new_profile.vertical_depth_percent, 1) != 0 ||
			led_manager_increment_value(&led_manager.current_profile.overlap_percent, led_manager.new_profile.overlap_percent, 1) != 0;

		if (area_changed) {
			led_manager_calculate_area();
		}

		unsigned int profile_changed =
			led_manager_increment_value(&led_manager.current_profile.value_gain_percent, led_manager.new_profile.value_gain_percent, 10) != 0 ||
			led_manager_increment_value(&led_manager.current_profile.saturation_gain_percent, led_manager.new_profile.saturation_gain_percent, 10) != 0;

		if (profile_changed) {
			led_manager.fsaturation = led_manager.current_profile.saturation_gain_percent / 100.0;
			led_manager.fvalue = led_manager.current_profile.value_gain_percent / 100.0;
		}

		if (!area_changed && !profile_changed) {
			memcpy(&led_manager.current_profile, &led_manager.new_profile, sizeof(led_manager_profile_t));
			return 0;
		}

		return 1;
	}

	return 0;
}

static unsigned char led_manager_get_channel(char type, const unsigned char * pixel)
{
	switch (type) {
	case 'b':
	case 'B':
		return pixel[0];
	case 'g':
	case 'G':
		return pixel[1];
	case 'r':
	case 'R':
		return pixel[2];
	default:;
	}
	return 0;
}

int led_manager_argb8888_to_leds(const unsigned char *buffer, unsigned char *data)
{
	unsigned char r, g, b, changed = 0;
	unsigned short address, total_r, total_g, total_b, px_count, i;
	short x, y;

	led_manager_recalculate_profile();
	if ((led_manager.state || led_manager.current_brightness > 0)) {
		if (!led_manager.state) {
			if (led_manager_increment_value(&led_manager.current_brightness, 0, 5) != 0) {
				led_manager.fbrightness = led_manager.current_brightness / 100.0;
			}
		}
		else {
			if (led_manager_increment_value(&led_manager.current_brightness, led_manager.new_brightness, 5) != 0) {
				led_manager.fbrightness = led_manager.current_brightness / 100.0;
			}
		}

		for (i = 0; i < led_manager.leds_count; i++) {
			total_r = 0;
			total_g = 0;
			total_b = 0;
			px_count = 0;
			for (x = led_manager.led_area[i].x1; x < led_manager.led_area[i].x2; x++) {
				for (y = led_manager.led_area[i].y1; y < led_manager.led_area[i].y2; y++) {
					if (x >= 0 && x < led_manager.config.image_width && y >= 0 && y < led_manager.config.image_height) {
						address = ((led_manager.config.image_height - 1 - y) * (led_manager.config.image_width * 4)) + ((led_manager.config.image_width - 1 - x) * 4);
						total_r += led_manager_get_channel(led_manager.config.color_order[0], &buffer[address]);
						total_g += led_manager_get_channel(led_manager.config.color_order[1], &buffer[address]);
						total_b += led_manager_get_channel(led_manager.config.color_order[2], &buffer[address]);
						px_count++;
					}
				}
			}

			if (px_count) {
				r = total_r / px_count;
				g = total_g / px_count;
				b = total_b / px_count;

				led_manager_transform(&r, &g, &b, led_manager.fsaturation, led_manager.fvalue);

				r *= led_manager.fbrightness;
				g *= led_manager.fbrightness;
				b *= led_manager.fbrightness;
			}
			else {
				r = 0;
				g = 0;
				b = 0;
			}

			if (led_manager.config.led_order == 1) {
				address = (led_manager.leds_count - i - 1) * 3;
			}
			else {
				address = i * 3;
			}
					   
			if (r != 0 || data[address + 0] != 0 || g != 0 || data[address + 1] != 0 || b != 0 || data[address + 2] != 0) {
				changed = 1;
			}

			data[address + 0] = r;
			data[address + 1] = g;
			data[address + 2] = b;
		}
	}
	return changed;
}

int led_manager_init(led_manager_config_t * config, led_manager_profile_t * profile)
{
	memcpy(&led_manager.config, config, sizeof(led_manager_config_t));

	led_manager.leds_count = ((led_manager.config.h_leds_count + led_manager.config.v_leds_count) * 2) - led_manager.config.bottom_gap;
	led_manager.current_brightness = 0;
	led_manager.new_brightness = 100;
	led_manager.state = 1;

	led_manager.led_area = malloc(sizeof(led_manager_area_t) * led_manager.leds_count);
	memset(led_manager.led_area, 0, sizeof(led_manager_area_t) * led_manager.leds_count);

	memcpy(&led_manager.new_profile, profile, sizeof(led_manager_profile_t));
	memcpy(&led_manager.current_profile, profile, sizeof(led_manager_profile_t));

	led_manager.fsaturation = led_manager.current_profile.saturation_gain_percent / 100.0;
	led_manager.fvalue = led_manager.current_profile.value_gain_percent / 100.0;

	led_manager_calculate_area();

	return led_manager.leds_count;
}

int led_manager_set_profile(led_manager_profile_t * profile)
{
	memcpy(&led_manager.new_profile, profile, sizeof(led_manager_profile_t));
	return 0;
}

int led_manager_get_profile(led_manager_profile_t * profile)
{
	if (profile) {
		memcpy(profile, &led_manager.new_profile, sizeof(led_manager_profile_t));
		return 0;
	}
	return -1;
}

unsigned int led_manager_get_profile_index()
{
	return led_manager.current_profile.index;
}

int led_manager_set_state(unsigned char state)
{
	if (led_manager.state != state) {
		led_manager.state = state;
		return 1;
	}
	return 0;
}

int led_manager_get_state()
{
	return led_manager.state || led_manager.current_brightness;
}

int led_manager_set_brightness(unsigned int brightness)
{
	if (brightness >= 0 && brightness <= 100) {
		if (brightness != led_manager.new_brightness) {
			led_manager.new_brightness = brightness;
			return 1;
		}
		return 0;
	}
	return -1;
}

int led_manager_get_brightness()
{
	return led_manager.new_brightness;
}