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

#ifndef PRIME_API_T_H
#define PRIME_API_T_H

#include <limits.h>
#include <math.h>

namespace prime { namespace api
{
	// These are system-wide type definitions
	#define API_DELIMINATOR		"¿"
	
	
	#define UDS_DEFAULT_EXTENSION		".uds"
	
	// Default UDS Paths WITHOUT .uds extension
	#define UDS_DEFAULT_PATH_DEVRTM		"/tmp/dev.rtm"
	#define UDS_DEFAULT_PATH_DEVRTM_LOG	"/tmp/dev.rtm.logger"
	#define UDS_DEFAULT_PATH_DEV_UI		"/tmp/dev.ui"
	#define UDS_DEFAULT_PATH_DEV_UI_LOG	"/tmp/dev.ui.logger"
	#define UDS_DEFAULT_PATH_RTMDEV		"/tmp/rtm.dev"
	#define UDS_DEFAULT_PATH_RTMDEV_LOG	"/tmp/rtm.dev.logger"
	#define UDS_DEFAULT_PATH_RTMAPP		"/tmp/rtm.app"
	#define UDS_DEFAULT_PATH_RTMAPP_LOG	"/tmp/rtm.app.logger"
	#define UDS_DEFAULT_PATH_RTMAPP_UI	"/tmp/rtm.app.ui"
	#define UDS_DEFAULT_PATH_APPRTM		"/tmp/app.rtm"
	#define UDS_DEFAULT_PATH_APPRTM_LOG	"/tmp/app.rtm.logger"
	#define UDS_DEFAULT_PATH_APP_UI_LOG	"/tmp/app.ui.logger"
	#define UDS_DEFAULT_PATH_APP_UI		"/tmp/app.ui"
	#define UDS_DEFAULT_PATH_LOGGER		"/tmp/logger"
	#define UDS_DEFAULT_PATH_UI			"/tmp/ui"
	
	

	// Numeric types
	typedef signed int disc_t;
	typedef float cont_t;

	// Min and max limits
	static const disc_t PRIME_DISC_MIN = INT_MIN;
	static const disc_t PRIME_DISC_MAX = INT_MAX;
	static const cont_t PRIME_CONT_MIN = -INFINITY;
	static const cont_t PRIME_CONT_MAX = INFINITY;

	enum rtm_app_msg_t{
		PRIME_API_APP_RETURN_KNOB_DISC_GET = '0',
		PRIME_API_APP_RETURN_KNOB_CONT_GET = '1'
	};

	enum app_rtm_msg_t{
		PRIME_API_APP_KNOB_DISC_MIN = '2',
		PRIME_API_APP_KNOB_DISC_MAX = '3',
		PRIME_API_APP_KNOB_CONT_MIN = '4',
		PRIME_API_APP_KNOB_CONT_MAX = '5',
		PRIME_API_APP_KNOB_DISC_GET = '6',
		PRIME_API_APP_KNOB_CONT_GET = '7',
		PRIME_API_APP_MON_DISC_MIN = '8',
		PRIME_API_APP_MON_DISC_MAX = '9',
		PRIME_API_APP_MON_DISC_WEIGHT = 'a',
		PRIME_API_APP_MON_CONT_MIN = 'b',
		PRIME_API_APP_MON_CONT_MAX = 'c',
		PRIME_API_APP_MON_CONT_WEIGHT = 'd',
		PRIME_API_APP_MON_DISC_SET = 'e',
		PRIME_API_APP_MON_CONT_SET = 'f'
	};

	enum rtm_dev_msg_t{
		PRIME_API_DEV_KNOB_DISC_SET = 'g',
		PRIME_API_DEV_KNOB_CONT_SET = 'h',
		PRIME_API_DEV_MON_DISC_GET = 'i',
		PRIME_API_DEV_MON_CONT_GET = 'j'
	};

	enum dev_rtm_msg_t{
		PRIME_API_DEV_RETURN_MON_DISC_GET = 'k',
		PRIME_API_DEV_RETURN_MON_CONT_GET = 'l'
	};


} }

#endif
