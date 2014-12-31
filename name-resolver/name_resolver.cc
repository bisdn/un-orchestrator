/**
*	@author: Ivano Cerrato
*	@email: ivano.cerrato at polito.it
*	@date: 13th Jun 2014
*/

#include "rest_server.h"
#include <getopt.h>

bool parse_command_line(int argc, char *argv[], char **file_name);
bool usage(void);

int main(int argc, char *argv[])
{
	//Check for root privileges 
	if(geteuid() != 0)
	{	
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Root permissions are required to run %s\n",argv[0]);
		exit(EXIT_FAILURE);	
	}

	char *file_name = NULL;
	if(!parse_command_line(argc,argv,&file_name))
		exit(EXIT_FAILURE);	
	
	string file(file_name);

	if(!RestServer::init(file))
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot start the database");
		exit(EXIT_FAILURE);	
	}
	
	struct MHD_Daemon *daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, REST_PORT, NULL, NULL,
		&RestServer::answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED, &RestServer::request_completed, NULL,
		MHD_OPTION_END);
	
	if (NULL == daemon) 
	{
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Error when starting the HTTP deamon");
		logger(ORCH_ERROR, MODULE_NAME, __FILE__, __LINE__, "Cannot start the database");
		return EXIT_FAILURE;
	}

	getchar ();
	MHD_stop_daemon (daemon);
	return 0;
}


bool parse_command_line(int argc, char *argv[], char **file_name)
{
	int opt;
	char **argvopt;
	int option_index;
	static struct option lgopts[] = {
		{"f", 1, 0, 0},
		{"h", 0, 0, 0},
		{NULL, 0, 0, 0}
	};

	uint32_t arg_f = 0;
	argvopt = argv;

	file_name[0] = '\0';

	while ((opt = getopt_long(argc, argvopt, "", lgopts, &option_index)) != EOF)
    {
		switch (opt)
		{
			/* long options */
			case 0:
				if (!strcmp(lgopts[option_index].name, "f"))/* port */
	   			{
	   				*file_name = optarg;
	   				
	   				arg_f++;
	   			}
	   			else if (!strcmp(lgopts[option_index].name, "h"))/* help */
	   			{
	   				return usage();
	   			}
	   			else
	   			{
	   				fprintf(stderr,"[%s] Invalid command line parameter '%s'\n",MODULE_NAME,lgopts[option_index].name);
	   				return usage();
	   			}
				break;
			default:
				return usage();
		}
	}

	/* Check that all mandatory arguments are provided */
	if (arg_f == 0)
	{
		fprintf(stderr,"[%s] Not all mandatory arguments are present in the command line\n",MODULE_NAME);
		return usage();
	}

	return true;
}

bool usage(void)
{
	char message[]=	\

	"Usage:                                                                                   \n" \
	"  sudo ./name-resolver --f file_name                                                     \n" \
	"                                                                                         \n" \
	"Parameters:                                                                              \n" \
	"  --f file_name                                                                          \n" \
	"        Name of the xml file describing the possible implementations for the network     \n" \
	"        functions.                                                                       \n" \
	"                                                                                         \n" \
	"Options:                                                                                 \n" \
	"  --h                                                                                    \n" \
	"        Print this help.                                                                 \n" \
	"                                                                                         \n" \
	"Example:                                                                                 \n" \
	"  sudo ./name-resolver --f ./config/example.xml                                          \n\n";

	fprintf(stderr,"\n\n[%s] %s\n",MODULE_NAME,message);
	
	return false;
}
