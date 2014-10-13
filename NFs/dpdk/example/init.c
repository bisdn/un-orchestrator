/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "init.h"

FILE *logFile = NULL;

// Prototypes

void usage(void);
int parse_command_line(int argc, char *argv[]);
void init_shared_resources(void);
void sig_handler(int received_signal);


// Functions

void usage(void)
{
	char message[]=	\

	"Usage:                                                                                   \n" \
	"  sudo ./nf -c core_mask -n memory_channels --proc-type=secondary -- --p port_name       \n" \
	"                --s semaphore_name --l file_name                                         \n" \
	"                                                                                         \n" \
	"Parameters:                                                                              \n" \
	"  -c core_mask                                                                           \n" \
	"        Number of lcores used by the NFs. Note that currently only one lcore is actually \n" \
	"        used.                                                                            \n" \
	"  -n memory_channels	                                                                  \n" \
	"        Number of channels used by the NFs to access to the memory.                      \n" \
	"  --proc-type=secondary                                                                  \n" \
	"        The NF must be executed as a DPDK secondary process.                             \n" \
	"  --p port_name                                                                          \n" \
	"        Name of a port of the NF. This parameter can be repeated many times, once for    \n" \
	"        each port to be used by the NF.                                                  \n" \
	"  --s semaphore_name                                                                     \n" \
	"        Name of the semaphore to be used by the NF.                                      \n" \
	"  --l file_name                                                                          \n" \
	"        Name of the file used to log information.                                        \n" \
	"                                                                                         \n" \
	"Options:                                                                                 \n" \
	"  --h                                                                                    \n" \
	"        Print this help.                                                                 \n" \
	"                                                                                         \n" \
	"Example:                                                                                 \n" \
	"  sudo ./nf -c 0x1 -n 2 --proc-type=secondary -- --p port1 --p port2 --s sem             \n\n";

	fprintf(stderr,"\n\n[%s] %s\n",NAME,message);
}

/**
* @brief Parses the command line used to run the NF
*
* @param argc	Number of parameters in the command line (excluding those used
*				the EAL)
* @param argv	The command line (except the parameters used by the EAL)
*/
int parse_command_line(int argc, char *argv[])
{
	char *logfilename = NULL;
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	static struct option lgopts[] = {
		{"s", 1, 0, 0},
		{"p", 1, 0, 0},
		{"l", 1, 0, 0},
		{"h", 0, 0, 0},
		{NULL, 0, 0, 0}
	};

	uint32_t arg_s = 0, arg_p = 0, arg_l = 0;
	argvopt = argv;

	nf_params.num_ports = (argc - 5) / 2;
	nf_params.ports = calloc(nf_params.num_ports,sizeof(struct nf_port_t));

	while ((opt = getopt_long(argc, argvopt, "", lgopts, &option_index)) != EOF)
    {
		switch (opt)
		{
			/* long options */
			case 0:
				if (!strcmp(lgopts[option_index].name, "p"))/* port */
	   			{
	   				nf_params.ports[arg_p].name = (char*)malloc(sizeof(char)*(strlen(optarg)+1));
	   				strcpy(nf_params.ports[arg_p].name,optarg);
	   				
	   				arg_p++;
	   			}
				else if (!strcmp(lgopts[option_index].name, "s"))/* semaphore */
	   			{
	   				if(arg_s != 0)
	   				{
	   					fprintf(stderr,"[%s] The parameter '--s' appear too many times in the command line\n",NAME);
	   					return -1;
	   				}
	   				
#ifdef ENABLE_SEMAPHORE
	   				nf_params.sem_name = (char*)malloc(sizeof(char)*(strlen(optarg)+1));
	   				strcpy(nf_params.sem_name,optarg);
#endif	   				
	   				arg_s++;
	   			}
	   			else if (!strcmp(lgopts[option_index].name, "l"))/* file to log */
	   			{
	   				if(arg_l != 0)
	   				{
	   					fprintf(stderr,"[%s] The parameter '--l' appear too many times in the command line\n",NAME);
	   					return -1;
	   				}
	   				
	   				logfilename = (char*)malloc(sizeof(char)*(strlen(optarg)+1));
	   				strcpy(logfilename,optarg);
	   				
	   				arg_l++;
	   			}
				else if (!strcmp(lgopts[option_index].name, "h"))/* help */
	   			{
	   				return -1;
	   			}
	   			else
	   			{
	   				fprintf(stderr,"[%s] Invalid command line parameter '%s'\n",NAME,lgopts[option_index].name);
	   				return -1;
	   			}
				break;
			default:
				return -1;
		}
	}

	/* Check that all mandatory arguments are provided */
	if (
#ifdef ENABLE_SEMAPHORE
		(arg_s == 0)||
#endif		
		(arg_p == 0)||(arg_l == 0))
	{
		fprintf(stderr,"[%s] Not all mandatory arguments are present in the command line\n",NAME);
		return -1;
	}

	//Open the file to be used to log information

	if((strcmp(logfilename,"stdout") != 0) && (strcmp(logfilename,"stderr") != 0))
	{
		logFile = fopen (logfilename,"w"); 
		if (logFile==NULL)
		{
			fprintf(stderr,"[%s] Unable to open file to log!\n",logfilename);
			fclose (logFile);
			exit (EXIT_FAILURE);
		}
	}
	else
	{
		if(strcmp(logfilename,"stdout") == 0)
			logFile = stdout;
		else
			logFile = stderr;
	}

	if (optind >= 0)
		argv[optind - 1] = prgname;

	ret = optind - 1;
	optind = 0; /* reset getopt lib */
	
	return ret;
}

/**
*	@brief 	The NF attaches to the resources used to exchange 
*			packets with the xDPD
*/
void init_shared_resources(void)
{
	unsigned int i;
	char queue_name[NAME_LENGTH];
	
	/*
	*	Connect to the rte_rings
	*/
	for(i = 0; i < nf_params.num_ports; i++)
	{
		snprintf(queue_name, NAME_LENGTH, "%s-to-nf", nf_params.ports[i].name);
		nf_params.ports[i].to_nf_queue = rte_ring_lookup(queue_name);
		if (nf_params.ports[i].to_nf_queue == NULL)
		{
			fprintf(logFile,"[%s] Cannot get rte_ring '%s'\n", NAME,queue_name);
			exit(1);
		}
		fprintf(logFile,"[%s] Attached to rte_ring '%s'\n", NAME,queue_name);
	
		snprintf(queue_name, NAME_LENGTH, "%s-to-xdpd", nf_params.ports[i].name);
		nf_params.ports[i].to_xdpd_queue = rte_ring_lookup(queue_name);
		if (nf_params.ports[i].to_xdpd_queue == NULL)
		{
			fprintf(logFile,"[%s] Cannot get rte_ring '%s'\n",NAME, queue_name);
			exit(1);
		}
		
		fprintf(logFile,"[%s] Attached to rte_ring '%s'\n", NAME,queue_name);
	}
	
#ifdef ENABLE_SEMAPHORE	
	/*
	*	Connect to the POSIX named semaphore
	*/
	nf_params.semaphore = sem_open(nf_params.sem_name,0);//, O_CREAT, 0644, 0);
	if(nf_params.semaphore == SEM_FAILED)
	{
		fprintf(logFile,"[%s] Cannot get the semaphore '%s'\n",NAME, nf_params.sem_name);
	    exit(1);
	}
	fprintf(logFile,"[%s] Attached to semaphore '%s'\n", NAME, nf_params.sem_name);
#endif
}

void sig_handler(int received_signal)
{
	fprintf(logFile,"[%s] Received SIGINT. I'm going to terminate...\n",NAME);

    if (received_signal == SIGINT)
    {
#ifdef ENABLE_SEMAPHORE   
    	sem_unlink(nf_params.sem_name);
#endif
        exit(0);
    }
}

int init(int argc, char *argv[])
{
	if(parse_command_line(argc, argv) < 0)
	{
		usage();
		return -1;
	}
		
    init_shared_resources();

	/*
	* Set the signal handler and global variables for termination
	*/
	signal(SIGINT, sig_handler);
	
	return 0;
}
