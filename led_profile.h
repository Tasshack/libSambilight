/*
 *  tasshack
 *  (c) 2019 - 2022
 *
 *  License: GPLv3
 *
 */

#ifndef LED_PROFILE_H
#define LED_PROFILE_H

typedef struct {
	unsigned short index;
	char name[10];
	unsigned short saturation_gain_percent;
	unsigned short value_gain_percent;
	short brightness_correction;
	unsigned short horizontal_depth_percent;
	unsigned short vertical_depth_percent;
	short overlap_percent;
	short h_padding_percent;
	short v_padding_percent;
} led_manager_profile_t;

static led_manager_profile_t led_profiles[10] = {
	{
		1,		/// index
		"16:9",	/// name
		130,	/// saturation_gain_percent
		90,		/// value_gain_percent
		-7,		/// brightness_correction
		26,		/// horizontal_depth_percent
		15,		/// vertical_depth_percent
		0,		/// overlap_percent
		0,		/// h_padding_percent
		0,		/// v_padding_percent
	},
	{
		2,		/// index
		"21:9",	/// name
		120,	/// saturation_gain_percent
		90,		/// value_gain_percent
		-7,		/// brightness_correction
		15,		/// horizontal_depth_percent
		7,		/// vertical_depth_percent
		0,		/// overlap_percent
		12,		/// h_padding_percent
		0,		/// v_padding_percent
	},
	{
		3,		/// index
		"4:3",	/// name
		100,	/// saturation_gain_percent
		100,	/// value_gain_percent
		0,		/// brightness_correction
		12,		/// horizontal_depth_percent
		12,		/// vertical_depth_percent
		0,		/// overlap_percent
		0,		/// h_padding_percent
		12,		/// v_padding_percent
	},
	{
		4,		/// index
		"3:2",	/// name
		100,	/// saturation_gain_percent
		100,	/// value_gain_percent
		0,		/// brightness_correction
		12,		/// horizontal_depth_percent
		12,		/// vertical_depth_percent
		0,		/// overlap_percent
		0,		/// h_padding_percent
		8,		/// v_padding_percent
	},
	{ 0,0,0,0,0,0,0,0,0,0 } /// Terminator
};
#endif