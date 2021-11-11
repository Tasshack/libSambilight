/*
 *  tasshack
 *  (c) 2019 - 2021
 *
 *  License: GPLv3
 *
 */

#include "led_manager.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct {
	int x1;
	int x2;
	int y1;
	int y2;
} led_manager_led_t;

typedef struct {
	led_manager_config_t config;
	led_manager_led_t* leds;
	led_manager_profile_t current_profile;
	led_manager_profile_t new_profile;

	unsigned long leds_count;

	unsigned short new_intensity;
	unsigned short current_intensity;

	unsigned char state;

	float fsaturation;
	float fvalue;
	float fintensity;
	short brightness_correction;

	unsigned char _r;
	unsigned char _g;
	unsigned char _b;
	unsigned char _px_r;
	unsigned char _px_g;
	unsigned char _px_b;
	unsigned long long _address;
	unsigned char _changed;
	unsigned char _profile_changed;
	unsigned long long _total_r;
	unsigned long long _total_g;
	unsigned long long _total_b;
	unsigned long _i;
	unsigned char _rPos;
	unsigned char _gPos;
	unsigned char _bPos;
	unsigned long _px_count;
	unsigned long _width_33;
	unsigned long _width_50;
	unsigned long _width_66;
	unsigned long _height_33;
	unsigned long _height_66;
	unsigned long _height_50;
	long _x;
	long _y;
} led_manager_t;

static led_manager_t led_manager;

unsigned char led_manager_correction(unsigned char color, short brightness_correction)
{
	if (brightness_correction) {
		if (brightness_correction > 0) {
			if (color + brightness_correction < 255) {
				return color + brightness_correction;
			}
			return 255;
		}

		if (color + brightness_correction > 0) {
			return color + brightness_correction;
		}
		return 0;
	}
	return color;
}

void led_manager_get_pixel(const unsigned char* buffer, unsigned char* red, unsigned char* green, unsigned char* blue, long x, long y)
{
	if (x >= 0 && x < led_manager.config.image_width && y >= 0 && y < led_manager.config.image_height) {
		unsigned long address;
		if (led_manager.config.capture_pos) {
			address = (y * (led_manager.config.image_width * 4)) + ((led_manager.config.image_width - 1 - x) * 4);
		}
		else {
			address = ((led_manager.config.image_height - 1 - y) * (led_manager.config.image_width * 4)) + ((led_manager.config.image_width - 1 - x) * 4);
		}

		*red = led_manager_correction(buffer[address + led_manager._rPos], led_manager.brightness_correction);
		*green = led_manager_correction(buffer[address + led_manager._gPos], led_manager.brightness_correction);
		*blue = led_manager_correction(buffer[address + led_manager._bPos], led_manager.brightness_correction);
	}
	else {
		*red = 0;
		*green = 0;
		*blue = 0;
	}
}

unsigned char led_manager_check_black(const unsigned char* buffer, long x, long y, unsigned char threshold)
{
	led_manager_get_pixel(buffer, &led_manager._px_r, &led_manager._px_g, &led_manager._px_b, x, y);
	if (led_manager._px_r < threshold && led_manager._px_g < threshold && led_manager._px_b < threshold) {
		return 1;
	}
	return 0;
}

unsigned char led_manager_get_borders(const unsigned char* buffer, unsigned short* h_border, unsigned short* v_border) {
	*h_border = -1;
	*v_border = -1;

	for (led_manager._i = 0; led_manager._i < led_manager._width_33; ++led_manager._i)
	{
		if (!led_manager_check_black(buffer, led_manager._i, led_manager._height_50, 3) ||
			!led_manager_check_black(buffer, led_manager._i, led_manager._height_33, 3) ||
			!led_manager_check_black(buffer, led_manager._i, led_manager._height_66, 3)) {
			*v_border = ((100 * led_manager._i) / led_manager.config.image_width);
			break;
		}
	}

	for (led_manager._i = 0; led_manager._i < led_manager._height_33; ++led_manager._i)
	{
		if (!led_manager_check_black(buffer, led_manager._width_50, led_manager._i, 3) ||
			!led_manager_check_black(buffer, led_manager._width_33, led_manager._i, 3) ||
			!led_manager_check_black(buffer, led_manager._width_66, led_manager._i, 3)) {
			*h_border = ((100 * led_manager._i) / led_manager.config.image_height);
			break;
		}
	}

	//led_manager.new_profile.v_padding_percent = ((100 * non_black_x) / led_manager.config.image_width);
	//led_manager.new_profile.h_padding_percent = ((100 * non_black_y) / led_manager.config.image_height);

	if (*h_border == -1 || *v_border == -1) {
		return 0;
	}
	return 1;
}

void led_manager_deinit() {
	free(led_manager.leds);
}

static void led_manager_rgb2hsv(unsigned char red, unsigned char green, unsigned char blue, unsigned short* hue, unsigned short* saturation, unsigned short* value)
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

static void led_manager_hsv2rgb(unsigned short hue, unsigned short saturation, unsigned char value, unsigned char* red, unsigned char* green, unsigned char* blue)
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

static void led_manager_transform(unsigned char* red, unsigned char* green, unsigned char* blue, float saturationGain, float valueGain)
{
	if (saturationGain != 1.0 || valueGain != 1.0) {
		unsigned short hue;
		unsigned short saturation, value;
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

static int led_manager_increment_value(short* currentValue, short newValue, unsigned short step)
{
	if (*currentValue != newValue) {
		if (*currentValue > newValue) {
			if (*currentValue - step > newValue) {
				*currentValue -= step;
			}
			else {
				*currentValue = newValue;
			}
			return 1;
		}

		if (*currentValue < newValue) {
			if (*currentValue + step < newValue) {
				*currentValue += step;
			}
			else {
				*currentValue = newValue;
			}
			return -1;
		}
	}
	return 0;
}

static unsigned char led_manager_get_channel(char type)
{
	switch (type) {
	case 'b':
	case 'B':
		return 0;
	case 'g':
	case 'G':
		return 1;
	case 'r':
	case 'R':
		return 2;
	default:;
	}
	return 0;
}

int led_manager_calculate_area()
{
	unsigned long padded_width, padded_height, vertical_depth, horizontal_depth, index, i, j, overlap_px_x, overlap_px_y, v_padding_px, h_padding_px;
	float x_area, y_area;
	v_padding_px = ((led_manager.current_profile.v_padding_percent / 100.0) * led_manager.config.image_width);
	h_padding_px = ((led_manager.current_profile.h_padding_percent / 100.0) * led_manager.config.image_height);

	padded_width = led_manager.config.image_width - (v_padding_px * 2) - 1;
	padded_height = led_manager.config.image_height - (h_padding_px * 2) - 1;

	vertical_depth = ((led_manager.current_profile.vertical_depth_percent / 100.0) * padded_width);
	horizontal_depth = ((led_manager.current_profile.horizontal_depth_percent / 100.0) * padded_height);

	x_area = (float)padded_width / led_manager.config.h_leds_count;
	y_area = (float)padded_height / led_manager.config.v_leds_count;

	overlap_px_x = ((led_manager.current_profile.overlap_percent / 100.0) * x_area);
	overlap_px_y = ((led_manager.current_profile.overlap_percent / 100.0) * y_area);

	memset(led_manager.leds, 0, sizeof(led_manager_led_t) * led_manager.leds_count);

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
				led_manager.leds[index].y1 = h_padding_px;
			}
			else {
				led_manager.leds[index].y1 = led_manager.config.image_height - vertical_depth - h_padding_px - 1;
			}
			led_manager.leds[index].y2 = led_manager.leds[index].y1 + vertical_depth;

			led_manager.leds[index].x1 = (short)ceilf(x_area * i) + v_padding_px - overlap_px_x;
			led_manager.leds[index].x2 = led_manager.leds[index].x1 + x_area + overlap_px_x + 1;

			if (led_manager.leds[index].x1 < 0) {
				led_manager.leds[index].x1 = 0;
			}
			if (led_manager.leds[index].x2 >= padded_width) {
				led_manager.leds[index].x2 = padded_width - 1;
			}
		}
	}

	for (i = 0; i < led_manager.config.v_leds_count; i++) {
		for (j = 0; j < 2; j++) {
			index = i + led_manager.config.h_leds_count + (j * (((led_manager.config.v_leds_count - i) * 2) + led_manager.config.h_leds_count - led_manager.config.bottom_gap - 1)) + (led_manager.leds_count - led_manager.config.start_offset);
			if (index >= led_manager.leds_count) {
				index = index - led_manager.leds_count;
			}

			if (j == 0) {
				led_manager.leds[index].x1 = led_manager.config.image_width - horizontal_depth - v_padding_px - 1;
			}
			else {
				led_manager.leds[index].x1 = v_padding_px;
			}
			led_manager.leds[index].x2 = led_manager.leds[index].x1 + horizontal_depth;

			led_manager.leds[index].y1 = (short)ceilf(y_area * i) + h_padding_px - overlap_px_y;
			led_manager.leds[index].y2 = led_manager.leds[index].y1 + y_area + overlap_px_y;

			if (led_manager.leds[index].y1 < 0) {
				led_manager.leds[index].y1 = 0;
			}
			if (led_manager.leds[index].y2 >= padded_height) {
				led_manager.leds[index].y2 = padded_height - 1;
			}
		}
	}

	return 0;
}

int led_manager_recalculate_profile()
{
	if (memcmp(&led_manager.current_profile, &led_manager.new_profile, sizeof(led_manager_profile_t))) {
		unsigned int area_changed = led_manager_increment_value(&led_manager.current_profile.v_padding_percent, led_manager.new_profile.v_padding_percent, 1);
		area_changed = led_manager_increment_value(&led_manager.current_profile.h_padding_percent, led_manager.new_profile.h_padding_percent, 1) || area_changed;
		area_changed = led_manager_increment_value(&led_manager.current_profile.horizontal_depth_percent, led_manager.new_profile.horizontal_depth_percent, 1) || area_changed;
		area_changed = led_manager_increment_value(&led_manager.current_profile.vertical_depth_percent, led_manager.new_profile.vertical_depth_percent, 1) || area_changed;
		area_changed = led_manager_increment_value(&led_manager.current_profile.overlap_percent, led_manager.new_profile.overlap_percent, 1) || area_changed;

		if (area_changed) {
			led_manager_calculate_area();
		}

		unsigned int profile_changed = led_manager_increment_value(&led_manager.current_profile.value_gain_percent, led_manager.new_profile.value_gain_percent, 4);
		profile_changed = led_manager_increment_value(&led_manager.current_profile.saturation_gain_percent, led_manager.new_profile.saturation_gain_percent, 4) || profile_changed;
		profile_changed = led_manager_increment_value(&led_manager.current_profile.brightness_correction, led_manager.new_profile.brightness_correction, 2) || profile_changed;

		if (profile_changed) {
			led_manager.fsaturation = led_manager.current_profile.saturation_gain_percent / 100.0;
			led_manager.fvalue = led_manager.current_profile.value_gain_percent / 100.0;
			led_manager.brightness_correction = led_manager.current_profile.brightness_correction;
		}

		if (!area_changed && !profile_changed) {
			memcpy(&led_manager.current_profile, &led_manager.new_profile, sizeof(led_manager_profile_t));
			return 0;
		}

		return 1;
	}

	return 0;
}

int led_manager_argb8888_to_leds(const unsigned char* buffer, unsigned char* data)
{
	led_manager._changed = 0;
	if (led_manager._profile_changed && !led_manager_recalculate_profile()) {
		led_manager._profile_changed = 0;
	}

	if ((led_manager.state || led_manager.current_intensity > 0)) {
		if (!led_manager.state) {
			if (led_manager_increment_value(&led_manager.current_intensity, 0, 4)) {
				led_manager.fintensity = led_manager.current_intensity / 100.0;
			}
		}
		else {
			if (led_manager_increment_value(&led_manager.current_intensity, led_manager.new_intensity, 4)) {
				led_manager.fintensity = led_manager.current_intensity / 100.0;
			}
		}

		for (led_manager._i = 0; led_manager._i < led_manager.leds_count; led_manager._i++) {
			led_manager._total_r = 0;
			led_manager._total_g = 0;
			led_manager._total_b = 0;
			led_manager._px_count = 0;

			for (led_manager._x = led_manager.leds[led_manager._i].x1; led_manager._x < led_manager.leds[led_manager._i].x2; led_manager._x++) {
				for (led_manager._y = led_manager.leds[led_manager._i].y1; led_manager._y < led_manager.leds[led_manager._i].y2; led_manager._y++) {
					if (led_manager._x >= 0 && led_manager._x < led_manager.config.image_width && led_manager._y >= 0 && led_manager._y < led_manager.config.image_height) {
						led_manager_get_pixel(buffer, &led_manager._px_r, &led_manager._px_g, &led_manager._px_b, led_manager._x, led_manager._y);

						led_manager._total_r += led_manager._px_r;
						led_manager._total_g += led_manager._px_g;
						led_manager._total_b += led_manager._px_b;

						led_manager._px_count++;
					}
				}
			}

			if (led_manager._px_count) {
				led_manager._r = (unsigned char)(led_manager._total_r / led_manager._px_count);
				led_manager._g = (unsigned char)(led_manager._total_g / led_manager._px_count);
				led_manager._b = (unsigned char)(led_manager._total_b / led_manager._px_count);

				led_manager_transform(&led_manager._r, &led_manager._g, &led_manager._b, led_manager.fsaturation, led_manager.fvalue);

				led_manager._r = (unsigned char)(led_manager._r * led_manager.fintensity);
				led_manager._g = (unsigned char)(led_manager._g * led_manager.fintensity);
				led_manager._b = (unsigned char)(led_manager._b * led_manager.fintensity);
			}
			else {
				led_manager._r = 0;
				led_manager._g = 0;
				led_manager._b = 0;
			}

			if (led_manager.config.led_order) {
				led_manager._address = led_manager._i * 3;
			}
			else {
				led_manager._address = (led_manager.leds_count - led_manager._i - 1) * 3;
			}

			if (!led_manager._changed && (led_manager._r != data[led_manager._address + 0] || led_manager._g != data[led_manager._address + 1] || led_manager._b != data[led_manager._address + 2])) {
				led_manager._changed = 1;
			}

			data[led_manager._address + 0] = led_manager._r;
			data[led_manager._address + 1] = led_manager._g;
			data[led_manager._address + 2] = led_manager._b;
		}
	}
	return led_manager._changed;
}

int led_manager_init(led_manager_config_t* config, led_manager_profile_t* profile)
{
	memcpy(&led_manager.config, config, sizeof(led_manager_config_t));

	led_manager.leds_count = ((led_manager.config.h_leds_count + led_manager.config.v_leds_count) * 2) - led_manager.config.bottom_gap;
	led_manager.current_intensity = 0;
	led_manager.new_intensity = 100;
	led_manager.state = 1;
	led_manager._profile_changed = 1;

	led_manager.leds = malloc(sizeof(led_manager_led_t) * led_manager.leds_count);
	memset(led_manager.leds, 0, sizeof(led_manager_led_t) * led_manager.leds_count);

	memcpy(&led_manager.new_profile, profile, sizeof(led_manager_profile_t));
	memcpy(&led_manager.current_profile, profile, sizeof(led_manager_profile_t));

	led_manager.fsaturation = led_manager.current_profile.saturation_gain_percent / 100.0;
	led_manager.fvalue = led_manager.current_profile.value_gain_percent / 100.0;
	led_manager.brightness_correction = led_manager.current_profile.brightness_correction;

	led_manager._rPos = led_manager_get_channel(led_manager.config.color_order[0]);
	led_manager._gPos = led_manager_get_channel(led_manager.config.color_order[1]);
	led_manager._bPos = led_manager_get_channel(led_manager.config.color_order[2]);

	led_manager._width_33 = led_manager.config.image_width / 3;
	led_manager._height_33 = led_manager.config.image_height / 3;
	led_manager._width_66 = led_manager._width_33 * 2;
	led_manager._height_66 = led_manager._height_33 * 2;
	led_manager._width_50 = led_manager.config.image_width / 2;
	led_manager._height_50 = led_manager.config.image_height / 2;

	led_manager_calculate_area();

	return led_manager.leds_count;
}

int led_manager_set_profile(const led_manager_profile_t* profile)
{
	memcpy(&led_manager.new_profile, profile, sizeof(led_manager_profile_t));
	led_manager._profile_changed = 1;
	return 0;
}

int led_manager_get_profile(led_manager_profile_t* profile)
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
	return led_manager.state || led_manager.current_intensity;
}

int led_manager_set_intensity(unsigned int intensity, unsigned int current)
{
	if (intensity <= 100) {
		if (current) {
			led_manager.current_intensity = intensity;
			led_manager.new_intensity = intensity;
			led_manager.fintensity = led_manager.current_intensity / 100.0;
			return 1;
		}
		else {
			if (intensity != led_manager.new_intensity) {
				led_manager.new_intensity = intensity;
				return 1;
			}
		}
		return 0;
	}
	return -1;
}

int led_manager_get_intensity()
{
	return led_manager.new_intensity;
}

int led_manager_print_area(char* buffer)
{
	char buff[500] = {};
	sprintf(buffer, "%d x %d \n", led_manager.config.image_width, led_manager.config.image_height);
	for (int i = 0; i < led_manager.leds_count; i++) {
		memset(buff, 0, sizeof(buff));
		sprintf(buff, "LED %02d: %03d:%03d - %03d:%03d (%03dx%03d %02d)\n", i, led_manager.leds[i].x1, led_manager.leds[i].x2, led_manager.leds[i].y1, led_manager.leds[i].y2, led_manager.leds[i].x2 - led_manager.leds[i].x1, led_manager.leds[i].y2 - led_manager.leds[i].y1, (led_manager.leds[i].x2 - led_manager.leds[i].x1) * (led_manager.leds[i].y2 - led_manager.leds[i].y1));
		strcat(buffer, buff);
	}
	return 0;
}