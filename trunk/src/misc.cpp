/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/

/*
* Miscellaneous functions, including savesates and CD-ROM loading.
*/

#include "misc.h"
#include "cdrom.h"
#include "mdec.h"
#include "port.h"

char CdromId[10] = "";
char CdromLabel[33] = "";

int toSaveState=0;
int toLoadState=0;
int toExit=0;
char *SaveState_filename=NULL;

/* PSX Executable types */
#define PSX_EXE     1
#define CPE_EXE     2
#define COFF_EXE    3
#define INVALID_EXE 4

#define ISODCL(from, to) (to - from + 1)

struct iso_directory_record {
	char length			[ISODCL (1, 1)]; /* 711 */
	char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	char extent			[ISODCL (3, 10)]; /* 733 */
	char size			[ISODCL (11, 18)]; /* 733 */
	char date			[ISODCL (19, 25)]; /* 7 by 711 */
	char flags			[ISODCL (26, 26)];
	char file_unit_size		[ISODCL (27, 27)]; /* 711 */
	char interleave			[ISODCL (28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	unsigned char name_len		[ISODCL (33, 33)]; /* 711 */
	char name			[1];
};

void mmssdd( char *b, char *p )
 {
	int m, s, d;
#ifndef __arm__
	int block = (b[0]&0xff) | ((b[1]&0xff)<<8) | ((b[2]&0xff)<<16) | (b[3]<<24);
#else
	int block = b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24);
#endif
	
	block += 150;
	m = block / 4500;			// minuten
	block = block - m * 4500;	// minuten rest
	s = block / 75;				// sekunden
	d = block - s * 75;			// sekunden rest
	
	m = ( ( m / 10 ) << 4 ) | m % 10;
	s = ( ( s / 10 ) << 4 ) | s % 10;
	d = ( ( d / 10 ) << 4 ) | d % 10;	
	
	p[0] = m;
	p[1] = s;
	p[2] = d;
}
#define incTime() \
	time[0] = btoi(time[0]); time[1] = btoi(time[1]); time[2] = btoi(time[2]); \
	time[2]++; \
	if(time[2] == 75) { \
		time[2] = 0; \
		time[1]++; \
		if (time[1] == 60) { \
			time[1] = 0; \
			time[0]++; \
		} \
	} \
	time[0] = itob(time[0]); time[1] = itob(time[1]); time[2] = itob(time[2]);

#define READTRACK() \
	if (CDR_readTrack(time) == -1) return -1; \
	buf = CDR_getBuffer(); \
	if (buf == NULL) return -1;

#define READDIR(_dir) \
	READTRACK(); \
	memcpy(_dir, buf+12, 2048); \
 \
	incTime(); \
	READTRACK(); \
	memcpy(_dir+2048, buf+12, 2048);

int GetCdromFile(u8 *mdir, u8 *time, const char *filename) {
	struct iso_directory_record *dir;
	char ddir[4096];
	u8 *buf;
	int i;

	// only try to scan if a filename is given
	if (!strlen(filename)) return -1;

	i = 0;
	while (i < 4096) {
		dir = (struct iso_directory_record*) &mdir[i];
		if (dir->length[0] == 0) {
			return -1;
		}
		i += dir->length[0];

		if (dir->flags[0] & 0x2) { // it's a dir
			if (!strncasecmp((char *)&dir->name[0], filename, dir->name_len[0])) {
				if (filename[dir->name_len[0]] != '\\') continue;

				filename += dir->name_len[0] + 1;

				mmssdd(dir->extent, (char *)time);
				READDIR(ddir);
				i = 0;
				mdir = (u8*)ddir;
			}
		} else {
			if (!strncasecmp((char *)&dir->name[0], filename, strlen(filename))) {
				mmssdd(dir->extent, (char *)time);
				break;
			}
		}
	}
	return 0;
}

int LoadCdrom() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_LoadCdrom++;
#endif
	EXE_HEADER tmpHead;
	struct iso_directory_record *dir;
	u8 time[4];
	unsigned char *buf;
	u8 mdir[4096];
	char exename[256];

#ifndef DEBUG_CPU
	if (!Config.HLE) {
		psxRegs.pc = psxRegs.GPR.n.ra;
		return 0;
	}
#endif

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record*) &buf[12+156]; 

	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	// Load SYSTEM.CNF and scan for the main executable
	if (GetCdromFile(mdir, time, "SYSTEM.CNF;1") == -1) {
		// if SYSTEM.CNF is missing, start an existing PSX.EXE
		if (GetCdromFile(mdir, time, "PSX.EXE;1") == -1) return -1;

		READTRACK();
	}
	else {
		// read the SYSTEM.CNF
		READTRACK();

		sscanf((char *)buf + 12, "BOOT = cdrom:\\%256s", exename);
		if (GetCdromFile(mdir, time, exename) == -1) {
			sscanf((char *)buf + 12, "BOOT = cdrom:%256s", exename);
			if (GetCdromFile(mdir, time, exename) == -1) {
				char *ptr = strstr((char*)buf + 12, "cdrom:");
				if (ptr != NULL) {
					ptr += 6;
					while (*ptr == '\\' || *ptr == '/') ptr++;
					strncpy(exename, ptr, 255);
					exename[255] = '\0';
					ptr = exename;
					while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') ptr++;
					*ptr = '\0';
					if (GetCdromFile(mdir, time, exename) == -1)
						return -1;
				} else
					return -1;
			}
		}

		// Read the EXE-Header
		READTRACK();
	}

	memcpy(&tmpHead, buf + 12, sizeof(EXE_HEADER));

#ifdef DEBUG_CPU
	if (Config.HLE)
#endif
	{
		psxRegs.pc = SWAP32(tmpHead.pc0);
		psxRegs.GPR.n.gp = SWAP32(tmpHead.gp0);
		psxRegs.GPR.n.sp = SWAP32(tmpHead.s_addr); 
		if (psxRegs.GPR.n.sp == 0) psxRegs.GPR.n.sp = 0x801fff00;
#ifdef DEBUG_CPU
		psxRegs.GPR.r[1]=0x00000025;
		psxRegs.GPR.r[2]=0x00000001;
		psxRegs.GPR.r[4]=0x00000001;
		psxRegs.GPR.r[7]=0x0000002A;
		psxRegs.GPR.r[8]=0x801FFF00;
		psxRegs.GPR.r[10]=0x0000002D;
		psxRegs.GPR.r[11]=0x8002B8C0;
		psxRegs.GPR.r[12]=0x00000023;
		psxRegs.GPR.r[13]=0x0000002B;
		psxRegs.GPR.r[14]=0xA0010000;
		psxRegs.GPR.r[16]=0xA000B870;
		psxRegs.GPR.r[24]=0x00000001;
		psxRegs.GPR.r[26]=0xBFC0D968;
		psxRegs.GPR.r[27]=0x00000F1C;
		psxRegs.GPR.r[29]=0x801FFF00;
		psxRegs.GPR.r[30]=0x801FFF00;
		psxRegs.GPR.r[31]=0xBFC03D60;
		psxRegs.GPR.r[33]=0x00000008;
		if (autobias) {
			psxRegs.cycle=38332544;
			psxRegs.io_cycle_counter=38332584;
			psxNextCounter=2137;
			psxNextsCounter=38330447;
			rcnts[0].cycleStart=38317816;
			rcnts[1].cycleStart=38317871;
			rcnts[2].cycleStart=38317926;
			rcnts[3].cycleStart=38330430;
			psxRegs.intCycle[2]=38224629;
			psxRegs.intCycle[3]=4096;
			psxRegs.intCycle[18]=37699116;
			psxRegs.intCycle[19]=225792;
			psxSetSyncs(249,16);
		} else {
			psxRegs.cycle=73033893;
			psxRegs.io_cycle_counter=73035678;
			psxNextCounter=2130;
			psxNextsCounter=73033548;
			rcnts[0].cycleStart=73019481;
			rcnts[1].cycleStart=73019547;
			rcnts[2].cycleStart=73019613;
			rcnts[3].cycleStart=73033524;
			psxRegs.intCycle[2]=72891387;
			psxRegs.intCycle[3]=4096;
			psxRegs.intCycle[18]=72365592;
			psxRegs.intCycle[19]=225792;
			psxSetSyncs(286,4);
		}
		rcnts[0].mode=5120;
		rcnts[1].mode=5120;
		rcnts[2].mode=5120;
		rcnts[3].mode=3080;
		rcnts[0].cycle=65535;
		rcnts[1].cycle=65535;
		rcnts[2].cycle=65535;
		rcnts[3].cycle=2154;
	} else {
		unsigned new_pc=SWAP32(tmpHead.pc0);
		int back=isdbg();
		dbg_disable();
		psxRegs.pc = psxRegs.GPR.n.ra;
		while (psxRegs.pc != new_pc)
			psxCpu->ExecuteBlock(new_pc);
		if (back)
			dbg_enable();
#endif
	}

	tmpHead.t_size = SWAP32(tmpHead.t_size);
	tmpHead.t_addr = SWAP32(tmpHead.t_addr);

	#ifdef PSXREC
		psxCpu->Clear(tmpHead.t_addr, tmpHead.t_size / 4);
	#endif

	// Read the rest of the main executable
	while (tmpHead.t_size & ~2047) {
		void *ptr = (void *)PSXM(tmpHead.t_addr);

		incTime();
		READTRACK();

		if (ptr != NULL) memcpy(ptr, buf+12, 2048);

		tmpHead.t_size -= 2048;
		tmpHead.t_addr += 2048;
	}

#ifdef DEBUG_CPU
	dbgpsxregs();
	dbgf("Cycle %u, Next %u %u, Syncs %u %u\n",psxRegs.cycle,psxNextCounter,psxNextsCounter,psxGetHSync(),psxGetSpuSync());
	for(unsigned n=0;n<4;n++)
		dbgf("CNT%u mode=%i,target=%i,rate=%i,irq=%i,cnt=%i,stt=%i,c=%i,cs=%i\n",n,rcnts[n].mode,rcnts[n].target,rcnts[n].rate,rcnts[n].irq,rcnts[n].counterState,rcnts[n].irqState,rcnts[n].cycle,rcnts[n].cycleStart);
	dbgsum("Load",psxRegs.psxH,0x10000);
	for(unsigned i=0;i<0x10000;i++)
		if (((unsigned char *)psxRegs.psxH)[i])
			dbgf("psxH[0x%X]=0x%X\n",i,(((unsigned char *)psxRegs.psxH)[i]));
	dbgf("Counters %u %u, IntCycle:",psxNextCounter,psxNextsCounter);
	for(unsigned i=0;i<32;i++){
		if (!(i&7)) dbgf("\n\t%.2u:",i);
		dbgf(" %u", psxRegs.intCycle[i]);
	}
	dbg("");
#endif
	return 0;
}

int LoadCdromFile(const char *filename, EXE_HEADER *head) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_LoadCdromFile++;
#endif
	struct iso_directory_record *dir;
	u8 time[4],*buf;
	unsigned char mdir[4096];
	char exename[256];
	u32 size, addr;

	sscanf(filename, "cdrom:\\%256s", exename);

	time[0] = itob(0); time[1] = itob(2); time[2] = itob(0x10);

	READTRACK();

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record *)&buf[12 + 156]; 

	mmssdd(dir->extent, (char*)time);

	READDIR(mdir);

	if (GetCdromFile(mdir, time, exename) == -1) return -1;

	READTRACK();

	memcpy(head, buf + 12, sizeof(EXE_HEADER));
	size = head->t_size;
	addr = head->t_addr;

	#ifdef PSXREC
		psxCpu->Clear(addr, size/4);
	#endif

	while (size & ~2047) {
		incTime();
		READTRACK();

		memcpy((void *)PSXM(addr), buf + 12, 2048);

		size -= 2048;
		addr += 2048;
	}

	return 0;
}

int CheckCdrom() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_CheckCdrom++;
#endif
	struct iso_directory_record *dir;
	unsigned char time[4], *buf;
	unsigned char mdir[4096];
	char exename[256];
	int i, c;

	time[0] = itob(0);
	time[1] = itob(2);
	time[2] = itob(0x10);

	READTRACK();

	CdromLabel[0] = '\0';
	CdromId[0] = '\0';

	strncpy(CdromLabel, (char*)buf + 52, 32);

	// skip head and sub, and go to the root directory record
	dir = (struct iso_directory_record *)&buf[12 + 156]; 

	mmssdd(dir->extent, (char *)time);

	READDIR(mdir);

	if (GetCdromFile(mdir, time, "SYSTEM.CNF;1") != -1) {
		READTRACK();

		sscanf((char *)buf + 12, "BOOT = cdrom:\\%256s", exename);
		if (GetCdromFile(mdir, time, exename) == -1) {
			sscanf((char *)buf + 12, "BOOT = cdrom:%256s", exename);
			if (GetCdromFile(mdir, time, exename) == -1) {
				char *ptr = strstr((char *)buf + 12, "cdrom:");			// possibly the executable is in some subdir
				if (ptr != NULL) {
					ptr += 6;
					while (*ptr == '\\' || *ptr == '/') ptr++;
					strncpy(exename, ptr, 255);
					exename[255] = '\0';
					ptr = exename;
					while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') ptr++;
					*ptr = '\0';
					if (GetCdromFile(mdir, time, exename) == -1)
					 	return -1;		// main executable not found
				} else
					return -1;
			}
		}
	} else if (GetCdromFile(mdir, time, "PSX.EXE;1") != -1) {
		strcpy(exename, "PSX.EXE;1");
		strcpy(CdromId, "SLUS99999");
	} else
		return -1;		// SYSTEM.CNF and PSX.EXE not found

	if (CdromId[0] == '\0') {
		i = strlen(exename);
		if (i >= 2) {
			if (exename[i - 2] == ';') i-= 2;
			c = 8; i--;
			while (i >= 0 && c >= 0) {
				if (isalnum(exename[i])) CdromId[c--] = exename[i];
				i--;
			}
		}
	}

	if (Config.PsxAuto) { // autodetect system (pal or ntsc)
		if (CdromId[2] == 'e' || CdromId[2] == 'E')
			Config.PsxType = PSX_TYPE_PAL; // pal
		else Config.PsxType = PSX_TYPE_NTSC; // ntsc
		psxRcntUVtarget();
	}

	if (CdromLabel[0] == ' ') {
		strncpy(CdromLabel, CdromId, 9);
	}
#ifndef DEBUG_BIOS
	printf("CD-ROM Label: %.32s\n", CdromLabel);
	printf("CD-ROM ID: %.9s\n", CdromId);
#endif

	return 0;
}

static int PSXGetFileType(FILE *f) {
	unsigned long current;
	u8 mybuf[2048];
	EXE_HEADER *exe_hdr;
	FILHDR *coff_hdr;

	current = ftell(f);
	fseek(f, 0L, SEEK_SET);
    fread(mybuf, 2048, 1, f);
	fseek(f, current, SEEK_SET);

	exe_hdr = (EXE_HEADER *)mybuf;
	if (memcmp(exe_hdr->id, "PS-X EXE", 8) == 0)
		return PSX_EXE;

	if (mybuf[0] == 'C' && mybuf[1] == 'P' && mybuf[2] == 'E')
		return CPE_EXE;

	coff_hdr = (FILHDR *)mybuf;
	if (SWAPu16(coff_hdr->f_magic) == 0x0162)
		return COFF_EXE;

	return INVALID_EXE;
}
#define FREAD(c,a,b) fread(a,sizeof(u8),b,c)
/* TODO Error handling - return integer for each error case below, defined in an enum. Pass variable on return */
int Load(const char *ExePath) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_Load++;
#endif
	FILE *tmpFile;
	EXE_HEADER tmpHead;
	int type;
	int retval = 0;
	u8 opcode;
	u32 section_address, section_size;
	void *mem;

	strncpy(CdromId, "SLUS99999", 9);
	strncpy(CdromLabel, "SLUS_999.99", 11);

	tmpFile = fopen(ExePath, "rb");
	if (tmpFile == NULL) {
		printf("Error opening file: %s.\n", ExePath);
		retval = -1;
	} else {
		type = PSXGetFileType(tmpFile);
		switch (type) {
			case PSX_EXE:
                fread(&tmpHead,sizeof(EXE_HEADER),1,tmpFile);
				section_address = SWAP32(tmpHead.t_addr);
				section_size = SWAP32(tmpHead.t_size);
				mem = PSXM(section_address);
				if (mem != NULL) {
					fseek(tmpFile, 0x800, SEEK_SET);		
                    fread(mem, section_size, 1, tmpFile);
					#ifdef PSXREC
						psxCpu->Clear(section_address, section_size / 4);
					#endif
				}
				fclose(tmpFile);
				psxRegs.pc = SWAP32(tmpHead.pc0);
				psxRegs.GPR.n.gp = SWAP32(tmpHead.gp0);
				psxRegs.GPR.n.sp = SWAP32(tmpHead.s_addr); 
				if (psxRegs.GPR.n.sp == 0)
					psxRegs.GPR.n.sp = 0x801fff00;
				retval = 0;
				break;
			case CPE_EXE:
				fseek(tmpFile, 6, SEEK_SET); /* Something tells me we should go to 4 and read the "08 00" here... */
				do {
                    fread(&opcode, 1, 1, tmpFile);
					switch (opcode) {
						case 1: /* Section loading */
                            fread(&section_address, 4, 1, tmpFile);
                            fread(&section_size, 4, 1, tmpFile);
							section_address = SWAPu32(section_address);
							section_size = SWAPu32(section_size);
#ifdef EMU_LOG
							EMU_LOG("Loading %08X bytes from %08X to %08X\n", section_size, ftell(tmpFile), section_address);
#endif
							mem = PSXM(section_address);
							if (mem != NULL) {
                                fread(mem, section_size, 1, tmpFile);
								#ifdef PSXREC
									psxCpu->Clear(section_address, section_size / 4);
								#endif
							}
							break;
						case 3: /* register loading (PC only?) */
							fseek(tmpFile, 2, SEEK_CUR); /* unknown field */
                            fread(&psxRegs.pc, 4, 1, tmpFile);
							psxRegs.pc = SWAPu32(psxRegs.pc);
							break;
						case 0: /* End of file */
							break;
						default:
							printf("Unknown CPE opcode %02x at position %08x.\n", opcode, ftell(tmpFile) - 1);
							retval = -1;
							break;
					}
				} while (opcode != 0 && retval == 0);
				break;
			case COFF_EXE:
				printf("COFF files not supported.\n");
				retval = -1;
				break;
			case INVALID_EXE:
				printf("This file does not appear to be a valid PSX file.\n");
				retval = -1;
				break;
		}
	}

	if (retval != 0) {
		CdromId[0] = '\0';
		CdromLabel[0] = '\0';
	}

	return retval;
}

// STATES

static const char PcsxHeader[32] = "STv4 PCSX v" PACKAGE_VERSION;

// Savestate Versioning!
// If you make changes to the savestate version, please increment the value below.
static const u32 SaveVersion = 0x8b410004;
#define FWRITE(c,a,b) fwrite(a,sizeof(u8),b,c)
int SaveState(const char *file) {
    FILE* f;
	GPUFreeze_t *gpufP;
	SPUFreeze_t *spufP;
	int Size;
	unsigned char *pMem;

    f = fopen(file, "wb");
	if (f == NULL) return -1;

	port_mute();
	
    FWRITE(f, (void*)PcsxHeader, 32);
    FWRITE(f, (void *)&SaveVersion, sizeof(u32));
    FWRITE(f, (void *)&Config.HLE, sizeof(boolean));

	pMem = (unsigned char *) malloc(128*96*3);
	if (pMem == NULL) return -1;
    FWRITE(f, pMem, 128*96*3);
	free(pMem);

	if (Config.HLE)
		psxBiosFreeze(1);

    FWRITE(f, psxM, 0x00200000);
    FWRITE(f, psxR, 0x00080000);
    FWRITE(f, psxH, 0x00010000);
    FWRITE(f, (void*)&psxRegs, sizeof(psxRegs));

	// gpu
	gpufP = (GPUFreeze_t *) malloc(sizeof(GPUFreeze_t));
	gpufP->ulFreezeVersion = 1;
    if(!GPU_freeze(1, gpufP)) return -3;
    FWRITE(f, gpufP, sizeof(GPUFreeze_t));
	free(gpufP);

	// spu
	spufP = (SPUFreeze_t *) malloc(16);
	SPU_freeze(2, spufP);
    Size = spufP->Size; FWRITE(f, &Size, 4);
	free(spufP);
	spufP = (SPUFreeze_t *) malloc(Size);
    if(!SPU_freeze(1, spufP)) return -4;
    FWRITE(f, spufP, Size);
	free(spufP);

	sioFreeze(f, 1);
	cdrFreeze(f, 1);
	psxHwFreeze(f, 1);
	psxRcntFreeze(f, 1);
	mdecFreeze(f, 1);

    fclose(f);

	port_sync();
	
	return 0;
}

int LoadState(const char *file) {
    FILE* f;
	GPUFreeze_t *gpufP;
	SPUFreeze_t *spufP;
	int Size;
	char header[32];
	u32 version;
	boolean hle;
    f = fopen(file, "rb");
	if (f == NULL) return -1;
    FREAD(f, header, sizeof(header));
    FREAD(f, &version, sizeof(u32));
    FREAD(f, &hle, sizeof(boolean));

	if (strncmp("STv4 PCSX", header, 9) != 0 || version != SaveVersion || hle != Config.HLE) {
        fclose(f);
        return -2;
	}
    psxCpu->Reset();

    fseek(f, 128*96*3, SEEK_CUR);

    FREAD(f, psxM, 0x00200000);
    FREAD(f, psxR, 0x00080000);
    FREAD(f, psxH, 0x00010000);
#ifdef DEBUG_BIOS
	u32 repeated=0, regs_offset=gztell(f);
#endif
    FREAD(f, (void*)&psxRegs, sizeof(psxRegs));
	psxRegs.psxM=psxM;
	psxRegs.psxP=psxP;
	psxRegs.psxR=psxR;
	psxRegs.psxH=psxH;
	psxRegs.io_cycle_counter=0;
#if !defined(DEBUG_CPU) && (defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES))
	psxRegs.cycle_add=0;
	gzseek(f, gztell(f)-4, SEEK_SET);
#endif

	if (Config.HLE)
		psxBiosFreeze(0);

#ifdef DEBUG_BIOS
 label_repeat_:
#endif
	// gpu

    gpufP = (GPUFreeze_t *) malloc (sizeof(GPUFreeze_t));
    FREAD(f, gpufP, sizeof(GPUFreeze_t));
    if(!GPU_freeze(0, gpufP)) return -3;
    free(gpufP);

    // spu
    FREAD(f, &Size, 4);
    spufP = (SPUFreeze_t *) malloc (Size);
    FREAD(f, spufP, Size);
    if(!SPU_freeze(0, spufP)) return -4;
    free(spufP);

	sioFreeze(f, 0);
	cdrFreeze(f, 0);
	psxHwFreeze(f, 0);
	psxRcntFreeze(f, 0);
	mdecFreeze(f, 0);

    fclose(f);

#ifdef DEBUG_CPU_OPCODES
#ifndef DEBUG_BIOS
	dbgsum("LOADSTATE MEMORY SUM:",(void *)&psxM[0],0x200000);
#else
	if (Config.HLE) {
		if (!repeated && !strcmp("dbgbios_hle",file)){
			repeated=1;
			f = gzopen("dbgbios_bios", "rb");
			if (f) {
				gzseek(f,regs_offset,SEEK_SET);
                FREAD(f, (void*)&psxRegs, sizeof(psxRegs));
				psxRegs.psxM=psxM;
				psxRegs.psxP=psxP;
				psxRegs.psxR=psxR;
				psxRegs.psxH=psxH;
				psxRegs.io_cycle_counter=0;
				goto label_repeat_;
			}
		}
	}
	printf("INTS 0x%X, 0x%X (0x%X), HSync=%u %u\n",psxHu32(0x1070),psxHu32(0x1074),psxRegs.CP0.n.Status,psxGetHSync(),psxGetSpuSync());
#endif
	dbgregs((void*)&psxRegs);
	dbgregsCop((void *)&psxRegs);
#endif

	return 0;
}

int CheckState(const char *file) {
    FILE* f;
	char header[32];
	u32 version;
	boolean hle;

    f = fopen(file, "rb");
	if (f == NULL) return -1;

    FREAD(f, header, sizeof(header));
    FREAD(f, &version, sizeof(u32));
    FREAD(f, &hle, sizeof(boolean));

    fclose(f);

	if (strncmp("STv4 PCSX", header, 9) != 0 || version != SaveVersion || hle != Config.HLE)
		return -1;

	return 0;
}
