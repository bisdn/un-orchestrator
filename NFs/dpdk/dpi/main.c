#include "main.h"
#include "init.h"

int MAIN(int argc, char *argv[])
{

	//Check for root privileges 
	if(geteuid() != 0)
	{
		fprintf(stderr,"[%s] Root permissions are required to run %s\n",NAME,argv[0]);	
		exit(EXIT_FAILURE);	
	}

	uint32_t lcore;
	int ret;

	/* Init EAL */
	ret = rte_eal_init(argc, argv);

	if (ret < 0)
		return -1;

	argc -= ret;
	argv += ret;

	if(init(argc, argv) < 0)
		return -1;

	fprintf(logFile,"[%s] Network function started!\n",NAME);
	fflush(logFile);

	rte_eal_mp_remote_launch(do_nf, NULL, CALL_MASTER);

	//In this version, the NF uses just one lcores.. Other potential lcores are no
	//used
	RTE_LCORE_FOREACH_SLAVE(lcore) 
	{ 
		if (rte_eal_wait_lcore(lcore)/*Wait until an lcore finishes its job.*/ < 0) 
		{
			return -1;
		}
	}

	return 0;
}
