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
 *  adONis
 *  (c) 2018
 *
 *  tasshack
 *  (c) 2019
 *
 *  License: GPLv3
 *
 */
 //////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h> 
#include <sys/stat.h>  
#include <syslog.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <memory.h>
#include <glob.h>
#include <stdarg.h>
#include <pthread.h>
#include <execinfo.h>

#include "common.h"
#include "hook.h"

#include "led_manager.h"

//////////////////////////////////////////////////////////////////////////////

#define LIB_NAME "Sambilight"
#define LIB_VERSION "v1.0.0"
#define LIB_TV_MODELS "E/F/H"
#define LIB_HOOKS sambilight_hooks
#define hCTX sambilight_hook_ctx
#define WIDTH 160 //96
#define HEIGHT 90 //54

#include "util.h"

unsigned char tv_remote_enabled = 1;
unsigned char external_led_enabled = 1;
unsigned char external_led_state = 1;
STATIC int show_msg_box(const char * text);

//////////////////////////////////////////////////////////////////////////////

typedef union {
	const void *procs[25];
	struct {
		const int(*SdDisplay_CaptureScreenE)(int *CDSize, unsigned char *buffer, int *CaptureInfo);
		const int(*SdDisplay_CaptureScreenF)(int *CDSize, unsigned char *buffer, int *CaptureInfo, int zero);
		const int(*SdDisplay_CaptureScreenH)(int *CDSize, unsigned char *buffer, int *CaptureInfo, int *rect, int zero);

		int *g_IPanel;
		void *g_TaskManager;
		void **g_pTaskManager;
		void **g_TVViewerMgr;
		void **g_pTVViewerMgr;
		int **g_TVViewerWStringEng;
		int **g_WarningWStringEng;
		void **g_pCustomTextResMgr;
		int **g_CustomTextWStringEng;
		void *(*CResourceManager_GetWString)(void *TVViewerMgr, int stringnum);
		const char *(*PCWString_Convert2)(unsigned short *w_string, char *c_string, int len, int isUTF8, int *zero);
		void *(*CTaskManager_GetApplication)(void *taskMan, int two);
		void *(*CViewerApp_GetViewerManager)(void *this);
		void *(*ALMOND0300_CDesktopManager_GetInstance)(int one);
		void *(*ALMOND0300_CDesktopManager_GetApplication)(void *this, char *viewerstring, int zero);
		const int(*CViewerManager_ShowSystemAlert)(void *this, int num, int one);
		const int(*CViewerManager_ShowSystemAlertB)(void *this, int num);
		const int(*CViewerManager_ShowCustomText)(void *this, int num);
		const int(*CViewerManager_ShowCustomTextF)(void *this, int num, int one);
		const int(*CViewerManager_ShowRCMode)(void *this, int num);
		const int(*CCustomTextApp_t_OnActivated)(void *this, char *nothing, int num);
	};
} samyGO_whacky_t;

samyGO_whacky_t hCTX =
{
	// libScreenshot
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP23SdDisplay_CaptureInfo_t",
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP23SdDisplay_CaptureInfo_t12SdMainChip_k",
	(const void*)"_Z23SdDisplay_CaptureScreenP8SdSize_tPhP24SdVideoCommonFrameData_tP8SdRect_t12SdMainChip_k",

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
};

_HOOK_IMPL(int, CViewerApp_t_OnInputOccur, void *this, int *a2)
{
	char key = (char)a2[5];
	const char * msgHeader = "Ambient Lighting";
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
			index++;
		}
		else {
			index = 1;
		}
		led_manager_set_profile(&led_profiles[index - 1]);
		sprintf(msg, "%s %s Mode", msgHeader, led_profiles[index - 1].name);
		show_msg_box(msg);
		return 1;
	case 21: // KEY_YELLOW
		switch (led_manager_get_brightness()) {
		case 100:
			led_manager_set_brightness(75);
			break;
		case 75:
			led_manager_set_brightness(50);
			break;
		case 50:
			led_manager_set_brightness(25);
			break;
		default:;
			led_manager_set_brightness(100);
			break;
		}
		sprintf(msg, "%s %%%d", msgHeader, led_manager_get_brightness());
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


//////////////////////////////////////////////////////////////////////////////
STATIC int _hooked = 0;
EXTERN_C void lib_init(void *_h, const char *libpath)
{
	if (_hooked) {
		log("Injecting once is enough!\n");
		return;
	}

	int argc, CDSize[2], CaptureInfo[20] = { 0 }, SdRect[4] = { 0 }, led_test = 0;
	unsigned char *buffer;
	char *argv[200], device[100] = "/dev/ttyUSB0", * optstr;
	typedef int func(void);
	func *f;
	unsigned char *data;
	unsigned short leds_count, data_size, bytesWritten, bytesRemaining;

	unlink(LOG_FILE);
	log("SamyGO "LIB_TV_MODELS" lib"LIB_NAME" "LIB_VERSION" - (c) tasshack 2019\n");
	void *h = dlopen(0, RTLD_LAZY);
	if (!h) {
		char *serr = dlerror();
		return;
	}

	patch_adbg_CheckSystem(h);

	samyGO_whacky_t_init(h, &hCTX, ARRAYSIZE(hCTX.procs));

	led_manager_config_t led_config = {
		35,		/// h_leds_count
		19,		/// v_leds_count
		7,		/// bottom_gap
		68,		/// start_offset
		WIDTH,	/// image_width
		HEIGHT,	/// image_height
		"RGB",	/// color_order
		0,		/// led_order
	};

	argc = getArgCArgV(libpath, argv);
		
	optstr = getOptArg(argv, argc, "H_LEDS:");
	if (optstr && strlen(optstr))
		led_config.h_leds_count = atoi(optstr);

	optstr = getOptArg(argv, argc, "V_LEDS:");
	if (optstr && strlen(optstr))
		led_config.v_leds_count= atoi(optstr);

	optstr = getOptArg(argv, argc, "BOTTOM_GAP:");
	if (optstr && strlen(optstr))
		led_config.bottom_gap = atoi(optstr);

	optstr = getOptArg(argv, argc, "START_OFFSET:");
	if (optstr && strlen(optstr))
		led_config.start_offset = atoi(optstr);

	optstr = getOptArg(argv, argc, "CLOCKWISE");
	if (optstr)
		led_config.led_order = atoi(optstr);

	optstr = getOptArg(argv, argc, "COLOR_ORDER");
	if (optstr && strlen(optstr) == 3)
		strncpy(led_config.color_order, optstr, 3);	

	optstr = getOptArg(argv, argc, "BLACK_BORDER:");
	if (optstr && strlen(optstr))
		led_profiles[0].black_border = atoi(optstr);

	optstr = getOptArg(argv, argc, "SMOOTHING:");
	if (optstr && strlen(optstr))
		led_profiles[0].smoothing_fps = atoi(optstr);

	optstr = getOptArg(argv, argc, "TV_REMOTE");
	if (optstr)
		tv_remote_enabled = 1;

	optstr = getOptArg(argv, argc, "EXTERNAL");
	if (optstr)
		external_led_enabled = 1;

	optstr = getOptArg(argv, argc, "DEVICE:");
	if (optstr && strlen(optstr))
		strncpy(device, optstr, strlen(optstr));

	optstr = getOptArg(argv, argc, "TEST");
	if (optstr)
		led_test = 1;

	if (tv_remote_enabled) {
		if (dyn_sym_tab_init(h, dyn_hook_fn_tab, ARRAYSIZE(dyn_hook_fn_tab)) >= 0) {
			set_hooks(LIB_HOOKS, ARRAYSIZE(LIB_HOOKS));
			_hooked = 1;
		}
	}

	log("Sambilight started\n");
	int serial = open(device, O_RDWR | O_NOCTTY);
	if (serial >= 0) {
		struct termios SerialPortSettings;
		tcgetattr(serial, &SerialPortSettings);
		cfsetispeed(&SerialPortSettings, B921600);
		cfsetospeed(&SerialPortSettings, B921600);
		SerialPortSettings.c_cflag &= ~PARENB;
		SerialPortSettings.c_cflag &= ~CSTOPB;
		SerialPortSettings.c_cflag &= ~CSIZE;
		SerialPortSettings.c_cflag |= CS8;
		SerialPortSettings.c_cflag &= ~CRTSCTS;
		SerialPortSettings.c_cflag |= CREAD | CLOCAL;
		SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);
		SerialPortSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		SerialPortSettings.c_oflag &= ~OPOST;
		tcsetattr(serial, TCSANOW, &SerialPortSettings);

		leds_count = led_manager_init(&led_config, &led_profiles[0]);

		data_size = (leds_count * 3) + 6;
		if (external_led_enabled) {
			data_size++;
		}

		data = malloc(data_size);
		memset(data, 0, data_size);
		strcat(data, "Ada");
		data[3] = ((leds_count - 1) >> 8) & 0xff;
		data[4] = (leds_count - 1) & 0xff;
		data[5] = data[3] ^ data[4] ^ 0x55;

		log("Grabbing started\n");
		show_msg_box("Ambient Lighting On");

		buffer = malloc(4 * WIDTH * HEIGHT);
		CDSize[0] = WIDTH;
		CDSize[1] = HEIGHT;

		while (1) {
			if (led_manager_get_state()) {
				if (hCTX.SdDisplay_CaptureScreenF) {
					hCTX.SdDisplay_CaptureScreenF(CDSize, buffer, CaptureInfo, 0);
				}
				else if (hCTX.SdDisplay_CaptureScreenE) {
					hCTX.SdDisplay_CaptureScreenE(CDSize, buffer, CaptureInfo);
				}
				else if (hCTX.SdDisplay_CaptureScreenH) {
					f = (func*)hCTX.g_IPanel[3];
					SdRect[2] = f();
					f = (func*)hCTX.g_IPanel[4];
					SdRect[3] = f();
					hCTX.SdDisplay_CaptureScreenH(CDSize, buffer, CaptureInfo, SdRect, 0);
				}

				if (led_manager_argb8888_to_leds(buffer, &data[6]) == 1) {
					bytesWritten = 0;
					if (external_led_enabled) {
						data[data_size - 1] = external_led_state;
					}
					bytesRemaining = data_size;
					while (bytesRemaining > 0) {
						bytesWritten = write(serial, data + bytesWritten, bytesRemaining);
						bytesRemaining -= bytesWritten;
					}

					if (led_test) {
						break;
					}
				}
				else {
					usleep(100000);
				}
			}

			while (!led_manager_get_state()) {
				usleep(100000);
			}

			//usleep(100000);
		} // while			   

		free(buffer);
		free(data);

		close(serial);
	}
	else {
		log("Could not open serial port!\n");
	}


	log("Sambilight ended\n");
	dlclose(h);
}

EXTERN_C void lib_deinit(
	void *_h)
{
	log(">>> %s\n", __func__);

	log("If you see this message you forget to specify -r when invoking samyGOso :)\n");

	if (_hooked)
		remove_hooks(LIB_HOOKS, ARRAYSIZE(LIB_HOOKS));

	log("<<< %s\n", __func__);
}



//////////////////////////////////////////////////////////////////////////////

STATIC int show_msg_box(const char * text)
{
	void *CViewerManager, *CViewerInstance, *CViewerApp, *CCustomTextApp;

	int newLength, strPos, i, seekLen = 13, strLen = 0;
	unsigned short *tvString = NULL;

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
		int *CSystemAlertApp = hCTX.ALMOND0300_CDesktopManager_GetApplication(CViewerInstance, "samsung.native.systemalert", 0);
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
	mprotect((uint32_t *)paligned, 0x2000, PROT_READ | PROT_WRITE | PROT_EXEC);

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