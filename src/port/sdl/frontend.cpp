#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "port.h"
#include "r3000a.h"
#include "plugins.h"
#include "cdrom.h"
#include "cdriso.h"
#include <SDL.h>

/* PATH_MAX inclusion */
#ifdef __MINGW32__
#include <limits.h>
#endif

#ifdef SPU_PCSXREARMED
#include "spu/spu_pcsxrearmed/spu_config.h"		// To set spu-specific configuration
#endif

// New gpulib from Notaz's PCSX Rearmed handles duties common to GPU plugins
#ifdef USE_GPULIB
#include "gpu/gpulib/gpu.h"
#endif

#ifdef GPU_UNAI
#include "gpu/gpu_unai/gpu.h"
#endif

#define timer_delay(a)	wait_ticks(a*1000)

enum  {
	KEY_UP=0x1,	KEY_LEFT=0x4,		KEY_DOWN=0x10,	KEY_RIGHT=0x40,
	KEY_START=1<<8,	KEY_SELECT=1<<9,	KEY_L=1<<10,	KEY_R=1<<11,
	KEY_A=1<<12,	KEY_B=1<<13,		KEY_X=1<<14,	KEY_Y=1<<15,
};

static u32 ret = 0;

static inline void key_reset() { ret = 0; }

static unsigned int key_read(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event))  {
		switch (event.type) {
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_UP:		ret |= KEY_UP;    break;
			case SDLK_DOWN:		ret |= KEY_DOWN;  break;
			case SDLK_LEFT:		ret |= KEY_LEFT;  break;
			case SDLK_RIGHT:	ret |= KEY_RIGHT; break;

			case SDLK_LCTRL:	ret |= KEY_A; break;
			case SDLK_LALT:		ret |= KEY_B; break;
			case SDLK_SPACE:	ret |= KEY_X; break;
			case SDLK_LSHIFT:	ret |= KEY_Y; break;

			case SDLK_TAB:		ret |= KEY_L; break;
			case SDLK_BACKSPACE:	ret |= KEY_R; break;

			case SDLK_RETURN:	ret |= KEY_START; break;
			case SDLK_ESCAPE:	ret |= KEY_SELECT; break;
			case SDLK_m:		ret |= KEY_SELECT | KEY_Y; break;

			default: break;
			}
			break;
		case SDL_KEYUP:
			switch(event.key.keysym.sym) {
			case SDLK_UP:		ret &= ~KEY_UP;    break;
			case SDLK_DOWN:		ret &= ~KEY_DOWN;  break;
			case SDLK_LEFT:		ret &= ~KEY_LEFT;  break;
			case SDLK_RIGHT:	ret &= ~KEY_RIGHT; break;

			case SDLK_LCTRL:	ret &= ~KEY_A; break;
			case SDLK_LALT:		ret &= ~KEY_B; break;
			case SDLK_SPACE:	ret &= ~KEY_X; break;
			case SDLK_LSHIFT:	ret &= ~KEY_Y; break;

			case SDLK_TAB:		ret &= ~KEY_L; break;
			case SDLK_BACKSPACE:	ret &= ~KEY_R; break;

			case SDLK_RETURN:	ret &= ~KEY_START; break;
			case SDLK_ESCAPE:	ret &= ~KEY_SELECT; break;
			case SDLK_m:		ret &= ~(KEY_SELECT | KEY_Y); break;

			default: break;
			}
			break;
		default: break;
		}
	}


	return ret;
}

struct dir_item {
	char	*name;
	s32	type; // 0=dir, 1=file, 2=zip archive
};

int compare_names(struct dir_item *a, struct dir_item *b)
{
	bool aIsParent = strcmp(a->name, "..") == 0;
	bool bIsParent = strcmp(b->name, "..") == 0;

	if (aIsParent && bIsParent)
		return 0;
	else if (aIsParent)
		return -1;
	else if (bIsParent)
		return 1;

	if ((a->type != b->type) && (a->type == 0 || b->type == 0)) {
		return a->type == 0 ? -1 : 1;
	}

	return strcasecmp(a->name, b->name);
}

void sort_dir(struct dir_item *list, int num_items)
{
	qsort((void *)list,num_items,sizeof(struct dir_item),(int (*)(const void*, const void*))compare_names);
}

static char gamepath[PATH_MAX] = "./";
static struct dir_item filereq_dir_items[1024] = { { 0, 0 }, };

#define MENU_X		8
#define MENU_Y		8
#define MENU_LS		(MENU_Y + 10)
#define MENU_HEIGHT	22

static inline void ChDir(char *dir)
{
	int unused = chdir(dir);
	(void)unused;
}

static inline char *GetCwd(void)
{
	char *unused = getcwd(gamepath, PATH_MAX);
	(void)unused;
#ifdef __WIN32__
		for (int i = 0; i < PATH_MAX; i++) {
			if (gamepath[i] == 0)
				break;
			if (gamepath[i] == '\\')
				gamepath[i] = '/';
		}
#endif
	strcpy(Config.LastDir, gamepath);

	return gamepath;
}

#define FREE_LIST() \
do { \
	for (int i = 0; i < num_items; i++) \
		if (filereq_dir_items[i].name) { \
			free(filereq_dir_items[i].name); \
			filereq_dir_items[i].name = NULL; \
		} \
	num_items = 0; \
} while (0)

static const char *wildcards[] = {
	//senquack - we do not (yet) support these 3 PocketISO compressed formats
	// TODO: adapt PCSX Rearmed's cdrcimg.c plugin to get these
	//"z", "bz", "znx",

	"bin", "img", "mdf", "iso", "cue",
	"pbp", "cbn", NULL
};

static s32 check_ext(const char *name)
{
	const char *p = strrchr(name, '.');

	if (!p)
		return 0;

	for (int i = 0; wildcards[i] != NULL; i++) {
		if (strcasecmp(wildcards[i], p + 1) == 0)
			return 1;
	}

	return 0;
}

static s32 get_entry_type(char *cwd, char *d_name)
{
	s32 type;
	struct stat item;
	char *path = (char *)malloc(strlen(cwd) + strlen(d_name) + 2);

	sprintf(path, "%s/%s", cwd, d_name);
	if (!stat(path, &item)) {
		if (S_ISDIR(item.st_mode)) {
			type = 0;
		} else {
			type = 1;
		}
	} else {
		type = 1;
	}

	free(path);
	return type;
}

char *FileReq(char *dir, const char *ext, char *result)
{
	static char *cwd = NULL;
	static s32 cursor_pos = 1;
	static s32 first_visible;
	static s32 num_items = 0;
	DIR *dirstream;
	struct dirent *direntry;
	static s32 row;
	char tmp_string[41];
	u32 keys;

	if (dir)
		ChDir(dir);

	if (!cwd) {
		cwd = Config.LastDir ? Config.LastDir : GetCwd();
	}

	for (;;) {
		keys = key_read();

		video_clear();

		if (keys & KEY_SELECT) {
			FREE_LIST();
			key_reset();
			return NULL;
		}

		if (num_items == 0) {
			dirstream = opendir(cwd);
			if (dirstream == NULL) {
				port_printf(0, 20, "error opening directory");
				return NULL;
			}
			// read directory entries
			while ((direntry = readdir(dirstream))) {
				s32 type = get_entry_type(cwd, direntry->d_name);

				// this is a very ugly way of only accepting a certain extension
				if ((type == 0 && strcmp(direntry->d_name, ".")) ||
				     check_ext(direntry->d_name) ||
				    (ext && (strlen(direntry->d_name) > 4 &&0 == strncasecmp(direntry->d_name + (strlen(direntry->d_name) - strlen(ext)), ext, strlen(ext))))) {
					// Hide ".." if at Unix root dir. Don't display Unix hidden files (.file).
					if ((!strcmp(direntry->d_name, "..") && strcmp(cwd, "/")) || direntry->d_name[0] != '.')
					{
						filereq_dir_items[num_items].name = (char *)malloc(strlen(direntry->d_name) + 1);
						strcpy(filereq_dir_items[num_items].name, direntry->d_name);
						filereq_dir_items[num_items].type = type;
						num_items++;
						if (num_items > 1024) break;
					}
				}
			}
			closedir(dirstream);

			sort_dir(filereq_dir_items, num_items);
			cursor_pos = 0;
			first_visible = 0;
		}

		// display current directory
		int len = strlen(cwd);

		if (len > 40) {
			strcpy(tmp_string, "..");
			strcat(tmp_string, cwd + len - 38);
			port_printf(0, MENU_Y, tmp_string);
		} else
			port_printf(0, MENU_Y, cwd);

		if (keys & KEY_DOWN) { //down
			if (++cursor_pos >= num_items) {
				cursor_pos = 0;
				first_visible = 0;
			}
			if ((cursor_pos - first_visible) >= MENU_HEIGHT) first_visible++;
		} else if (keys & KEY_UP) { // up
			if (--cursor_pos < 0) {
				cursor_pos = num_items - 1;
				first_visible = cursor_pos - MENU_HEIGHT + 1;
				if (first_visible < 0) first_visible = 0;
			}
			if (cursor_pos < first_visible) first_visible--;
		} else if (keys & KEY_LEFT) { //left
			if (cursor_pos >= 10) cursor_pos -= 10;
			else cursor_pos = 0;
			if (cursor_pos < first_visible) first_visible = cursor_pos;
		} else if (keys & KEY_RIGHT) { //right
			if (cursor_pos < (num_items - 11)) cursor_pos += 10;
			else cursor_pos = num_items - 1;
			if ((cursor_pos - first_visible) >= MENU_HEIGHT)
				first_visible = cursor_pos - (MENU_HEIGHT - 1);
		} else if (keys & KEY_A) { // button 1
			// directory selected
			if (filereq_dir_items[cursor_pos].type == 0) {
				strcat(cwd, "/");
				strcat(cwd, filereq_dir_items[cursor_pos].name);

				ChDir(cwd);
				cwd = GetCwd();

				FREE_LIST();
				key_reset();
			} else {
				sprintf(result, "%s/%s", cwd, filereq_dir_items[cursor_pos].name);

				video_clear();
				port_printf(16 * 8, 120, "LOADING");
				video_flip();

				FREE_LIST();
				key_reset();
				return result;
			}
		} else if (keys & KEY_B) {
			cursor_pos = 0;
			first_visible = 0;
			key_reset();
		}

		// display directory contents
		row = 0;
		while (row < num_items && row < MENU_HEIGHT) {
			if (row == (cursor_pos - first_visible)) {
				// draw cursor
				port_printf(MENU_X + 16, MENU_LS + (10 * row), "-->");
			}

			if (filereq_dir_items[row + first_visible].type == 0)
				port_printf(MENU_X, MENU_LS + (10 * row), "DIR");
			int len = strlen(filereq_dir_items[row + first_visible].name);
			if (len > 32) {
				snprintf(tmp_string, 16, "%s", filereq_dir_items[row + first_visible].name);
				strcat(tmp_string, "..");
				strcat(tmp_string, &filereq_dir_items[row + first_visible].name[len - 15]);
			} else
			snprintf(tmp_string, 33, "%s", filereq_dir_items[row + first_visible].name);
			port_printf(MENU_X + (8 * 5), MENU_LS + (10 * row), tmp_string);
			row++;
		}
		while (row < MENU_HEIGHT)
			row++;

		video_flip();
		timer_delay(75);

		if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R |
			    KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
			timer_delay(50);
	}

	return NULL;
}

typedef struct {
	char *name;
	int (*on_press_a)();
	int (*on_press)(u32 keys);
	char *(*showval)();
} MENUITEM;

typedef struct {
	int num;
	int cur;
	int x, y;
	MENUITEM *m; // array of items
} MENU;

/* Forward declaration */
static int gui_RunMenu(MENU *menu);
static int gui_LoadIso();
static int gui_Settings();
static int gui_Quit();

static int state_alter(u32 keys)
{
	extern int saveslot;

	if (keys & KEY_RIGHT) {
		if (saveslot < 9) saveslot++;
	} else if (keys & KEY_LEFT) {
		if (saveslot > 0) saveslot--;
	}

	return 0;
}

static char *state_show()
{
	extern int saveslot;
	static char buf[16];
	sprintf(buf, "%d", saveslot);
	return buf;
}

static int gui_Credits()
{
	for (;;) {
		u32 keys = key_read();

		video_clear();

		// check keys
		if (keys) {
			key_reset();
			return 0;
		}

		// diplay menu
		port_printf(15 * 8 + 4, 10, "CREDITS:");
		port_printf( 2 * 8, 30, "pcsx team, pcsx-df team, pcsx-r team");

		port_printf( 6 * 8, 50, "Franxis and Chui - PCSX4ALL");
		port_printf( 4 * 8, 60, "Unai - fast PCSX4ALL GPU plugin");

		port_printf( 5 * 8, 80, "Ulrich Hecht - psx4all-dingoo");

		port_printf(10 * 8, 90, "notaz - PCSX-ReArmed");

		port_printf( 0 * 8, 120, "Dmitry Smagin - porting and optimizing");
		port_printf( 0 * 8, 130, "                of mips recompiler,");
		port_printf( 0 * 8, 140, "                gui coding");

		port_printf( 0 * 8,160, "senquack - fixing polygons in gpu_unai,");
		port_printf( 0 * 8,170, "           porting spu and other stuff");
		port_printf( 0 * 8,180, "           from pcsx_rearmed and pcsx-r,");
		port_printf( 0 * 8,190, "           many fixes and improvements");

		port_printf( 0 * 8, 210, "zear     - gui fixing and testing");

		video_flip();
		timer_delay(75);
	}

	return 0;
}

static MENUITEM gui_MainMenuItems[] = {
	{(char *)"Load game", &gui_LoadIso, NULL, NULL},
	{(char *)"Settings", &gui_Settings, NULL, NULL},
	{(char *)"Credits", &gui_Credits, NULL, NULL},
	{(char *)"Quit", &gui_Quit, NULL, NULL},
	{0}
};

#define MENU_SIZE ((sizeof(gui_MainMenuItems) / sizeof(MENUITEM)) - 1)
static MENU gui_MainMenu = { MENU_SIZE, 0, 112, 120, (MENUITEM *)&gui_MainMenuItems };

static int gui_state_load()
{
	if (state_load() < 0) {
		// Load failure
		// TODO: Add user interaction to handle gracefully, improve interface
		return 0;
	}

	return 1;
}

static int gui_state_save()
{
	video_clear();
	port_printf(160-(6*8/2), 120-(8/2), "SAVING");
	video_flip();

	if (state_save() < 0) {
		// Error saving

		for (;;) {
			u32 keys = key_read();
			video_clear();
			// check keys
			if (keys) {
				key_reset();
				return 0;
			}

			port_printf(160-(11*8/2), 120-12, "SAVE FAILED");
			port_printf(160-(18*8/2), 120+12, "Out of disk space?");
			video_flip();
			timer_delay(75);
		}
	}

	return 1;
}

//To choose which of a multi-CD image should be used. Can be called
// from front-end 'Swap CD' menu item, in which case parameter
// 'swapping_cd' is true. Or, can be called via callback function
// gui_select_multicd_to_boot_from() inside cdriso.cpp, in which
// case swapping_cd parameter is false.
static int gui_select_multicd(bool swapping_cd)
{
	if (cdrIsoMultidiskCount <= 1)
		return 0;

	// Only max of 8 ISO images inside an Eboot multi-disk .pbp are supported
	//  by cdriso.cpp PBP code, but enforce it here to be sure:
	int num_rows = (cdrIsoMultidiskCount > 8) ? 8 : cdrIsoMultidiskCount;

	int cursor_pos = cdrIsoMultidiskSelect;
	if ((cursor_pos >= num_rows) || (cursor_pos < 0))
		cursor_pos = 0;

	for (;;) {
		video_clear();
		u32 keys = key_read();

		if ((swapping_cd) && (keys & KEY_SELECT)) {
			key_reset();
			return 0;
		}

		if (!swapping_cd)
			port_printf(MENU_X, MENU_Y, "Multi-CD image detected:");

		char tmp_string[41];
		for (int row=0; row < num_rows; ++row) {
			if (row == cursor_pos) {
				// draw cursor
				port_printf(MENU_X + 16, MENU_LS + 10 + (10 * row), "-->");
			}

			sprintf(tmp_string, "CD %d", (row+1));

			if (swapping_cd && (row == cdrIsoMultidiskSelect)) {
				// print indication of which CD is already inserted
				strcat(tmp_string, " (inserted)");
			}

			port_printf(MENU_X + (8 * 5), MENU_LS + 10 + (10 * row), tmp_string);
		}

		if (keys & KEY_DOWN) { //down
			if (++cursor_pos >= num_rows)
				cursor_pos = 0;
		} else if (keys & KEY_UP) { // up
			if (--cursor_pos < 0)
				cursor_pos = num_rows - 1;
		} else if (keys & KEY_LEFT) { //left
			cursor_pos = 0;
		} else if (keys & KEY_RIGHT) { //right
			cursor_pos = num_rows - 1;
		} else if (keys & KEY_A) { // button 1
			key_reset();
			cdrIsoMultidiskSelect = cursor_pos;
			video_clear();
			video_flip();
			return 1;
		}

		video_flip();
		timer_delay(75);

		if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R |
			    KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
			timer_delay(50);
	}

}

//Called via callback when handlepbp() in cdriso.cpp detects a multi-CD
// .pbp image is being loaded, so user can choose CD to boot from.
// This is necessary because we do not know if a given CD image is
// a multi-CD image until after cdrom plugin has gone through many
// steps and verifications.
static CALLBACK void gui_select_multicd_to_boot_from(void)
{
	if (cdrIsoMultidiskSelect >= cdrIsoMultidiskCount)
		cdrIsoMultidiskSelect = 0;

	//Pass false to indicate a CD is not being swapped through front-end menu
	gui_select_multicd(false);

	//Before return to emu, clear/flip once more in case we're triple-buffered
	video_clear();
	video_flip();
}

static int gui_swap_cd(void)
{
	//Is a multi-cd image loaded? (EBOOT .pbp files support this)
	bool using_multicd = cdrIsoMultidiskCount > 1;

	if (using_multicd) {
		// Pass true to gui_select_multicd() so it knows CD is being swapped
		if (!gui_select_multicd(true)) {
			// User cancelled, return to menu
			return 0;
		} else {
			printf("CD swap selected image %d of %d in multi-CD\n", cdrIsoMultidiskSelect+1, cdrIsoMultidiskCount);
		}
	} else {
		static char isoname[PATH_MAX];
		const char *name = FileReq(NULL, NULL, isoname);
		if (name == NULL)
			return 0;

		SetIsoFile(name);
		printf("CD swap selected file: %s\n", name);
	}

	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	//Unregister multi-CD callback so handlepbp() or other cdriso
	// plugins don't ask for CD to boot from
	cdrIsoMultidiskCallback = NULL;

	if (ReloadCdromPlugin() < 0) {
		printf("Failed to re-initialize cdr\n");
		return 0;
	}

	if (CDR_open() < 0) {
		printf("Failed to open cdr\n");
		return 0;
	}

	SetCdOpenCaseTime(time(NULL) + 2);
	LidInterrupt();
	return 1;
}

static MENUITEM gui_GameMenuItems[] = {
	{(char *)"Swap CD", &gui_swap_cd, NULL, NULL},
	{(char *)"Load state", &gui_state_load, &state_alter, &state_show},
	{(char *)"Save state", &gui_state_save, &state_alter, &state_show},
	{(char *)"Quit", &gui_Quit, NULL, NULL},
	{0}
};

#define GMENU_SIZE ((sizeof(gui_GameMenuItems) / sizeof(MENUITEM)) - 1)
static MENU gui_GameMenu = { GMENU_SIZE, 0, 112, 120, (MENUITEM *)&gui_GameMenuItems };

#ifdef PSXREC
static int emu_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.Cpu == 1) Config.Cpu = 0;
	} else if (keys & KEY_LEFT) {
		if (Config.Cpu == 0) Config.Cpu = 1;
	}

	return 0;
}

static char *emu_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.Cpu ? "int" : "rec");
	return buf;
}

extern u32 cycle_multiplier; // in mips/recompiler.cpp

static int cycle_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (cycle_multiplier < 0x300) cycle_multiplier += 0x04;
	} else if (keys & KEY_LEFT) {
		if (cycle_multiplier > 0x050) cycle_multiplier -= 0x04;
	}

	return 0;
}

static char *cycle_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%d.%02d", cycle_multiplier >> 8, (cycle_multiplier & 0xff) * 100 / 256);
	return buf;
}
#endif

static int bios_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.HLE == 1) Config.HLE = 0;
	} else if (keys & KEY_LEFT) {
		if (Config.HLE == 0) Config.HLE = 1;
	}

	return 0;
}

static char *bios_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.HLE ? "HLE" : "scph1001.bin");
	return buf;
}

static int fps_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.ShowFps == false) Config.ShowFps = true;
	} else if (keys & KEY_LEFT) {
		if (Config.ShowFps == true) Config.ShowFps = false;
	}
	return 0;
}

static char *fps_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.ShowFps == true ? "on" : "off");
	return buf;
}

static int framelimit_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.FrameLimit == false) Config.FrameLimit = true;
	} else if (keys & KEY_LEFT) {
		if (Config.FrameLimit == true) Config.FrameLimit = false;
	}

	return 0;
}

static char *framelimit_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.FrameLimit == true ? "on" : "off");
	return buf;
}

#ifdef GPU_UNAI
static int dithering_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (gpu_unai_config_ext.dithering == false)
			gpu_unai_config_ext.dithering = true;
	} else if (keys & KEY_LEFT) {
		if (gpu_unai_config_ext.dithering == true)
			gpu_unai_config_ext.dithering = false;
	}

	return 0;
}

static char *dithering_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", gpu_unai_config_ext.dithering == true ? "on" : "off");
	return buf;
}
#endif

static int xa_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.Xa == 1) Config.Xa = 0;
	} else if (keys & KEY_LEFT) {
		if (Config.Xa == 0) Config.Xa = 1;
	}

	return 0;
}

static char *xa_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.Xa ? "off" : "on");
	return buf;
}

static int cdda_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.Cdda == 1) Config.Cdda = 0;
	} else if (keys & KEY_LEFT) {
		if (Config.Cdda == 0) Config.Cdda = 1;
	}

	return 0;
}

static char *cdda_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.Cdda ? "off" : "on");
	return buf;
}

static int forcedxa_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.ForcedXAUpdates == 0) Config.ForcedXAUpdates = 1;
	} else if (keys & KEY_LEFT) {
		if (Config.ForcedXAUpdates == 1) Config.ForcedXAUpdates = 0;
	}

	return 0;
}

static char *forcedxa_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.ForcedXAUpdates ? "on" : "off");
	return buf;
}

static int RCntFix_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.RCntFix < 1) Config.RCntFix = 1;
	} else if (keys & KEY_LEFT) {
		if (Config.RCntFix > 0) Config.RCntFix = 0;
	}

	return 0;
}

static char *RCntFix_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.RCntFix ? "on" : "off");
	return buf;
}

static int VSyncWA_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.VSyncWA < 1) Config.VSyncWA = 1;
	} else if (keys & KEY_LEFT) {
		if (Config.VSyncWA > 0) Config.VSyncWA = 0;
	}

	return 0;
}

static char *VSyncWA_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.VSyncWA ? "on" : "off");
	return buf;
}

static int syncaudio_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.SyncAudio < 1) Config.SyncAudio = 1;
	} else if (keys & KEY_LEFT) {
		if (Config.SyncAudio > 0) Config.SyncAudio = 0;
	}

	return 0;
}

static char *syncaudio_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.SyncAudio ? "on" : "off");
	return buf;
}

static int spuirq_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (Config.SpuIrq < 1) Config.SpuIrq = 1;
	} else if (keys & KEY_LEFT) {
		if (Config.SpuIrq > 0) Config.SpuIrq = 0;
	}

	return 0;
}

static char *spuirq_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%s", Config.SpuIrq ? "on" : "off");
	return buf;
}

#ifdef SPU_PCSXREARMED
static int interpolation_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (spu_config.iUseInterpolation < 3) spu_config.iUseInterpolation++;
	} else if (keys & KEY_LEFT) {
		if (spu_config.iUseInterpolation > 0) spu_config.iUseInterpolation--;
	}

	return 0;
}

static char *interpolation_show()
{
	static char buf[16] = "\0";
	switch (spu_config.iUseInterpolation) {
	case 0: strcpy(buf, "none"); break;
	case 1: strcpy(buf, "simple"); break;
	case 2: strcpy(buf, "gaussian"); break;
	case 3: strcpy(buf, "cubic"); break;
	default: buf[0] = '\0'; break;
	}
	return buf;
}
#endif //SPU_PCSXREARMED

static int settings_back()
{
	return 1;
}

static int settings_defaults()
{
	/* Restores settings to default values. */
	Config.Xa = 0;
	Config.Mdec = 0;
	Config.PsxAuto = 1;
	Config.Cdda = 0;
	Config.HLE = 1;
	Config.RCntFix = 0;
	Config.VSyncWA = 0;
#ifdef PSXREC
	Config.Cpu = 0;
#else
	Config.Cpu = 1;
#endif
	Config.PsxType = 0;
	Config.SpuIrq = 0;
	Config.SyncAudio = 1;
	Config.ForcedXAUpdates = 1;
	Config.ShowFps = 0;
	Config.FrameLimit = 0;
#ifdef SPU_PCSXREARMED
	spu_config.iUseInterpolation = 0;
#endif
#ifdef PSXREC
	cycle_multiplier = 0x200; // == 2.0
#endif
	return 0;
}

static MENUITEM gui_SettingsItems[] = {
#ifdef PSXREC
	{(char *)"[PSX] Emulation core    ", NULL, &emu_alter, &emu_show},
	{(char *)"[PSX] Cycle multiplier  ", NULL, &cycle_alter, &cycle_show},
#endif
	{(char *)"[PSX] BIOS              ", NULL, &bios_alter, &bios_show},
	{(char *)"[PSX] XA audio          ", NULL, &xa_alter, &xa_show},
	{(char *)"[PSX] CDDA audio        ", NULL, &cdda_alter, &cdda_show},
	{(char *)"[PSX] Forced XA updates ", NULL, &forcedxa_alter, &forcedxa_show},
	{(char *)"[PSX] RCntFix           ", NULL, &RCntFix_alter, &RCntFix_show},
	{(char *)"[PSX] VSyncWA           ", NULL, &VSyncWA_alter, &VSyncWA_show},
	{(char *)"[GPU] Show FPS          ", NULL, &fps_alter, &fps_show},
	{(char *)"[GPU] Frame Limit       ", NULL, &framelimit_alter, &framelimit_show},
#ifdef GPU_UNAI
	{(char *)"[GPU] Dithering         ", NULL, &dithering_alter, &dithering_show},
#endif
	{(char *)"[SPU] Audio sync        ", NULL, &syncaudio_alter, &syncaudio_show},
	{(char *)"[SPU] IRQ fix           ", NULL, &spuirq_alter, &spuirq_show},
#ifdef SPU_PCSXREARMED
	{(char *)"[SPU] Interpolation     ", NULL, &interpolation_alter, &interpolation_show},
#endif
	{(char *)"Restore default values  ", &settings_defaults, NULL, NULL},
	{NULL, NULL, NULL, NULL},
	{(char *)"Back to main menu", &settings_back, NULL, NULL},
	{0}
};

#define SET_SIZE ((sizeof(gui_SettingsItems) / sizeof(MENUITEM)) - 1)
static MENU gui_SettingsMenu = { SET_SIZE, 0, 24, 60, (MENUITEM *)&gui_SettingsItems };

static int gui_LoadIso()
{
	static char isoname[PATH_MAX];
	const char *name = FileReq(NULL, NULL, isoname);

	if (name) {
		SetIsoFile(name);

		//If a multi-CD Eboot .pbp is detected, cdriso.cpp's handlepbp() will
		// call this function later to allow choosing which CD to boot
		cdrIsoMultidiskCallback = gui_select_multicd_to_boot_from;

		return 1;
	}

	return 0;
}

static int gui_Settings()
{
	gui_RunMenu(&gui_SettingsMenu);

	return 0;
}

static int gui_Quit()
{
	exit(0);
	return 0;
}

static void ShowMenuItem(int x, int y, MENUITEM *mi)
{
	static char string[PATH_MAX];

	if (mi->name) {
		if (mi->showval) {
			sprintf(string, "%s %s", mi->name, (*mi->showval)());
			port_printf(x, y, string);
		} else
			port_printf(x, y, mi->name);
	}
}

static void ShowMenu(MENU *menu)
{
	MENUITEM *mi = menu->m;

	// show menu lines
	for(int i = 0; i < menu->num; i++, mi++) {
		ShowMenuItem(menu->x, menu->y + i * 10, mi);
	}

	// show cursor
	port_printf(menu->x - 3 * 8, menu->y + menu->cur * 10, "-->");

	// general copyrights info
	port_printf( 4 * 8,  0, "pcsx4all 2.3 by Franxis and Chui");
	port_printf( 0 * 8, 10, "based on pcsx-r 1.9 and psx4all-dingoo");
	port_printf( 4 * 8, 20, "with some code from pcsx_rearmed");

	char string[64];
	sprintf(string, "Built on %s at %s", __DATE__, __TIME__);
	port_printf( 4 * 8, 29 * 8, string);
}

static int gui_RunMenu(MENU *menu)
{
	MENUITEM *mi;
	u32 keys;

	for (;;) {
		mi = menu->m + menu->cur;
		keys = key_read();

		video_clear();

		// check keys
		if (keys & KEY_SELECT) {
			key_reset();
			return 0;
		} else if (keys & KEY_UP) {
			do {
				if (--menu->cur < 0)
					menu->cur = menu->num - 1;
			} while (!(menu->m + menu->cur)->name); // Skip over an empty menu entry.

		} else if (keys & KEY_DOWN) {
			do {
				if (++menu->cur == menu->num)
					menu->cur = 0;
			} while (!(menu->m + menu->cur)->name); // Skip over an empty menu entry.
		} else if (keys & KEY_A) {
			if (mi->on_press_a) {
				key_reset();
				int result = (*mi->on_press_a)();
				if (result)
					return result;
			}
		} else if (keys & KEY_B) {
			menu->cur = menu->num - 1;
			key_reset();
		}

		if ((keys & (KEY_LEFT | KEY_RIGHT)) && mi->on_press) {
			int result = (*mi->on_press)(keys);
			if (result)
				return result;
		}

		// diplay menu
		ShowMenu(menu);

		video_flip();
		timer_delay(75);

		if (keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_L | KEY_R |
			    KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN))
			timer_delay(50);
	}

	return 0;
}

/* 0 - exit, 1 - game loaded */
int SelectGame()
{
	return gui_RunMenu(&gui_MainMenu);
}

int GameMenu()
{
	return gui_RunMenu(&gui_GameMenu);
}
