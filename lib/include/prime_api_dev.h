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

#ifndef PRIME_API_DEV_H
#define PRIME_API_DEV_H

#include <string>
#include <vector>
#include <mutex>
#include "prime_api_t.h"
#include "prime_api_dev_t.h"
#include "uds.h"
#include "util.h"
#include <boost/property_tree/ptree.hpp>
#include "args/args.hxx"

namespace prime { namespace api { namespace dev
{
	int parse_cli(std::string device_name, prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::api::dev::dev_args_t* args, int argc, const char * argv[]);
	
	class rtm_interface
	{
	public:
		//rtm_interface(std::string archfilename);
		rtm_interface(
			std::string archfilename, 
			prime::uds::socket_addrs_t *dev_addrs
		);
		
		/* ----------------------------------- UDS Constructors ----------------------------------- */
		/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
		rtm_interface(std::string archfilename, bool logger_en = true);
		
		/* UDS API, UDS Logger: Specified Logger path. */
		rtm_interface(
			std::string archfilename, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);
		
		/* UDS API, UDS Logger: Specified API path. */
		rtm_interface(
			std::string archfilename, 
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
		
		/* UDS API, UDS Logger: Specified logger and API paths. */
		rtm_interface(
			std::string archfilename, 
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
			std::string archfilename, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);
			
		/* UDS API, Network Logger: Specified API path and logger address & port. */
		rtm_interface(
			std::string archfilename, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en = true
		);
			
			
		/* Network API, default UDS logger. */
		rtm_interface(
			std::string archfilename, 
			unsigned int api_local_port,
			bool logger_en = true
		);
			
		/* Network API, UDS Logger: Specified logger path and API port. */
		rtm_interface(
			std::string archfilename, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			unsigned int api_local_port,
			bool logger_en = true
		);
		
		
		/* Network API & Logger. */
		rtm_interface(
			std::string archfilename, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			unsigned int api_local_port,
			bool logger_en = true
		);
	/* ---------------------------------------------------------------------------------------- */
		
		
		~rtm_interface();

		void add_knob_disc(
			unsigned int id,
			knob_type_t type,
			prime::api::disc_t min,
			prime::api::disc_t max,
			prime::api::disc_t val,
			prime::api::disc_t init,
			boost::function<void(prime::api::disc_t)> set_handler
		);
		void remove_knob_disc(unsigned int id);

		void add_knob_cont(
			unsigned int id,
			knob_type_t type,
			prime::api::cont_t min,
			prime::api::cont_t max,
			prime::api::cont_t val,
			prime::api::cont_t init,
			boost::function<void(prime::api::cont_t)> set_handler
		);
		void remove_knob_cont(unsigned int id);

		void add_mon_disc(
			unsigned int id,
			mon_type_t type,
			prime::api::disc_t val,
			prime::api::disc_t min,
			prime::api::disc_t max,
			boost::function<prime::api::dev::mon_disc_ret_t(void)> get_handler
		);
		void remove_mon_disc(unsigned int id);

		void add_mon_cont(
			unsigned int id,
			mon_type_t type,
			prime::api::cont_t val,
			prime::api::cont_t min,
			prime::api::cont_t max,
			boost::function<prime::api::dev::mon_cont_ret_t(void)> get_handler
		);
		void remove_mon_cont(unsigned int id);

		boost::property_tree::ptree get_architecture(void){ return architecture; }
		void print_architecture(void);

		std::vector<std::pair<knob_disc_t, boost::function<void(prime::api::disc_t)>>> get_disc_knobs(void){ return knobs_disc; }
		std::vector<std::pair<knob_cont_t, boost::function<void(prime::api::cont_t)>>> get_cont_knobs(void){ return knobs_cont; }
		std::vector<std::pair<mon_disc_t, boost::function<prime::api::dev::mon_disc_ret_t(void)>>> get_disc_mons(void){ return mons_disc; }
		std::vector<std::pair<mon_cont_t, boost::function<prime::api::dev::mon_cont_ret_t(void)>>> get_cont_mons(void){ return mons_cont; }

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs);
		bool default_addrs;
		void message_handler(std::vector<char> message);

		void knob_disc_set(unsigned int id, prime::api::disc_t val);
		void knob_cont_set(unsigned int id, prime::api::cont_t val);

		prime::api::disc_t mon_disc_get(unsigned int id);
		prime::api::cont_t mon_cont_get(unsigned int id);

		void return_knob_disc_size(void);
		void return_knob_cont_size(void);

		void return_knob_disc_reg(void);
		void return_knob_cont_reg(void);

		void return_mon_disc_size(void);
		void return_mon_cont_size(void);

		void return_mon_disc_reg(void);
		void return_mon_cont_reg(void);

		void return_mon_disc_get(unsigned int id);
		void return_mon_cont_get(unsigned int id);

		void return_arch_get(void);
		std::string archfilename;
		bool logger_en;
		boost::property_tree::ptree architecture;

		prime::uds socket;
		prime::uds logger_socket;

		std::mutex knobs_disc_m;
		std::vector<std::pair<knob_disc_t, boost::function<void(prime::api::disc_t)>>> knobs_disc;
		std::mutex knobs_cont_m;
		std::vector<std::pair<knob_cont_t, boost::function<void(prime::api::cont_t)>>> knobs_cont;
		std::mutex mons_disc_m;
		std::vector<std::pair<mon_disc_t, boost::function<prime::api::dev::mon_disc_ret_t(void)>>> mons_disc;
		std::mutex mons_cont_m;
		std::vector<std::pair<mon_cont_t, boost::function<prime::api::dev::mon_cont_ret_t(void)>>> mons_cont;
	};

	class ui_interface
	{
	public:
		ui_interface(
			boost::function<void(void)> dev_stop_handler,
			prime::uds::socket_addrs_t *dev_addrs
		);
		
		ui_interface(
			boost::function<void(void)> dev_stop_handler,
			bool logger_en = true
		);
		
		ui_interface(
			boost::function<void(void)> dev_stop_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en = true
		);
		
		ui_interface(
			boost::function<void(void)> dev_stop_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en = true
		);
		
		~ui_interface();

		void message_handler(std::vector<char> message);

		void return_ui_dev_start(void); //Signal to the UI that the device started cleanly
		void return_ui_dev_stop(void); //Signal to the UI that the device stopped cleanly
		void ui_dev_error(std::string message); //Signal to the UI that a warning or non-critical error occured in the device

	private:
		static bool check_addrs(prime::uds::socket_addrs_t *socket_addrs);
		bool default_addrs;
		boost::function<void(void)> dev_stop_handler;

		bool logger_en;
		prime::uds socket;
		prime::uds logger_socket;
	};
} } }

#endif
