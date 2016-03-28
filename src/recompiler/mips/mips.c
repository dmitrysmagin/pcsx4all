#include <sys/cachectl.h>
void clear_insn_cache(void* start, void* end, int flags)
{
  cacheflush(start, end-start, ICACHE);
}
