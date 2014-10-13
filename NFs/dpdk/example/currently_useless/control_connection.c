#include "control_connection.h"

void *doControlConnection(void *parameters);

void startControlConnection(void)
{
	pthread_t thread[1];
	// Create a new thread
	if(pthread_create(&thread[0],NULL,doControlConnection,NULL))
	{
		fprintf(logFile,"%s Error while creating the thread for the control connection\n",module_name);
		exit(EXIT_FAILURE);
	}
	pthread_detach(thread[0]);
	
}
	
void *doControlConnection(void *parameters)
{
	int AddressFamily = AF_INET; //use IPv4
	int TransportProtocol = SOCK_STREAM; //use TCP

	char ErrBuf[1024];
	char DataBuffer[1024];
	int ChildSocket;				// keeps the socket ID for connections from clients
	struct addrinfo Hints;			// temporary struct to keep settings needed to open the new socket
	struct addrinfo *AddrInfo;		// keeps the addrinfo chain; required to open a new socket
	struct sockaddr_storage From;	// temp variable that keeps the parameters of the incoming connection
	int ReadBytes, WrittenBytes;
	int ServerSocket;

	// Prepare to open a new server socket
	memset(&Hints, 0, sizeof(struct addrinfo));

	Hints.ai_family= AddressFamily;
	Hints.ai_socktype= TransportProtocol;	// Open a TCP/UDP connection
	Hints.ai_flags = AI_PASSIVE;			// This is a server: ready to bind() a socket
	
	if (sock_initaddress (NULL, nf_params.tcp_port, &Hints, &AddrInfo, ErrBuf, sizeof(ErrBuf)) == sockFAILURE)
	{
		fprintf(logFile,"%s Error resolving given port (%s): %s\n",module_name, nf_params.tcp_port,ErrBuf);
		exit(EXIT_FAILURE);
	}

	if ( (ServerSocket= sock_open(AddrInfo, 1, 10,  ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
	{
		// AddrInfo is no longer required
		sock_freeaddrinfo(AddrInfo);
		fprintf(logFile,"%s Cannot opening the socket: %s\n",module_name, ErrBuf);
		exit(EXIT_FAILURE);
	}

	// AddrInfo is no longer required
	sock_freeaddrinfo(AddrInfo);

	while(1)
	{
		if ( (ChildSocket= sock_accept(ServerSocket, &From, ErrBuf, sizeof(ErrBuf))) == sockFAILURE)
		{
			fprintf(logFile,"%s Error when accepting a new connection: %s\n",module_name, ErrBuf);
			exit(EXIT_FAILURE);
		}

		ReadBytes= sock_recv(ChildSocket, DataBuffer, sizeof(DataBuffer), SOCK_RECEIVEALL_NO, 0/*no timeout*/, ErrBuf, sizeof(ErrBuf));
		if (ReadBytes == sockFAILURE)
		{
			fprintf(logFile,"%s Error reading data: %s\n",module_name, ErrBuf);
			exit(EXIT_FAILURE);
		}

		// Terminate buffer, just for printing purposes
		// Warning: this can originate a buffer overflow
		DataBuffer[ReadBytes]= 0;

		fprintf(logFile,"%sData received (%d bytes):\n",module_name, ReadBytes);
		fprintf(logFile,"%s %s\n",module_name,DataBuffer);

		char answer[1024];
		sprintf(answer,"Greeting from network function\"%s\"",nf_params.nf_name);
		
		fprintf(logFile,"%s Answer to be sent: %s\n",module_name,answer);
		WrittenBytes= sock_send(ChildSocket, answer, strlen(answer), ErrBuf, sizeof(ErrBuf));
		if (WrittenBytes == sockFAILURE)
		{
			fprintf(logFile,"%s Error sending data: %s",module_name, ErrBuf);
			exit(EXIT_FAILURE);

		}

		sock_close(ChildSocket,ErrBuf,sizeof(ErrBuf));
	}
}
