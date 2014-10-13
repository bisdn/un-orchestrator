/**
* @file main.h
* @author Ivano Cerrato<ivano.cerrato (at) polito.it>
* @date 14th Jul 2014
*/

#include <stdio.h>
#include <pcap.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FROM	"eth0"
#define TO		"eth1"

#define NAME	"example"

#define likely(x)       ( __builtin_expect(!!(x), 1) )
#define unlikely(x)     (  __builtin_expect(!!(x), 0) )

int main(int argc, char *argv[])
{
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *from, *to;
	struct pcap_pkthdr *header;
	const unsigned char *pkt_data;
	unsigned char *new_pkt;

	int res;

	printf("[%s] I'm going to start\n",NAME);

	//Check for root privileges 
	if(geteuid() != 0)
	{
		fprintf(stderr,"[%s] Root permissions are required to run %s\n",NAME,argv[0]);	
		exit(EXIT_FAILURE);	
	}
	
	printf("[%s] I'm going to start\n",NAME);

	from = pcap_open_live(FROM, BUFSIZ, 1, 1000, errbuf);
	if(from == NULL)
	{
		fprintf(stderr,"[%s] %s\n", NAME, errbuf);
		exit(EXIT_FAILURE);	
	}

	to = pcap_open_live(TO, BUFSIZ, 1, 1000, errbuf);
	if(to == NULL)
	{
		fprintf(stderr,"[%s] %s\n", NAME, errbuf);
		exit(EXIT_FAILURE);	
	}

	printf("[%s] Devices open!\n",NAME);

	while ((res = pcap_next_ex(from, &header, &pkt_data)) >= 0)
	{
		if(unlikely(res>0)){
#ifdef ENABLE_LOG
			fprintf(stdout,"[%s] *******************************************",NAME);
			fprintf(stdout,"[%s] Packet received:\n",NAME);
			fprintf(stdout,"[%s] \tlength: %d bytes\n",NAME,header->caplen);
			fprintf(stdout,"[%s] \t%x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x\n",NAME,pkt_data[6],pkt_data[7],pkt_data[8],pkt_data[9],pkt_data[10],pkt_data[11],pkt_data[0],pkt_data[1],pkt_data[2],pkt_data[3],pkt_data[4],pkt_data[5]);
#endif			
		
			new_pkt = (unsigned char*)pkt_data;
			new_pkt[0] = new_pkt[1] = new_pkt[2] = new_pkt[3] = new_pkt[4] = new_pkt[5] = 0xa;

#ifdef ENABLE_LOG
			fprintf(stdout,"[%s] New packet:\n",NAME);
			fprintf(stdout,"[%s] \t%x:%x:%x:%x:%x:%x -> %x:%x:%x:%x:%x:%x\n",NAME,new_pkt[6],new_pkt[7],new_pkt[8],new_pkt[9],new_pkt[10],new_pkt[11],new_pkt[0],new_pkt[1],new_pkt[2],new_pkt[3],new_pkt[4],new_pkt[5]);
			fprintf(stdout,"[%s] *******************************************",NAME);
			fprintf(stdout,"[%s]",NAME);
#endif

			if (unlikely (pcap_sendpacket(to, new_pkt, header->caplen) != 0))
			{
				fprintf(stdout,"[%s] Error sending the packet: %s\n", NAME,pcap_geterr(to));
				exit(EXIT_FAILURE);
			}

		}
	}
	exit(EXIT_SUCCESS);
}

