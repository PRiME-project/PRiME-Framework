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

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iomanip>
#include <stdexcept>
#include "uds.h"
#include "util.h"
#include "prime_api_rtm.h"

//#define DEBUG
//#define API_DEBUG

#define CREATE_JSON_ROOT(type) \
    boost::property_tree::ptree root; \
    root.put("ts", prime::util::get_timestamp()); \
    root.put("type", type); \
    boost::property_tree::ptree data_node

#define ADD_JSON_DATA(key, value) \
    data_node.put(key, value)

#define SEND_JSON() \
    root.add_child("data", data_node); \
    { std::stringstream ss; \
    write_json(ss, root); \
    std::string json_string = ss.str(); \
    prime::util::send_message(socket, json_string); \
    if(logger_en) {	prime::util::send_message(logger_socket, json_string); } }

#define SEND_JSON_DEBUG() \
    root.add_child("data", data_node); \
    { std::stringstream ss; \
    write_json(ss, root); \
    std::string json_string = ss.str(); \
	std::cout << json_string << std::endl; \
    prime::util::send_message(socket, json_string); \
    if(logger_en) {	prime::util::send_message(logger_socket, json_string); } }

#define SEND_JSON_PID(pid) \
    root.add_child("data", data_node); \
    std::stringstream ss; \
    write_json(ss, root); \
    std::string json_string = ss.str(); \
    std::shared_ptr<prime::uds> socket_ptr; \
    app_sockets_m.lock(); \
    for(auto app_socket : app_sockets) { \
	if(app_socket.first == pid) { \
	    socket_ptr = app_socket.second; \
	} \
    } \
    app_sockets_m.unlock(); \
    prime::util::send_message(*socket_ptr.get(), json_string); \
    if(logger_en) {	prime::util::send_message(logger_socket, json_string); }  \


namespace prime { namespace api { namespace rtm
{
	bool app_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs)
	{
		// Check that addresses are present.
		bool ret = false;

		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/rtm.app.logger.uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;
			socket_addrs->remote_logger = false;
			ret = true;
		}

		if(!socket_addrs->api_addr) {
			//No API address present, use default.
			socket_addrs->api_local_address = "/tmp/rtm.app.uds";
			socket_addrs->api_remote_address = "/tmp/app.rtm.uds";
			socket_addrs->api_addr = true;
			socket_addrs->remote_api = false;
			ret = true;
		}

		if(!socket_addrs->ui_addr) {
			//No UI address present, use default.
			socket_addrs->ui_local_address = "/tmp/rtm.app.ui.uds";
			socket_addrs->ui_remote_address = "/tmp/ui.uds";
			socket_addrs->ui_addr = true;
			socket_addrs->remote_ui = false;
			ret = true;
		}

		return ret;
	}

	/* ---------------------------------- Struct Constructors --------------------------------- */
	app_interface::app_interface(
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
		) :
		next_unique_knob_id(1),
		next_unique_mon_id(1),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		knob_disc_reg_handler(knob_disc_reg_handler),
		knob_cont_reg_handler(knob_cont_reg_handler),
		knob_disc_dereg_handler(knob_disc_dereg_handler),
		knob_cont_dereg_handler(knob_cont_dereg_handler),
		mon_disc_reg_handler(mon_disc_reg_handler),
		mon_cont_reg_handler(mon_cont_reg_handler),
		mon_disc_dereg_handler(mon_disc_dereg_handler),
		mon_cont_dereg_handler(mon_cont_dereg_handler),
		knob_disc_min_change_handler(knob_disc_min_change_handler),
		knob_disc_max_change_handler(knob_disc_max_change_handler),
		knob_cont_min_change_handler(knob_cont_min_change_handler),
		knob_cont_max_change_handler(knob_cont_max_change_handler),
		mon_disc_min_change_handler(mon_disc_min_change_handler),
		mon_disc_max_change_handler(mon_disc_max_change_handler),
		mon_disc_weight_change_handler(mon_disc_weight_change_handler),
		mon_disc_val_change_handler(mon_disc_val_change_handler),
		mon_cont_min_change_handler(mon_cont_min_change_handler),
		mon_cont_max_change_handler(mon_cont_max_change_handler),
		mon_cont_weight_change_handler(mon_cont_weight_change_handler),
		mon_cont_val_change_handler(mon_cont_val_change_handler),

		default_addrs(check_addrs(app_addrs)),
		logger_en(app_addrs->logger_en),
		socket(
			prime::uds::socket_layer_t::RTM_APP,
			app_addrs,
			boost::bind(&app_interface::message_handler, this, _1)),
		ui_socket(prime::uds::socket_layer_t::UI, app_addrs),
		logger_socket(prime::uds::socket_layer_t::LOGGER, app_addrs)
	{
#ifdef API_DEBUG
		std::cout << "APP Interface:" << std::endl;
		std::cout << "\tAPI Socket: " << socket.get_endpoints() << std::endl;
		std::cout << "\tUI Socket: " << ui_socket.get_endpoints() << std::endl;
		std::cout << "\tLogger Socket: " << logger_socket.get_endpoints() << std::endl;
#endif
	}


	/* ----------------------------------- UDS Constructors ----------------------------------- */
	/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		app_interface(app_reg_handler,
				app_dereg_handler,
				knob_disc_reg_handler,
				knob_cont_reg_handler,
				knob_disc_dereg_handler,
				knob_cont_dereg_handler,
				mon_disc_reg_handler,
				mon_cont_reg_handler,
				mon_disc_dereg_handler,
				mon_cont_dereg_handler,
				knob_disc_min_change_handler,
				knob_disc_max_change_handler,
				knob_cont_min_change_handler,
				knob_cont_max_change_handler,
				mon_disc_min_change_handler,
				mon_disc_max_change_handler,
				mon_disc_weight_change_handler,
				mon_disc_val_change_handler,
				mon_cont_min_change_handler,
				mon_cont_max_change_handler,
				mon_cont_weight_change_handler,
				mon_cont_val_change_handler,
				"/tmp/rtm.app.logger.uds",
				"/tmp/logger.uds",
				"/tmp/rtm.app.uds",
				"/tmp/app.rtm.uds",
				logger_en)
	{ }

	/* UDS API, UDS Logger: Specified Logger path. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		app_interface(app_reg_handler,
				app_dereg_handler,
				knob_disc_reg_handler,
				knob_cont_reg_handler,
				knob_disc_dereg_handler,
				knob_cont_dereg_handler,
				mon_disc_reg_handler,
				mon_cont_reg_handler,
				mon_disc_dereg_handler,
				mon_cont_dereg_handler,
				knob_disc_min_change_handler,
				knob_disc_max_change_handler,
				knob_cont_min_change_handler,
				knob_cont_max_change_handler,
				mon_disc_min_change_handler,
				mon_disc_max_change_handler,
				mon_disc_weight_change_handler,
				mon_disc_val_change_handler,
				mon_cont_min_change_handler,
				mon_cont_max_change_handler,
				mon_cont_weight_change_handler,
				mon_cont_val_change_handler,
				logger_local_filename,
				logger_remote_filename,
				"/tmp/rtm.app.uds",
				"/tmp/app.rtm.uds",
				logger_en)
	{ }

	/* UDS API, UDS Logger: Specified API path. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		app_interface(app_reg_handler,
				app_dereg_handler,
				knob_disc_reg_handler,
				knob_cont_reg_handler,
				knob_disc_dereg_handler,
				knob_cont_dereg_handler,
				mon_disc_reg_handler,
				mon_cont_reg_handler,
				mon_disc_dereg_handler,
				mon_cont_dereg_handler,
				knob_disc_min_change_handler,
				knob_disc_max_change_handler,
				knob_cont_min_change_handler,
				knob_cont_max_change_handler,
				mon_disc_min_change_handler,
				mon_disc_max_change_handler,
				mon_disc_weight_change_handler,
				mon_disc_val_change_handler,
				mon_cont_min_change_handler,
				mon_cont_max_change_handler,
				mon_cont_weight_change_handler,
				mon_cont_val_change_handler,
				"/tmp/rtm.app.logger.uds",
				"/tmp/logger.uds",
				api_local_filename,
				api_remote_filename,
				logger_en)
	{ }

	/* UDS API, UDS Logger: Specified logger and API paths. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		next_unique_knob_id(1),
		next_unique_mon_id(1),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		knob_disc_reg_handler(knob_disc_reg_handler),
		knob_cont_reg_handler(knob_cont_reg_handler),
		knob_disc_dereg_handler(knob_disc_dereg_handler),
		knob_cont_dereg_handler(knob_cont_dereg_handler),
		mon_disc_reg_handler(mon_disc_reg_handler),
		mon_cont_reg_handler(mon_cont_reg_handler),
		mon_disc_dereg_handler(mon_disc_dereg_handler),
		mon_cont_dereg_handler(mon_cont_dereg_handler),
		knob_disc_min_change_handler(knob_disc_min_change_handler),
		knob_disc_max_change_handler(knob_disc_max_change_handler),
		knob_cont_min_change_handler(knob_cont_min_change_handler),
		knob_cont_max_change_handler(knob_cont_max_change_handler),
		mon_disc_min_change_handler(mon_disc_min_change_handler),
		mon_disc_max_change_handler(mon_disc_max_change_handler),
		mon_disc_weight_change_handler(mon_disc_weight_change_handler),
		mon_disc_val_change_handler(mon_disc_val_change_handler),
		mon_cont_min_change_handler(mon_cont_min_change_handler),
		mon_cont_max_change_handler(mon_cont_max_change_handler),
		mon_cont_weight_change_handler(mon_cont_weight_change_handler),
		mon_cont_val_change_handler(mon_cont_val_change_handler),
		ui_socket("/tmp/rtm.app.ui.uds", "/tmp/ui.uds"),
		logger_en(logger_en),
		socket(
			api_local_filename,
			api_remote_filename,
			boost::bind(&app_interface::message_handler, this, _1)),
		logger_socket(
			logger_local_filename,
			logger_remote_filename)
	{ }
	/* ---------------------------------------------------------------------------------------- */

	/* --------------------------------- Socket Constructors ---------------------------------- */
	/* Default UDS API, Network Logger address & path. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		app_interface(app_reg_handler,
				app_dereg_handler,
				knob_disc_reg_handler,
				knob_cont_reg_handler,
				knob_disc_dereg_handler,
				knob_cont_dereg_handler,
				mon_disc_reg_handler,
				mon_cont_reg_handler,
				mon_disc_dereg_handler,
				mon_cont_dereg_handler,
				knob_disc_min_change_handler,
				knob_disc_max_change_handler,
				knob_cont_min_change_handler,
				knob_cont_max_change_handler,
				mon_disc_min_change_handler,
				mon_disc_max_change_handler,
				mon_disc_weight_change_handler,
				mon_disc_val_change_handler,
				mon_cont_min_change_handler,
				mon_cont_max_change_handler,
				mon_cont_weight_change_handler,
				mon_cont_val_change_handler,
				logger_remote_address,
				logger_remote_port,
				"/tmp/rtm.app.uds",
				"/tmp/app.rtm.uds",
				logger_en)
	{ }

	/* UDS API, Network Logger: Specified API path and logger address & port. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		next_unique_knob_id(1),
		next_unique_mon_id(1),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		knob_disc_reg_handler(knob_disc_reg_handler),
		knob_cont_reg_handler(knob_cont_reg_handler),
		knob_disc_dereg_handler(knob_disc_dereg_handler),
		knob_cont_dereg_handler(knob_cont_dereg_handler),
		mon_disc_reg_handler(mon_disc_reg_handler),
		mon_cont_reg_handler(mon_cont_reg_handler),
		mon_disc_dereg_handler(mon_disc_dereg_handler),
		mon_cont_dereg_handler(mon_cont_dereg_handler),
		knob_disc_min_change_handler(knob_disc_min_change_handler),
		knob_disc_max_change_handler(knob_disc_max_change_handler),
		knob_cont_min_change_handler(knob_cont_min_change_handler),
		knob_cont_max_change_handler(knob_cont_max_change_handler),
		mon_disc_min_change_handler(mon_disc_min_change_handler),
		mon_disc_max_change_handler(mon_disc_max_change_handler),
		mon_disc_weight_change_handler(mon_disc_weight_change_handler),
		mon_disc_val_change_handler(mon_disc_val_change_handler),
		mon_cont_min_change_handler(mon_cont_min_change_handler),
		mon_cont_max_change_handler(mon_cont_max_change_handler),
		mon_cont_weight_change_handler(mon_cont_weight_change_handler),
		mon_cont_val_change_handler(mon_cont_val_change_handler),
		ui_socket("/tmp/rtm.app.ui.uds", "/tmp/ui.uds"),
		logger_en(logger_en),
		socket(
			api_local_filename,
			api_remote_filename,
			boost::bind(&app_interface::message_handler, this, _1)),
		logger_socket(
			logger_remote_address,
			logger_remote_port)
	{ }


	/* Network API, default UDS logger. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		app_interface(app_reg_handler,
				app_dereg_handler,
				knob_disc_reg_handler,
				knob_cont_reg_handler,
				knob_disc_dereg_handler,
				knob_cont_dereg_handler,
				mon_disc_reg_handler,
				mon_cont_reg_handler,
				mon_disc_dereg_handler,
				mon_cont_dereg_handler,
				knob_disc_min_change_handler,
				knob_disc_max_change_handler,
				knob_cont_min_change_handler,
				knob_cont_max_change_handler,
				mon_disc_min_change_handler,
				mon_disc_max_change_handler,
				mon_disc_weight_change_handler,
				mon_disc_val_change_handler,
				mon_cont_min_change_handler,
				mon_cont_max_change_handler,
				mon_cont_weight_change_handler,
				mon_cont_val_change_handler,
				"/tmp/rtm.app.logger.uds",
				"/tmp/logger.uds",
				api_local_port,
				logger_en)
	{ }

	/* Network API, UDS Logger: Specified logger path and API port. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		next_unique_knob_id(1),
		next_unique_mon_id(1),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		knob_disc_reg_handler(knob_disc_reg_handler),
		knob_cont_reg_handler(knob_cont_reg_handler),
		knob_disc_dereg_handler(knob_disc_dereg_handler),
		knob_cont_dereg_handler(knob_cont_dereg_handler),
		mon_disc_reg_handler(mon_disc_reg_handler),
		mon_cont_reg_handler(mon_cont_reg_handler),
		mon_disc_dereg_handler(mon_disc_dereg_handler),
		mon_cont_dereg_handler(mon_cont_dereg_handler),
		knob_disc_min_change_handler(knob_disc_min_change_handler),
		knob_disc_max_change_handler(knob_disc_max_change_handler),
		knob_cont_min_change_handler(knob_cont_min_change_handler),
		knob_cont_max_change_handler(knob_cont_max_change_handler),
		mon_disc_min_change_handler(mon_disc_min_change_handler),
		mon_disc_max_change_handler(mon_disc_max_change_handler),
		mon_disc_weight_change_handler(mon_disc_weight_change_handler),
		mon_disc_val_change_handler(mon_disc_val_change_handler),
		mon_cont_min_change_handler(mon_cont_min_change_handler),
		mon_cont_max_change_handler(mon_cont_max_change_handler),
		mon_cont_weight_change_handler(mon_cont_weight_change_handler),
		mon_cont_val_change_handler(mon_cont_val_change_handler),
		ui_socket("/tmp/rtm.app.ui.uds", "/tmp/ui.uds"),
		logger_en(logger_en),
		socket(
			api_local_port,
			boost::bind(&app_interface::message_handler, this, _1)),
		logger_socket(
			logger_local_filename,
			logger_remote_filename)
	{ }


	/* Network API & Logger. */
	app_interface::app_interface(
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
			bool logger_en
		) :
		next_unique_knob_id(1),
		next_unique_mon_id(1),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		knob_disc_reg_handler(knob_disc_reg_handler),
		knob_cont_reg_handler(knob_cont_reg_handler),
		knob_disc_dereg_handler(knob_disc_dereg_handler),
		knob_cont_dereg_handler(knob_cont_dereg_handler),
		mon_disc_reg_handler(mon_disc_reg_handler),
		mon_cont_reg_handler(mon_cont_reg_handler),
		mon_disc_dereg_handler(mon_disc_dereg_handler),
		mon_cont_dereg_handler(mon_cont_dereg_handler),
		knob_disc_min_change_handler(knob_disc_min_change_handler),
		knob_disc_max_change_handler(knob_disc_max_change_handler),
		knob_cont_min_change_handler(knob_cont_min_change_handler),
		knob_cont_max_change_handler(knob_cont_max_change_handler),
		mon_disc_min_change_handler(mon_disc_min_change_handler),
		mon_disc_max_change_handler(mon_disc_max_change_handler),
		mon_disc_weight_change_handler(mon_disc_weight_change_handler),
		mon_disc_val_change_handler(mon_disc_val_change_handler),
		mon_cont_min_change_handler(mon_cont_min_change_handler),
		mon_cont_max_change_handler(mon_cont_max_change_handler),
		mon_cont_weight_change_handler(mon_cont_weight_change_handler),
		mon_cont_val_change_handler(mon_cont_val_change_handler),
		ui_socket("/tmp/rtm.app.ui.uds", "/tmp/ui.uds"),
		logger_en(logger_en),
		socket(
			api_local_port,
			boost::bind(&app_interface::message_handler, this, _1)),
		logger_socket(
			logger_remote_address,
			logger_remote_port)
	{ }
	/* ---------------------------------------------------------------------------------------- */



	app_interface::~app_interface()
	{
	}

	void app_interface::message_handler(std::vector<char> message)
	{
		if(message[0] != '{') { // Not json, process quickly
			std::string delim = API_DELIMINATOR;
			size_t position;
			std::string pid, id, val, type;
			std::string message_string( (reinterpret_cast<char*>(message.data())) + 1 + delim.length());		// This is 4ms+ faster than the two lines below!

#ifdef DEBUG
			std::string ts;
			std::cout << "Message String: " << message[0] << API_DELIMINATOR << message_string << std::endl;
#endif
			switch((prime::api::app_rtm_msg_t)message[0]) {
				case PRIME_API_APP_KNOB_DISC_MIN:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					knob_disc_min_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_APP_KNOB_DISC_MAX:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					knob_disc_max_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_APP_KNOB_CONT_MIN:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					knob_cont_min_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stof(val));
					break;

				case PRIME_API_APP_KNOB_CONT_MAX:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					knob_cont_max_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stof(val));
					break;

				case PRIME_API_APP_KNOB_DISC_GET:
					// Four fields: Type, ID, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					return_knob_disc_get((pid_t)std::stoi(pid), std::stoul(id));
					break;

				case PRIME_API_APP_KNOB_CONT_GET:
					// Four fields: Type, ID, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					return_knob_cont_get((pid_t)std::stoi(pid), std::stoul(id));
					break;

				case PRIME_API_APP_MON_DISC_MIN:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_disc_min_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_APP_MON_DISC_MAX:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_disc_max_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_APP_MON_DISC_WEIGHT:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_disc_weight_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_APP_MON_CONT_MIN:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_cont_min_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stof(val));
					break;

				case PRIME_API_APP_MON_CONT_MAX:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_cont_max_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stof(val));
					break;

				case PRIME_API_APP_MON_CONT_WEIGHT:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_cont_weight_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stof(val));
					break;

				case PRIME_API_APP_MON_DISC_SET:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_disc_val_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_APP_MON_CONT_SET:
					// Five fields: Type, ID, Val, PID, TS
					position = message_string.find(delim);
					id = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					pid = message_string.substr(0, position);

					mon_cont_val_change_handler((pid_t)std::stoi(pid), std::stoul(id), std::stof(val));
					break;

				default:
#ifdef DEBUG
					std::cout << "Error: unknown message: " <<  message_string << std::endl;
#endif
					break;
			}

		} else {	// This is json, send it to the slow lane.
			std::string message_string(reinterpret_cast<char*>(message.data()));		// This is 4ms+ faster than the two lines below!
			//std::string message_string(message.begin(), message.end());
			//boost::trim_right_if(message_string, boost::is_any_of(std::string("\0", 1)));
			std::stringstream ss;
			ss << message_string;
			boost::property_tree::ptree root;
			boost::property_tree::read_json(ss, root);

			std::string message_type(root.get<std::string>("type"));
			boost::property_tree::ptree data = root.get_child("data");

			try{
				if(!message_type.compare("PRIME_API_APP_REG")) {
					pid_t proc_id = data.get<pid_t>("proc_id");
					unsigned long int ur_id = data.get<unsigned long int>("ur_id");
					std::string local_endpoint_address = std::string("/tmp/rtm.app.") + std::to_string(proc_id) + std::string(".uds");
					std::string remote_endpoint_address = std::string("/tmp/app.rtm.") + std::to_string(proc_id) + std::string(".uds");
					auto app_socket = std::make_pair(
								proc_id,
								std::shared_ptr<prime::uds>(new prime::uds(
												local_endpoint_address,
												remote_endpoint_address,
												boost::bind(&app_interface::message_handler, this, _1))));

					app_sockets_m.lock();
					app_sockets.push_back(std::move(app_socket));
					app_sockets_m.unlock();
					return_app_reg(proc_id);
					app_reg_handler(proc_id, ur_id);
				}
				else if(!message_type.compare("PRIME_API_APP_DEREG")) {
					pid_t proc_id = data.get<pid_t>("proc_id");
					return_app_dereg(proc_id);
					app_dereg_handler(proc_id);
				}
				else if(!message_type.compare("PRIME_API_APP_KNOB_DISC_REG")) {
					pid_t proc_id = data.get<pid_t>("proc_id");
					prime::api::app::knob_disc_t knob;
					knob.proc_id = proc_id;
					knob.id = get_unique_knob_id();
					knob.type = (prime::api::app::knob_type_t)data.get<unsigned int>("type");
					knob.min = ((data.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : data.get<prime::api::disc_t>("min"));
					knob.max = ((data.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : data.get<prime::api::disc_t>("max"));
					knob.val = data.get<prime::api::disc_t>("val");
					knobs_disc_m.lock();
					knobs_disc.push_back(knob);
					knobs_disc_m.unlock();
					return_knob_disc_reg(knob);
					knob_disc_reg_handler(proc_id, knob);
				}
				else if(!message_type.compare("PRIME_API_APP_KNOB_CONT_REG")) {
					pid_t proc_id = data.get<pid_t>("proc_id");
					prime::api::app::knob_cont_t knob;
					knob.proc_id = proc_id;
					knob.id = get_unique_knob_id();
					knob.type = (prime::api::app::knob_type_t)data.get<unsigned int>("type");
					knob.min = ((data.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : data.get<prime::api::cont_t>("min"));
					knob.max = ((data.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : data.get<prime::api::cont_t>("max"));
					knob.val = data.get<prime::api::cont_t>("val");
					knobs_cont_m.lock();
					knobs_cont.push_back(knob);
					knobs_cont_m.unlock();
					return_knob_cont_reg(knob);
					knob_cont_reg_handler(proc_id, knob);
				}
				else if(!message_type.compare("PRIME_API_APP_KNOB_DISC_DEREG")) {
					prime::api::app::knob_disc_t knob;
					boost::property_tree::ptree knob_node = data.get_child("knob");
					knob.proc_id = knob_node.get<pid_t>("proc_id");
					knob.id = knob_node.get<unsigned int>("id");
					knob.type = (prime::api::app::knob_type_t)knob_node.get<unsigned int>("type");
					knob.val = knob_node.get<prime::api::disc_t>("val");
					knob_disc_dereg_handler(knob);
				}
				else if(!message_type.compare("PRIME_API_APP_KNOB_CONT_DEREG")) {
					prime::api::app::knob_cont_t knob;
					boost::property_tree::ptree knob_node = data.get_child("knob");
					knob.proc_id = knob_node.get<pid_t>("proc_id");
					knob.id = knob_node.get<unsigned int>("id");
					knob.type = (prime::api::app::knob_type_t)knob_node.get<unsigned int>("type");
					knob.val = knob_node.get<prime::api::cont_t>("val");
					knob_cont_dereg_handler(knob);
				}
				else if(!message_type.compare("PRIME_API_APP_KNOB_DISC_GET")) {
					prime::api::app::knob_disc_t knob;
					boost::property_tree::ptree knob_node = data.get_child("knob");
					knob.proc_id = knob_node.get<pid_t>("proc_id");
					knob.id = knob_node.get<unsigned int>("id");
					knob.type = (prime::api::app::knob_type_t)knob_node.get<unsigned int>("type");
					knob.min = ((knob_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : knob_node.get<prime::api::disc_t>("min"));
					knob.max = ((knob_node.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : knob_node.get<prime::api::disc_t>("max"));
					knob.val = knob_node.get<prime::api::disc_t>("val");
					return_knob_disc_get(knob.proc_id, knob.id);
				}
				else if(!message_type.compare("PRIME_API_APP_KNOB_CONT_GET")) {
					prime::api::app::knob_cont_t knob;
					boost::property_tree::ptree knob_node = data.get_child("knob");
					knob.proc_id = knob_node.get<pid_t>("proc_id");
					knob.id = knob_node.get<unsigned int>("id");
					knob.type = (prime::api::app::knob_type_t)knob_node.get<unsigned int>("type");
					knob.min = ((knob_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : knob_node.get<prime::api::cont_t>("min"));
					knob.max = ((knob_node.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : knob_node.get<prime::api::cont_t>("max"));
					knob.val = knob_node.get<prime::api::cont_t>("val");
					return_knob_cont_get(knob.proc_id, knob.id);
				}
				else if(!message_type.compare("PRIME_API_APP_MON_DISC_REG")) {
					pid_t proc_id = data.get<pid_t>("proc_id");
					prime::api::app::mon_disc_t mon;
					mon.proc_id = proc_id;
					mon.id = get_unique_mon_id();
					mon.type = (prime::api::app::mon_type_t)data.get<unsigned int>("type");
					mon.min = ((data.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : data.get<prime::api::disc_t>("min"));
					mon.max = ((data.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : data.get<prime::api::disc_t>("max"));
					mon.val = 0;
					mon.weight = data.get<prime::api::cont_t>("weight");
					return_mon_disc_reg(mon);
					mon_disc_reg_handler(proc_id, mon);
				}
				else if(!message_type.compare("PRIME_API_APP_MON_CONT_REG")) {
					pid_t proc_id = data.get<pid_t>("proc_id");
					prime::api::app::mon_cont_t mon;
					mon.proc_id = proc_id;
					mon.id = get_unique_mon_id();
					mon.type = (prime::api::app::mon_type_t)data.get<unsigned int>("type");
					mon.min = ((data.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : data.get<prime::api::cont_t>("min"));
					mon.max = ((data.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : data.get<prime::api::cont_t>("max"));
					mon.val = 0;
					mon.weight = data.get<prime::api::cont_t>("weight");
					return_mon_cont_reg(mon);
					mon_cont_reg_handler(proc_id, mon);
				}
				else if(!message_type.compare("PRIME_API_APP_MON_DISC_DEREG")) {
					prime::api::app::mon_disc_t mon;
					boost::property_tree::ptree mon_node = data.get_child("mon");
					mon.proc_id = mon_node.get<pid_t>("proc_id");
					mon.id = mon_node.get<unsigned int>("id");
					mon.type = (prime::api::app::mon_type_t)mon_node.get<unsigned int>("type");
					mon.min = ((mon_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : mon_node.get<prime::api::disc_t>("min"));
					mon.max = ((mon_node.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : mon_node.get<prime::api::disc_t>("max"));
					mon.val = mon_node.get<prime::api::disc_t>("val");
					mon.weight = mon_node.get<prime::api::cont_t>("weight");
					mon_disc_dereg_handler(mon);
				}
				else if(!message_type.compare("PRIME_API_APP_MON_CONT_DEREG")) {
					prime::api::app::mon_cont_t mon;
					boost::property_tree::ptree mon_node = data.get_child("mon");
					mon.proc_id = mon_node.get<pid_t>("proc_id");
					mon.id = mon_node.get<unsigned int>("id");
					mon.type = (prime::api::app::mon_type_t)mon_node.get<unsigned int>("type");
					mon.min = ((mon_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : mon_node.get<prime::api::cont_t>("min"));
					mon.max = ((mon_node.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : mon_node.get<prime::api::cont_t>("max"));
					mon.val = mon_node.get<prime::api::cont_t>("val");
					mon.weight = mon_node.get<prime::api::cont_t>("weight");
					mon_cont_dereg_handler(mon);
				}
				else {
					std::cout << "UNKNOWN MESSAGE TYPE (APP > RTM): " << message_type << std::endl;
				}
			}
			catch(std::exception &err)
			{
				std::cout << "API Violation in Message Type: " << err.what() << std::endl;
				//TODO: Send error message back to Application about Violation
				return;
			}
		}
	}

	unsigned int app_interface::get_unique_knob_id(void)
	{
		unsigned int id = next_unique_knob_id;
		next_unique_knob_id += 1;
		return id;
	}

	unsigned int app_interface::get_unique_mon_id(void)
	{
		unsigned int id = next_unique_mon_id;
		next_unique_mon_id += 1;
		return id;
	}

	void app_interface::knob_disc_set(prime::api::app::knob_disc_t knob, prime::api::disc_t val)
	{
		knobs_disc_m.lock();
		for(auto& knob_disc : knobs_disc) {
			if(knob_disc.id == knob.id) {
				knob_disc.val = val;
			}
		}
		knobs_disc_m.unlock();
	}

	void app_interface::knob_cont_set(prime::api::app::knob_cont_t knob, prime::api::cont_t val)
	{
		knobs_cont_m.unlock();
		for(auto& knob_cont : knobs_cont) {
			if(knob_cont.id == knob.id) {
				knob_cont.val = val;
			}
		}
		knobs_cont_m.unlock();
	}

	void app_interface::return_app_reg(pid_t proc_id)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_RETURN_APP_REG");
		ADD_JSON_DATA("proc_id", proc_id);
		SEND_JSON_PID(proc_id);
		prime::util::send_message(ui_socket, json_string);
	}

	void app_interface::return_app_dereg(pid_t proc_id)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_RETURN_APP_DEREG");
		ADD_JSON_DATA("proc_id", proc_id);
		SEND_JSON_PID(proc_id);
		prime::util::send_message(ui_socket, json_string);
	}

	void app_interface::return_knob_disc_get(pid_t proc_id, unsigned int id)
	{
		char type = PRIME_API_APP_RETURN_KNOB_DISC_GET;
		prime::api::disc_t val = 0;
		bool found_knob = false;

		knobs_disc_m.lock();
		for(auto knob_disc : knobs_disc) {
			if(knob_disc.id == id && knob_disc.proc_id == proc_id) {
				val = knob_disc.val;
				found_knob = true;
			}
		}
		knobs_disc_m.unlock();

		std::stringstream ss;
		ss << type << API_DELIMINATOR << id << API_DELIMINATOR << val << API_DELIMINATOR << proc_id << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

        std::shared_ptr<prime::uds> socket_ptr;
        app_sockets_m.lock();
        for(auto app_socket : app_sockets) {
            if(app_socket.first == proc_id) {
                socket_ptr = app_socket.second;
            }
        }
        app_sockets_m.unlock();
        prime::util::send_message(*socket_ptr.get(), json_string);
        if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
	}

	void app_interface::return_knob_cont_get(pid_t proc_id, unsigned int id)
	{
		char type = PRIME_API_APP_RETURN_KNOB_CONT_GET;
		prime::api::cont_t val = 0;

		knobs_cont_m.lock();
		for(auto knob_cont : knobs_cont) {
			if(knob_cont.id == id && knob_cont.proc_id == proc_id) {
				val = knob_cont.val;
			}
		}
		knobs_cont_m.unlock();

		std::stringstream ss;
		ss << type << API_DELIMINATOR << id << API_DELIMINATOR << val << API_DELIMINATOR << proc_id << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

        std::shared_ptr<prime::uds> socket_ptr;
        app_sockets_m.lock();
        for(auto app_socket : app_sockets) {
            if(app_socket.first == proc_id) {
                socket_ptr = app_socket.second;
            }
        }
        app_sockets_m.unlock();
        prime::util::send_message(*socket_ptr.get(), json_string);
        if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
	}

	void app_interface::return_knob_disc_reg(prime::api::app::knob_disc_t knob)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_RETURN_KNOB_DISC_REG");
		boost::property_tree::ptree knob_node;
		knob_node.put("proc_id", knob.proc_id);
		knob_node.put("id", knob.id);
		knob_node.put("type", knob.type);
		knob_node.put("min", knob.min);
		knob_node.put("max", knob.max);
		knob_node.put("val", knob.val);
		data_node.add_child("knob", knob_node);
		SEND_JSON_PID(knob.proc_id);
	}

	void app_interface::return_knob_cont_reg(prime::api::app::knob_cont_t knob)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_RETURN_KNOB_CONT_REG");
		boost::property_tree::ptree knob_node;
		knob_node.put("proc_id", knob.proc_id);
		knob_node.put("id", knob.id);
		knob_node.put("type", knob.type);
		knob_node.put("min", knob.min);
		knob_node.put("max", knob.max);
		knob_node.put("val", knob.val);
		data_node.add_child("knob", knob_node);
		SEND_JSON_PID(knob.proc_id);
	}

	void app_interface::return_mon_disc_reg(prime::api::app::mon_disc_t mon)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_RETURN_MON_DISC_REG");
		boost::property_tree::ptree mon_node;
		mon_node.put("proc_id", mon.proc_id);
		mon_node.put("id", mon.id);
		mon_node.put("type", mon.type);
		mon_node.put("min", mon.min);
		mon_node.put("max", mon.max);
		mon_node.put("val", mon.val);
		mon_node.put("weight", mon.weight);
		data_node.add_child("mon", mon_node);
		SEND_JSON_PID(mon.proc_id);
        prime::util::send_message(ui_socket, json_string);
	}

	void app_interface::return_mon_cont_reg(prime::api::app::mon_cont_t mon)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_RETURN_MON_CONT_REG");
		boost::property_tree::ptree mon_node;
		mon_node.put("proc_id", mon.proc_id);
		mon_node.put("id", mon.id);
		mon_node.put("type", mon.type);
		mon_node.put("min", mon.min);
		mon_node.put("max", mon.max);
		mon_node.put("val", mon.val);
		mon_node.put("weight", mon.weight);
		data_node.add_child("mon", mon_node);
		SEND_JSON_PID(mon.proc_id);
        prime::util::send_message(ui_socket, json_string);
	}


	bool dev_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs)
	{
		// Check that addresses are present.
		bool ret = false;

		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/rtm.dev.logger.uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;
			socket_addrs->remote_logger = false;
			ret = true;
		}

		if(!socket_addrs->api_addr) {
			//No API address present, use default.
			socket_addrs->api_local_address = "/tmp/rtm.dev.uds";
			socket_addrs->api_remote_address = "/tmp/dev.rtm.uds";
			socket_addrs->api_addr = true;
			socket_addrs->remote_api = false;
			ret = true;
		}

		return ret;
	}

	/* ---------------------------------- Struct Constructors --------------------------------- */
	dev_interface::dev_interface(prime::uds::socket_addrs_t *dev_addrs) :
		default_addrs(check_addrs(dev_addrs)),
		logger_en(dev_addrs->logger_en),
		socket(
			prime::uds::socket_layer_t::RTM_DEV,
			dev_addrs,
			boost::bind(&dev_interface::message_handler, this, _1)),
		logger_socket(prime::uds::socket_layer_t::LOGGER, dev_addrs)
	{
#ifdef API_DEBUG
		std::cout << "DEV Interface: Default Addrs?: " << std::to_string(default_addrs) << std::endl;
		std::cout << "\tAPI Socket: " << socket.get_endpoints() << std::endl;
		std::cout << "\t\tRequested addrs: " << dev_addrs->api_local_address << " " << dev_addrs->api_remote_address << std::endl;
		std::cout << "\tLogger Socket: " << logger_socket.get_endpoints() << std::endl;
		std::cout << "\t\tRequested addrs: " << dev_addrs->logger_local_address << " " << dev_addrs->logger_remote_address << std::endl;
#endif
	}

	/* ----------------------------------- UDS Constructors ----------------------------------- */
	/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
	dev_interface::dev_interface(bool logger_en) :
		dev_interface(
			"/tmp/rtm.dev.logger.uds",
			"/tmp/logger.uds",
			"/tmp/rtm.dev.uds",
			"/tmp/dev.rtm.uds",
			logger_en)
	{
#ifdef API_DEBUG
		std::cout << "DEV Interface:" << std::endl;
		std::cout << "\tAPI Socket: " << socket.get_endpoints() << std::endl;
		std::cout << "\tLogger Socket: " << logger_socket.get_endpoints() << std::endl;
#endif
	}

	/* UDS API, UDS Logger: Specified Logger path. */
	dev_interface::dev_interface(
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en
		) :
		dev_interface(
			logger_local_filename,
			logger_remote_filename,
			"/tmp/rtm.dev.uds",
			"/tmp/dev.rtm.uds",
			logger_en)
	{ }

	/* UDS API, UDS Logger: Specified API path. */
	dev_interface::dev_interface(
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) :
		dev_interface(
			"/tmp/rtm.dev.logger.uds",
			"/tmp/logger.uds",
			api_local_filename,
			api_remote_filename,
			logger_en)
	{ }

	/* UDS API, UDS Logger: Specified logger and API paths. */
	dev_interface::dev_interface(
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) :
		logger_en(logger_en),
		socket(
			api_local_filename,
			api_remote_filename,
			boost::bind(&dev_interface::message_handler, this, _1)),
		logger_socket(
			logger_local_filename,
			logger_remote_filename)
	{ }
	/* ---------------------------------------------------------------------------------------- */

	/* --------------------------------- Socket Constructors ---------------------------------- */
	/* Default UDS API, Network Logger address & path. */
	dev_interface::dev_interface(
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en
		) :
		dev_interface(
			logger_remote_address,
			logger_remote_port,
			"/tmp/rtm.dev.uds",
			"/tmp/dev.rtm.uds",
			logger_en)
	{ }

	/* UDS API, Network Logger: Specified API path and logger address & port. */
	dev_interface::dev_interface(
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) :
		logger_en(logger_en),
		socket(
			api_local_filename,
			api_remote_filename,
			boost::bind(&dev_interface::message_handler, this, _1)),
		logger_socket(logger_remote_address, logger_remote_port)
	{ }


	/* Network API, default UDS logger. */
	dev_interface::dev_interface(
			bool local_logger,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en
		) :
		dev_interface(
			"/tmp/rtm.dev.logger.uds",
			"/tmp/logger.uds",
			api_remote_address,
			api_remote_port,
			logger_en)
	{ }

	/* Network API, UDS Logger: Specified logger path and API port. */
	dev_interface::dev_interface(
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en
		) :
		logger_en(logger_en),
		socket(api_remote_address,
			api_remote_port,
			boost::bind(&dev_interface::message_handler, this, _1)),
		logger_socket(logger_local_filename, logger_remote_filename)
	{ }


	/* Network API & Logger. */
	dev_interface::dev_interface(
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en
		) :
		logger_en(logger_en),
		socket(api_remote_address,
			api_remote_port,
			boost::bind(&dev_interface::message_handler, this, _1)),
		logger_socket(logger_remote_address, logger_remote_port)
	{ }
	/* ---------------------------------------------------------------------------------------- */



	dev_interface::~dev_interface()
	{
	}

	void dev_interface::message_handler(std::vector<char> message)
	{
		if(message[0] != '{') { // Not json, process quickly
			std::string delim = API_DELIMINATOR;
			size_t position;
			std::string id, val, min, max, type;
			std::string message_string( (reinterpret_cast<char*>(message.data())) + 1 + delim.length());
#ifdef DEBUG
			std::string ts;
			std::cout << "Message String: " <<  message_string << std::endl;
#endif
			switch((prime::api::rtm_dev_msg_t)message[0]) {
				case PRIME_API_DEV_RETURN_MON_DISC_GET:
					// Get a monitor value, 6 fields: Type, ID, Value, min & max (plus Timestamp)
					position = message_string.find(delim);
					id = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					min = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					max = message_string.substr(0, position);
					
					disc_return_m.lock();
					disc_return = std::stoul(val);
					disc_max = std::stoul(max);
					disc_min = std::stoul(min);
					disc_return_m.unlock();

					mon_disc_get_flag = true;
					mon_disc_get_cv.notify_one();
#ifdef DEBUG
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					ts = message_string.substr(0, position);
					std::cout << "PRIME_API_DEV_RETURN_MON_DISC_GET\tts: " << ts << "\tid: " << id << "\tval: " << val << "\n" << std::endl;
#endif
					break;

				case PRIME_API_DEV_RETURN_MON_CONT_GET:
					// Get a monitor value, 6 fields: Type, ID, Value, min & max (plus Timestamp)
					position = message_string.find(delim);
					id = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					min = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					max = message_string.substr(0, position);

					cont_return_m.lock();
					cont_return = std::stof(val);
					cont_min = std::stof(min);
					cont_max = std::stof(max);
					cont_return_m.unlock();

					mon_cont_get_flag = true;
					mon_cont_get_cv.notify_one();
#ifdef DEBUG
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					ts = message_string.substr(0, position);
					std::cout << "PRIME_API_DEV_RETURN_MON_CONT_GET\tts: " << ts << "\tid: " << id << "\tval: " << val << "\n" << std::endl;
#endif
					break;

				default:
#ifdef DEBUG
					std::cout << "Error: unknown message: " <<  message_string << std::endl;
#endif
					break;
			}

		} else {	// This is json, send it to the slow lane.
			std::string message_string(reinterpret_cast<char*>(message.data()));		// This is 4ms+ faster than the two lines below!
			//std::string message_string(message.begin(), message.end());
			//boost::trim_right_if(message_string, boost::is_any_of(std::string("\0", 1)));

			std::stringstream ss;
			ss << message_string;
			boost::property_tree::ptree root;
			boost::property_tree::read_json(ss, root);

			std::string message_type(root.get<std::string>("type"));

			if(!message_type.compare("PRIME_API_DEV_RETURN_KNOB_DISC_SIZE")) {
				boost::property_tree::ptree data = root.get_child("data");
				unsigned_int_return_m.lock();
				unsigned_int_return = data.get<unsigned int>("");
				unsigned_int_return_m.unlock();
				knob_disc_size_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_KNOB_CONT_SIZE")) {
				boost::property_tree::ptree data = root.get_child("data");
				unsigned_int_return_m.lock();
				unsigned_int_return = data.get<unsigned int>("");
				unsigned_int_return_m.unlock();
				knob_cont_size_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_KNOB_DISC_REG")) {
				boost::property_tree::ptree data = root.get_child("data");
				for (const auto& pair : data.get_child("")) {
					boost::property_tree::ptree knob_node = pair.second;
					prime::api::dev::knob_disc_t knob;
					knob.id = knob_node.get<unsigned int>("id");
					knob.type = (prime::api::dev::knob_type_t)knob_node.get<unsigned int>("type");
					knob.min = ((knob_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : knob_node.get<prime::api::disc_t>("min"));
					knob.max = ((knob_node.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : knob_node.get<prime::api::disc_t>("max"));
					knob.val = knob_node.get<prime::api::disc_t>("val");
					knob.init = knob_node.get<prime::api::disc_t>("init");
					knobs_disc_m.lock();
					knobs_disc.push_back(knob);
					knobs_disc_m.unlock();
				}
				knob_disc_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_KNOB_CONT_REG")) {
				boost::property_tree::ptree data = root.get_child("data");
				for (const auto& pair : data.get_child("")) {
					boost::property_tree::ptree knob_node = pair.second;
					prime::api::dev::knob_cont_t knob;
					knob.id = knob_node.get<unsigned int>("id");
					knob.type = (prime::api::dev::knob_type_t)knob_node.get<unsigned int>("type");
					knob.min = ((knob_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : knob_node.get<prime::api::cont_t>("min"));
					knob.max = ((knob_node.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : knob_node.get<prime::api::cont_t>("max"));
					knob.val = knob_node.get<prime::api::cont_t>("val");
					knob.init = knob_node.get<prime::api::cont_t>("init");
					knobs_cont_m.lock();
					knobs_cont.push_back(knob);
					knobs_cont_m.unlock();
				}
				knob_cont_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_MON_DISC_SIZE")) {
				boost::property_tree::ptree data = root.get_child("data");
				unsigned_int_return_m.lock();
				unsigned_int_return = data.get<unsigned int>("");
				unsigned_int_return_m.unlock();
				mon_disc_size_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_MON_CONT_SIZE")) {
				boost::property_tree::ptree data = root.get_child("data");
				unsigned_int_return_m.lock();
				unsigned_int_return = data.get<unsigned int>("");
				unsigned_int_return_m.unlock();
				mon_cont_size_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_MON_DISC_REG")) {
				boost::property_tree::ptree data = root.get_child("data");
				for (const auto& pair : data.get_child("")) {
					boost::property_tree::ptree mon_node = pair.second;
					prime::api::dev::mon_disc_t mon;
					mon.id = mon_node.get<unsigned int>("id");
					mon.type = (prime::api::dev::mon_type_t)mon_node.get<unsigned int>("type");
					mon.val = mon_node.get<prime::api::disc_t>("val");
					mon.min = mon_node.get<prime::api::disc_t>("min");
					mon.max = mon_node.get<prime::api::disc_t>("max");
					mons_disc_m.lock();
					mons_disc.push_back(mon);
					mons_disc_m.unlock();
				}
				mon_disc_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_MON_CONT_REG")) {
				boost::property_tree::ptree data = root.get_child("data");
				for (const auto& pair : data.get_child("")) {
					boost::property_tree::ptree mon_node = pair.second;
					prime::api::dev::mon_cont_t mon;
					mon.id = mon_node.get<unsigned int>("id");
					mon.type = (prime::api::dev::mon_type_t)mon_node.get<unsigned int>("type");
					mon.val = mon_node.get<prime::api::cont_t>("val");
					mon.min = mon_node.get<prime::api::cont_t>("min");
					mon.max = mon_node.get<prime::api::cont_t>("max");
					mons_cont_m.lock();
					mons_cont.push_back(mon);
					mons_cont_m.unlock();
				}
				mon_cont_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_MON_DISC_GET")) {
				disc_return_m.lock();
				disc_return = root.get<prime::api::disc_t>("val");
				disc_return_m.unlock();
				mon_disc_get_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_MON_CONT_GET")) {
				cont_return_m.lock();
				cont_return = root.get<prime::api::cont_t>("val");
				cont_return_m.unlock();
				mon_cont_get_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_DEV_RETURN_ARCH_GET")) {
				boost::property_tree::ptree data = root.get_child("data");
				dev_arch_return_m.lock();
				boost::property_tree::read_json(data.get<std::string>(""), dev_architecture);
				dev_arch_return_m.unlock();
				dev_arch_get_cv.notify_one();
			}
			else {
				std::cout << "UNKNOWN MESSAGE TYPE (DEV > RTM): " << message_type << std::endl;
			}
		}
	}

	unsigned int dev_interface::knob_disc_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_KNOB_DISC_SIZE");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(knob_disc_size_m);
		knob_disc_size_cv.wait(lock);

		unsigned_int_return_m.lock();
		auto result = unsigned_int_return;
		unsigned_int_return_m.unlock();

		return result;
	}

	unsigned int dev_interface::knob_cont_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_KNOB_CONT_SIZE");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(knob_cont_size_m);
		knob_cont_size_cv.wait(lock);

		unsigned_int_return_m.lock();
		auto result = unsigned_int_return;
		unsigned_int_return_m.unlock();

		return result;
	 }

	std::vector<prime::api::dev::knob_disc_t> dev_interface::knob_disc_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_KNOB_DISC_REG");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(knob_disc_reg_m);
		knob_disc_reg_cv.wait(lock);

		knobs_disc_m.lock();
		auto result = knobs_disc;
		knobs_disc.clear();
		knobs_disc_m.unlock();

		return result;
	}

	std::vector<prime::api::dev::knob_cont_t> dev_interface::knob_cont_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_KNOB_CONT_REG");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(knob_cont_reg_m);
		knob_cont_reg_cv.wait(lock);

		knobs_cont_m.lock();
		auto result = knobs_cont;
		knobs_cont.clear();
		knobs_cont_m.unlock();

		return result;
	}

	prime::api::dev::knob_type_t dev_interface::knob_disc_type(prime::api::dev::knob_disc_t knob)
	{
		return knob.type;
	}

	prime::api::disc_t dev_interface::knob_disc_min(prime::api::dev::knob_disc_t knob)
	{
		return knob.min;
	}

	prime::api::disc_t dev_interface::knob_disc_max(prime::api::dev::knob_disc_t knob)
	{
		return knob.max;
	}

	prime::api::disc_t dev_interface::knob_disc_init(prime::api::dev::knob_disc_t knob)
	{
		return knob.init;
	}

	prime::api::dev::knob_type_t dev_interface::knob_cont_type(prime::api::dev::knob_cont_t knob)
	{
		return knob.type;
	}

	prime::api::cont_t dev_interface::knob_cont_min(prime::api::dev::knob_cont_t knob)
	{
		return knob.min;
	}

	prime::api::cont_t dev_interface::knob_cont_max(prime::api::dev::knob_cont_t knob)
	{
		return knob.max;
	}

	prime::api::cont_t dev_interface::knob_cont_init(prime::api::dev::knob_cont_t knob)
	{
		return knob.init;
	}

	void dev_interface::knob_disc_set(prime::api::dev::knob_disc_t knob, prime::api::disc_t val)
	{
		char type = (rtm_dev_msg_t)PRIME_API_DEV_KNOB_DISC_SET;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << knob.id << API_DELIMINATOR << val << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		prime::util::send_message(logger_socket, json_string);
	}

	void dev_interface::knob_cont_set(prime::api::dev::knob_cont_t knob, prime::api::cont_t val)
	{
		char type = (rtm_dev_msg_t)PRIME_API_DEV_KNOB_CONT_SET;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << knob.id << API_DELIMINATOR << val << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		prime::util::send_message(logger_socket, json_string);
	}

	void dev_interface::knob_disc_dereg(std::vector<prime::api::dev::knob_disc_t>& knobs)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_KNOB_DISC_DEREG");
		boost::property_tree::ptree knobs_node;
		for(auto knob : knobs) {
			boost::property_tree::ptree knob_node;
			knob_node.put("id", knob.id);
			knob_node.put("type", knob.type);
			knobs_node.push_back(std::make_pair("", knob_node));
		}
		data_node.add_child("knobs", knobs_node);
		SEND_JSON();
	}

	void dev_interface::knob_cont_dereg(std::vector<prime::api::dev::knob_cont_t>& knobs)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_KNOB_CONT_DEREG");
		boost::property_tree::ptree knobs_node;
		for(auto knob : knobs) {
			boost::property_tree::ptree knob_node;
			knob_node.put("id", knob.id);
			knob_node.put("type", knob.type);
			knobs_node.push_back(std::make_pair("", knob_node));
		}
		data_node.add_child("knobs", knobs_node);
		SEND_JSON();
	}

	unsigned int dev_interface::mon_disc_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_MON_DISC_SIZE");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(mon_disc_size_m);
		mon_disc_size_cv.wait(lock);

		unsigned_int_return_m.lock();
		auto result = unsigned_int_return;
		unsigned_int_return_m.unlock();

		return result;
	}

	unsigned int dev_interface::mon_cont_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_MON_CONT_SIZE");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(mon_cont_size_m);
		mon_cont_size_cv.wait(lock);

		unsigned_int_return_m.lock();
		auto result = unsigned_int_return;
		unsigned_int_return_m.unlock();

		return result;
	}

	std::vector<prime::api::dev::mon_disc_t> dev_interface::mon_disc_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_MON_DISC_REG");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(mon_disc_reg_m);
		mon_disc_reg_cv.wait(lock);

		mons_disc_m.lock();
		auto result = mons_disc;
		mons_disc.clear();
		mons_disc_m.unlock();

		return result;
	}

	std::vector<prime::api::dev::mon_cont_t> dev_interface::mon_cont_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_MON_CONT_REG");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(mon_cont_reg_m);
		mon_cont_reg_cv.wait(lock);

		mons_cont_m.lock();
		auto result = mons_cont;
		mons_cont.clear();
		mons_cont_m.unlock();

		return result;
	}

	prime::api::dev::mon_type_t dev_interface::mon_disc_type(prime::api::dev::mon_disc_t mon)
	{
		return mon.type;
	}

	prime::api::dev::mon_type_t dev_interface::mon_cont_type(prime::api::dev::mon_cont_t mon)
	{
		return mon.type;
	}

	prime::api::disc_t dev_interface::mon_disc_get(prime::api::dev::mon_disc_t& mon)
	{
		char type = (rtm_dev_msg_t)PRIME_API_DEV_MON_DISC_GET;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		mon_disc_get_flag = false;
		prime::util::send_message(socket, json_string);
		prime::util::send_message(logger_socket, json_string);

		std::unique_lock<std::mutex> lock(mon_disc_get_m);
		while(!mon_disc_get_flag){
			mon_disc_get_cv.wait(lock);
		}

		disc_return_m.lock();
		auto result = disc_return;
		mon.val = disc_return;
		mon.min = disc_min;
		mon.max = disc_max;
		disc_return_m.unlock();

		return result;
	}

	prime::api::cont_t dev_interface::mon_cont_get(prime::api::dev::mon_cont_t& mon)
	{
		char type = (rtm_dev_msg_t)PRIME_API_DEV_MON_CONT_GET;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		mon_cont_get_flag = false;
		prime::util::send_message(socket, json_string);
		prime::util::send_message(logger_socket, json_string);

		std::unique_lock<std::mutex> lock(mon_cont_get_m);
		while(!mon_cont_get_flag){
			mon_cont_get_cv.wait(lock);
		}

		cont_return_m.lock();
		auto result = cont_return;
		mon.val = cont_return;
		mon.min = cont_min;
		mon.max = cont_max;
		cont_return_m.unlock();

		return result;
	}

	void dev_interface::mon_disc_dereg(std::vector<prime::api::dev::mon_disc_t>& mons)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_MON_DISC_DEREG");
		boost::property_tree::ptree mon_list;
		for(auto mon : mons) {
			boost::property_tree::ptree mon_node;
			mon_node.put("id", mon.id);
			mon_node.put("type", mon.type);
			mon_list.push_back(std::make_pair("", mon_node));
		}
		data_node.add_child("mons", mon_list);
		SEND_JSON();
	}

	void dev_interface::mon_cont_dereg(std::vector<prime::api::dev::mon_cont_t>& mons)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_MON_CONT_DEREG");
		boost::property_tree::ptree mon_list;
		for(auto mon : mons) {
			boost::property_tree::ptree mon_node;
			mon_node.put("id", mon.id);
			mon_node.put("type", mon.type);
			mon_list.push_back(std::make_pair("", mon_node));
		}
		data_node.add_child("mons", mon_list);
		SEND_JSON();
	}

	boost::property_tree::ptree dev_interface::dev_arch_get(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_ARCH_GET");
		SEND_JSON();

		std::unique_lock<std::mutex> lock(dev_arch_get_m);
		dev_arch_get_cv.wait(lock);

		dev_arch_return_m.lock();
		auto result = dev_architecture;
		dev_arch_return_m.unlock();

		return result;
	}


	bool ui_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs)
	{
		// Check that addresses are present.
		bool ret = false;

		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/dev.ui.logger.uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;
			ret = true;
		}

		if(!socket_addrs->ui_addr) {
			//No UI address present, use default.
			socket_addrs->ui_local_address = "/tmp/rtm.ui.uds";
			socket_addrs->ui_remote_address = "/tmp/ui.uds";
			socket_addrs->ui_addr = true;
			ret = true;
		}

		return ret;
	}

	/* ---------------------------------- Struct Constructors --------------------------------- */
	ui_interface::ui_interface(
		boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
		boost::function<void(void)> rtm_stop_handler,
		prime::uds::socket_addrs_t *ui_addrs
		) :
		default_addrs(check_addrs(ui_addrs)),
		logger_en(ui_addrs->logger_en),
		socket(
			prime::uds::socket_layer_t::UI,
			ui_addrs,
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(prime::uds::socket_layer_t::LOGGER, ui_addrs),
		app_weight_handler(app_weight_handler),
		rtm_stop_handler(rtm_stop_handler)
	{
#ifdef API_DEBUG
		std::cout << "UI Interface:" << std::endl;
		std::cout << "\tUI Socket: " << socket.get_endpoints() << std::endl;
		std::cout << "\tLogger Socket: " << logger_socket.get_endpoints() << std::endl;
#endif
	}


	/* ----------------------------------- UDS Constructors ----------------------------------- */
	ui_interface::ui_interface(
		boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
		boost::function<void(void)> rtm_stop_handler,
		bool logger_en
		) :
		ui_interface(app_weight_handler,
			rtm_stop_handler,
			"/tmp/dev.ui.logger.uds",
			"/tmp/logger.uds",
			logger_en)
	{ }


	ui_interface::ui_interface(
		boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
		boost::function<void(void)> rtm_stop_handler,
		std::string logger_local_filename,
		std::string logger_remote_filename,
		bool logger_en
		) :
		socket(
			std::string("/tmp/rtm.ui.uds"),
			std::string("/tmp/ui.uds"),
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(logger_local_filename, logger_remote_filename),
		app_weight_handler(app_weight_handler),
		rtm_stop_handler(rtm_stop_handler),
		logger_en(logger_en)
	{ }
	/* ---------------------------------------------------------------------------------------- */

	/* --------------------------------- Socket Constructors ---------------------------------- */
	ui_interface::ui_interface(
		boost::function<void(pid_t, prime::api::cont_t)> app_weight_handler,
		boost::function<void(void)> rtm_stop_handler,
		std::string logger_remote_address,
		unsigned int logger_remote_port,
		bool logger_en
		) :
		socket(
			std::string("/tmp/rtm.ui.uds"),
			std::string("/tmp/ui.uds"),
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(logger_remote_address, logger_remote_port),
		app_weight_handler(app_weight_handler),
		rtm_stop_handler(rtm_stop_handler),
		logger_en(logger_en)
	{ }
	/* ---------------------------------------------------------------------------------------- */

	ui_interface::~ui_interface()
	{
	}

	void ui_interface::message_handler(std::vector<char> message)
	{
		std::string message_string(reinterpret_cast<char*>(message.data()));		// This is 4ms+ faster than the two lines below!
		//std::string message_string(message.begin(), message.end());
		//boost::trim_right_if(message_string, boost::is_any_of(std::string("\0", 1)));
		std::stringstream ss;
		ss << message_string;
		boost::property_tree::ptree root;
		boost::property_tree::read_json(ss, root);

		std::string message_type(root.get<std::string>("type"));
		boost::property_tree::ptree data = root.get_child("data");

		if(!message_type.compare("PRIME_UI_RTM_STOP")) {
			rtm_stop_handler();
		}
		else if(!message_type.compare("PRIME_UI_APP_WEIGHT")) {
			app_weight_handler(data.get<pid_t>("proc_id"), data.get<prime::api::cont_t>("weight"));
		}
		else {
			std::cout << "UNKNOWN MESSAGE TYPE (UI > RTM): " << message_type << std::endl;
		}
	}

	void ui_interface::return_ui_rtm_start(void)
	{
		CREATE_JSON_ROOT("PRIME_UI_RTM_RETURN_RTM_START");
		SEND_JSON();
	}

	void ui_interface::return_ui_rtm_stop(void)
	{
		CREATE_JSON_ROOT("PRIME_UI_RTM_RETURN_RTM_STOP");
		SEND_JSON();
	}

	void ui_interface::ui_rtm_error(std::string message)
	{
		CREATE_JSON_ROOT("PRIME_UI_RTM_ERROR");
		ADD_JSON_DATA("msg", message);
		SEND_JSON();
	}

} } }
