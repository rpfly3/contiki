#include "core/net/mac/prk-mcs/prkmcs.h"
#include "cpu/arm/stm32f103/nodeid.h"
#include "core/dev/watchdog.h"

PROCESS_NAME(rtimer_process);
/* local node address */
linkaddr_t node_addr, pair_addr;

/* platform related initialization */
static void platform_init()
{
	InitializeUSART1();
	watchdog_init();
	clock_init();
	rtimer_init();
	ctimer_init();
	rf231_init();
	node_addr = getNodeId();
	log_info("Mote %d is booted", node_addr);
}
int main()
{
	platform_init();
	
	process_init();
	process_start(&etimer_process, NULL);

	process_start(&rtimer_process, NULL);

	// There is an error on linking example files, fix it later
	//autostart_start(autostart_processes);
	log_info("Contiki kernel is started");

#ifdef PRKMCS_ENABLE
	prkmcsInit();
#endif // PRKMCS_ENABLE

	while (1)
	{
		process_run();
	}
	return 0;
}
