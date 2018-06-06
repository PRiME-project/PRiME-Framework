/* This file is part of the PRiME Framework.
 * 
 * The PRiME Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * The PRiME Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the PRiME Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Charles Leech, Graeme Bragg & James Bantock
 */

#ifndef PRIME_API_APP_T_H
#define PRIME_API_APP_T_H

#include "prime_api_t.h"

namespace prime { namespace api { namespace app
{
	// These are application-level type definitions

	// Available types of application-level knob
	enum knob_type_t {PRIME_PAR, // Parallelism
						PRIME_PREC, // Precision
						PRIME_DEV_SEL, // Device selection
						PRIME_ITR,  // Iterations
						PRIME_AFF,	//Affinity
						PRIME_GEN  // Generic
					};

	// Available types of application-level monitor
	enum mon_type_t {PRIME_PERF, // Performance
						PRIME_ACC, // Accuracy
						PRIME_ERR, // Error
						PRIME_POW	// for KOCL...
					};

	// Application-level discrete knob container
	typedef struct knob_disc_t {
		pid_t proc_id;
		unsigned int id;
		knob_type_t type;
		prime::api::disc_t min;
		prime::api::disc_t max;
		prime::api::disc_t val;
	} knob_disc_t;

	// Application-level continuous knob container
	typedef struct knob_cont_t {
		pid_t proc_id;
		unsigned int id;
		knob_type_t type;
		prime::api::cont_t min;
		prime::api::cont_t max;
		prime::api::cont_t val;
	} knob_cont_t;

	// Application-level discrete monitor container
	typedef struct mon_disc_t {
		pid_t proc_id;
		unsigned int id;
		mon_type_t type;
		prime::api::disc_t min;
		prime::api::disc_t max;
		prime::api::disc_t val;
		prime::api::cont_t weight;
	} mon_disc_t;

	// Application-level continuous monitor container
	typedef struct mon_cont_t {
		pid_t proc_id;
		unsigned int id;
		mon_type_t type;
		prime::api::cont_t min;
		prime::api::cont_t max;
		prime::api::cont_t val;
		prime::api::cont_t weight;
	} mon_cont_t;
} } }

#endif
