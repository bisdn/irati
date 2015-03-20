/*
 * RMT (Relaying and Multiplexing Task) kRPI
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
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

#ifndef RINA_RMT_PS_H
#define RINA_RMT_PS_H

#include <linux/types.h>

#include "rmt.h"
#include "pdu.h"
#include "rds/rfifo.h"
#include "ps-factory.h"

struct rmt_ps {
        struct ps_base base;

        /* Behavioural policies. */
        void (* max_q_policy_tx)(struct rmt_ps *,
                                 struct pdu *,
                                 struct rfifo *);
        void (* max_q_policy_rx)(struct rmt_ps *,
                                 struct sdu *,
                                 struct rfifo *);
        void (* rmt_q_monitor_policy_tx)(struct rmt_ps *,
                                 struct pdu *,
                                 struct rfifo *);
        void (* rmt_q_monitor_policy_rx)(struct rmt_ps *,
                                 struct sdu *,
                                 struct rfifo *);
        struct rfifo * (* rmt_scheduling_policy_tx)(struct rmt_ps *,
                                        struct rmt_n1_port *, bool restart);
        struct rfifo * (* rmt_scheduling_policy_rx)(struct rmt_ps *,
                                        struct rmt_n1_port *, bool restart);

        /* Parametric policies. */
        int          max_q;

        /* Reference used to access the RMT data model. */
        struct rmt * dm;

        /* Data private to the policy-set implementation. */
        void *       priv;
};

/*
 * The ownership of @factory is not passed. Plugin module is therefore
 * in charge of deallocate its memory, if necessary.
 */
int rmt_ps_publish(struct ps_factory * factory);
int rmt_ps_unpublish(const char * name);

#endif
