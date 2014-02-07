/*
 * DTCP (Data Transfer Control Protocol)
 *
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RINA_DTCP_H
#define RINA_DTCP_H

#include "common.h"
#include "du.h"

struct dtp;
struct dtcp;

struct dtcp * dtcp_create(void);
int           dtcp_destroy(struct dtcp * instance);

int           dtcp_bind(struct dtcp * instance,
                        struct dtp *  peer);
int           dtcp_unbind(struct dtcp * instance);

/* NOTE: Takes the ownership of the passed PDU */
int           dtcp_send(struct dtcp * instance,
                        struct sdu *  sdu);

int           dtcp_notify_seq_rxmtq(struct dtcp * instance,
                                     seq_num_t     seq);

#endif
