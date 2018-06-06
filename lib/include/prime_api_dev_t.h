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

#ifndef PRIME_API_DEV_T_H
#define PRIME_API_DEV_T_H

#include "prime_api_t.h"

namespace prime { namespace api { namespace dev
{
	// These are device-level type definitions
	
	struct dev_args_t {
			bool power = false;
			bool temp = false;
		};

	// Available types of device-level knob
	enum knob_type_t {PRIME_VOLT,
						PRIME_FREQ,
						PRIME_EN,
						PRIME_PMC_CNT,
						PRIME_GOVERNOR,
						PRIME_FREQ_EN
					};

	// Available types of device-level monitor
	enum mon_type_t {PRIME_POW,
						PRIME_TEMP,
						PRIME_CYCLES,
						PRIME_PMC
					};

	// Device-level discrete knob container
	typedef struct knob_disc_t {
		unsigned int id;
		knob_type_t type;
		prime::api::disc_t min;
		prime::api::disc_t max;
		prime::api::disc_t val;
		prime::api::disc_t init;
	} knob_disc_t;

	// Device-level continuous knob container
	typedef struct knob_cont_t {
		unsigned int id;
		knob_type_t type;
		prime::api::cont_t min;
		prime::api::cont_t max;
		prime::api::cont_t val;
		prime::api::cont_t init;
	} knob_cont_t;

	// Device-level discrete monitor container
	typedef struct mon_disc_t {
		unsigned int id;
		mon_type_t type;
		prime::api::disc_t min;
		prime::api::disc_t max;
		prime::api::disc_t val;
	} mon_disc_t;

	// Device-level continuous monitor container
	typedef struct mon_cont_t {
		unsigned int id;
		mon_type_t type;
		prime::api::cont_t min;
		prime::api::cont_t max;
		prime::api::cont_t val;
	} mon_cont_t;
	
	// Monitor return types
	typedef struct mon_disc_ret_t {
		prime::api::disc_t min = 0;
		prime::api::disc_t max = 0;
		prime::api::disc_t val;
	} mon_disc_ret_t;
	
	typedef struct mon_cont_ret_t {
		prime::api::cont_t min = 0;
		prime::api::cont_t max = 0;
		prime::api::cont_t val;
	} mon_cont_ret_t;
} } }

#endif
