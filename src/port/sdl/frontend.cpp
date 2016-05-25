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
#include "profiler.h"
#include <SDL.h>

/* PATH_MAX inclusion */
#ifdef __MINGW32__
#include <limits.h>
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

void sort_dir(struct dir_item *list, int num_items, int sepdir)
{
	s32 i;
	struct dir_item temp;

	for (i = 0; i < (num_items - 1); i++) {
		if (strcmp(list[i].name, list[i + 1].name) > 0) {
			temp = list[i];
			list[i] = list[i + 1];
			list[i + 1] = temp;
			i = 0;
		}
	}
	if (sepdir) {
		for (i = 0; i < (num_items - 1); i++) {
			if ((list[i].type != 0) && (list[i + 1].type == 0)) {
				temp = list[i];
				list[i] = list[i + 1];
				list[i + 1] = temp;
				i = 0;
			}
		}
	}
}

static char gamepath[PATH_MAX] = "./";
static struct dir_item filereq_dir_items[1024] = { { 0, 0 }, };

#define MENU_X		8
#define MENU_Y		90
#define MENU_LS		(MENU_Y + 10)
#define MENU_HEIGHT	13

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

static char *wildcards[] = {
	"bin", "img", "mdf", "iso", "cue", "z",
	"bz",  "znx", "pbp", "cbn", NULL
};

static s32 check_ext(const char *name)
{
	int len = strlen(name);

	if (len < 4)
		return 0;

	if (name[len-4] != '.')
		return 0;

	for (int i = 0; wildcards[i] != NULL; i++) {
		if (strcasecmp(wildcards[i], &name[len-3]) == 0)
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
	char tmp_string[32];
	u32 keys;

	if (dir)
		ChDir(dir);

	if (!cwd) {
		cwd = GetCwd();
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
					filereq_dir_items[num_items].name = (char *)malloc(strlen(direntry->d_name) + 1);
					strcpy(filereq_dir_items[num_items].name, direntry->d_name);
					filereq_dir_items[num_items].type = type;
					num_items++;
					if (num_items > 1024) break;
				}
			}
			closedir(dirstream);

			sort_dir(filereq_dir_items, num_items, 1);
			cursor_pos = 0;
			first_visible = 0;
		}

		// display current directory
		port_printf(0, MENU_Y, cwd);

		if (keys & KEY_DOWN) { //down
			if (cursor_pos < (num_items - 1)) cursor_pos++;
			if ((cursor_pos - first_visible) >= MENU_HEIGHT) first_visible++;
		} else if (keys & KEY_UP) { // up
			if (cursor_pos > 0) cursor_pos--;
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
			if (len > 28) {
				snprintf(tmp_string, 11, "%s", filereq_dir_items[row + first_visible].name);
				strcat(tmp_string, "....");
				strcat(tmp_string, &filereq_dir_items[row + first_visible].name[len - 14]);
			} else
			snprintf(tmp_string, 30, "%s", filereq_dir_items[row + first_visible].name);
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

static MENUITEM gui_MainMenuItems[] = {
	{(char *)"Load game", &gui_LoadIso, NULL, NULL},
	{(char *)"Settings", &gui_Settings, NULL, NULL},
	{(char *)"Load state", NULL, &state_alter, &state_show},
	{(char *)"Save state", NULL, &state_alter, &state_show},
	{(char *)"Quit", &gui_Quit, NULL, NULL},
	{0}
};

#define MENU_SIZE ((sizeof(gui_MainMenuItems) / sizeof(MENUITEM)) - 1)
static MENU gui_MainMenu = { MENU_SIZE, 0, 112, 120, (MENUITEM *)&gui_MainMenuItems };

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
#ifdef gpu_unai
	extern bool show_fps;

	if (keys & KEY_RIGHT) {
		if (show_fps == false) show_fps = true;
	} else if (keys & KEY_LEFT) {
		if (show_fps == true) show_fps = false;
	}

#endif
	return 0;
}

static char *fps_show()
{
	static char buf[16] = "\0";
#ifdef gpu_unai
	extern bool show_fps;
	sprintf(buf, "%s", show_fps == true ? "on" : "off");
#endif
	return buf;
}

static int framelimit_alter(u32 keys)
{
#ifdef gpu_unai
	extern bool frameLimit;

	if (keys & KEY_RIGHT) {
		if (frameLimit == false) frameLimit = true;
	} else if (keys & KEY_LEFT) {
		if (frameLimit == true) frameLimit = false;
	}

#endif
	return 0;
}

static char *framelimit_show()
{
	static char buf[16] = "\0";
#ifdef gpu_unai
	extern bool frameLimit;
	sprintf(buf, "%s", frameLimit == true ? "on" : "off");
#endif
	return buf;
}

// Disable, because altering those do more harm
#if 0
static int cycle_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (autobias) { autobias = 0; BIAS = 1; }
		else if (BIAS < 4) BIAS++;
	} else if (keys & KEY_LEFT) {
		if (BIAS > 1) BIAS--;
		else { autobias = 1; BIAS = 2; }
	}

	return 0;
}

static char *cycle_show()
{
	static char buf[16] = "\0";
	if (autobias) strcpy(buf, "auto");
	else sprintf(buf, "%d", BIAS);
	return buf;
}

static int clock_alter(u32 keys)
{
	if (keys & KEY_RIGHT) {
		if (PSXCLK < 64000000) PSXCLK += 1000000;
	} else if (keys & KEY_LEFT) {
		if (PSXCLK > 10000000) PSXCLK -= 1000000;
	}

	return 0;
}

static char *clock_show()
{
	static char buf[16] = "\0";
	sprintf(buf, "%d.%d", PSXCLK / 1000000, (PSXCLK % 1000000) / 10000);
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

#ifdef spu_pcsxrearmed
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
#endif

static int settings_back()
{
	return 1;
}

static MENUITEM gui_SettingsItems[] = {
	{(char *)"[PSX] BIOS              ", NULL, &bios_alter, &bios_show},
// Disable, because altering those do more harm
#if 0
	{(char *)"[PSX] Cycle multiplier  ", NULL, &cycle_alter, &cycle_show},
	{(char *)"[PSX] Processor clock   ", NULL, &clock_alter, &clock_show},
#endif
	{(char *)"[PSX] XA audio          ", NULL, &xa_alter, &xa_show},
	{(char *)"[PSX] CDDA audio        ", NULL, &cdda_alter, &cdda_show},
	{(char *)"[PSX] Forced XA updates ", NULL, &forcedxa_alter, &forcedxa_show},
	{(char *)"[PSX] RCntFix           ", NULL, &RCntFix_alter, &RCntFix_show},
	{(char *)"[PSX] VSyncWA           ", NULL, &VSyncWA_alter, &VSyncWA_show},
#ifdef gpu_unai
	{(char *)"[GPU] Show FPS          ", NULL, &fps_alter, &fps_show},
	{(char *)"[GPU] Frame Limit       ", NULL, &framelimit_alter, &framelimit_show},
#endif
	{(char *)"[SPU] Audio sync        ", NULL, &syncaudio_alter, &syncaudio_show},
	{(char *)"[SPU] IRQ fix           ", NULL, &spuirq_alter, &spuirq_show},
#ifdef spu_pcsxrearmed
	{(char *)"[SPU] Interpolation     ", NULL, &interpolation_alter, &interpolation_show},
#endif
	{(char *)"Back to main menu", &settings_back, NULL, NULL},
	{0}
};

#define SET_SIZE ((sizeof(gui_SettingsItems) / sizeof(MENUITEM)) - 1)
static MENU gui_SettingsMenu = { SET_SIZE, 0, 24, 110, (MENUITEM *)&gui_SettingsItems };

static int gui_LoadIso()
{
	static char isoname[PATH_MAX];
	const char *name = FileReq(NULL, NULL, isoname);

	if (name) {
		SetIsoFile(name);
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
	pcsx4all_exit();
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
	port_printf( 4 * 8, 20, "mips recompiler by Ulrich Hecht");
	port_printf( 7 * 8, 30, "optimized by Dmitry Smagin");
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
			if (--menu->cur < 0)
				menu->cur = menu->num - 1;
		} else if (keys & KEY_DOWN) {
			if (++menu->cur == menu->num)
				menu->cur = 0;
		} else if (keys & KEY_A) {
			if (mi->on_press_a) {
				key_reset();
				int result = (*mi->on_press_a)();
				if (result)
					return result;
			}
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