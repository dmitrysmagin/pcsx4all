#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "wiz_lib.h"
#include "frontend_menu.h"
#include "frontend_splash.h"

int game_num_avail=0;
static int last_game_selected=0;
char playemu[] = "pcsx4all_caanoo\0";
char playgame[256] = "builtinn\0";
char frontend_filename[] = "pcsx4all_caanoo.gpe";
#define NUMGAMES 256
char games[NUMGAMES][256] = {0};

int wiz_freq=700;
int wiz_ramtweaks=0;
int wiz_framelimit=0;
int wiz_frameskip=0;
int wiz_interlace=0;
int wiz_sound=4;
int wiz_psxclock=100;
int wiz_bias=0;
int wiz_core=0;
int wiz_gpu=0;
int wiz_bios=0;
int wiz_fixes=0;
int wiz_alt_fps=0;
int wiz_autosave=0;

static void load_bmp_8bpp(unsigned char *out, unsigned char *in) 
{
	int i,x,y;
	unsigned char r,g,b,c;
	in+=14; /* Skip HEADER */
	in+=40; /* Skip INFOHD */
	/* Set Palette */
	for (i=0;i<256;i++) {
		b=*in++;
		g=*in++;
		r=*in++;
		c=*in++;
		wiz_video_color8(i,r,g,b);
	}
	wiz_video_setpalette();
	/* Set Bitmap */	
	for (y=239;y!=-1;y--) {
		for (x=0;x<320;x++) {
			*out++=in[x+y*320];
		}
	}
}

static void wiz_intro_screen(void) {
	wiz_video_flip();
	load_bmp_8bpp(fb1_8bit,splash_bmp);
	wiz_video_flip();
	wiz_timer_delay(4000000); // 4 seconds
}

static void game_list_sort(void)
{
	int i = 0;
	int j = 0;
	char tmp[256];

	for(i = 0; i < game_num_avail; ++i)
	{
		for(j = i + 1; j < game_num_avail; ++j)
		{
			if(strcmp(games[i], games[j]) > 0)
			{
				strcpy(tmp,games[i]);
				strcpy(games[i],games[j]);
				strcpy(games[j],tmp);
			}
		}
	}
}

static void game_list_init(void)
{
	DIR *d=opendir("isos");
	if (d)
	{
		struct dirent *actual=readdir(d);
		while(actual)
		{
			if (((strstr(actual->d_name,".bin")!=NULL) ||
				(strstr(actual->d_name,".img")!=NULL) ||
				(strstr(actual->d_name,".mdf")!=NULL) ||
				(strstr(actual->d_name,".iso")!=NULL)) &&
				(strstr(actual->d_name,".cfg")==NULL))
			{
				strcpy(games[game_num_avail],"isos/");
				strcat(games[game_num_avail],actual->d_name);
				game_num_avail++;
			}
			actual=readdir(d);
		}
		closedir(d);
	}
	d=opendir("exec");
	if (d)
	{
		struct dirent *actual=readdir(d);
		while(actual)
		{
			if (strstr(actual->d_name,".exe")!=NULL)
			{
				strcpy(games[game_num_avail],"exec/");
				strcat(games[game_num_avail],actual->d_name);
				game_num_avail++;
			}
			actual=readdir(d);
		}
		closedir(d);
	}
	
	if (game_num_avail)
	{
		game_list_sort();
	}
}

static void game_list_view(int *pos) {

	int i;
	int view_pos;
	int aux_pos=0;
	int screen_y = 45;
	int screen_x = 38;

	/* Draw background image */
	load_bmp_8bpp(fb1_8bit,menu_bmp);

	/* Check Limits */
	if (*pos<0)
		*pos=game_num_avail-1;
	if (*pos>(game_num_avail-1))
		*pos=0;
					   
	/* Set View Pos */
	if (*pos<10) {
		view_pos=0;
	} else {
		if (*pos>game_num_avail-11) {
			view_pos=game_num_avail-21;
			view_pos=(view_pos<0?0:view_pos);
		} else {
			view_pos=*pos-10;
		}
	}

	/* Show List */
	for (i=0;i<game_num_avail;i++) {
		if (aux_pos>=view_pos && aux_pos<=view_pos+20) {
			wiz_gamelist_text_out( screen_x, screen_y, games[i]);
			if (aux_pos==*pos) {
				wiz_gamelist_text_out( screen_x-10, screen_y,">" );
				wiz_gamelist_text_out( screen_x-13, screen_y-1,"-" );
			}
			screen_y+=8;
		}
		aux_pos++;
	}
}

static int show_options(char *game)
{
	unsigned long ExKey=0;
	int selected_option=0;
	int x_Pos = 41;
	int y_Pos = 58;
	int options_count = 12;
	char text[256];
	char fnameconf[512];
	FILE *f;

	/* Read game configuration */
	sprintf(fnameconf,"conf/%s.cfg",games[last_game_selected]+5);
	f=fopen(fnameconf,"r");
	if (!f) {
		sprintf(fnameconf,"%s.cfg",games[last_game_selected]+5);
		f=fopen(fnameconf,"r");
	}
	if (!f) {
		sprintf(fnameconf,"isos/%s.cfg",games[last_game_selected]+5);
		f=fopen(fnameconf,"r");
	}
	if (f) {
		fscanf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&wiz_freq,&wiz_ramtweaks,&wiz_framelimit,&wiz_frameskip,
			&wiz_interlace,&wiz_sound,&wiz_psxclock,&wiz_bias,&wiz_core,&wiz_gpu,&wiz_bios,&wiz_fixes,&wiz_alt_fps,
			&wiz_autosave);
		fclose(f);
	}
	
	while(1)
	{
		/* Draw background image */
		load_bmp_8bpp(fb1_8bit,menu_bmp);

		/* Draw the options */
		strncpy (text,games[last_game_selected],33);
		text[32]='\0';
		wiz_gamelist_text_out(x_Pos,y_Pos-10,text);

		/* (0) WIZ Clock */
		wiz_gamelist_text_out_fmt(x_Pos,y_Pos+10, "CAANOO Clock  %d MHz", wiz_freq);

		/* (1) RAM Tweaks */
		if (wiz_ramtweaks)
			wiz_gamelist_text_out(x_Pos,y_Pos+20,"RAM Tweaks    ON");
		else
			wiz_gamelist_text_out(x_Pos,y_Pos+20,"RAM Tweaks    OFF");

		/* (2) Frame-Limit */
		if (wiz_framelimit)
			wiz_gamelist_text_out(x_Pos,y_Pos+30,"Frame-Limit   ON");
		else
			wiz_gamelist_text_out(x_Pos,y_Pos+30,"Frame-Limit   OFF");
			
		/* (3) Frame-Skip */
		wiz_gamelist_text_out_fmt(x_Pos,y_Pos+40,"Frame-Skip    %d %s",wiz_frameskip,(wiz_alt_fps?"(Game)":"(Video)"));

		/* (4) Interlace */
		switch(wiz_interlace)
		{
			case 0: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Interlace     %s","OFF"); break;
			case 1: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Interlace     %s","Simple"); break;
			case 2: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+50,"Interlace     %s","Progressive"); break;
		}
		
		/* (5) Sound */
		switch(wiz_sound)
		{
			case 0: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+60,"Sound         %s","OFF"); break;
			case 1: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+60,"Sound         %s","ON (basic)"); break;
			case 2: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+60,"Sound         %s","ON (XA)"); break;
			case 3: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+60,"Sound         %s","ON (CD-Audio)"); break;
			case 4: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+60,"Sound         %s","ON (XA+CD-Audio)"); break;
		}

		/* (6) CPU Clock */
		wiz_gamelist_text_out_fmt(x_Pos,y_Pos+70,"CPU Clock     %d%%",wiz_psxclock);

		/* (7) CPU Bias */
		if (wiz_bias)
			wiz_gamelist_text_out_fmt(x_Pos,y_Pos+80,"CPU Bias      %d",wiz_bias);
		else
			wiz_gamelist_text_out_fmt(x_Pos,y_Pos+80,"CPU Bias      Auto");

		/* (8) CPU Core */
		if (wiz_core==0)
		{
			switch(wiz_bios)
			{
				case 0: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Core      %s","HLE"); break;
				case 1: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Core      %s","BIOS"); break;
				case 2: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Core      %s","HLE-Secure"); break;
			}
		}
		else
		{
			switch(wiz_bios)
			{
				case 0: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Core      %s","HLE (Interpreter)"); break;
				case 1: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Core      %s","BIOS (Interpreter)"); break;
				case 2: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+90,"CPU Core      %s","HLE (Interpreter)"); break;
			}
		}
		
		/* (9) GPU Options */
		switch(wiz_gpu)
		{
			case 0: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+100,"GPU Type      %s","Software"); break;
			case 1: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+100,"GPU Type      %s","No Light"); break;
			case 2: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+100,"GPU Type      %s","No Blend"); break;
			case 3: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+100,"GPU Type      %s","No Light+Blend"); break;
		}
		
		/* (10) Auto-Save */
		switch(wiz_autosave)
		{
			case 0: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+110,"Auto-Save     %s","OFF"); break;
			case 1: wiz_gamelist_text_out_fmt(x_Pos,y_Pos+110,"Auto-Save     %s","ON"); break;
		}
		
		/* (11) Game Fixes */
		switch(wiz_fixes)
		{
			case 0: wiz_gamelist_text_out(x_Pos,y_Pos+120,"No Game Fixes"); break;
			case 1: wiz_gamelist_text_out(x_Pos,y_Pos+120,"Sio Irq Always Enabled"); break;
			case 2: wiz_gamelist_text_out(x_Pos,y_Pos+120,"Spu Irq Always Enabled"); break;
			case 3: wiz_gamelist_text_out(x_Pos,y_Pos+120,"Parasite Eve 2, Vandal Hearts 1/2 fix"); break;
			case 4: wiz_gamelist_text_out(x_Pos,y_Pos+120,"InuYasha Sengoku Battle fix"); break;
			case 5: wiz_gamelist_text_out(x_Pos,y_Pos+120,"Abbe's Odyssey fix"); break;
		}
	
		wiz_gamelist_text_out(x_Pos,y_Pos+140,"Press B to confirm, X to return\0");

		/* Show currently selected item */
		wiz_gamelist_text_out(x_Pos-16,y_Pos+(selected_option*10)+10," >");

		wiz_video_flip();
		while(wiz_joystick_read(0)&0xFFFE00) { wiz_timer_delay(150000); }
		while(!(ExKey=wiz_joystick_read(0)&0xFFFE00)) { }
		if(ExKey & WIZ_DOWN){
			selected_option++;
			selected_option = selected_option % options_count;
		}
		else if(ExKey & WIZ_UP){
			selected_option--;
			if(selected_option<0)
				selected_option = options_count - 1;
		}
		else if(ExKey & WIZ_R || ExKey & WIZ_L)
		{
			switch(selected_option) {
			case 0:
				/* WIZ Clock */
				if(ExKey & WIZ_R){
					switch (wiz_freq) {
                        case 200: wiz_freq=300;break;
                        case 300: wiz_freq=400;break;
                        case 400: wiz_freq=500;break;
                        case 500: wiz_freq=533;break;
                        case 533: wiz_freq=650;break;
                        case 650: wiz_freq=700;break;
                        case 700: wiz_freq=750;break;
                        case 750: wiz_freq=760;break;
                        case 760: wiz_freq=770;break;
                        case 770: wiz_freq=780;break;
                        case 780: wiz_freq=790;break;
                        case 790: wiz_freq=795;break;
                        case 795: wiz_freq=800;break;
                        case 800: wiz_freq=805;break;
                        case 805: wiz_freq=810;break;
                        case 810: wiz_freq=815;break;
                        case 815: wiz_freq=820;break;
                        case 820: wiz_freq=825;break;
                        case 825: wiz_freq=830;break;
                        case 830: wiz_freq=835;break;
                        case 835: wiz_freq=840;break;
                        case 840: wiz_freq=845;break;
                        case 845: wiz_freq=850;break;
                        case 850: wiz_freq=855;break;
                        case 855: wiz_freq=860;break;
                        case 860: wiz_freq=865;break;
                        case 865: wiz_freq=870;break;
                        case 870: wiz_freq=875;break;
                        case 875: wiz_freq=880;break;
                        case 880: wiz_freq=885;break;
                        case 885: wiz_freq=890;break;
                        case 890: wiz_freq=895;break;
                        case 895: wiz_freq=900;break;
                        case 900: wiz_freq=200;break;
                        default:  wiz_freq=533;break;
					}
				} else {
					switch (wiz_freq) {
                        case 200: wiz_freq=900;break;
                        case 300: wiz_freq=200;break;
                        case 400: wiz_freq=300;break;
                        case 500: wiz_freq=400;break;
                        case 533: wiz_freq=500;break;
                        case 650: wiz_freq=533;break;
                        case 700: wiz_freq=650;break;
                        case 750: wiz_freq=700;break;
                        case 760: wiz_freq=750;break;
                        case 770: wiz_freq=760;break;
                        case 780: wiz_freq=770;break;
                        case 790: wiz_freq=780;break;
                        case 795: wiz_freq=790;break;
                        case 800: wiz_freq=795;break;
                        case 805: wiz_freq=800;break;
                        case 810: wiz_freq=805;break;
                        case 815: wiz_freq=810;break;
                        case 820: wiz_freq=815;break;
                        case 825: wiz_freq=820;break;
                        case 830: wiz_freq=825;break;
                        case 835: wiz_freq=830;break;
                        case 840: wiz_freq=835;break;
                        case 845: wiz_freq=840;break;
                        case 850: wiz_freq=845;break;
                        case 855: wiz_freq=850;break;
                        case 860: wiz_freq=855;break;
                        case 865: wiz_freq=860;break;
                        case 870: wiz_freq=865;break;
                        case 875: wiz_freq=870;break;
                        case 880: wiz_freq=875;break;
                        case 885: wiz_freq=880;break;
                        case 890: wiz_freq=885;break;
                        case 895: wiz_freq=890;break;
                        case 900: wiz_freq=895;break;
                        default:  wiz_freq=533;break;
					}
				}
				break;
			case 1:
				wiz_ramtweaks=!wiz_ramtweaks;
				break;
			case 2:
				wiz_framelimit=!wiz_framelimit;
				break;
			case 3:
				if(ExKey & WIZ_R)
				{
					wiz_frameskip++;
					if (wiz_frameskip>11) { wiz_frameskip=0; wiz_alt_fps=!wiz_alt_fps; }
				}
				else
				{
					wiz_frameskip--;
					if (wiz_frameskip<0) { wiz_frameskip=11; wiz_alt_fps=!wiz_alt_fps; }
				}
				break;
			case 4:
				wiz_interlace++; if (wiz_interlace>2) wiz_interlace=0;
				break;
			case 5:
				if(ExKey & WIZ_R)
				{
					wiz_sound++;
					if (wiz_sound>4) wiz_sound=0;
				}
				else
				{
					wiz_sound--;
					if (wiz_sound<0) wiz_sound=4;
				}
				break;
			case 6:
				if(ExKey & WIZ_R)
				{
					wiz_psxclock += 10; /* Add 10% */
					if (wiz_psxclock > 200) /* 200% is the max */
						wiz_psxclock = 200;
				}
				else
				{
					wiz_psxclock -= 10; /* Subtract 10% */
					if (wiz_psxclock < 10) /* 10% is the min */
						wiz_psxclock = 10;
				}
				break;
			case 7:
				if(ExKey & WIZ_R)
				{
					wiz_bias++;
					if (wiz_bias>16) wiz_bias=0;
				}
				else
				{
					wiz_bias--;
					if (wiz_bias<0) wiz_bias=16;
				}
				break;
			case 8:
				wiz_bios=wiz_bios+1; if (wiz_bios>2) { wiz_bios=0; wiz_core=!wiz_core; }
				break;
			case 9:
				wiz_gpu=wiz_gpu+1;
				if (wiz_gpu>3) wiz_gpu=0;
				break;
			case 10:
				wiz_autosave=!wiz_autosave;
				break;
            case 11:
				if(ExKey & WIZ_R)
				{
					wiz_fixes++;
					if (wiz_fixes>5) wiz_fixes=0;
				}
				else
				{
					wiz_fixes--;
					if (wiz_fixes<0) wiz_fixes=5;
				}
                break;
			}
		}

		if ((ExKey & WIZ_A) || (ExKey & WIZ_B) || (ExKey & WIZ_MENU)) 
		{
			/* Write game configuration */
//			sprintf(fnameconf,"conf/%s.cfg",games[last_game_selected]+5);
			f=fopen(fnameconf,"w");
			if (f) {
				fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",wiz_freq,wiz_ramtweaks,wiz_framelimit,wiz_frameskip,
					wiz_interlace,wiz_sound,wiz_psxclock,wiz_bias,wiz_core,wiz_gpu,wiz_bios,wiz_fixes,wiz_alt_fps,
					wiz_autosave);
				fclose(f);
				sync();
			}

			/* Selected game will be run */
			return 1;
		}
		else if ((ExKey & WIZ_X) || (ExKey & WIZ_Y) || (ExKey & WIZ_SELECT) || (ExKey & WIZ_HOME))
		{
			/* Return To Menu */
			return 0;
		}
	}
}

static void wiz_exit(void)
{
	wiz_deinit();
	chdir("/usr/gp2x"); /*go to menu*/
   	execl("gp2xmenu", "gp2xmenu", NULL);
}

static void select_game(char *emu, char *game)
{

	unsigned long ExKey;

	/* No Selected game */
	strcpy(game,"builtinn");

	/* Clean screen */
	wiz_video_flip();

	/* Wait until user selects a game */
	while(1)
	{
		game_list_view(&last_game_selected);
		wiz_video_flip();

		if( (wiz_joystick_read(0)&0xFFFE00)) 
			wiz_timer_delay(100000); 
		while(!(ExKey=wiz_joystick_read(0)&0xFFFE00))
		{ 
			if ((ExKey & WIZ_L) && (ExKey & WIZ_R)) { wiz_exit(); }
		}

		if (ExKey & WIZ_UP) last_game_selected--;
		if (ExKey & WIZ_DOWN) last_game_selected++;
		if (ExKey & WIZ_L) last_game_selected-=21;
		if (ExKey & WIZ_R) last_game_selected+=21;
		if (ExKey & WIZ_HOME) wiz_exit();

		if ((ExKey & WIZ_A) || (ExKey & WIZ_B) || (ExKey & WIZ_MENU))
		{
			/* Select the game */
			strcpy(game,games[last_game_selected]);

			/* Emulation Options */
			if(show_options(game))
			{
				break;
			}
		}
	}
}

void execute_game (char *playemu, char *playgame)
{
	char *args[255];
	char str[8][256];
	int n=0;
	int i=0;
	
	/* Executable */
	args[n]=playemu; n++;

	/* WIZ Clock */
	args[n]="-clock"; n++;
	sprintf(str[i],"%d",wiz_freq);
	args[n]=str[i]; i++; n++;
	
	/* RAM Tweaks */
	if (wiz_ramtweaks)
	{
		args[n]="-ramtweaks"; n++;
	}
	
	/* Frame Limit */
	if (wiz_framelimit)
	{
		args[n]="-framelimit"; n++;
	}
	
	/* Frame Skip */
	args[n]="-skip"; n++;
	sprintf(str[i],"%d",wiz_frameskip);
	args[n]=str[i]; i++; n++;
	
	/* Interlace */
	if (wiz_interlace==1)
	{
		args[n]="-interlace"; n++;
	}
	if (wiz_interlace==2)
	{
		args[n]="-progressive"; n++;
	}

	/* Sound */
	if (wiz_sound==0)
	{
		args[n]="-silent"; n++;
	}
	if ((wiz_sound==2) || (wiz_sound==4))
	{
		args[n]="-xa"; n++;
	}
	if ((wiz_sound==3) || (wiz_sound==4))
	{
		args[n]="-cdda"; n++;
	}
	
	/* CPU Clock */
	args[n]="-adjust"; n++;
	sprintf(str[i],"%f",(float)wiz_psxclock/100.0f);
	args[n]=str[i]; i++; n++;
	
	/* CPU Bias */
	args[n]="-bias"; n++; 
	sprintf(str[i],"%d",wiz_bias);
	args[n]=str[i]; i++; n++;
	
	/* CPU Core */
	if (wiz_core)
	{
		args[n]="-interpreter"; n++;
	}
	
	/* GPU Type */
	if ((wiz_gpu==1) || (wiz_gpu==3))
	{
		args[n]="-no_light"; n++;
	}
	if ((wiz_gpu==2) || (wiz_gpu==3))
	{
		args[n]="-no_blend"; n++;
	}
	
	/* BIOS */
	if (wiz_bios==1)
	{
		args[n]="-bios"; n++;
	}
	if (wiz_bios==2)
	{
		args[n]="-secure_writes"; n++;
	}
	
	/* Game Fixes */
	switch(wiz_fixes)
	{
		case 0: break;
		case 1: args[n]="-sioirq"; n++; break;
		case 2: args[n]="-spuirq"; n++; break;
		case 3: args[n]="-rcntfix"; n++; break;
		case 4: args[n]="-vsyncwa"; n++; break;
		case 5: args[n]="-abbey"; n++; break;
	}
	
	/* Alternate FPS algorithm */
	if (wiz_alt_fps)
	{
		args[n]="-alt_fps"; n++;
	}
	
	args[n]="-frontend"; n++;
	args[n]=frontend_filename; n++;
	
	if (strstr(playgame,"exec/")!=NULL)
	{
		args[n]="-file"; n++;
	}
	else
	{
		args[n]="-iso"; n++;
	}
	
	/* playgame */
	args[n]=playgame; n++;
	
	/* save states */
	args[n]="-savestate"; n++;
	sprintf(str[i],"save/%s.sav",playgame+5);
	args[n]=str[i]; i++; n++;
	if (wiz_autosave)
	{
		args[n]="-autosavestate"; n++;
	}
	
	args[n]=NULL;
	
	for (i=0; i<n; i++)
	{
		printf("%s ",args[i]);
	}
	printf("\n");
	
	wiz_deinit();
	execv(args[0], args); 
}


int main (int argc, char **argv)
{
	/* WIZ Initialization */
	wiz_init(8,22050,16,0);

	/* Show intro screen */
	wiz_intro_screen();

	/* Initialize list of available games */
	game_list_init();
	if (game_num_avail==0)
	{
		wiz_video_flip();
		wiz_gamelist_text_out(35, 110, "ERROR: NO AVAILABLE GAMES FOUND");
		wiz_video_flip();
		wiz_joystick_press(0);
		wiz_exit();
	}
	
	/* Select Game */
	select_game(playemu,playgame); 

	/* Execute Game */
	execute_game (playemu,playgame);
	
	exit (0);
}
