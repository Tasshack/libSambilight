/*
 *  tasshack
 *  (c) 2019
 *
 *  License: GPLv3
 *
 */

#ifndef LED_PROFILE_H
#define LED_PROFILE_H

typedef struct {
	unsigned short index;
	const char * name;
	unsigned short saturation_gain_percent;
	unsigned short value_gain_percent;
	unsigned short horizontal_depth_percent;
	unsigned short vertical_depth_percent;
	short overlap_percent; // TODO
	short h_padding_px;
	short v_padding_px;
	unsigned short smoothing_fps; // TODO
	unsigned char black_border; // TODO
} led_manager_profile_t;

static led_manager_profile_t led_profiles[] = {
	{
		1,		/// index
		"16:9",	/// name
		130,	/// saturation_gain_percent
		100,	/// value_gain_percent
		7,		/// horizontal_depth_percent
		12,		/// vertical_depth_percent
		0,		/// overlap_percent
		0,		/// h_padding_px
		0,		/// v_padding_px
		0,		/// smoothing_fps
		0		/// black_border
	},
	{
		2,		/// index
		"21:9",	/// name
		130,	/// saturation_gain_percent
		100,	/// value_gain_percent
		7,		/// horizontal_depth_percent
		12,		/// vertical_depth_percent
		0,		/// overlap_percent
		10,		/// h_padding_px
		0,		/// v_padding_px
		0,		/// smoothing_fps
		0		/// black_border
	},
	{
		3,		/// index
		"4:3",	/// name
		120,	/// saturation_gain_percent
		100,	/// value_gain_percent
		7,		/// horizontal_depth_percent
		12,		/// vertical_depth_percent
		0,		/// overlap_percent
		0,		/// h_padding_px
		20,		/// v_padding_px
		0,		/// smoothing_fps
		0		/// black_border
	},
	{
		4,		/// index
		"3:2",	/// name
		120,	/// saturation_gain_percent
		100,	/// value_gain_percent
		7,		/// horizontal_depth_percent
		12,		/// vertical_depth_percent
		0,		/// overlap_percent
		0,		/// h_padding_px
		12,		/// v_padding_px
		0,		/// smoothing_fps
		0		/// black_border
	},
	//{
	//	5,		/// index
	//	"Game",	/// name
	//	150,	/// saturation_gain_percent
	//	100,	/// value_gain_percent
	//	10,		/// horizontal_depth_percent
	//	20,		/// vertical_depth_percent
	//	0,		/// overlap_percent
	//	15,		/// h_padding_px
	//	30,		/// v_padding_px
	//	0,		/// smoothing_fps
	//	0		/// black_border
	//},
	//{
	//	6,		/// index
	//	"Auto",	/// name
	//	150,	/// saturation_gain_percent
	//	100,	/// value_gain_percent
	//	10,		/// horizontal_depth_percent
	//	20,		/// vertical_depth_percent
	//	0,		/// overlap_percent
	//	0,		/// h_padding_px
	//	0,		/// v_padding_px
	//	0,		/// smoothing_fps
	//	0		/// black_border
	//},
	{ 0,0,0,0,0,0,0,0,0,0 } /// Terminator
};
#endif