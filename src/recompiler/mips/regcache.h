#define REG_CACHE_START		MIPSREG_S0
#define REG_CACHE_END		(MIPSREG_S7+1)

#define REG_LOAD		0
#define REG_FIND		1
#define REG_LOADBRANCH		2

#define REG_EMPTY		0
#define REG_REGISTER		1
#define REG_TEMPORARY		2
#define REG_RESERVED		3
#define REG_REGISTERBRANCH	4

#define DEBUGF printf

/* Regcache data */
typedef struct {
	u32	mappedto;
	u32	host_age;
	u32	host_use;
	u32	host_type;
	bool	ismapped;
	int	host_islocked;
} HOST_RecRegister;

typedef struct {
	u32	mappedto;
	bool	ismapped;
	bool	psx_ischanged;
} PSX_RecRegister;

typedef struct {
	PSX_RecRegister		psx[32];
	HOST_RecRegister	host[32];
	u32			reglist[32];
	u32			reglist_cnt;
} RecRegisters;

RecRegisters regcache;

/* Spill regs to psxRegs if they are in host regs and were modified */
static void regClearJump(void)
{
	int i;

	for (i = 0; i < 32; i++) {
		if (regcache.psx[i].ismapped) {
			int mappedto = regcache.psx[i].mappedto;

			if (i != 0 && regcache.psx[i].psx_ischanged) {
				//DEBUGG("mappedto %d pr %d\n", mappedto, PERM_REG_1);
				SW(mappedto, PERM_REG_1, offGPR(i));
			}

			regcache.psx[i].psx_ischanged = false;
			regcache.host[mappedto].ismapped = regcache.psx[i].ismapped = false;
			regcache.host[mappedto].mappedto = regcache.psx[i].mappedto = 0;
			regcache.host[mappedto].host_type = REG_EMPTY;
			regcache.host[mappedto].host_age = 0;
			regcache.host[mappedto].host_use = 0;
			regcache.host[mappedto].host_islocked = 0;
		}
	}
}

static void regFreeRegs(void)
{
	//DEBUGF("regFreeRegs\n");
	int i = 0;
	int firstfound = 0;

	while (regcache.reglist[i] != 0xFF) {
		int hostreg = regcache.reglist[i];
		//DEBUGF("spilling %dth reg (%d)", i, hostreg);

		if (!regcache.host[hostreg].host_islocked) {
			int psxreg = regcache.host[hostreg].mappedto;

			if (psxreg && regcache.psx[psxreg].psx_ischanged) {
				SW(hostreg, PERM_REG_1, offGPR(psxreg));
			}

			regcache.psx[psxreg].psx_ischanged = false;
			regcache.host[hostreg].ismapped = regcache.psx[psxreg].ismapped = false;
			regcache.host[hostreg].mappedto = regcache.psx[psxreg].mappedto = 0;
			regcache.host[hostreg].host_type = REG_EMPTY;
			regcache.host[hostreg].host_age = 0;
			regcache.host[hostreg].host_use = 0;
			regcache.host[hostreg].host_islocked = 0;

			if (firstfound == 0) {
				regcache.reglist_cnt = i;
				//DEBUGF("setting reglist_cnt %d", i);
				firstfound = 1;
			}
		}
		//else DEBUGF("locked :(");

		i++;
	}

	if (!firstfound) DEBUGF("FATAL ERROR: unable to free register");
}

static u32 regMipsToArmHelper(u32 regpsx, u32 action, u32 type)
{
	//DEBUGF("regMipsToArmHelper regpsx %d action %d type %d reglist_cnt %d", regpsx, action, type, regcache.reglist_cnt);
	int regnum = regcache.reglist[regcache.reglist_cnt];

	//DEBUGF("regnum 1 %d", regnum);

	while (regnum != 0xFF) {
		//DEBUGF("checking reg %d", regnum);
		if (regcache.host[regnum].host_type == REG_EMPTY) {
			break;
		}

		regcache.reglist_cnt++;
		//DEBUGF("setting reglist_cnt %d", regcache.reglist_cnt);
		regnum = regcache.reglist[regcache.reglist_cnt];
	}

	//DEBUGF("regnum 2 %d", regnum);
	if (regnum == 0xFF) {
		regFreeRegs();
		regnum = regcache.reglist[regcache.reglist_cnt];
		if (regnum == 0xff)
			regClearJump();
	}

	regcache.host[regnum].host_type = type;
	regcache.host[regnum].host_islocked++;
	regcache.psx[regpsx].psx_ischanged = false;

	if (action != REG_LOADBRANCH) {
		regcache.host[regnum].host_age = 0;
		regcache.host[regnum].host_use = 0;
		regcache.host[regnum].ismapped = true;
		regcache.host[regnum].mappedto = regpsx;
		regcache.psx[regpsx].ismapped = true;
		regcache.psx[regpsx].mappedto = regnum;
	} else {
		regcache.host[regnum].host_age = 0;
		regcache.host[regnum].host_use = 0xFF;
		regcache.host[regnum].ismapped = false;
		regcache.host[regnum].mappedto = 0;

		if (regpsx) {
			LW(regnum, PERM_REG_1, offGPR(regpsx));
		} else {
			LI16(regnum, 0);
		}

		regcache.reglist_cnt++;
		//DEBUGF("setting reglist_cnt %d", regcache.reglist_cnt);

		//DEBUGF("regnum 3 %d", regnum);
		return regnum;
	}

	if (action == REG_LOAD) {
		if (regpsx) {
			LW(regcache.psx[regpsx].mappedto, PERM_REG_1, offGPR(regpsx));
		} else {
			LI16(regcache.psx[regpsx].mappedto, 0);
		}
	}

	regcache.reglist_cnt++;
	//DEBUGF("setting reglist_cnt %d (regnum 4 %d)", regcache.reglist_cnt, regnum);

	return regnum;
}

static u32 regMipsToArm(u32 regpsx, u32 action, u32 type)
{
	/* zero reg is not mapped anywhere */
	if (!regpsx)
		return 0;

	//DEBUGF("starting for regpsx %d, action %d, type %d", regpsx, action, type);
	if (regcache.psx[regpsx].ismapped) {
		//DEBUGF("regpsx %d is mapped", regpsx);
		if (action != REG_LOADBRANCH) {
			int hostreg = regcache.psx[regpsx].mappedto;
			regcache.host[hostreg].host_islocked++;

			return hostreg;
		} else {
			//DEBUGF("loadbranch regpsx %d", regpsx);
			u32 mappedto = regcache.psx[regpsx].mappedto;

			if (regpsx && regcache.psx[regpsx].psx_ischanged) {
				SW(mappedto, PERM_REG_1, offGPR(regpsx));
			}

			regcache.psx[regpsx].psx_ischanged = false;
			regcache.psx[regpsx].ismapped = false;
			regcache.psx[regpsx].mappedto = 0;

			regcache.host[mappedto].host_type = type;
			regcache.host[mappedto].host_age = 0;
			regcache.host[mappedto].host_use = 0xFF;
			regcache.host[mappedto].ismapped = false;
			regcache.host[mappedto].host_islocked++;
			regcache.host[mappedto].mappedto = 0;

			return mappedto;
		}
	}

	//DEBUGF("calling helper");
	return regMipsToArmHelper(regpsx, action, type);
}

static void regMipsChanged(u32 regpsx)
{
	/* do nothing for zero reg */
	if (!regpsx)
		return;

	regcache.psx[regpsx].psx_ischanged = true;
}

static void regBranchUnlock(u32 reghost)
{
	/* do nothing for zero reg */
	if (!reghost)
		return;

	if (regcache.host[reghost].host_islocked > 0)
		regcache.host[reghost].host_islocked--;
}

static void regClearBranch(void)
{
	int i;
	for (i = 1; i < 32; i++) {
		if (regcache.psx[i].ismapped && regcache.psx[i].psx_ischanged) {
			SW(regcache.psx[i].mappedto, PERM_REG_1, offGPR(i));
		}
	}
}

static void regReset()
{
	int i, i2;
	for (i = 0; i < 32; i++) {
		regcache.psx[i].psx_ischanged = false;
		regcache.psx[i].ismapped = false;
		regcache.psx[i].mappedto = 0;
	}

	for (i = 0; i < 32; i++) {
		regcache.host[i].host_type = REG_EMPTY;
		regcache.host[i].host_age = 0;
		regcache.host[i].host_use = 0;
		regcache.host[i].host_islocked = 0;
		regcache.host[i].ismapped = false;
		regcache.host[i].mappedto = 0;
	}

	for (i = 0; i < 8 ; i++)
		regcache.host[i].host_type = REG_RESERVED;

	for (i = MIPSREG_T0; i <= MIPSREG_T7; i++)
		regcache.host[i].host_type = REG_RESERVED;

	for (i = 24 ; i < 32; i++)
		regcache.host[i].host_type = REG_RESERVED;

	regcache.host[PERM_REG_1].host_type = REG_RESERVED;
	regcache.host[TEMP_1].host_type = REG_RESERVED;
	regcache.host[TEMP_2].host_type = REG_RESERVED;
	regcache.host[TEMP_3].host_type = REG_RESERVED;

	for (i = 0, i2 = 0; i < 32; i++) {
		if (regcache.host[i].host_type == REG_EMPTY) {
			regcache.reglist[i2] = i;
			i2++;
		}
	}

	regcache.reglist[i2] = 0xFF;
	//DEBUGF("reglist len %d", i2);
}

static void regUpdate(void)
{
	int ilock;

	for (ilock = REG_CACHE_START; ilock < REG_CACHE_END; ilock++) {
		if (regcache.host[ilock].ismapped) {
			regcache.host[ilock].host_age++;
			regcache.host[ilock].host_islocked = 0;
		}
	}
}
