/*****************************************************************************\
 *  slurm_acct_gather_energy.h - implementation-independent job energy
 *  accounting plugin definitions
 *****************************************************************************
 *  Written by Bull-HN-PHX/d.rusak,
 *  Copyright (C) 2012 Bull-HN-PHX
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#ifndef __SLURM_ACCT_GATHER_ENERGY_H__
#define __SLURM_ACCT_GATHER_ENERGY_H__

#if HAVE_CONFIG_H
#  include "config.h"
#  if HAVE_INTTYPES_H
#    include <inttypes.h>
#  else
#    if HAVE_STDINT_H
#      include <stdint.h>
#    endif
#  endif			/* HAVE_INTTYPES_H */
#else				/* !HAVE_CONFIG_H */
#  include <inttypes.h>
#endif				/*  HAVE_CONFIG_H */

#include <sys/resource.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "slurm/slurm.h"
#include "slurm/slurmdb.h"

#include "src/common/macros.h"
#include "src/common/pack.h"
#include "src/common/list.h"
#include "src/common/xmalloc.h"
#include "src/common/slurm_acct_gather.h"
#include "src/common/slurm_jobacct_gather.h"

typedef struct acct_energy_data {
	uint64_t	power;
	uint64_t	cpu_freq;
} acct_energy_data_t;


extern int acct_gather_energy_init(void); /* load the plugin */
extern int acct_gather_energy_fini(void); /* unload the plugin */
extern acct_gather_energy_t *acct_gather_energy_alloc(void);
extern void acct_gather_energy_destroy(acct_gather_energy_t *energy);
extern void acct_gather_energy_pack(acct_gather_energy_t *energy, Buf buffer,
				    uint16_t protocol_version);
extern int acct_gather_energy_unpack(acct_gather_energy_t **energy, Buf buffer,
				     uint16_t protocol_version);

extern int acct_gather_energy_g_update_node_energy(void);
extern int acct_gather_energy_g_get_data(enum acct_energy_type data_type,
					 acct_gather_energy_t *energy);
extern int acct_gather_energy_g_set_data(enum acct_energy_type data_type,
					 acct_gather_energy_t *energy);
extern void acct_gather_energy_g_conf_options(s_p_options_t **full_options,
					      int *full_options_cnt);
extern void acct_gather_energy_g_conf_set(s_p_hashtbl_t *tbl);

#endif /*__SLURM_ACCT_GATHER_ENERGY_H__*/
