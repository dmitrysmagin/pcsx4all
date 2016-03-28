#include "../common.h"

#if 0
void rec_test_pc()
{
	FILE* test_log_fp = fopen("/mnt/sd/translation_testpc_log.txt", "a+");
  	fprintf(test_log_fp, "Block PC %p VAL %x\n", psxRegs, psxRegs->pc);
	gp2x_sync();
	fclose(test_log_fp);
}
#endif
