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

#ifndef PRIME_API_APP_H
#define PRIME_API_APP_H

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "prime_api_t.h"
#include "prime_api_app_t.h"
#include "uds.h"

namespace prime { namespace api { namespace app
{
	class rtm_interface
	{
	public:
	/* ---------------------------------- Struct Constructors --------------------------------- */
		rtm_interface(pid_t proc_id, prime::uds::socket_addrs_t *app_addrs);
		
	/* ----------------------------------- UDS Constructors ----------------------------------- */
		/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
		rtm_interface(pid_t proc_id, bool logger_en = true);
		
		/* UDS API, UDS Logger: Specified Logger path. */
		rtm_interface(
			pid_t proc_id, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);
		
		/* UDS API, UDS Logger: Specified API path. */
		rtm_interface(
			pid_t proc_id, 
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
		
		/* UDS API, UDS Logger: Specified logger and API paths. */
		rtm_interface(
			pid_t proc_id, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */
		
	/* --------------------------------- Socket Constructors ---------------------------------- */
		/* Default UDS API, Network Logger address & path. */
		rtm_interface(
			pid_t proc_id, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);
			
		/* UDS API, Network Logger: Specified API path and logger address & port. */
		rtm_interface(
			pid_t proc_id, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
			
			
		/* Network API, default UDS logger. */
		rtm_interface(
			pid_t proc_id, 
			bool local_logger,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en = true
		);
			
		/* Network API, UDS Logger: Specified logger path and API port. */
		rtm_interface(
			pid_t proc_id, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en = true
		);
		
		
		/* Network API & Logger. */
		rtm_interface(
			pid_t proc_id, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */
		
		~rtm_interface();

		void message_handler(std::vector<char> message);

		void reg(pid_t proc_id, unsigned long int ur_id);
		void dereg(pid_t proc_id);

		knob_disc_t knob_disc_reg(knob_type_t type, prime::api::disc_t min, prime::api::disc_t max, prime::api::disc_t val);
		knob_cont_t knob_cont_reg(knob_type_t type, prime::api::cont_t min, prime::api::cont_t max, prime::api::cont_t val);

		void knob_disc_min(knob_disc_t knob, prime::api::disc_t min);
		void knob_disc_max(knob_disc_t knob, prime::api::disc_t max);
		void knob_cont_min(knob_cont_t knob, prime::api::cont_t min);
		void knob_cont_max(knob_cont_t knob, prime::api::cont_t max);

		prime::api::disc_t knob_disc_get(knob_disc_t knob);
		prime::api::cont_t knob_cont_get(knob_cont_t knob);

		void knob_disc_dereg(knob_disc_t knob);
		void knob_cont_dereg(knob_cont_t knob);

		mon_disc_t mon_disc_reg(mon_type_t type, prime::api::disc_t min, prime::api::disc_t max, prime::api::cont_t weight);
		mon_cont_t mon_cont_reg(mon_type_t type, prime::api::cont_t min, prime::api::cont_t max, prime::api::cont_t weight);
		void mon_disc_min(mon_disc_t mon, prime::api::disc_t min);
		void mon_disc_max(mon_disc_t mon, prime::api::disc_t max);
		void mon_disc_weight(mon_disc_t mon, prime::api::cont_t weight);
		void mon_cont_min(mon_cont_t mon, prime::api::cont_t min);
		void mon_cont_max(mon_cont_t mon, prime::api::cont_t max);
		void mon_cont_weight(mon_cont_t mon, prime::api::cont_t weight);

		void mon_disc_set(mon_disc_t mon, prime::api::disc_t val);
		void mon_cont_set(mon_cont_t mon, prime::api::cont_t val);

		void mon_disc_dereg(mon_disc_t mon);
		void mon_cont_dereg(mon_cont_t mon);

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs, pid_t proc_id);
		
		pid_t proc_id;
		bool default_addrs;
		bool logger_en;
		prime::uds socket;
		prime::uds ui_socket;
		prime::uds logger_socket;

		std::mutex knobs_disc_m;
		std::vector<knob_disc_t> knobs_disc;
		std::mutex knobs_cont_m;
		std::vector<knob_cont_t> knobs_cont;
		std::mutex mons_disc_m;
		std::vector<mon_disc_t> mons_disc;
		std::mutex mons_cont_m;
		std::vector<mon_cont_t> mons_cont;

		std::mutex disc_return_m;
		prime::api::disc_t disc_return;
		std::mutex cont_return_m;
		prime::api::cont_t cont_return;

		std::mutex knob_disc_return_m;
		knob_disc_t knob_disc_return;
		std::mutex knob_cont_return_m;
		knob_cont_t knob_cont_return;
		std::mutex mon_disc_return_m;
		mon_disc_t mon_disc_return;
		std::mutex mon_cont_return_m;
		mon_cont_t mon_cont_return;

		std::mutex app_reg_m;
		std::condition_variable app_reg_cv;
		std::mutex app_dereg_m;
		std::condition_variable app_dereg_cv;

		std::mutex knob_disc_reg_m;
		std::condition_variable knob_disc_reg_cv;
		std::mutex knob_cont_reg_m;
		std::condition_variable knob_cont_reg_cv;
		std::mutex knob_disc_get_m;
		std::condition_variable knob_disc_get_cv;
		bool knob_disc_get_flag;
		std::mutex knob_cont_get_m;
		std::condition_variable knob_cont_get_cv;
		bool knob_cont_get_flag;
		std::mutex mon_disc_reg_m;
		std::condition_variable mon_disc_reg_cv;
		std::mutex mon_cont_reg_m;
		std::condition_variable mon_cont_reg_cv;
		std::mutex mon_disc_get_m;
		std::condition_variable mon_disc_get_cv;
		std::mutex mon_cont_get_m;
		std::condition_variable mon_cont_get_cv;
	};

	class ui_interface
	{
	public:
	/* ---------------------------------- Struct Constructors --------------------------------- */
		ui_interface(pid_t proc_id,
			boost::function<void(pid_t)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_min_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_max_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_weight_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_min_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_max_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_weight_handler,
			boost::function<void(pid_t)> app_stop_handler,
			prime::uds::socket_addrs_t *ui_addrs
			);
			
	/* ----------------------------------- UDS Constructors ----------------------------------- */
		ui_interface(pid_t proc_id,
			boost::function<void(pid_t)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_min_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_max_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_weight_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_min_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_max_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_weight_handler,
			boost::function<void(pid_t)> app_stop_handler,
			bool logger_en = true
		);
		
		ui_interface(pid_t proc_id,
			boost::function<void(pid_t)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_min_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_max_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_weight_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_min_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_max_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_weight_handler,
			boost::function<void(pid_t)> app_stop_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */
	
	/* --------------------------------- Socket Constructors ---------------------------------- */
		ui_interface(pid_t proc_id,
			boost::function<void(pid_t)> app_reg_handler,
			boost::function<void(pid_t)> app_dereg_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_min_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_max_handler,
			boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_weight_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_min_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_max_handler,
			boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_weight_handler,
			boost::function<void(pid_t)> app_stop_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */
		
		~ui_interface();

		void message_handler(std::vector<char> message);

		void return_ui_app_start(void);
		void return_ui_app_stop(void);
		void ui_app_error(std::string message);

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs, pid_t proc_id);
		pid_t proc_id;
		bool logger_en;
		bool default_addrs;

		boost::function<void(pid_t)> app_reg_handler;
		boost::function<void(pid_t)> app_dereg_handler;
		boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_min_handler;
		boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_max_handler;
		boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_weight_handler;
		boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_min_handler;
		boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_max_handler;
		boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_weight_handler;
		boost::function<void(pid_t)> app_stop_handler;

		prime::uds socket;
		prime::uds logger_socket;
	};
} } }
#endif
