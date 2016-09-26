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
* SIO functions (Pads / Memcards)
*/

#include "sio.h"
#include "psxevents.h"
#include <sys/stat.h>
#include <unistd.h>

// Status Flags
#define TX_RDY		0x0001
#define RX_RDY		0x0002
#define TX_EMPTY	0x0004
#define PARITY_ERR	0x0008
#define RX_OVERRUN	0x0010
#define FRAMING_ERR	0x0020
#define SYNC_DETECT	0x0040
#define DSR			0x0080
#define CTS			0x0100
#define IRQ			0x0200

// Control Flags
#define TX_PERM		0x0001
#define DTR			0x0002
#define RX_PERM		0x0004
#define BREAK		0x0008
#define RESET_ERR	0x0010
#define RTS			0x0020
#define SIO_RESET	0x0040

// *** FOR WORKS ON PADS AND MEMORY CARDS *****

static unsigned char buf[256];
//senquack - updated to match PCSXR code:
static unsigned char cardh1[4] = { 0xff, 0x08, 0x5a, 0x5d };
static unsigned char cardh2[4] = { 0xff, 0x08, 0x5a, 0x5d };

//static unsigned short StatReg = 0x002b;
// Transfer Ready and the Buffer is Empty
static unsigned short StatReg = TX_RDY | TX_EMPTY;
static unsigned short ModeReg;
static unsigned short CtrlReg;
static unsigned short BaudReg;

static unsigned int bufcount;
static unsigned int parp;
static unsigned int mcdst,rdwr;
static unsigned char adrH,adrL;
static unsigned int padst;

char Mcd1Data[MCD_SIZE], Mcd2Data[MCD_SIZE];
char McdDisable[2]; //senquack - added from PCSXR

static u32 sio_cycle; /* for SIO_INT() */

void sioInit(void) {
	if (autobias) {
		sio_cycle=136*8;
	} else {
		//senquack-Rearmed uses 535 in all cases, so we'll use that instead:
		//sio_cycle=200*BIAS; /* for SIO_INT() */

		// clk cycle byte
		// 4us * 8bits = (PSXCLK / 1000000) * 32; (linuzappz)
		sio_cycle=535;
	}
}

//senquack - Updated to use PSXINT_* enum and intCycle struct (much cleaner than before)
//           Also, takes an argument eCycle whereas before it took none.
//           TODO: PCSXR code has SIO_INT take a parameter that always ends up being
//             535 (SIO_CYCLES) but I've left PCSX4ALL using this older SIO_INT(void)
//           TODO: Add support for newer PCSXR Config.Sio option
// clk cycle byte
static inline void SIO_INT(void) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_SIO_Int++;
#endif
	psxEventQueue.enqueue(PSXINT_SIO, sio_cycle);
}

void sioWrite8(unsigned char value) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioWrite8++;
#endif
#ifdef PAD_LOG
	PAD_LOG("sio write8 %x\n", value);
#endif
	//senquack - all calls to SIO_INT() here now pass param SIO_CYCLES
	//           whereas before they passed nothing:
	switch (padst) {
		case 1: SIO_INT();
			if ((value&0x40) == 0x40) {
				padst = 2; parp = 1;

					switch (CtrlReg&0x2002) {
						case 0x0002:
							buf[parp] = PAD1_poll();
							break;
						case 0x2002:
							buf[parp] = PAD2_poll();
							break;
					}

				if (!(buf[parp] & 0x0f)) {
					bufcount = 2 + 32;
				} else {
					bufcount = 2 + (buf[parp] & 0x0f) * 2;
				}
				if (buf[parp] == 0x41) {
					switch (value) {
						case 0x43:
							buf[1] = 0x43;
							break;
						case 0x45:
							buf[1] = 0xf3;
							break;
					}
				}
			}
			else padst = 0;
			return;
		case 2:
			parp++;

				switch (CtrlReg&0x2002) {
					case 0x0002: buf[parp] = PAD1_poll(); break;
					case 0x2002: buf[parp] = PAD2_poll(); break;
				}

			if (parp == bufcount) { padst = 0; return; }
			SIO_INT();
			return;
	}

	switch (mcdst) {
		case 1:
			SIO_INT();
			if (rdwr) { parp++; return; }
			parp = 1;
			switch (value) {
				case 0x52: rdwr = 1; break;
				case 0x57: rdwr = 2; break;
				default: mcdst = 0;
			}
			return;
		case 2: // address H
			SIO_INT();
			adrH = value;
			*buf = 0;
			parp = 0;
			bufcount = 1;
			mcdst = 3;
			return;
		case 3: // address L
			SIO_INT();
			adrL = value;
			*buf = adrH;
			parp = 0;
			bufcount = 1;
			mcdst = 4;
			return;
		case 4:
			SIO_INT();
			parp = 0;
			switch (rdwr) {
				case 1: // read
					buf[0] = 0x5c;
					buf[1] = 0x5d;
					buf[2] = adrH;
					buf[3] = adrL;
					switch (CtrlReg&0x2002) {
						case 0x0002:
							memcpy(&buf[4], Mcd1Data + (adrL | (adrH << 8)) * 128, 128);
							break;
						case 0x2002:
							memcpy(&buf[4], Mcd2Data + (adrL | (adrH << 8)) * 128, 128);
							break;
					}
					{
					char cxor = 0;
					int i;
					for (i=2;i<128+4;i++)
						cxor^=buf[i];
					buf[132] = cxor;
					}
					buf[133] = 0x47;
					bufcount = 133;
					break;
				case 2: // write
					buf[0] = adrL;
					buf[1] = value;
					buf[129] = 0x5c;
					buf[130] = 0x5d;
					buf[131] = 0x47;
					bufcount = 131;
					break;
			}
			mcdst = 5;
			return;
		case 5:	
			parp++;
			//senquack - updated to match PCSXR code:
			if ((rdwr == 1 && parp == 132) ||
			    (rdwr == 2 && parp == 129)) {
				// clear "new card" flags
				if (CtrlReg & 0x2000)
					cardh2[1] &= ~8;
				else
					cardh1[1] &= ~8;
			}
			if (rdwr == 2) {
				if (parp < 128) buf[parp + 1] = value;
			}
			SIO_INT();
			return;
	}

	switch (value) {
		case 0x01: // start pad
			StatReg |= RX_RDY;		// Transfer is Ready

				switch (CtrlReg&0x2002) {
					case 0x0002: buf[0] = PAD1_startPoll(); break;
					case 0x2002: buf[0] = PAD2_startPoll(); break;
				}

			bufcount = 2;
			parp = 0;
			padst = 1;
			SIO_INT();
			return;
		case 0x81: // start memcard
			if (CtrlReg & 0x2000)
			{
				if (McdDisable[1])
					goto no_device;
				memcpy(buf, cardh2, 4);
			}
			else
			{
				if (McdDisable[0])
					goto no_device;
				memcpy(buf, cardh1, 4);
			}
			StatReg |= RX_RDY;
			parp = 0;
			bufcount = 3;
			mcdst = 1;
			rdwr = 0;
			SIO_INT();
			return;
		default:
		no_device:
			StatReg |= RX_RDY;
			buf[0] = 0xff;
			parp = 0;
			bufcount = 0;
			return;
	}
}

void sioWriteStat16(unsigned short value) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioWriteStat16++;
#endif
}

void sioWriteMode16(unsigned short value) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioWriteMode16++;
#endif
	ModeReg = value;
}

void sioWriteCtrl16(unsigned short value) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioWriteCtrl16++;
#endif
	CtrlReg = value & ~RESET_ERR;
	if (value & RESET_ERR) StatReg &= ~IRQ;
	//senquack - Updated to match PCSX Rearmed
	// 'no DTR resets device, tested on the real thing'
	//if ((CtrlReg & SIO_RESET) || (!CtrlReg)) {
	if ((CtrlReg & SIO_RESET) || !(CtrlReg & DTR)) {
		padst = 0; mcdst = 0; parp = 0;
		StatReg = TX_RDY | TX_EMPTY;
		psxEventQueue.dequeue(PSXINT_SIO);
	}
}

void sioWriteBaud16(unsigned short value) {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioWriteBaud16++;
#endif
	if (autobias)
		sio_cycle=value*8;
	BaudReg = value;
}

unsigned char sioRead8() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioRead8++;
#endif
	unsigned char ret = 0;

	if ((StatReg & RX_RDY)/* && (CtrlReg & RX_PERM)*/) {
//		StatReg &= ~RX_OVERRUN;
		ret = buf[parp];
		if (parp == bufcount) {
			StatReg &= ~RX_RDY;		// Receive is not Ready now
			if (mcdst == 5) {
				mcdst = 0;
				if (rdwr == 2) {
					switch (CtrlReg & 0x2002) {
						case 0x0002:
							memcpy(Mcd1Data + (adrL | (adrH << 8)) * 128, &buf[1], 128);
							SaveMcd(MCD1, Mcd1Data, (adrL | (adrH << 8)) * 128, 128);
							break;
						case 0x2002:
							memcpy(Mcd2Data + (adrL | (adrH << 8)) * 128, &buf[1], 128);
							SaveMcd(MCD2, Mcd2Data, (adrL | (adrH << 8)) * 128, 128);
							break;
					}
				}
			}
			if (padst == 2) padst = 0;
			if (mcdst == 1) {
				mcdst = 2;
				StatReg|= RX_RDY;
			}
		}
	}

#ifdef PAD_LOG
	PAD_LOG("sio read8 ;ret = %x\n", ret);
#endif
	return ret;
}
unsigned short sioReadStat16() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioReadStat16++;
#endif
	return StatReg;
}

unsigned short sioReadMode16() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioReadMode16++;
#endif
	return ModeReg;
}

unsigned short sioReadCtrl16() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioReadCtrl16++;
#endif
	return CtrlReg;
}

unsigned short sioReadBaud16() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioReadBaud16++;
#endif
	return BaudReg;
}

void sioInterrupt() {
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_sioInterrupt++;
#endif
#ifdef PAD_LOG
	PAD_LOG("Sio Interrupt (CP0.Status = %x)\n", psxRegs.CP0.n.Status);
#endif
	//printf("Sio Interrupt\n");
	//  pcsx_rearmed: only do IRQ if it's bit has been cleared
	if (!(StatReg & IRQ)) {
		StatReg |= IRQ;
		psxHu32ref(0x1070) |= SWAPu32(0x80);
		// Ensure psxBranchTest() is called soon when IRQ is pending:
		ResetIoCycle();
	}
}

////////////////////////
// Memcard operations //
////////////////////////

//#define DEBUG_MEMCARDS

struct Memcard {
	Memcard() :
		file(NULL),
		filename(NULL),
		cur_offset(0)
	{}

	~Memcard()
	{
		if (file) {
			const char *tmpstr = filename ? filename : "";
			printf("Warning: memcard file not closed, closing via destructor: %s\n", tmpstr);
			fclose(file);
			file = NULL;
		}
	}

	FILE* file;         // File ptr when memcard is being written to
	char* filename;
	long  cur_offset;   // Current file offset (when file is open for writing)
};

Memcard memcards[2];

//Number of cycles after last memcard write to wait until
// PSXINT_SIO_SYNC_MCD event closes the memcard file.
#define MEMCARD_SYNC_DELAY (PSXCLK / 4)

// FlushMcd() ensures a memcard file temporarily opened for writing gets
//  closed. If 'sync_file' is true, it will call fsync() before closing it.
int FlushMcd(enum MemcardNum mcd_num, boolean sync_file)
{
	Memcard &mc = memcards[mcd_num];

	if (!mc.file)
		return 0;

	int retval = 0;
	if (sync_file) {
		if (fflush(mc.file)) retval = -1;
		if (fsync(fileno(mc.file))) retval = -1;
	}
	if (fclose(mc.file)) retval = -1;
	mc.file = NULL;
	mc.cur_offset = 0;
	if (retval < 0) {
		perror(__func__);
		const char* tmpstr = mc.filename ? mc.filename : "";
		printf("Error in %s() writing memcard file %s\n", __func__, tmpstr);
	}
	return retval;
}

// Intended to be called from within the running emulator after a small
//  amount of time has passed after a memcard has been written to.
//  This is done by scheduling a PSXINT_SIO_SYNC_MCD event. This allows
//  memcard I/O to be done against a file kept open for a decent amount of
//  time, allowing buffering and fewer open/close system calls.
//  Will flush, sync, and close any memcard files opened for writing.
void sioSyncMcds()
{
	FlushMcd(MCD1, true);
	FlushMcd(MCD2, true);
#ifdef DEBUG_MEMCARDS
	printf("%s()\n", __func__);
#endif
}

int LoadMcd(enum MemcardNum mcd_num, char* filename)
{
	FILE *f;
	size_t bytes_read;
	char *data = NULL;
	struct stat stat_buf;
	bool convert_data = false;
	Memcard &mc = memcards[mcd_num];

	FlushMcd(mcd_num, false);
	mc.filename = NULL;
	mc.file = NULL;
	mc.cur_offset = 0;

	if (filename == NULL) {
		printf("Error in %s(): NULL filename param\n", __func__);
		return -1;
	}

	if (mcd_num == MCD1) {
		data = Mcd1Data;
		cardh1[1] |= 8; // mark as new
	} else {
		data = Mcd2Data;
		cardh2[1] |= 8;
	}

	mc.filename = filename;
	if (*mc.filename == 0) {
		sprintf(mc.filename, "memcards/card%d.mcd", mcd_num+1);
		printf("No memory card value was specified - creating a default card %s\n", mc.filename);
	}

	if ((f = fopen(mc.filename, "rb")) == NULL) {
		printf("The memory card %s doesn't exist - creating it\n", mc.filename);
		CreateMcd(mc.filename);
		if ((f = fopen(mc.filename, "rb")) == NULL)
			goto error;
	}

	printf("Loading memory card %s\n", mc.filename);
	if (fstat(fileno(f), &stat_buf) != -1) {
		if (stat_buf.st_size == MCD_SIZE + 64) {
			// Assume Connectix Virtual Gamestation .vgs/.mem format
			printf("Detected Connectix VGS memcard format.\n");
			convert_data = true;
			if (fseek(f, 64, SEEK_SET) == -1) {
				printf("Error seeking to position 64 (VGS data offset).\n");
				goto error;
			}
		} else if (stat_buf.st_size == MCD_SIZE + 3904) {
			// Assume DexDrive .gme format
			printf("Detected DexDrive memcard format.\n");
			convert_data = true;
			if (fseek(f, 3904, SEEK_SET) == -1) {
				printf("Error seeking to position 3904 (DexDrive data offset).\n");
				goto error;
			}
		} else if (stat_buf.st_size != MCD_SIZE) {
			printf("Error: unknown memcard format (not raw image, DexDrive, or Connectix VGS)\n");
			goto error;
		}
	}

	if ((bytes_read = fread(data, 1, MCD_SIZE, f)) != MCD_SIZE) {
		printf("Failed reading data from memory card %s!\n", mc.filename);
		printf("Wanted %zu bytes and got %zu\n", (size_t)MCD_SIZE, bytes_read);
		goto error;
	}

	fclose(f);

	// Convert file to native raw memcard format, if not already
	if (convert_data) {
		// Truncate file to 0 bytes, then write just the memcard data back
		if ( (f = fopen(mc.filename, "wb")) == NULL ||
		     fwrite(data, 1, MCD_SIZE, f) != MCD_SIZE ) {
			printf("Error converting memcard file %s\n", mc.filename);
			printf("Writing to memcard file disabled.\n");
			if (f) fclose(f);
			mc.filename = NULL;
			return -1;
		}
		printf("Converted memcard file to native raw format.\n");
		fclose(f);
	}

	return 0;

error:
	printf("Error in %s:", __func__);
	perror(NULL);
	if (f) {
		printf("Error reading memcard file %s\n", mc.filename);
		fclose(f);
	} else {
		printf("Error opening memcard file %s\n", mc.filename);
	}
	mc.filename = NULL;
	return -1;
}

int SaveMcd(enum MemcardNum mcd_num, char *data, uint32_t adr, int size)
{
	Memcard &mc = memcards[mcd_num];

	// File isn't currently open for writing
	if (mc.file == NULL) {
		if (mc.filename == NULL || *mc.filename == '\0')
			return 0;
		mc.cur_offset = 0;
		if ((mc.file = fopen(mc.filename, "r+b")) == NULL)
			goto error;
	}

	if (mc.cur_offset != adr) {
		if (fseek(mc.file, adr, SEEK_SET))
			goto error;
		mc.cur_offset = adr;
	}

	if (fwrite(data + adr, 1, size, mc.file) != size)
		goto error;

	mc.cur_offset += size;

	// (Re)schedule a memcard file flush/sync/close, leaving
	//  file open for writing for now.
	psxEventQueue.enqueue(PSXINT_SIO_SYNC_MCD, MEMCARD_SYNC_DELAY);
	return 0;

error:
	{
		printf("Error in %s:", __func__);
		perror(NULL);
		const char* tmpstr = mc.filename ? mc.filename : "";
		if (mc.file) {
			printf("Error writing to memcard file %s\n", tmpstr);
			fclose(mc.file);
		} else {
			printf("Error opening memcard file %s\n", tmpstr);
		}
		printf("Writing disabled for memcard file.\n");
		mc.file = NULL;
		mc.filename = NULL;
	}
	return -1;
}

#define errchk_fputc(c, file)       \
do {                                \
     if (fputc((c), (file)) == EOF) \
          goto error;               \
} while (0)

int CreateMcd(char *filename)
{
	FILE *f;	
	int s = MCD_SIZE;
	int i = 0, j;

	if (filename == NULL || filename[0] == '\0') {
		printf("Error: NULL or empty filename parameter in %s\n", __func__);
		return -1;
	}

	if ( (f = fopen(filename, "wb")) == NULL)
		goto error;

	if (strcasestr(filename, ".gme"))
	{
		// DexDrive .gme format
		s = s + 3904;
		errchk_fputc('1', f);
		s--;
		errchk_fputc('2', f);
		s--;
		errchk_fputc('3', f);
		s--;
		errchk_fputc('-', f);
		s--;
		errchk_fputc('4', f);
		s--;
		errchk_fputc('5', f);
		s--;
		errchk_fputc('6', f);
		s--;
		errchk_fputc('-', f);
		s--;
		errchk_fputc('S', f);
		s--;
		errchk_fputc('T', f);
		s--;
		errchk_fputc('D', f);
		s--;
		for (i = 0; i < 7; i++) {
			errchk_fputc(0, f);
			s--;
		}
		errchk_fputc(1, f);
		s--;
		errchk_fputc(0, f);
		s--;
		errchk_fputc(1, f);
		s--;
		errchk_fputc('M', f);
		s--;
		errchk_fputc('Q', f);
		s--;
		for (i = 0; i < 14; i++) {
			errchk_fputc(0xa0, f);
			s--;
		}
		errchk_fputc(0, f);
		s--;
		errchk_fputc(0xff, f);
		while (s-- > (MCD_SIZE + 1))
			errchk_fputc(0, f);
	} else if (strcasestr(filename, ".mem") || strcasestr(filename, ".vgs"))
	{
		// Connectix Virtual Gamestation .vgs/.mem format
		s = s + 64;
		errchk_fputc('V', f);
		s--;
		errchk_fputc('g', f);
		s--;
		errchk_fputc('s', f);
		s--;
		errchk_fputc('M', f);
		s--;
		for (i = 0; i < 3; i++) {
			errchk_fputc(1, f);
			s--;
			errchk_fputc(0, f);
			s--;
			errchk_fputc(0, f);
			s--;
			errchk_fputc(0, f);
			s--;
		}
		errchk_fputc(0, f);
		s--;
		errchk_fputc(2, f);
		while (s-- > (MCD_SIZE + 1))
			errchk_fputc(0, f);
	}
	errchk_fputc('M', f);
	s--;
	errchk_fputc('C', f);
	s--;
	while (s-- > (MCD_SIZE - 127))
		errchk_fputc(0, f);
	errchk_fputc(0xe, f);
	s--;

	for (i = 0; i < 15; i++) { // 15 blocks
		errchk_fputc(0xa0, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		for (j = 0; j < 117; j++) {
			errchk_fputc(0x00, f);
			s--;
		}
		errchk_fputc(0xa0, f);
		s--;
	}

	for (i = 0; i < 20; i++) {
		errchk_fputc(0xff, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0x00, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		errchk_fputc(0xff, f);
		s--;
		for (j = 0; j < 118; j++) {
			errchk_fputc(0x00, f);
			s--;
		}
	}

	while ((s--) >= 0)
		errchk_fputc(0, f);

	if ( fflush(f) ||
	     fsync(fileno(f)) )
		goto error;

	fclose(f);
	return 0;

error:
	printf("Error in %s:", __func__);
	perror(NULL);
	if (f) {
		printf("Error writing to memcard file %s\n", filename);
		fclose(f);
	} else {
		printf("Error creating memcard file %s\n", filename);
	}

	return -1;
}

// remove the leading and trailing spaces in a string
static void trim(char *str) {
	int pos = 0;
	char *dest = str;

	// skip leading blanks
	while (str[pos] <= ' ' && str[pos] > 0)
		pos++;

	while (str[pos]) {
		*(dest++) = str[pos];
		pos++;
	}

	*(dest--) = '\0'; // store the null

	// remove trailing blanks
	while (dest >= str && *dest <= ' ' && *dest > 0)
		*(dest--) = '\0';
}

void GetMcdBlockInfo(enum MemcardNum mcd_num, int block, McdBlock *Info) {
	char *data = NULL, *ptr, *str, *sstr;
	unsigned short clut[16];
	unsigned short c;
	int i, x;

	memset(Info, 0, sizeof(McdBlock));

	if (McdDisable[mcd_num])
		return;

	data = (mcd_num == MCD1) ? Mcd1Data : Mcd2Data;

	ptr = data + block * 8192 + 2;

	Info->IconCount = *ptr & 0x3;

	ptr+= 2;

	x = 0;

	str = Info->Title;
	sstr = Info->sTitle;

	for (i=0; i < 48; i++) {
		c = *(ptr) << 8;
		c|= *(ptr+1);
		if (!c) break;

		if (c >= 0x8281 && c <= 0x829A)
			c = (c - 0x8281) + 'a';
		else if (c >= 0x824F && c <= 0x827A)
			c = (c - 0x824F) + '0';
		else if (c == 0x8140) c = ' ';
		else if (c == 0x8143) c = ',';
		else if (c == 0x8144) c = '.';
		else if (c == 0x8146) c = ':';
		else if (c == 0x8147) c = ';';
		else if (c == 0x8148) c = '?';
		else if (c == 0x8149) c = '!';
		else if (c == 0x815E) c = '/';
		else if (c == 0x8168) c = '"';
		else if (c == 0x8169) c = '(';
		else if (c == 0x816A) c = ')';
		else if (c == 0x816D) c = '[';
		else if (c == 0x816E) c = ']';
		else if (c == 0x817C) c = '-';
		else {
			str[i] = ' ';
			sstr[x++] = *ptr++; sstr[x++] = *ptr++;
			continue;
		}

		str[i] = sstr[x++] = c;
		ptr+=2;
	}

	trim(str);
	trim(sstr);

	ptr = data + block * 8192 + 0x60; // icon palete data

	for (i = 0; i < 16; i++) {
		clut[i] = *((unsigned short*)ptr);
		ptr += 2;
	}

	for (i = 0; i < Info->IconCount; i++) {
		short *icon = &Info->Icon[i*16*16];

		ptr = data + block * 8192 + 128 + 128 * i; // icon data

		for (x = 0; x < 16 * 16; x++) {
			icon[x++] = clut[*ptr & 0xf];
			icon[x] = clut[*ptr >> 4];
			ptr++;
		}
	}

	ptr = data + block * 128;

	Info->Flags = *ptr;

	ptr += 0xa;
	strncpy(Info->ID, ptr, 12);
	ptr += 12;
	strncpy(Info->Name, ptr, 16);
}

int sioFreeze(void* f, FreezeMode mode)
{
	if (    freeze_rw(f, mode, buf, sizeof(buf))
	     || freeze_rw(f, mode, &StatReg, sizeof(StatReg))
	     || freeze_rw(f, mode, &ModeReg, sizeof(ModeReg))
	     || freeze_rw(f, mode, &CtrlReg, sizeof(CtrlReg))
	     || freeze_rw(f, mode, &BaudReg, sizeof(BaudReg))
	     || freeze_rw(f, mode, &bufcount, sizeof(bufcount))
	     || freeze_rw(f, mode, &parp, sizeof(parp))
	     || freeze_rw(f, mode, &mcdst, sizeof(mcdst))
	     || freeze_rw(f, mode, &rdwr, sizeof(rdwr))
	     || freeze_rw(f, mode, &adrH, sizeof(adrH))
	     || freeze_rw(f, mode, &adrL, sizeof(adrL))
	     || freeze_rw(f, mode, &padst, sizeof(padst)) )
		return -1;
	else
		return 0;
}
