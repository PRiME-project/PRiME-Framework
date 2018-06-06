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
 
#ifndef PRIME_API_RTM_H
#define PRIME_API_RTM_H

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <boost/property_tree/ptree.hpp>
#include "uds.h"
#include "prime_api_t.h"
#include "prime_api_app_t.h"
#include "prime_api_dev_t.h"

namespace prime { namespace api { namespace rtm
{
	class app_interface
	{
	public:
		/* ---------------------------------- Struct Constructors --------------------------------- */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			prime::uds::socket_addrs_t *app_addrs
		);

		/* ----------------------------------- UDS Constructors ----------------------------------- */
		/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			bool logger_en = true
		);

		/* UDS API, UDS Logger: Specified Logger path. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);

		/* UDS API, UDS Logger: Specified API path. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);

		/* UDS API, UDS Logger: Specified logger and API paths. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */

	/* --------------------------------- Socket Constructors ---------------------------------- */
		/* Default UDS API, Network Logger address & path. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);

		/* UDS API, Network Logger: Specified API path and logger address & port. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);


		/* Network API, default UDS logger. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			unsigned int api_local_port,
			bool logger_en = true
		);

		/* Network API, UDS Logger: Specified logger path and API port. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			unsigned int api_local_port,
			bool logger_en = true
		);


		/* Network API & Logger. */
		app_interface(
			boost::function<void(pid_t, unsigned long int)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler,
			boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler,
			boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler,
			boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler,
			boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler,
			boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler,
			boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler,
			boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			unsigned int api_local_port,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */

		~app_interface();

		void knob_disc_set(prime::api::app::knob_disc_t knob, prime::api::disc_t val);
		void knob_cont_set(prime::api::app::knob_cont_t knob, prime::api::cont_t val);
		unsigned int get_unique_knob_id(void);
		unsigned int get_unique_mon_id(void);


		void message_handler(std::vector<char> message);

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs);
		void return_app_reg(pid_t proc_id);
		void return_app_dereg(pid_t proc_id);

		void return_knob_disc_reg(prime::api::app::knob_disc_t knob);
		void return_knob_cont_reg(prime::api::app::knob_cont_t knob);
		void return_mon_disc_reg(prime::api::app::mon_disc_t mon);
		void return_mon_cont_reg(prime::api::app::mon_cont_t mon);

		void return_knob_disc_get(pid_t proc_id, unsigned int id);
		void return_knob_cont_get(pid_t proc_id, unsigned int id);

		boost::function<void(pid_t, unsigned long int)> app_reg_handler;
		boost::function<void(pid_t)> app_dereg_handler;
		boost::function<void(pid_t, prime::api::app::knob_disc_t)> knob_disc_reg_handler;
		boost::function<void(pid_t, prime::api::app::knob_cont_t)> knob_cont_reg_handler;
		boost::function<void(prime::api::app::knob_disc_t)> knob_disc_dereg_handler;
		boost::function<void(prime::api::app::knob_cont_t)> knob_cont_dereg_handler;
		boost::function<void(pid_t, prime::api::app::mon_disc_t)> mon_disc_reg_handler;
		boost::function<void(pid_t, prime::api::app::mon_cont_t)> mon_cont_reg_handler;
		boost::function<void(prime::api::app::mon_disc_t)> mon_disc_dereg_handler;
		boost::function<void(prime::api::app::mon_cont_t)> mon_cont_dereg_handler;

		boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_min_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::disc_t)> knob_disc_max_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_min_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::cont_t)> knob_cont_max_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_min_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_max_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_weight_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::disc_t)> mon_disc_val_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_min_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_max_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_weight_change_handler;
		boost::function<void(pid_t, unsigned int, prime::api::cont_t)> mon_cont_val_change_handler;

		std::vector<prime::api::app::knob_disc_t> knobs_disc;
		std::mutex knobs_disc_m;
		std::vector<prime::api::app::knob_cont_t> knobs_cont;
		std::mutex knobs_cont_m;
		std::vector<prime::api::app::mon_disc_t> mons_disc;
		std::mutex mons_disc_m;
		std::vector<prime::api::app::mon_cont_t> mons_cont;
		std::mutex mons_cont_m;

		std::vector<std::pair<pid_t, std::shared_ptr<prime::uds>>> app_sockets;
		std::mutex app_sockets_m;

		bool default_addrs;
		bool logger_en;

		prime::uds socket;
		prime::uds logger_socket;
		prime::uds ui_socket;

		unsigned int next_unique_knob_id;
		unsigned int next_unique_mon_id;
	};

	class dev_interface
	{
	public:

		dev_interface(prime::uds::socket_addrs_t *dev_addrs);

	/* ----------------------------------- UDS Constructors ----------------------------------- */
		/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
		dev_interface(bool logger_en = true);

		/* UDS API, UDS Logger: Specified Logger path. */
		dev_interface(
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);

		/* UDS API, UDS Logger: Specified API path. */
		dev_interface(
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);

		/* UDS API, UDS Logger: Specified logger and API paths. */
		dev_interface(
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */

	/* --------------------------------- Socket Constructors ---------------------------------- */
		/* Default UDS API, Network Logger address & path. */
		dev_interface(
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);

		/* UDS API, Network Logger: Specified API path and logger address & port. */
		dev_interface(
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);


		/* Network API, default UDS logger. */
		dev_interface(
			bool local_logger,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en = true
		);

		/* Network API, UDS Logger: Specified logger path and API port. */
		dev_interface(
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en = true
		);


		/* Network API & Logger. */
		dev_interface(
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */

		~dev_interface();

		void message_handler(std::vector<char> message);

		unsigned int knob_disc_size(void);
		unsigned int knob_cont_size(void);

		std::vector<prime::api::dev::knob_disc_t> knob_disc_reg(void);
		std::vector<prime::api::dev::knob_cont_t> knob_cont_reg(void);

		prime::api::dev::knob_type_t knob_disc_type(prime::api::dev::knob_disc_t knob);
		prime::api::disc_t knob_disc_min(prime::api::dev::knob_disc_t knob);
		prime::api::disc_t knob_disc_max(prime::api::dev::knob_disc_t knob);
		prime::api::disc_t knob_disc_init(prime::api::dev::knob_disc_t knob);
		prime::api::dev::knob_type_t knob_cont_type(prime::api::dev::knob_cont_t knob);
		prime::api::cont_t knob_cont_min(prime::api::dev::knob_cont_t knob);
		prime::api::cont_t knob_cont_max(prime::api::dev::knob_cont_t knob);
		prime::api::cont_t knob_cont_init(prime::api::dev::knob_cont_t knob);

		void knob_disc_set(prime::api::dev::knob_disc_t knob, prime::api::disc_t val);
		void knob_cont_set(prime::api::dev::knob_cont_t knob, prime::api::cont_t val);

		void knob_disc_dereg(std::vector<prime::api::dev::knob_disc_t>& knobs);
		void knob_cont_dereg(std::vector<prime::api::dev::knob_cont_t>& knobs);

		unsigned int mon_disc_size(void);
		unsigned int mon_cont_size(void);

		std::vector<prime::api::dev::mon_disc_t> mon_disc_reg(void);
		std::vector<prime::api::dev::mon_cont_t> mon_cont_reg(void);

		prime::api::dev::mon_type_t mon_disc_type(prime::api::dev::mon_disc_t mon);
		prime::api::dev::mon_type_t mon_cont_type(prime::api::dev::mon_cont_t mon);

		prime::api::disc_t mon_disc_get(prime::api::dev::mon_disc_t& mon);
		prime::api::cont_t mon_cont_get(prime::api::dev::mon_cont_t& mon);

		void mon_disc_dereg(std::vector<prime::api::dev::mon_disc_t>& mons);
		void mon_cont_dereg(std::vector<prime::api::dev::mon_cont_t>& mons);

		boost::property_tree::ptree dev_arch_get(void);

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs);
		bool default_addrs;
		bool logger_en;

		std::vector<prime::api::dev::knob_disc_t> knobs_disc;
		std::mutex knobs_disc_m;
		std::vector<prime::api::dev::knob_cont_t> knobs_cont;
		std::mutex knobs_cont_m;
		std::vector<prime::api::dev::mon_disc_t> mons_disc;
		std::mutex mons_disc_m;
		std::vector<prime::api::dev::mon_cont_t> mons_cont;
		std::mutex mons_cont_m;

		prime::uds socket;
		prime::uds logger_socket;

		boost::property_tree::ptree dev_architecture;

		std::mutex disc_return_m;
		prime::api::disc_t disc_return;
		prime::api::disc_t disc_min;
		prime::api::disc_t disc_max;
		std::mutex cont_return_m;
		prime::api::cont_t cont_return;
		prime::api::cont_t cont_min;
		prime::api::cont_t cont_max;
		std::mutex unsigned_int_return_m;
		unsigned int unsigned_int_return;

		std::mutex knob_disc_size_m;
		std::condition_variable knob_disc_size_cv;
		std::mutex knob_cont_size_m;
		std::condition_variable knob_cont_size_cv;
		std::mutex knob_disc_reg_m;
		std::condition_variable knob_disc_reg_cv;
		std::mutex knob_cont_reg_m;
		std::condition_variable knob_cont_reg_cv;
		std::mutex mon_disc_size_m;
		std::condition_variable mon_disc_size_cv;
		std::mutex mon_cont_size_m;
		std::condition_variable mon_cont_size_cv;
		std::mutex mon_disc_reg_m;
		std::condition_variable mon_disc_reg_cv;
		std::mutex mon_cont_reg_m;
		std::condition_variable mon_cont_reg_cv;

		std::mutex mon_disc_get_m;
		std::condition_variable mon_disc_get_cv;
		bool mon_disc_get_flag;
		std::mutex mon_cont_get_m;
		std::condition_variable mon_cont_get_cv;
		bool mon_cont_get_flag;

		std::mutex dev_arch_return_m;
		std::mutex dev_arch_get_m;
		std::condition_variable dev_arch_get_cv;
	};

	class ui_interface
	{
	public:

		ui_interface(
			boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
			boost::function<void(void)> rtm_stop_handler,
			prime::uds::socket_addrs_t *ui_addrs
		);

		ui_interface(
			boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
			boost::function<void(void)> rtm_stop_handler,
			bool logger_en = true
		);

		ui_interface(
			boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
			boost::function<void(void)> rtm_stop_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);

		ui_interface(
			boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
			boost::function<void(void)> rtm_stop_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);

		~ui_interface();

		void message_handler(std::vector<char> message);

		void return_ui_rtm_start(void); //Signal to the UI that the RTM started cleanly
		void return_ui_rtm_stop(void); //Signal to the UI that the RTM stopped cleanly
		void ui_rtm_error(std::string message); //Signal to the UI that a warning or non-critical error occured in the RTM

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs);
		boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler;
		boost::function<void(void)> rtm_stop_handler;

		bool default_addrs;
		bool logger_en;

		prime::uds socket;
		prime::uds logger_socket;
	};

} } }
#endif
