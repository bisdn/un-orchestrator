/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file init.h
* @author Ivano Cerrato<ivano.cerrato (at) polito.it>
*
* @brief Initialize the NF.
*/

#ifndef _INIT_H_
#define _INIT_H_ 1

#pragma once

#include <signal.h>
#include <getopt.h>

#include <rte_memcpy.h>
#include <rte_mbuf.h>

#include "main.h"

/**
*	@brief Initialize the NF
*/
int init(int argc, char *argv[]);

#endif //_INIT_H_
