/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @author Ivano Cerrato<ivano.cerrato (at) polito.it>
* @data  Jul 30th, 2014
*
* @brief This NF implements a DPI on packets coming from its first interface.
* It works with two interfaces, and implements a bridge between them.
* By default, it drops the HTTP packets containing the words "porn" and "sex". 
* You can add other words by exiting the file "runtime.c"
*/

#ifndef _MAIN_H_
#define _MAIN_H_ 1

#pragma once

#ifdef ENABLE_SEMAPHORE
	#include <semaphore.h>
#endif	
	
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <pcre.h>

#define NAME 					"DPI"
#define PKT_TO_NF_THRESHOLD 	200
#define NAME_LENGTH				100

#define NUM_PORTS 				2

#define OVECTSIZE 				15

/**
*	Constants used to parse the packet
*/
#define ETH_TYPE_POSITION 			12
#define ETH_TYPE_IP					0x800
#define IP_POSITION					14
#define IP_PROTOCOL_POSITION		9
#define IP_HLEN_POSITION		0
#define IP_PROTOCOL_TCP			6
#define TCP_DST_PORT_POSITION		2
#define TCP_DST_PORT_HTTP		80
#define TCP_DATA_OFFSET_POSITION	12

extern FILE *logFile;

typedef struct mbuf_array
{
	/**
	*	@brief: packets received / to be sent
	*/
	struct rte_mbuf *array[PKT_TO_NF_THRESHOLD];
	
	/**
	*	@brief: number of packets received / to be sent
	*/
	int n_mbufs;
}mbuf_array_t;

struct nf_port_t
{
	/**
	*	@brief: queue used to receive packets
	*/
	struct rte_ring *to_nf_queue;
	
	/**
	*	@brief: queue used to transmit packets
	*/
	struct rte_ring *to_xdpd_queue;
	
	/**
	*	@brief: name of the port
	*/
	char *name;
};// __rte_cache_aligned;

struct nf_params_t 
{	
	/**
	*	@brief: ports to be used to transmit/receive packets
	*/
	struct nf_port_t *ports;

#ifdef ENABLE_SEMAPHORE
	/**
	*	@brief: name of the semaphore
	*/
	char *sem_name;

	/**
	*	@brief: semaphore used to implement a blocking model
	*		and save CPU resources
	*/
	sem_t *semaphore;
#endif
	
	/* mbuf pools */
	//FIXME: currently it is not used - to be linked to the pool 
	//used by xDPD in order to create new packets
	struct rte_mempool *pool;
 
} __rte_cache_aligned;

/**
*	@brief: parameters used by the NF to work
*/
struct nf_params_t nf_params;

int do_nf(void*);

#ifdef RTE_EXEC_ENV_BAREMETAL
	#define MAIN _main
#else
	#define MAIN main
#endif


#endif /* _MAIN_H_ */
