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

#include "main.h"

int do_nf(void *useless)
{	
	(void) useless; //XXX: this line suppresses the "unused-parameter" error

	int i;
	unsigned int p;
	mbuf_array_t pkts_received;
	
	mbuf_array_t *pkts_to_send = (mbuf_array_t*)malloc(nf_params.num_ports * sizeof(mbuf_array_t));
	for(p = 0; p < nf_params.num_ports; p++)
		pkts_to_send[p].n_mbufs = 0;

	while(1)
	{
#ifdef ENABLE_SEMAPHORE
		sem_wait(nf_params.semaphore);
#endif

		/*0) Iterates on all the ports */
		for(p = 0; p < nf_params.num_ports; p++)
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
		
					
					//XXX: this NF sends a packet on the next port (i.e., if the packet has been received from port 1,
					//it is now sent on port 2), and changes the destination MAC address
					unsigned char *pkt = rte_pktmbuf_mtod(pkts_received.array[i],unsigned char *);					
#ifdef ENABLE_LOG					
					fprintf(logFile,"[%s] Packet size: %d\n",NAME,rte_pktmbuf_pkt_len(pkts_received.array[i]));
					
					fprintf(logFile,"[%s] %.2x:%.2x:%.2x:%.2x:%.2x:%.2x -> %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",NAME,pkt[6],pkt[7],pkt[8],pkt[9],pkt[10],pkt[11],pkt[0],pkt[1],pkt[2],pkt[3],pkt[4],pkt[5]);
#endif
					
					//Change the destination MAC address of packets
					pkt[0] = pkt[1] = pkt[2] = pkt[3] = pkt[4] = pkt[5] = 0xa;
					
					unsigned int output_port = (p+1) % nf_params.num_ports;
			
					pkts_to_send[output_port].array[pkts_to_send[output_port].n_mbufs] = pkts_received.array[i];
					pkts_to_send[output_port].n_mbufs++;
				}//end of iteration on the packets received from the current port
			} //end if(likely(pkts_received.n_mbufs > 0))
		}//end iteration on the ports
		
			
		/*3) Send the processed packet not transmitted yet*/
		for(p = 0; p < nf_params.num_ports; p++)
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

