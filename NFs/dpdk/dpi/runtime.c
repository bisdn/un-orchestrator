/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file runtime.c
* @author Ivano Cerrato<ivano.cerrato (at) polito.it>
*
* @brief Executes the NF.
*/

#include <assert.h>
#include <rte_mbuf.h>

#include <netinet/in.h>

#include "main.h"

/**
*	Forbidden words in HTTP requests
*/
#define PORN	"porn"
#define SEX		"sex"
//XXX: add here other words

/**
*	Variables related to the forbidden words in HTTP requests
*/
pcre *re_porn;				//Compiled regular expression (for libpcre)
pcre_extra *re_extra_porn;	//Data to speed up matching (libpcre-specific)
pcre *re_sex;				//Compiled regular expression (for libpcre)
pcre_extra *re_extra_sex;	//Data to speed up matching (libpcre-specific)
//XXX: add here other variables

/**
*	Private prototypes
*/
int initializeRegEx(pcre **re, pcre_extra *re_extra, const char *pattern);
int drop(unsigned char *packet,unsigned int len);

/**
*	Implementations
*/

int do_nf(void *useless)
{
	(void) useless; //XXX: this line suppresses the "unused-parameter" error

	int i;
	unsigned int p;
	mbuf_array_t pkts_received;

	//Init the regex engine
	if(!initializeRegEx(&re_porn, re_extra_porn,PORN))
		return 0;
	if(!initializeRegEx(&re_sex, re_extra_sex,SEX))
		return 0;

	mbuf_array_t *pkts_to_send = (mbuf_array_t*)malloc(NUM_PORTS * sizeof(mbuf_array_t));
	for(p = 0; p < NUM_PORTS; p++)
		pkts_to_send[p].n_mbufs = 0;

	while(1)
	{
#ifdef ENABLE_SEMAPHORE
		sem_wait(nf_params.semaphore);
#endif

		/*0) Iterates on all the ports */
		for(p = 0; p < NUM_PORTS; p++)
		{
			/*1) Receive incoming packets */

			pkts_received.n_mbufs = rte_ring_sc_dequeue_burst(nf_params.ports[p].to_nf_queue,(void **)&pkts_received.array[0],PKT_TO_NF_THRESHOLD);

			if(likely(pkts_received.n_mbufs > 0))
			{
#ifdef ENABLE_LOG
				fprintf(logFile,"[%s] Received %d pkts on port %d (%s)\n", NAME, pkts_received.n_mbufs,p,nf_params.ports[p].name);
#endif

				for (i=0;i < pkts_received.n_mbufs;i++)
				{
					/*2) Operate on the packet */

					unsigned char *pkt = rte_pktmbuf_mtod(pkts_received.array[i],unsigned char *);
#ifdef ENABLE_LOG
					fprintf(logFile,"[%s] Packet size: %d\n",NAME,rte_pktmbuf_pkt_len(pkts_received.array[i]));
					fprintf(logFile,"[%s] %.2x:%.2x:%.2x:%.2x:%.2x:%.2x -> %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",NAME,pkt[6],pkt[7],pkt[8],pkt[9],pkt[10],pkt[11],pkt[0],pkt[1],pkt[2],pkt[3],pkt[4],pkt[5]);
#endif

					/**
					*	If the packet arrives from the first port, check if it must be dropped
					*/
					if(p == 0)
					{
#ifdef ENABLE_LOG
						fprintf(logFile,"[%s] I'm going to check if the packet must be dropped.\n", NAME);
#endif
						if(drop(pkt,rte_pktmbuf_pkt_len(pkts_received.array[i])))
						{
							//The packet must be dropped
#ifdef ENABLE_LOG
 		                                       fprintf(logFile,"[%s] The packet is dropped.\n", NAME);
#endif
							rte_pktmbuf_free(pkts_received.array[i]);
							continue;
						}
					}
					unsigned int output_port = (p+1) % NUM_PORTS;

					pkts_to_send[output_port].array[pkts_to_send[output_port].n_mbufs] = pkts_received.array[i];
					pkts_to_send[output_port].n_mbufs++;
				}//end of iteration on the packets received from the current port
			} //end if(likely(pkts_received.n_mbufs > 0))
		}//end iteration on the ports

		/*3) Send the processed packet not transmitted yet*/
		for(p = 0; p < NUM_PORTS; p++)
		{
			if(likely(pkts_to_send[p].n_mbufs > 0))
			{
#ifdef ENABLE_LOG
				fprintf(logFile,"[%s] Sending %d packets on port %x (%s).\n", NAME,pkts_to_send[p].n_mbufs,p,nf_params.ports[p].name);
#endif
				int ret = rte_ring_sp_enqueue_burst(nf_params.ports[p].to_xdpd_queue,(void *const*)pkts_to_send[p].array,(unsigned)pkts_to_send[p].n_mbufs);

	        	if (unlikely(ret < pkts_to_send[p].n_mbufs))
		        {
		        	fprintf(logFile,"[%s] Not enough room in port %d towards xDPD to enqueue; the packet will be dropped.\n", NAME,p);
					do {
						struct rte_mbuf *pkt_to_free = pkts_to_send[p].array[ret];
						rte_pktmbuf_free(pkt_to_free);
					} while (++ret < pkts_to_send[p].n_mbufs);
				}
			}
			pkts_to_send[p].n_mbufs = 0;
		}/* End of iteration on the ports */

	}/*End of while true*/

	return 0;
}

int drop(unsigned char *packet, unsigned int len)
{
	uint16_t *et = (uint16_t*)&packet[12];
#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] Ethertype: %x\n",NAME,ntohs(*et));
#endif
	if(ntohs(*et) != ETH_TYPE_IP)
	{
#ifdef ENABLE_LOG
		fprintf(logFile,"[%s] The packet is not IPv4 (ethertype is %x.\n", NAME,*et);
#endif
		return 0;
	 }

#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] The packet is IPv4.\n", NAME);
#endif

	unsigned char *ip  = &packet[IP_POSITION];
#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] ip protocol: %x\n",NAME,ip[IP_PROTOCOL_POSITION]);
#endif
	if(ip[IP_PROTOCOL_POSITION] != IP_PROTOCOL_TCP)
	{
#ifdef ENABLE_LOG
		fprintf(logFile,"[%s] The packet is not TCP (ip protocol is %x).\n", NAME,ip[IP_PROTOCOL_POSITION]);
#endif
		return 0;
	}

	unsigned int hlen = ip[IP_HLEN_POSITION] & 0xF;
	hlen = hlen * 4;

#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] The IPv4 header consists of %d bytes.\n", NAME,hlen);
	fprintf(logFile,"[%s] The packet is TCP.\n", NAME);
#endif

	unsigned char *tcp = &ip[hlen];

	uint16_t *tcp_dst = (uint16_t*)&tcp[TCP_DST_PORT_POSITION];
#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] tcp destination port: %d\n",NAME,ntohs(*tcp_dst));
#endif
	if(ntohs(*tcp_dst) != 80)
	{
#ifdef ENABLE_LOG
		fprintf(logFile,"[%s] The packet is not an HTTP answer (tcp dst port is %x).\n", NAME,*tcp_dst);
#endif
		return 0;
	}

	unsigned int data_offset = (tcp[TCP_DATA_OFFSET_POSITION] & 0xF0) >> 4;
	data_offset = data_offset * 4;

#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] The TCP header consists of %d bytes.\n", NAME,data_offset);
#endif

	if((len - (14+hlen+data_offset)) == 0)
	{
#ifdef ENABLE_LOG
		fprintf(logFile,"[%s] The TCP header does not contain any data\n",NAME);
#endif
		return 0;
	}

#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] The packet is an HTTP request\n",NAME);
#endif

	unsigned char *http = &tcp[data_offset];

	int ovector[OVECTSIZE];
	//The following call can give PCRE_ERROR_JIT_STACKLIMIT error if the 32K stack size is not enough
	int rc = pcre_exec (re_porn, re_extra_porn, (const char*) http, len, 0, 0, ovector, OVECTSIZE);
	if( rc != PCRE_ERROR_NOMATCH)
	{
#ifdef ENABLE_LOG
		fprintf(logFile,"[%s] %s found in packet.\n", NAME,PORN);
#endif
		return 1;
	}

	rc = pcre_exec (re_sex, re_extra_sex, (const char*) http, len, 0, 0, ovector, OVECTSIZE);
	if( rc != PCRE_ERROR_NOMATCH)
	{
#ifdef ENABLE_LOG
		fprintf(logFile,"[%s] %s found in packet.\n", NAME,SEX);
#endif
		return 1;
	}

#ifdef ENABLE_LOG
	fprintf(logFile,"[%s] The packt must not be dropped.\n", NAME);
#endif

	return 0;
}

int initializeRegEx(pcre **re, pcre_extra *re_extra, const char *pattern)
{

	const char *re_err;
	int re_err_offset;
	int flag = PCRE_CASELESS;

	if ((*re = pcre_compile (pattern, flag, &re_err, &re_err_offset, NULL)) == NULL)
	{
		fprintf(logFile,"[%s] Error compiling regexp '%s' at character %d: %s.\n", NAME,pattern,re_err_offset, re_err);
		return 0;
	}
	else
	{
		int w = 0;
		pcre_config(PCRE_CONFIG_JIT, &w);
		if(w)
			re_extra = pcre_study (*re, PCRE_STUDY_JIT_COMPILE, &re_err);  //This setting needs PCRE 8.20 or greater
		else
		{
			re_extra = pcre_study (*re, 0, &re_err);
		}

		if (re_err != NULL)
		{
			fprintf(logFile,"[%s] Cannot study regular expression \"%s\":\n%s\n", NAME,pattern,re_err);
			return 0;
		}

		if(re_extra == NULL)
		{
			fprintf(logFile,"[%s] ?????\n", NAME);
			return 0;
		}

		fprintf(logFile,"[%s] Regular expression '%s' compiled\n", NAME,pattern);

		return 1;
	}

	return 1;
}

