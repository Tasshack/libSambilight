/*
 *  bugficks
 *	(c) 2013
 *
 *  sectroyer
 *	(c) 2014
 *
 *  zoelechat
 *	(c) 2015
 *
 *  tasshack
 *  (c) 2019 - 2021
 *
 *  License: GPLv3
 *
 */
 //////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h> 
#include <sys/stat.h>  
#include <fcntl.h>
#include <dlfcn.h>
#include <glob.h>
#include <stdarg.h>
#include <pthread.h>
#include <math.h>
#include <sys/syscall.h>
#include <linux/kdev_t.h>

//////////////////////////////////////////////////////////////////////////////

#define LIB_NAME "Sambilight"
#define LIB_VERSION "v1.2.9"
#define LIB_TV_MODELS "E/F/H"
#define LIB_HOOKS sambilight_hooks
#define hCTX sambilight_hook_ctx

#include "common.h"
#include "hook.h"
#include "led_manager.h"
#include "util.h"
#include "jsmn.h"


unsigned char osd_enabled = 1, black_border_state = 1, black_border_enabled = 1, external_led_state = 1, external_led_enabled = 0, tv_remote_enabled = 1, gfx_lib = 1, test_pattern = 0, default_profile = 0, test_capture = 0;
unsigned long fps_test_frames = 0, capture_frequency = 30;
int serial = -1;
void* h;
led_manager_config_t led_config = { 35, 19, 7, 68, 480, 270, "RGB", 0, 1 };

STATIC int show_msg_box(const char* text);

//////////////////////////////////////////////////////////////////////////////

typedef union {
	const void* procs[48];
	struct {
		const int(*SdDisplay_CaptureScreenE)(int*, unsigned char*, int*);
		const int(*SdDisplay_CaptureScreenF)(int*, unsigned char*, int*, int);
		const int(*SdDisplay_CaptureScreenH)(int*, unsigned char*, int*, int*, int);

		void* (*operator_new)(unsigned int);
		const int (*SCGC_Construct)(void* this, int);
		const int (*SCGC_Create)(void* this);
		const int (*SCGC_Destroy)(void* this);
		const int (*SCGC_Destruct)(void* this);
		const int (*SCBaseGC_Clear)(void* this, int);
		const int (*SCBaseGC_Flush)(void* this, int);
		const int (*SCGC_FillRect)(void*, unsigned int, int, int, int, int);
		const int (*SCGC_DrawText)(void* this, int x, int y, unsigned short* text, int one);
		const int (*SCGC_SetFontSize)(void* this, unsigned int size);
		const int (*SCGC_SetFont)(void* this, void* scfont);
		const int (*SCGC_SetFontStyle)(void* this, unsigned int style);
		const int (*SCGC_SetFgColor)(void* this, unsigned int color);
		void* (*CUSBAppResUtil_GetDefaultFont)(void);

		int* g_IPanel;
		void* g_TaskManager;
		void** g_pTaskManager;
		void** g_TVViewerMgr;
		void** g_pTVViewerMgr;
		int** g_TVViewerWStringEng;
		int** g_WarningWStringEng;
		void** g_pCustomTextResMgr;
		int** g_CustomTextWStringEng;
		void* (*CResourceManager_GetWString)(void* this, int);
		const char* (*PCWString_Convert2)(unsigned short*, char*, int, int, int*);
		void* (*CTaskManager_GetApplication)(void* this, int);
		void* (*CViewerApp_GetViewerManager)(void* this);
		void* (*ALMOND0300_CDesktopManager_GetInstance)(int);
		void* (*ALMOND0300_CDesktopManager_GetApplication)(void* this, char*, int);
		const int(*CViewerManager_ShowSystemAlert)(void* this, int, int);
		const int(*CViewerManager_ShowSystemAlertB)(void* this, int);
		const int(*CViewerManager_ShowCustomText)(void* this, int);
		const int(*CViewerManager_ShowCustomTextF)(void* this, int, int);
		const int(*CViewerManager_ShowRCMode)(void* this, int);
		const int(*CCustomTextApp_t_OnActivated)(void* this, char*, int);

		void* (*MsOS_PA2KSEG0)(int);
		const int (*MsOS_Dcache_Flush)(void*, int);
		void* (*MApi_MMAP_GetInfo)(int, int);
		void (*MApi_XC_W2BYTEMSK)(int, int, int);
		const int (*gfx_InitNonGAPlane)(void*, unsigned int, unsigned int, int, int);
		const int (*gfx_ReleasePlane)(void*, int);
		const int (*gfx_CaptureFrame)(void*, int, int, unsigned int, unsigned int, unsigned int, unsigned int, int, int, int, int);
		const int (*gfx_BitBltScale)(void*, unsigned int, unsigned int, unsigned int, unsigned int, void*, int, int, unsigned int, unsigned int);
		const int (*MApi_GOP_DWIN_CaptureOneFrame)();
		const int (*MApi_GOP_DWIN_GetWinProperty)(void*);
	};
} samyGO_whacky_t;

samyGO_whacky_t hCTX =
{
	// libScreenShot (libSDAL.so)
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP23SdDisplay_CaptureInfo_t",
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP23SdDisplay_CaptureInfo_t12SdMainChip_k",
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP24SdVideoCommonFrameData_tP8SdRect_t12SdMainChip_k",

	// libText
	(const void*)"_Znwj",
	(const void*)"_ZN4SCGCC2El",
	(const void*)"_ZN8SCBaseGC6CreateEv",
	(const void*)"_ZN4SCGC7DestroyEv",
	(const void*)"_ZN4SCGCD2Ev",
	(const void*)"_ZN4SCGC5ClearEPN6Shadow9BasicType4RectE",
	(const void*)"_ZN8SCBaseGC5FlushEPN6Shadow9BasicType4RectE",
	(const void*)"_ZN4SCGC8FillRectEN6Shadow5ColorEllll",
	(const void*)"_ZN4SCGC8DrawTextEllPtl",
	(const void*)"_ZN4SCGC11SetFontSizeEl",
	(const void*)"_ZN4SCGC7SetFontEP6SCFont",
	(const void*)"_ZN4SCGC12SetFontStyleEl",
	(const void*)"_ZN4SCGC10SetFgColorEN6Shadow5ColorE",
	(const void*)"_ZN14CUSBAppResUtil14GetDefaultFontEv",

	// libAlert
	(const void*)"g_IPanel",
	(const void*)"g_TaskManager",
	(const void*)"g_pTaskManager",
	(const void*)"g_TVViewerMgr",
	(const void*)"g_pTVViewerMgr",
	(const void*)"g_TVViewerWStringEng",
	(const void*)"g_WarningWStringEng",
	(const void*)"g_pCustomTextResMgr",
	(const void*)"g_CustomTextWStringEng",
	(const void*)"_ZN16CResourceManager10GetWStringEi",
	(const void*)"_ZN9PCWString7ConvertEPtPKciiPi",
	(const void*)"_ZN12CTaskManager14GetApplicationE15DTV_APPLICATION",
	(const void*)"_ZN10CViewerApp16GetViewerManagerEv",
	(const void*)"_ZN10ALMOND030015CDesktopManager11GetInstanceENS_13EDesktopIndexE",
	(const void*)"_ZN10ALMOND030015CDesktopManager14GetApplicationEPKcS2_",
	(const void*)"_ZN14CViewerManager15ShowSystemAlertEii",
	(const void*)"_ZN14CViewerManager15ShowSystemAlertEi",
	(const void*)"_ZN14CViewerManager14ShowCustomTextEi",
	(const void*)"_ZN14CViewerManager14ShowCustomTextEib",
	(const void*)"_ZN14CViewerManager10ShowRCModeEi",
	(const void*)"_ZN14CCustomTextApp13t_OnActivatedEPKci",

	// libUTOPIA.so
	(const void*)"MsOS_PA2KSEG0",
	(const void*)"MsOS_Dcache_Flush",
	(const void*)"MApi_MMAP_GetInfo",
	(const void*)"MApi_XC_W2BYTEMSK",
	(const void*)"gfx_InitNonGAPlane",
	(const void*)"gfx_ReleasePlane",
	(const void*)"gfx_CaptureFrame",
	(const void*)"gfx_BitBltScale",
	(const void*)"MApi_GOP_DWIN_CaptureOneFrame",
	(const void*)"MApi_GOP_DWIN_GetWinProperty"
};

_HOOK_IMPL(int, CViewerApp_t_OnInputOccur, void* this, int* a2)
{
	char key = (char)a2[5];
	const char* msgHeader = "Ambient Lighting";
	char msg[100];
	unsigned int index;

	switch (key) {
	case 108: // KEY_RED
		if (led_manager_get_state()) {
			led_manager_set_state(0);
			sprintf(msg, "%s %s", msgHeader, "Off");
		}
		else {
			led_manager_set_state(1);
			sprintf(msg, "%s %s", msgHeader, "On");
		}
		show_msg_box(msg);
		return 1;
	case 20: // KEY_GREEN
		index = led_manager_get_profile_index();
		if (led_profiles[index].index > 0) {
			if (black_border_enabled && black_border_state) {
				black_border_state = 0;
			}
			else {
				index++;
			}
		}
		else {
			if (black_border_enabled) {
				black_border_state = !black_border_state;
			}
			index = 1;
		}

		if (black_border_state) {
			sprintf(msg, "%s Auto Mode", msgHeader);
		}
		else {
			sprintf(msg, "%s %s Mode", msgHeader, led_profiles[index - 1].name);
		}
		show_msg_box(msg);

		led_manager_set_profile(&led_profiles[index - 1]);
		return 1;
	case 21: // KEY_YELLOW
		switch (led_manager_get_intensity()) {
		case 100:
			led_manager_set_intensity(75, 0);
			break;
		case 75:
			led_manager_set_intensity(50, 0);
			break;
		case 50:
			led_manager_set_intensity(25, 0);
			break;
		default:;
			led_manager_set_intensity(100, 0);
			break;
		}
		sprintf(msg, "%s %%%d", msgHeader, led_manager_get_intensity());
		show_msg_box(msg);
		return 1;
	case 22: // KEY_BLUE
		if (external_led_enabled) {
			if (external_led_state) {
				external_led_state = 0;
			}
			else {
				external_led_state = 1;
			}
			sprintf(msg, "External %s %s", msgHeader, external_led_state ? "On" : "Off");
			show_msg_box(msg);
			return 1;
		}
	default:;
	}

	_HOOK_DISPATCH(CViewerApp_t_OnInputOccur, this, a2);
	return (int)h_ret;
}

STATIC dyn_fn_t dyn_hook_fn_tab[] =
{
	{ 0, "_ZN10CViewerApp14t_OnInputOccurEPK7PTEvent" },
};

STATIC hook_entry_t LIB_HOOKS[] =
{
#define _HOOK_ENTRY(F, I) \
    &hook_##F, &dyn_hook_fn_tab[I], &x_##F
	{ _HOOK_ENTRY(CViewerApp_t_OnInputOccur, __COUNTER__) }
#undef _HOOK_ENTRY
};

STATIC int _hooked = 0;
EXTERN_C void lib_deinit(void* _h)
{
	if (_hooked)
		remove_hooks(LIB_HOOKS, ARRAYSIZE(LIB_HOOKS));

	_hooked = 0;
	log("%s\n", __func__);
}


static int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

void load_profiles_config(const char* path) {
	int length, index = 0, s;
	char* ptr, * json_buffer, tmp[50];
	jsmn_parser parser;
	jsmntok_t tokens[128];
	FILE* config_file;

	config_file = fopen(path, "r");
	if (config_file) {
		fseek(config_file, 0, SEEK_END);
		length = ftell(config_file);
		fseek(config_file, 0, SEEK_SET);
		json_buffer = malloc(length);
		s = fread(json_buffer, 1, length, config_file);
		fclose(config_file);

		jsmn_init(&parser);
		int r = jsmn_parse(&parser, json_buffer, length, tokens, sizeof(tokens) / sizeof(tokens[0]));
		if (r >= 0 && tokens[0].type == JSMN_ARRAY) {
			for (int i = 1; i < r; i++) {
				if (tokens[i].type == JSMN_OBJECT) {
					index++;
					if (index == 1) {
						memset(led_profiles, 0, sizeof(led_profiles));
					}
					led_profiles[index - 1].index = index;

					log("-------------%d-------------\n", index);
				}
				else if (index) {
					if (jsoneq(json_buffer, &tokens[i], "name") == 0) {
						s = tokens[i + 1].end - tokens[i + 1].start;
						if (s > sizeof(led_profiles[index - 1].name) - 1) {
							s = sizeof(led_profiles[index - 1].name) - 1;
						}
						strncpy(led_profiles[index - 1].name, json_buffer + tokens[i + 1].start, s);

						log("Name: %s\n", led_profiles[index - 1].name);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "saturation_gain_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 1) {
							s = 1;
						}
						led_profiles[index - 1].saturation_gain_percent = s;

						log("Saturation Gain: %d%%\n", led_profiles[index - 1].saturation_gain_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "value_gain_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 1) {
							s = 1;
						}
						led_profiles[index - 1].value_gain_percent = s;

						log("Value Gain: %d%%\n", led_profiles[index - 1].value_gain_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "brightness_correction") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < -255) {
							s = -255;
						}
						else if (s > 255) {
							s = 255;
						}
						led_profiles[index - 1].brightness_correction = s;

						log("Brightness Correction: %d\n", led_profiles[index - 1].brightness_correction);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "horizontal_depth_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].horizontal_depth_percent = s;

						log("Horizontal Depth: %d%%\n", led_profiles[index - 1].horizontal_depth_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "vertical_depth_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].vertical_depth_percent = s;

						log("Vertical Depth: %d%%\n", led_profiles[index - 1].vertical_depth_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "overlap_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].overlap_percent = s;

						log("Overlap: %d%%\n", led_profiles[index - 1].overlap_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "h_padding_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].h_padding_percent = strtol(tmp, &ptr, 10);

						log("Horizontal Padding: %d%%\n", led_profiles[index - 1].h_padding_percent);
						i++;
					}
					else if (jsoneq(json_buffer, &tokens[i], "v_padding_percent") == 0) {
						memset(tmp, 0, sizeof(tmp));
						strncpy(tmp, json_buffer + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
						s = strtol(tmp, &ptr, 10);
						if (s < 0) {
							s = 0;
						}
						else if (s > 100) {
							s = 100;
						}
						led_profiles[index - 1].v_padding_percent = s;

						log("Vertical Padding: %d%%\n", led_profiles[index - 1].v_padding_percent);
						i++;
					}
				}
			}
			log("%d Profile(s) Loaded From Config\n", index);
		}
		else {
			log("Failed To Parse Config!\n");
		}

		free(json_buffer);
	}
}

static int insmod(const char* module_name) {
	int fd, ret = -1;
	size_t image_size;
	struct stat st;
	void* image;
	char* params = "";
	fd = open(module_name, O_RDONLY);
	if (fd >= 0) {
		fstat(fd, &st);
		image_size = st.st_size;
		image = malloc(image_size);
		ret = read(fd, image, image_size);
		ret = syscall(__NR_init_module, image, image_size, params);
		usleep(100000);
		free(image);
		close(fd);
	}
	return ret;
}

static int mknod_acm(const char* device_name) {
	char dev[100] = "";
	char cmd[150] = "";
	FILE* fp;
	char* ptr;
	char master_str[10] = "", alias_str[10] = "";
	uint16_t master, alias;

	if (access(device_name, F_OK) == 0) {
		return 0;
	}

	memset(dev, 0, sizeof(dev));
	sprintf(dev, "/sys/class/tty/%s", device_name + 4);
	if (access(dev, F_OK) != 0) {
		return -1;
	}

	sprintf(cmd, "cat %s/dev", dev);
	fp = popen(cmd, "r");
	memset(dev, 0, sizeof(dev));
	ptr = fgets(dev, sizeof(dev), fp);
	fclose(fp);

	ptr = strstr(dev, ":");

	memcpy(master_str, dev, ptr - dev);
	master_str[ptr - dev] = 0;

	memcpy(alias_str, ptr + 1, strlen(dev) - (ptr - dev) - 2);
	alias_str[strlen(dev) - (ptr - dev) - 2] = 0;

	master = atol(master_str);
	alias = atol(alias_str);

	//mknod /dtv/ttyACM0 c $(echo $(cat /sys/class/tty/ttyACM0/dev) | tr ":" "\n")
	return mknod(device_name, S_IFCHR | 0666, MKDEV(master, alias));
}

static int open_serial(const char* device, unsigned int baudrate) {
	int fd = -1, i;
	char dev[50] = "";

	if (strlen(device)) {
		strcpy(dev, device);

		if (strstr(dev, "ACM")) {
			insmod("/lib/modules/cdc-acm.ko");
			mknod_acm(device);
		}
		else {
			insmod("/lib/modules/usbserial.ko");
			insmod("/lib/modules/ftdi_sio.ko");
		}
		fd = open(dev, O_RDWR | O_NOCTTY);

		if (fd < 0) {
			log("Could not open serial port %s!\n", dev);
		}
	}
	else {
		insmod("/lib/modules/usbserial.ko");
		insmod("/lib/modules/ftdi_sio.ko");

		for (i = 0; i < 3 && fd < 0; i++) {
			memset(dev, 0, sizeof(dev));
			sprintf(dev, "/dev/ttyUSB%d", i);
			fd = open(dev, O_RDWR | O_NOCTTY);
		}

		if (fd < 0) {
			insmod("/lib/modules/cdc-acm.ko");

			for (i = 0; i < 3 && fd < 0; i++) {
				memset(dev, 0, sizeof(dev));
				sprintf(dev, "/dtv/ttyACM%d", i);
				if (mknod_acm(dev) == 0) {
					fd = open(dev, O_RDWR | O_NOCTTY);
				}
			}
		}

		if (fd < 0) {
			log("Could not find serial port!\n");
		}
	}

	if (fd >= 0) {
		struct termios port_settings;
		tcgetattr(fd, &port_settings);

		port_settings.c_cflag &= ~PARENB;
		port_settings.c_cflag &= ~CSTOPB;
		port_settings.c_cflag |= CS8;
		port_settings.c_cflag &= ~CRTSCTS;
		port_settings.c_cflag |= CREAD | CLOCAL;
		port_settings.c_cflag &= ~HUPCL;
		port_settings.c_lflag &= ~ICANON;
		port_settings.c_lflag &= ~ECHO;
		port_settings.c_lflag &= ~ECHOE;
		port_settings.c_lflag &= ~ECHONL;
		port_settings.c_lflag &= ~ISIG;
		port_settings.c_iflag &= ~(IXON | IXOFF | IXANY);
		port_settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
		port_settings.c_oflag &= ~OPOST;
		port_settings.c_oflag &= ~ONLCR;

		if (strstr(dev, "ACM")) {
			log("Serial acm available %s\n", dev);
		}
		else {
			switch (baudrate) {
			case 2000000:
				cfsetispeed(&port_settings, B2000000);
				cfsetospeed(&port_settings, B2000000);
				break;
			case 1000000:
				cfsetispeed(&port_settings, B1000000);
				cfsetospeed(&port_settings, B1000000);
				break;
			case 576000:
				cfsetispeed(&port_settings, B576000);
				cfsetospeed(&port_settings, B576000);
				break;
			case 500000:
				cfsetispeed(&port_settings, B500000);
				cfsetospeed(&port_settings, B500000);
				break;
			case 460800:
				cfsetispeed(&port_settings, B460800);
				cfsetospeed(&port_settings, B460800);
				break;
			case 230400:
				cfsetispeed(&port_settings, B230400);
				cfsetospeed(&port_settings, B230400);
				break;
			case 115200:
				cfsetispeed(&port_settings, B115200);
				cfsetospeed(&port_settings, B115200);
				break;
			default:;
				cfsetispeed(&port_settings, B921600);
				cfsetospeed(&port_settings, B921600);
			}

			port_settings.c_cc[VTIME] = 1;
			port_settings.c_cc[VMIN] = 0;

			log("Serial available %s @%u\n", dev, baudrate);
		}

		tcsetattr(fd, TCSANOW, &port_settings);
	}
	return fd;
}

void save_capture(const unsigned char* frame, unsigned int width, unsigned int heigth, const char* color_order, unsigned int capture_pos) {
	unsigned char bmpHeader[54] = { 0x42, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char* buffer;
	int i, j, r = 2, g = 1, b = 0;
	unsigned int bufSize, in, out;
	FILE* file;

	unsigned char* bmpSize = (unsigned char*)&bufSize;
	unsigned char* bmpWidth = (unsigned char*)&width;
	unsigned char* bmpHeight = (unsigned char*)&heigth;

	for (i = 0; i < 3; i++) {
		switch (color_order[i]) {
		case 'b':
		case 'B':
			b = i;
			break;
		case 'g':
		case 'G':
			g = i;
			break;
		case 'r':
		case 'R':
			r = i;
			break;
		default:;
		}
	}

	bmpHeader[2] = bmpSize[0];
	bmpHeader[3] = bmpSize[1];
	bmpHeader[4] = bmpSize[2];
	bmpHeader[5] = bmpSize[3];
	bmpHeader[18] = bmpWidth[0];
	bmpHeader[19] = bmpWidth[1];
	bmpHeader[22] = bmpHeight[0];
	bmpHeader[23] = bmpHeight[1];

	bufSize = 4 * width * heigth + sizeof(bmpHeader);
	buffer = malloc(bufSize);
	memset(buffer, 0, bufSize);
	memcpy(buffer, bmpHeader, sizeof(bmpHeader));

	if (capture_pos == 0) {
		r -= 4;
		g -= 4;
		b -= 4;
	}

	for (i = 0; i < heigth; i++) {
		for (j = 0; j < width * 4; j += 4) {
			out = i * width * 4 + j + sizeof(bmpHeader);
			if (capture_pos == 1) {
				in = ((heigth - 1) * width * 4) - (i * (width * 4)) + (j);
			}
			else {
				in = (i * width * 4) + (width * 4) - j;
			}

			buffer[out + 2] = frame[in + b];
			buffer[out + 1] = frame[in + g];
			buffer[out + 0] = frame[in + r];
		}
	}

	file = fopen("/dtv/Sambilight.bmp", "wb");
	if (file) {
		fseek(file, 0, SEEK_SET);
		fwrite(buffer, 1, bufSize, file);
		fclose(file);

		log("Capture saved to /dtv/Sambilight.bmp\n");
	}

	free(buffer);
}

void render_areas(const led_manager_led_t* leds, unsigned short leds_count, unsigned short width, unsigned short height) {
	int x, y, h, w, i;
	char text[10] = "";
	unsigned short text16[8] = {};
	unsigned int color = 0;
	unsigned char* colorPtr = (unsigned char*)&color;

	void* scgc = hCTX.operator_new(0x170);
	hCTX.SCGC_Construct(scgc, 0);
	hCTX.SCGC_Create(scgc);
	hCTX.SCBaseGC_Clear(scgc, 0);
	hCTX.SCGC_SetFont(scgc, hCTX.CUSBAppResUtil_GetDefaultFont());

	// TODO: 10pt font will not work on H
	if (hCTX.SdDisplay_CaptureScreenH == NULL) {
		hCTX.SCGC_SetFontSize(scgc, 10);
		hCTX.SCGC_SetFontStyle(scgc, 0);
	}

	colorPtr[3] = 190;

	for (i = leds_count - 1; i >= 0; i--) {
		memset(text, 0, sizeof(text));
		sprintf(text, "%03u", i);

		switch (i % 13) {
		case 1:
			colorPtr[2] = 255;
			colorPtr[1] = 0;
			colorPtr[0] = 0;
			break;
		case 2:
			colorPtr[2] = 0;
			colorPtr[1] = 255;
			colorPtr[0] = 0;
			break;
		case 3:
			colorPtr[2] = 0;
			colorPtr[1] = 0;
			colorPtr[0] = 255;
			break;
		case 4:
			colorPtr[2] = 255;
			colorPtr[1] = 0;
			colorPtr[0] = 255;
			break;
		case 5:
			colorPtr[2] = 0;
			colorPtr[1] = 255;
			colorPtr[0] = 255;
			break;
		case 6:
			colorPtr[2] = 255;
			colorPtr[1] = 255;
			colorPtr[0] = 0;
			break;
		case 7:
			colorPtr[2] = 128;
			colorPtr[1] = 0;
			colorPtr[0] = 255;
			break;
		case 8:
			colorPtr[2] = 0;
			colorPtr[1] = 128;
			colorPtr[0] = 255;
			break;
		case 9:
			colorPtr[2] = 128;
			colorPtr[1] = 255;
			colorPtr[0] = 0;
			break;
		case 10:
			colorPtr[2] = 255;
			colorPtr[1] = 0;
			colorPtr[0] = 128;
			break;
		case 11:
			colorPtr[2] = 0;
			colorPtr[1] = 255;
			colorPtr[0] = 128;
			break;
		case 12:
			colorPtr[2] = 255;
			colorPtr[1] = 128;
			colorPtr[0] = 0;
			break;
		default:
			colorPtr[2] = 255;
			colorPtr[1] = 255;
			colorPtr[0] = 255;
			break;
		}

		x = floor((width * leds[i].x1) / (float)led_config.image_width / 1.5);
		y = floor((height * leds[i].y1) / (float)led_config.image_height / 1.5);
		w = ceil((width * (leds[i].x2 - leds[i].x1)) / (float)led_config.image_width / 1.5);
		h = ceil((height * (leds[i].y2 - leds[i].y1)) / (float)led_config.image_height / 1.5);

		hCTX.SCGC_FillRect(scgc, color, x, y, w, h);
		hCTX.SCGC_SetFgColor(scgc, 0xFF000000);
		hCTX.PCWString_Convert2(text16, text, 3, 1, 0);

		// TODO: 10pt font will not work on H
		if (hCTX.SdDisplay_CaptureScreenH == NULL) {
			hCTX.SCGC_DrawText(scgc, x + (w / 2) - 8, y + (h / 2) + 5, text16, 3);
		}
	}
	hCTX.SCBaseGC_Flush(scgc, 0);
	hCTX.SCGC_Destroy(scgc);
	hCTX.SCGC_Destruct(scgc);
}

void* sambiligth_thread(void* params) {
	int capture_info[20] = { 0 }, panel_size[4] = { 0, 0, 1920, 1080 }, cd_size[2] = { 0, 0 }, win_property[20];
	unsigned char* buffer, * data, init = 0;
	unsigned long fps_counter = 0, counter = 0, c, data_size, leds_count, fps_test_remaining_frames = 0, bytesWritten, bytesRemaining;
	unsigned short header_size = 6, h_border = 0, v_border = 0, h_new_border = 0, v_new_border = 0, scale = 2;
	clock_t capture_begin, capture_end, fps_begin, fps_end, elapsed;
	void* frame_buffer = NULL, * out_buffer = NULL;
	int* out_buffer_info = NULL, * frame_buffer_info = NULL;
	typedef int func(void);

	if (fps_test_frames != 1) {
		pthread_detach(pthread_self());
	}

	leds_count = led_manager_init(&led_config, &led_profiles[default_profile]);
	log("Led manager init\n");

	data_size = (leds_count * 3) + header_size;
	if (external_led_enabled) {
		data_size++;
	}

	data = malloc(data_size);
	memcpy(data, "Ada", 3);
	data[3] = ((leds_count - 1) >> 8) & 0xff;
	data[4] = (leds_count - 1) & 0xff;
	data[5] = data[3] ^ data[4] ^ 0x55;
	memset(data + header_size, 0, data_size - header_size);

	if (external_led_enabled) {
		data[data_size - 1] = external_led_state;
	}
	capture_frequency *= 1000;

	cd_size[0] = led_config.image_width;
	cd_size[1] = led_config.image_height;

	if (hCTX.g_IPanel) {
		panel_size[2] = ((func*)hCTX.g_IPanel[3])();
		panel_size[3] = ((func*)hCTX.g_IPanel[4])();
		log("Panel size %dx%d\n", panel_size[2], panel_size[3]);
	}

	gfx_lib = gfx_lib && fps_test_frames != 1 && hCTX.SdDisplay_CaptureScreenH == NULL && hCTX.gfx_InitNonGAPlane && hCTX.MsOS_PA2KSEG0 && hCTX.MApi_MMAP_GetInfo && hCTX.gfx_CaptureFrame && hCTX.MApi_GOP_DWIN_CaptureOneFrame && hCTX.MApi_GOP_DWIN_GetWinProperty;

	if (gfx_lib) {
		log("gfx_lib enabled\n");
	}
	else {
		log("gfx_lib not enabled\n");
	}

	scale = panel_size[2] / led_config.image_width;

	if (gfx_lib) {
		frame_buffer_info = hCTX.MApi_MMAP_GetInfo(59, 0);
		frame_buffer = hCTX.MsOS_PA2KSEG0(*frame_buffer_info);
	}

	if (gfx_lib && scale > 1) {
		out_buffer_info = hCTX.MApi_MMAP_GetInfo(60, 0);
		out_buffer = hCTX.MsOS_PA2KSEG0(*out_buffer_info);
		buffer = out_buffer;
	}
	else {
		buffer = malloc(4 * led_config.image_width * led_config.image_height);
		memset(buffer, 0, 4 * led_config.image_width * led_config.image_height);
	}

	if (fps_test_frames > 0) {
		if (fps_test_frames == 1) {
			log("Test Started\n");
			led_manager_set_intensity(100, 1);
		}
		else {
			log("Test Started for %ld frames\n", fps_test_frames);
		}
		fps_test_remaining_frames = fps_test_frames + 1;
		fps_begin = clock();
	}

	if (test_pattern) {
		log("Render test pattern\n");
		render_areas(led_manager_get_leds(), leds_count, panel_size[2], panel_size[3]);
	}

	usleep(10000);
	log("Grabbing started\n");

	capture_begin = clock();

	while (1) {
		if (led_manager_get_state()) {
			if (gfx_lib) {
				if (osd_enabled) {
					hCTX.MApi_XC_W2BYTEMSK(4166, 0x8000, 0x8000);
					hCTX.MApi_XC_W2BYTEMSK(4310, 0x2000, 0x2000);
				}

				hCTX.gfx_InitNonGAPlane(frame_buffer, panel_size[2], panel_size[3], 32, 0);

				hCTX.MApi_GOP_DWIN_GetWinProperty(win_property);
				if (win_property[4] != panel_size[2] / 2) {
					hCTX.gfx_CaptureFrame(frame_buffer, 0, 2, 0, 0, panel_size[2], panel_size[3], 0, 1, 1, 0);
					if (init) {
						usleep(10000);
						hCTX.gfx_ReleasePlane(frame_buffer, 0);
						continue;
					}
				}

				if (init) {
					hCTX.MApi_GOP_DWIN_CaptureOneFrame();
				}
				else {
					init = 1;
				}

				if (scale > 1) {
					hCTX.gfx_InitNonGAPlane(out_buffer, led_config.image_width, led_config.image_height, 32, 0);
					hCTX.gfx_BitBltScale(frame_buffer, 0, 0, panel_size[2], panel_size[3], out_buffer, 0, 0, led_config.image_width, led_config.image_height);
					hCTX.gfx_ReleasePlane(out_buffer, 0);
				}
				else {
					memcpy(buffer, frame_buffer, 4 * led_config.image_width * led_config.image_height);
				}
				hCTX.gfx_ReleasePlane(frame_buffer, 0);

				if (osd_enabled) {
					hCTX.MApi_XC_W2BYTEMSK(4166, 0, 0x8000);
					hCTX.MApi_XC_W2BYTEMSK(4310, 0, 0x2000);
				}
			}
			else if (hCTX.SdDisplay_CaptureScreenF) {
				hCTX.SdDisplay_CaptureScreenF(cd_size, buffer, capture_info, 0);
			}
			else if (hCTX.SdDisplay_CaptureScreenE) {
				hCTX.SdDisplay_CaptureScreenE(cd_size, buffer, capture_info);
			}
			else if (hCTX.SdDisplay_CaptureScreenH) {
				hCTX.SdDisplay_CaptureScreenH(cd_size, buffer, capture_info, panel_size, 0);
			}

			if (led_manager_argb8888_to_leds(buffer, &data[header_size])) {
				if (external_led_enabled) {
					data[data_size - 1] = external_led_state;
				}

				bytesWritten = write(serial, data, data_size);

				fps_counter++;
				if (black_border_enabled && fps_counter >= 30) {
					fps_counter = 0;
					if (black_border_state) {
						if (led_manager_get_borders(buffer, &h_new_border, &v_new_border)) {
							if (h_border != h_new_border || v_border != v_new_border) {
								counter = 0;
								h_border = h_new_border;
								v_border = v_new_border;
							}
							else if (counter == 5) {
								led_manager_profile_t profile;
								profile.index = 1;
								c = 0;
								while (profile.index) {
									profile = led_profiles[c];
									if (abs(profile.h_padding_percent - h_border) <= 2 && abs(profile.v_padding_percent - v_border) <= 2) {
										if (led_manager_get_profile_index() != profile.index) {
											led_manager_set_profile(&profile);
											log("Switched to %s Mode\n", profile.name);
										}
										break;
									}
									c++;
								}
							}
							counter++;
						}
					}
					else {
						counter = 0;
						h_border = 0;
						v_border = 0;
					}
				}
			}

			capture_end = clock();
			elapsed = capture_end - capture_begin;
			if (elapsed < capture_frequency && fps_test_frames != 1) {
				usleep(capture_frequency - elapsed);
			}

			if (fps_test_frames > 0) {
				fps_test_remaining_frames--;
				if (fps_test_remaining_frames == 0) {
					if (fps_test_frames == 1) {
						break;
					}
					fps_end = clock();
					log("FPS: %d\n", fps_test_frames * 1000 / ((fps_end - fps_begin) / 1000));
					fps_test_remaining_frames = fps_test_frames;
					fps_begin = clock();
				}
			}

			capture_begin = clock();
		}
		else {
			usleep(100000);
		}
	}

	if (fps_test_frames == 1 && test_capture) {
		save_capture(buffer, led_config.image_width, led_config.image_height, led_config.color_order, led_config.capture_pos);
	}

	if (gfx_lib) {
		hCTX.MsOS_Dcache_Flush(frame_buffer, frame_buffer_info[1]);
		if (scale > 1) {
			hCTX.MsOS_Dcache_Flush(out_buffer, out_buffer_info[1]);
		}
	}

	if (!gfx_lib || scale <= 1) {
		free(buffer);
	}

	free(data);
	close(serial);

	led_manager_deinit();

	log("Sambilight ended\n");

	dlclose(h);

	if (fps_test_frames != 1) {
		lib_deinit(h);
		pthread_exit(NULL);
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

EXTERN_C void lib_init(void* _h, const char* libpath)
{
	if (_hooked) {
		log("Injecting once is enough!\n");
		return;
	}

	int argc, exit = 0;
	char* argv[512], * optstr, path[PATH_MAX];
	char device[50] = "";
	unsigned long baudrate = 921600;
	pthread_t thread;
	pthread_t ipcthread;
	size_t len;

	argc = getArgCArgV(libpath, argv);

	optstr = getOptArg(argv, argc, "H_LEDS:");
	if (optstr && strlen(optstr))
		led_config.h_leds_count = atol(optstr);

	optstr = getOptArg(argv, argc, "V_LEDS:");
	if (optstr && strlen(optstr))
		led_config.v_leds_count = atol(optstr);

	optstr = getOptArg(argv, argc, "BOTTOM_GAP:");
	if (optstr && strlen(optstr))
		led_config.bottom_gap = atol(optstr);

	optstr = getOptArg(argv, argc, "START_OFFSET:");
	if (optstr && strlen(optstr))
		led_config.start_offset = atol(optstr);

	optstr = getOptArg(argv, argc, "CLOCKWISE:");
	if (optstr && strlen(optstr))
		led_config.led_order = atoi(optstr);

	optstr = getOptArg(argv, argc, "COLOR_ORDER:");
	if (optstr && strlen(optstr) == 3)
		strncpy(led_config.color_order, optstr, 3);

	optstr = getOptArg(argv, argc, "CAPTURE_POS:");
	if (optstr && strlen(optstr))
		led_config.capture_pos = atoi(optstr);

	optstr = getOptArg(argv, argc, "BLACK_BORDER:");
	if (optstr && strlen(optstr))
		black_border_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "CAPTURE_FREQ:");
	if (optstr && strlen(optstr))
		capture_frequency = atoi(optstr);

	optstr = getOptArg(argv, argc, "TV_REMOTE:");
	if (optstr && strlen(optstr))
		tv_remote_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "EXTERNAL:");
	if (optstr && strlen(optstr))
		external_led_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "CAPTURE_SIZE:");
	if (optstr && strlen(optstr)) {
		switch (atoi(optstr)) {
		case 1:
			led_config.image_width = 96;
			led_config.image_height = 54;
			break;
		case 2:
			led_config.image_width = 160;
			led_config.image_height = 90;
			break;
		case 3:
			led_config.image_width = 240;
			led_config.image_height = 135;
			break;
		case 4:
			led_config.image_width = 320;
			led_config.image_height = 180;
			break;
		case 6:
			led_config.image_width = 960;
			led_config.image_height = 540;
			break;
		case 7:
			led_config.image_width = 1920;
			led_config.image_height = 1080;
			break;
		case 5:
		default:
			led_config.image_width = 480;
			led_config.image_height = 270;
			break;
		}
	}

	optstr = getOptArg(argv, argc, "OSD_CAPTURE:");
	if (optstr && strlen(optstr))
		osd_enabled = atoi(optstr);

	optstr = getOptArg(argv, argc, "GFX_LIB:");
	if (optstr && strlen(optstr))
		gfx_lib = atoi(optstr);

	optstr = getOptArg(argv, argc, "DEVICE:");
	if (optstr && strlen(optstr))
		strncpy(device, optstr, sizeof(device));

	optstr = getOptArg(argv, argc, "BAUDRATE:");
	if (optstr && strlen(optstr))
		baudrate = atol(optstr);

	optstr = getOptArg(argv, argc, "TEST_FRAMES:");
	if (optstr && strlen(optstr))
		fps_test_frames = atol(optstr);

	optstr = getOptArg(argv, argc, "TEST_PATTERN:");
	if (optstr && strlen(optstr))
		test_pattern = atoi(optstr);

	optstr = getOptArg(argv, argc, "TEST_PROFILE:");
	if (optstr && strlen(optstr))
		default_profile = atoi(optstr);

	optstr = getOptArg(argv, argc, "TEST_SAVE_CAPTURE:");
	if (optstr && strlen(optstr))
		test_capture = atoi(optstr);


	if (fps_test_frames == 1) {
		tv_remote_enabled = 0;
	}

	unlink(LOG_FILE);
	log("SamyGO "LIB_TV_MODELS" lib"LIB_NAME" "LIB_VERSION" - (c) tasshack 2019 - 2021\n");

	h = dlopen(0, RTLD_LAZY);
	if (!h) {
		char* serr = dlerror();
		log("%s", serr);
		return;
	}

	patch_adbg_CheckSystem(h);
	samyGO_whacky_t_init(h, &hCTX, ARRAYSIZE(hCTX.procs));

	if (tv_remote_enabled) {
		if (dyn_sym_tab_init(h, dyn_hook_fn_tab, ARRAYSIZE(dyn_hook_fn_tab)) >= 0) {
			set_hooks(LIB_HOOKS, ARRAYSIZE(LIB_HOOKS));
			_hooked = 1;
		}
	}
	
	log("Sambilight started\n");

	log("H_LEDS: %d, V_LEDS: %d, BOTTOM_GAP: %d, START_OFFSET: %d, COLOR_ORDER: %s, CAPTURE_POS: %d, CLOCKWISE: %d\n", led_config.h_leds_count, led_config.v_leds_count, led_config.bottom_gap, led_config.start_offset, led_config.color_order, led_config.capture_pos, led_config.led_order);
	log("%dx%d %dms\n", led_config.image_width, led_config.image_height, capture_frequency);

	if (black_border_enabled) {
		log("Black border detection enabled\n");
	}

	if (tv_remote_enabled) {
		log("TV Remote control support enabed\n");
	}

	if (external_led_enabled) {
		log("External led control enabed\n");
	}

	strncpy(path, libpath, PATH_MAX);
	len = strlen(libpath) - 1;
	while (path[len] && path[len] != '.') {
		path[len] = 0;
		len--;
	}
	strncat(path, "config", PATH_MAX);
	load_profiles_config(path);

	serial = open_serial(device, baudrate);
	if (serial >= 0) {
		log("Serial open\n");
		if (fps_test_frames == 1) {
			sambiligth_thread(NULL);
			return;
		}

		pthread_create(&thread, NULL, &sambiligth_thread, NULL);
		return;
	}

	log("Sambilight ended\n");

	lib_deinit(h);
	dlclose(h);
}

//////////////////////////////////////////////////////////////////////////////
STATIC int show_msg_box(const char* text)
{
	void* CViewerManager, * CViewerInstance, * CViewerApp, * CCustomTextApp;

	int newLength, strPos, i, seekLen = 13, strLen = 0;
	unsigned short* tvString = NULL;

	unsigned char stringUtf[512];
	memset(stringUtf, 0, 512 * sizeof(char));

	strncpy(stringUtf, text, 511);

	unsigned short notAvail[] = { 0x004E, 0x006F, 0x0074, 0x0020, 0x0041, 0x0076, 0x0061, 0x0069, 0x006C, 0x0061, 0x0062, 0x006C, 0x0065, 0x0000,	// "Not available\0"
							   0x0050, 0x006C, 0x0065, 0x0061, 0x0073, 0x0065, 0x0020, 0x0067, 0x006F, 0x0020, 0x0069, 0x006E, 0x0074, 0x006F,	// "Please go into"
							   0x0055, 0x0053, 0x0042, 0x0020, 0x0054, 0x0065, 0x0073, 0x0074, 0x0020, 0x003A, 0x0020, 0x004F, 0x004B, 0x0000,	// "USB Test : OK\0"
							   0x004E, 0x006F, 0x0074, 0x0020, 0x0046, 0x006F, 0x0072, 0x0020, 0x0053, 0x0061, 0x006C, 0x0065, 0x0000, 0x0000,	// "Not For Sale\0\0"
							   0x0055, 0x0053, 0x0042, 0x0020, 0x0053, 0x0075, 0x0063, 0x0063, 0x0065, 0x0073, 0x0073, 0x0000, 0x0000, 0x0000 };	// "USB Success\0\0\0"

	if (hCTX.ALMOND0300_CDesktopManager_GetInstance) {
		CViewerInstance = hCTX.ALMOND0300_CDesktopManager_GetInstance(1);
		CViewerApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.tvviewer", 0);
		int* CSystemAlertApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.systemalert", 0);
		CSystemAlertApp[0x4b14 / 4] = 1;
		CCustomTextApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.rcmode", 0);
	}
	else if (hCTX.g_pTaskManager)
		CViewerApp = hCTX.CTaskManager_GetApplication(*hCTX.g_pTaskManager, 2);
	else if (hCTX.g_TaskManager)
		CViewerApp = hCTX.CTaskManager_GetApplication(hCTX.g_TaskManager, 2);
	else
		return -1;

	CViewerManager = hCTX.CViewerApp_GetViewerManager(CViewerApp);

	for (i = 0; i < seekLen; i++)
		notAvail[i] = notAvail[i + 42];

	if (hCTX.g_WarningWStringEng) {
		hCTX.g_TVViewerWStringEng = hCTX.g_CustomTextWStringEng;
		hCTX.g_pTVViewerMgr = hCTX.g_pCustomTextResMgr;
	}
	for (i = 0; i < 600; i++) {
		if (!memcmp(notAvail, hCTX.g_TVViewerWStringEng[i], seekLen * 2))
			break;
	}
	if (i == 600)
		return -1;
	strPos = i;

	if (hCTX.g_TVViewerMgr)
		tvString = hCTX.CResourceManager_GetWString(*hCTX.g_TVViewerMgr, strPos);
	else if (hCTX.g_pTVViewerMgr)
		tvString = hCTX.CResourceManager_GetWString(*hCTX.g_pTVViewerMgr, strPos);

	unsigned short oldString[512];
	memset(oldString, 0, 512 * sizeof(short));

	if (!tvString) {
		return -1;
	}

	uint32_t paligned = (uint32_t)tvString & ~0x0FFF;
	mprotect((uint32_t*)paligned, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

	for (i = 0; i <= 255; i++)
		oldString[i] = tvString[i];

	i = 0;
	while (stringUtf[i]) {
		if (stringUtf[i] >= 0 && stringUtf[i] <= 127) strLen += 1;
		else if ((stringUtf[i] & 0xE0) == 0xC0) strLen += 2;
		else if ((stringUtf[i] & 0xF0) == 0xE0) strLen += 3;
		else if ((stringUtf[i] & 0xF8) == 0xF0) strLen += 4;
		i++;
	}

	if (strLen < 255)
		newLength = strLen;
	else
		newLength = 255;

	for (i = 0; i <= newLength; i++)
		tvString[i] = 0;

	hCTX.PCWString_Convert2(tvString, stringUtf, newLength, 1, 0);
	tvString[newLength] = 0;

	if (hCTX.CCustomTextApp_t_OnActivated)
		hCTX.CCustomTextApp_t_OnActivated(CCustomTextApp, 0, 0);
	else if (hCTX.CViewerManager_ShowSystemAlertB)
		hCTX.CViewerManager_ShowRCMode(CViewerManager, 5);
	else if (hCTX.CViewerManager_ShowCustomText)
		hCTX.CViewerManager_ShowCustomText(CViewerManager, 5);
	else
		hCTX.CViewerManager_ShowCustomTextF(CViewerManager, 5, 1);

	usleep(200000);

	for (i = 0; i < 256; i++)
		tvString[i] = oldString[i];

	return 0;
}