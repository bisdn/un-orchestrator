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
#include <inttypes.h>

#include <netinet/in.h>

#include "main.h"

uint64_t counter=0;

int do_nf(void *useless)
{
	(void) useless; //XXX: this line suppresses the "unused-parameter" error

	mbuf_array_t pkts;
	int ret;
	struct rte_mbuf *pkt_to_free;

	while(1){
		pkts.n_mbufs = rte_ring_sc_dequeue_burst(nf_params.ports[0].to_nf_queue,(void **)&pkts.array[0],PKT_TO_NF_THRESHOLD);
		if(unlikely(pkts.n_mbufs == 0))
			continue;

		counter += pkts.n_mbufs;
		ret = rte_ring_sp_enqueue_burst(nf_params.ports[1].to_xdpd_queue,(void *const*)pkts.array,(unsigned)pkts.n_mbufs);

		if (unlikely(ret < pkts.n_mbufs)) {
			do {
				pkt_to_free = pkts.array[ret];
				rte_pktmbuf_free(pkt_to_free);
			} while (++ret < pkts.n_mbufs);
		}
	}/*End of while true*/

	return 0;
}
