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

#include "prime_api_app.h"
#include "uds.h"
#include "util.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <vector>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <iomanip>

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
    std::stringstream ss; \
    write_json(ss, root); \
    std::string json_string = ss.str(); \
    prime::util::send_message(socket, json_string); \
    if(logger_en) {	prime::util::send_message(logger_socket, json_string); }

#define SEND_JSON_DEBUG() \
    root.add_child("data", data_node); \
    { std::stringstream ss; \
    write_json(ss, root); \
    std::string json_string = ss.str(); \
    std::cout << json_string << std::endl; \
    prime::util::send_message(socket, json_string); \
    if(logger_en) {	prime::util::send_message(logger_socket, json_string); } }

namespace prime { namespace api { namespace app
{
	bool rtm_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs, pid_t proc_id)
	{
		// Check that addresses are present.
		bool ret = false;
		
		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/app.rtm.logger." + std::to_string(proc_id) + ".uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;	
			socket_addrs->remote_logger = false;	
			ret = true;
		}
		
		if(!socket_addrs->api_addr) {
			//No API address present, use default.
			socket_addrs->api_local_address = "/tmp/app.rtm." + std::to_string(proc_id) + ".uds";
			socket_addrs->api_remote_address = "/tmp/rtm.app.uds";
			socket_addrs->api_addr = true;		
			socket_addrs->remote_api = false;	
			ret = true;
		}
		
		if(!socket_addrs->ui_addr) {
			//No UI address present, use default.
			socket_addrs->ui_local_address = "/tmp/app.ui." + std::to_string(proc_id) + ".uds";
			socket_addrs->ui_remote_address = "/tmp/ui.uds";
			socket_addrs->ui_addr = true;	
			socket_addrs->remote_ui = false;		
			ret = true;
		}
			
		return ret;
	}
	
	bool ui_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs, pid_t proc_id)
	{
		// Check that addresses are present.
		bool ret = false;
		
		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/app.ui.logger." + std::to_string(proc_id) + ".uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;	
			socket_addrs->remote_logger = false;	
			ret = true;
		}
		
		if(!socket_addrs->ui_addr) {
			//No UI address present, use default.
			socket_addrs->ui_local_address = "/tmp/app.ui." + std::to_string(proc_id) + ".uds";
			socket_addrs->ui_remote_address = "/tmp/ui.uds";
			socket_addrs->ui_addr = true;	
			socket_addrs->remote_ui = false;		
			ret = true;
		}
			
		return ret;
	}
	
	/* ---------------------------------- Struct Constructors --------------------------------- */
	rtm_interface::rtm_interface(pid_t proc_id, prime::uds::socket_addrs_t *app_addrs) :
		default_addrs(check_addrs(app_addrs, proc_id)),
		proc_id(proc_id),
		logger_en(app_addrs->logger_en),
		socket(  
			prime::uds::socket_layer_t::APP_RTM,
			app_addrs,
			boost::bind(&rtm_interface::message_handler, this, _1)),
		ui_socket(prime::uds::socket_layer_t::UI, app_addrs),	
		logger_socket(prime::uds::socket_layer_t::LOGGER, app_addrs)
	{
#ifdef API_DEBUG	
		std::cout << "RTM Interface: Default Addrs?: " << std::to_string(default_addrs) << std::endl;
		std::cout << "\tAPI Socket: " << socket.get_endpoints() << std::endl;
		std::cout << "\t\tRequested addrs: " << app_addrs->api_local_address << " " << app_addrs->api_remote_address << std::endl;
		std::cout << "\tUI Socket: " << ui_socket.get_endpoints() << std::endl;
		std::cout << "\t\tRequested addrs: " << app_addrs->ui_local_address << " " << app_addrs->ui_remote_address << std::endl;
		std::cout << "\tLogger Socket: " << logger_socket.get_endpoints() << std::endl;
		std::cout << "\t\tRequested addrs: " << app_addrs->logger_local_address << " " << app_addrs->logger_remote_address << std::endl;
#endif		
	}
	
	/* ----------------------------------- UDS Constructors ----------------------------------- */
	/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
	rtm_interface::rtm_interface(pid_t proc_id, bool logger_en) :
		rtm_interface(proc_id,
			"/tmp/app.rtm.logger." + std::to_string(proc_id) + ".uds",
			"/tmp/logger.uds",
			"/tmp/app.rtm." + std::to_string(proc_id) + ".uds",
			"/tmp/rtm.app.uds",
			logger_en)
	{ }
	
	/* UDS API, UDS Logger: Specified Logger path. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en
		) :
		rtm_interface(proc_id,
			logger_local_filename,
			logger_remote_filename,
			std::string("/tmp/app.rtm.") + std::to_string(proc_id) + std::string(".uds"),
			std::string("/tmp/rtm.app.uds"),
			logger_en)
	{ }
	
	/* UDS API, UDS Logger: Specified API path. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) : 
		rtm_interface(proc_id, 
			std::string("/tmp/app.rtm.logger.") + std::to_string(::getpid()) + std::string(".uds"),
			std::string("/tmp/logger.uds"),
			api_local_filename,
			api_remote_filename,
			logger_en)
	{ }
	
	/* UDS API, UDS Logger: Specified logger and API paths. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) : 
		proc_id(proc_id),
		logger_en(logger_en),
		socket(  
			api_local_filename,
			api_remote_filename,
			boost::bind(&rtm_interface::message_handler, this, _1)),
		ui_socket(
			std::string("/tmp/app.ui.") + std::to_string(::getpid()) + std::string(".uds"),
			std::string("/tmp/ui.uds")),	
		logger_socket(
			logger_local_filename,
			logger_remote_filename)
	{ }
	/* ---------------------------------------------------------------------------------------- */
	
	/* --------------------------------- Socket Constructors ---------------------------------- */
	/* Default UDS API, Network Logger address & path. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en
		) :
		rtm_interface(proc_id,
			logger_remote_address,
			logger_remote_port,
			std::string("/tmp/app.rtm.") + std::to_string(proc_id) + std::string(".uds"),
			std::string("/tmp/rtm.app.uds"),
			logger_en)
	{ }
		
	/* UDS API, Network Logger: Specified API path and logger address & port. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) : 
		proc_id(proc_id),
		logger_en(logger_en),
		socket(  
			api_local_filename,
			api_remote_filename,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		ui_socket(
			std::string("/tmp/app.ui.") + std::to_string(::getpid()) + std::string(".uds"),
			std::string("/tmp/ui.uds")),
		logger_socket(logger_remote_address, logger_remote_port)
	{ }
	
	
	/* Network API, default UDS logger. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			bool local_logger,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en
		) :
		rtm_interface(proc_id,
			std::string("/tmp/app.rtm.logger.") + std::to_string(::getpid()) + std::string(".uds"),
			std::string("/tmp/logger.uds"),
			api_remote_address,
			api_remote_port,
			logger_en)
	{ }
		
	/* Network API, UDS Logger: Specified logger path and API port. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en
		) :
		proc_id(proc_id),
		logger_en(logger_en),
		socket(api_remote_address,
			api_remote_port,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		ui_socket(
			std::string("/tmp/app.ui.") + std::to_string(::getpid()) + std::string(".uds"),
			std::string("/tmp/ui.uds")),
		logger_socket(logger_local_filename, logger_remote_filename)
	{ }
	
	
	/* Network API & Logger. */
	rtm_interface::rtm_interface(
			pid_t proc_id, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_remote_address,
			unsigned int api_remote_port,
			bool logger_en
		) :
		proc_id(proc_id),
		logger_en(logger_en),
		socket(api_remote_address,
			api_remote_port,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		ui_socket(
			std::string("/tmp/app.ui.") + std::to_string(::getpid()) + std::string(".uds"),
			std::string("/tmp/ui.uds")),
		logger_socket(logger_remote_address, logger_remote_port)
	{ }
	/* ---------------------------------------------------------------------------------------- */

	rtm_interface::~rtm_interface()
	{
	}

	void rtm_interface::message_handler(std::vector<char> message)
	{
		if(message[0] != '{') { // Not json, process quickly
			std::string delim = API_DELIMINATOR;
			size_t position;
			std::string val;
			std::string message_string( (reinterpret_cast<char*>(message.data())) + 1 + delim.length());

#ifdef DEBUG
			std::cout << "Message String: " << message[0] << API_DELIMINATOR << message_string << std::endl;
#endif
			switch((prime::api::rtm_app_msg_t)message[0]) {

				case PRIME_API_APP_RETURN_KNOB_DISC_GET:
					// 4 fields, only care about the 3rd (val) field, 1st field already discarded.
					position = message_string.find(delim);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					disc_return_m.lock();
					disc_return = std::stoul(val);
					disc_return_m.unlock();

					knob_disc_get_flag = true;
					knob_disc_get_cv.notify_one();

					break;

				case PRIME_API_APP_RETURN_KNOB_CONT_GET:
					// 4 fields, only care about the 3rd (val) field, 1st field already discarded.
					position = message_string.find(delim);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);

					cont_return_m.lock();
					cont_return = std::stof(val);
					cont_return_m.unlock();

					knob_cont_get_flag = true;
					knob_cont_get_cv.notify_one();
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

			if(!message_type.compare("PRIME_API_APP_RETURN_APP_REG")) {
				socket.set_remote_endpoint(std::string("/tmp/rtm.app.") + std::to_string(proc_id) + std::string(".uds"));
				app_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_APP_RETURN_APP_DEREG")) {
				socket.set_remote_endpoint(std::string("/tmp/rtm.app.uds"));
				app_dereg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_APP_RETURN_KNOB_DISC_REG")) {
				knob_disc_return_m.lock();
				boost::property_tree::ptree knob_node = data.get_child("knob");
				knob_disc_return.proc_id = knob_node.get<pid_t>("proc_id");
				knob_disc_return.id = knob_node.get<unsigned int>("id");
				knob_disc_return.type = (prime::api::app::knob_type_t)knob_node.get<unsigned int>("type");
				knob_disc_return.min = ((knob_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : knob_node.get<prime::api::disc_t>("min"));
				knob_disc_return.max = ((knob_node.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : knob_node.get<prime::api::disc_t>("max"));
				knob_disc_return.val = knob_node.get<prime::api::disc_t>("val");
				knob_disc_return_m.unlock();
				knob_disc_reg_cv.notify_one();
				}
			else if(!message_type.compare("PRIME_API_APP_RETURN_KNOB_CONT_REG")) {
				knob_cont_return_m.lock();
				boost::property_tree::ptree knob_node = data.get_child("knob");
				knob_cont_return.proc_id = knob_node.get<pid_t>("proc_id");
				knob_cont_return.id = knob_node.get<unsigned int>("id");
				knob_cont_return.type = (prime::api::app::knob_type_t)knob_node.get<unsigned int>("type");
				knob_cont_return.min = ((knob_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : knob_node.get<prime::api::cont_t>("min"));
				knob_cont_return.max = ((knob_node.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : knob_node.get<prime::api::cont_t>("max"));
				knob_cont_return.val = knob_node.get<prime::api::cont_t>("val");
				knob_cont_return_m.unlock();
				knob_cont_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_APP_RETURN_KNOB_DISC_GET")) {
				disc_return_m.lock();
				disc_return = data.get<prime::api::disc_t>("");
				disc_return_m.unlock();
				knob_disc_get_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_APP_RETURN_KNOB_CONT_GET")) {
				cont_return_m.lock();
				cont_return = data.get<prime::api::cont_t>("");
				cont_return_m.unlock();
				knob_cont_get_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_APP_RETURN_MON_DISC_REG")) {
				mon_disc_return_m.lock();
				boost::property_tree::ptree mon_node = data.get_child("mon");
				mon_disc_return.proc_id = mon_node.get<pid_t>("proc_id");
				mon_disc_return.id = mon_node.get<unsigned int>("id");
				mon_disc_return.type = (prime::api::app::mon_type_t)mon_node.get<unsigned int>("type");
				mon_disc_return.min = ((mon_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_DISC_MIN : mon_node.get<prime::api::disc_t>("min"));
				mon_disc_return.max = ((mon_node.get<std::string>("max") == "inf") ? prime::api::PRIME_DISC_MAX : mon_node.get<prime::api::disc_t>("max"));
				mon_disc_return.val = mon_node.get<prime::api::disc_t>("val");
				mon_disc_return.weight = mon_node.get<prime::api::cont_t>("weight");
				mon_disc_return_m.unlock();
				mon_disc_reg_cv.notify_one();
			}
			else if(!message_type.compare("PRIME_API_APP_RETURN_MON_CONT_REG")) {
				mon_cont_return_m.lock();
				boost::property_tree::ptree mon_node = data.get_child("mon");
				mon_cont_return.proc_id = mon_node.get<pid_t>("proc_id");
				mon_cont_return.id = mon_node.get<unsigned int>("id");
				mon_cont_return.type = (prime::api::app::mon_type_t)mon_node.get<unsigned int>("type");
				mon_cont_return.min = ((mon_node.get<std::string>("min") == "-inf") ? prime::api::PRIME_CONT_MIN : mon_node.get<prime::api::cont_t>("min"));
				mon_cont_return.max = ((mon_node.get<std::string>("max") == "inf") ? prime::api::PRIME_CONT_MAX : mon_node.get<prime::api::cont_t>("max"));
				mon_cont_return.val = mon_node.get<prime::api::cont_t>("val");
				mon_cont_return.weight = mon_node.get<prime::api::cont_t>("weight");
				mon_cont_return_m.unlock();
				mon_cont_reg_cv.notify_one();
			}
			else {
				std::cout << "UNKNOWN MESSAGE TYPE" << std::endl;
			}
		}
	}

	void rtm_interface::reg(pid_t proc_id, unsigned long int ur_id)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_REG");
		ADD_JSON_DATA("proc_id", proc_id);
		ADD_JSON_DATA("ur_id", ur_id);
		SEND_JSON();
		prime::util::send_message(ui_socket, json_string);

		std::unique_lock<std::mutex> lock(app_reg_m);
		app_reg_cv.wait(lock);
	}

	void rtm_interface::dereg(pid_t proc_id)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_DEREG");
		ADD_JSON_DATA("proc_id", proc_id);
		SEND_JSON();
		prime::util::send_message(ui_socket, json_string);

		std::unique_lock<std::mutex> lock(app_dereg_m);
		app_dereg_cv.wait(lock);
	}

	knob_disc_t rtm_interface::knob_disc_reg(knob_type_t type, prime::api::disc_t min, prime::api::disc_t max, prime::api::disc_t val)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_KNOB_DISC_REG");
		ADD_JSON_DATA("proc_id", ::getpid());
		ADD_JSON_DATA("type", type);
		ADD_JSON_DATA("min", min);
		ADD_JSON_DATA("max", max);
		ADD_JSON_DATA("val", val);
		SEND_JSON();

		std::unique_lock<std::mutex> lock(knob_disc_reg_m);
		knob_disc_reg_cv.wait(lock);

		knob_disc_return_m.lock();
		auto result = knob_disc_return;
		knob_disc_return_m.unlock();

		return result;
	}

	knob_cont_t rtm_interface::knob_cont_reg(knob_type_t type, prime::api::cont_t min, prime::api::cont_t max, prime::api::cont_t val)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_KNOB_CONT_REG");
		ADD_JSON_DATA("proc_id", ::getpid());
		ADD_JSON_DATA("type", type);
		ADD_JSON_DATA("min", min);
		ADD_JSON_DATA("max", max);
		ADD_JSON_DATA("val", val);
		SEND_JSON();

		std::unique_lock<std::mutex> lock(knob_cont_reg_m);
		knob_cont_reg_cv.wait(lock);

		knob_cont_return_m.lock();
		auto result = knob_cont_return;
		knob_cont_return_m.unlock();

		return result;
	}

	void rtm_interface::knob_disc_min(knob_disc_t knob, prime::api::disc_t min)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_KNOB_DISC_MIN;

		std::stringstream ss;

		ss << type << API_DELIMINATOR << std::to_string((unsigned int)knob.id) << API_DELIMINATOR << std::to_string((unsigned int)min) << API_DELIMINATOR << std::to_string((unsigned int)knob.proc_id) << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		return;
	}

	void rtm_interface::knob_disc_max(knob_disc_t knob, prime::api::disc_t max)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_KNOB_DISC_MAX;

		std::stringstream ss;

		ss << type << API_DELIMINATOR << std::to_string((unsigned int)knob.id) << API_DELIMINATOR << std::to_string((unsigned int)max) << API_DELIMINATOR << std::to_string((unsigned int)knob.proc_id) << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		return;
	}

	void rtm_interface::knob_cont_min(knob_cont_t knob, prime::api::cont_t min)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_KNOB_CONT_MIN;

		std::stringstream ss;

		ss << type << API_DELIMINATOR << std::to_string((unsigned int)knob.id) << API_DELIMINATOR << std::to_string((float)min) << API_DELIMINATOR << std::to_string((unsigned int)knob.proc_id) << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		
		return;
	}

	void rtm_interface::knob_cont_max(knob_cont_t knob, prime::api::cont_t max)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_KNOB_CONT_MAX;

		std::stringstream ss;

		ss << type << API_DELIMINATOR << std::to_string((unsigned int)knob.id) << API_DELIMINATOR << std::to_string((float)max) << API_DELIMINATOR << std::to_string((unsigned int)knob.proc_id) << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		return;
	}

	prime::api::disc_t rtm_interface::knob_disc_get(knob_disc_t knob)
	{
		// Four fields: Type, ID, PID, TS
		char type = PRIME_API_APP_KNOB_DISC_GET;

		std::stringstream ss;

		ss << type << API_DELIMINATOR << knob.id << API_DELIMINATOR << knob.proc_id << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		knob_disc_get_flag = false;
		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		std::unique_lock<std::mutex> lock(knob_disc_get_m);
		while(!knob_disc_get_flag){
			knob_disc_get_cv.wait(lock);
		}

		disc_return_m.lock();
		auto result = disc_return;
		disc_return_m.unlock();

		return result;
	}

	prime::api::cont_t rtm_interface::knob_cont_get(knob_cont_t knob)
	{
		// Four fields: Type, ID, PID, TS
		char type = PRIME_API_APP_KNOB_CONT_GET;

		std::stringstream ss;

		ss << type << API_DELIMINATOR << knob.id << API_DELIMINATOR << knob.proc_id << API_DELIMINATOR << prime::util::get_timestamp();
		std::string json_string = ss.str();

		knob_cont_get_flag = false;
		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		std::unique_lock<std::mutex> lock(knob_cont_get_m);
		while(!knob_cont_get_flag){
			knob_cont_get_cv.wait(lock);
		}

		cont_return_m.lock();
		auto result = cont_return;
		cont_return_m.unlock();

		return result;
	}

	void rtm_interface::knob_disc_dereg(knob_disc_t knob)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_KNOB_DISC_DEREG");
		boost::property_tree::ptree knob_node;
		knob_node.put("proc_id", knob.proc_id);
		knob_node.put("id", knob.id);
		knob_node.put("type", knob.type);
		knob_node.put("min", knob.min);
		knob_node.put("max", knob.max);
		knob_node.put("val", knob.val);
		data_node.add_child("knob", knob_node);
		SEND_JSON();
	}

	void rtm_interface::knob_cont_dereg(knob_cont_t knob)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_KNOB_CONT_DEREG");
		boost::property_tree::ptree knob_node;
		knob_node.put("proc_id", knob.proc_id);
		knob_node.put("id", knob.id);
		knob_node.put("type", knob.type);
		knob_node.put("min", knob.min);
		knob_node.put("max", knob.max);
		knob_node.put("val", knob.val);
		data_node.add_child("knob", knob_node);
		SEND_JSON();
	}

	mon_disc_t rtm_interface::mon_disc_reg(mon_type_t type, prime::api::disc_t min, prime::api::disc_t max, prime::api::cont_t weight)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_MON_DISC_REG");
		ADD_JSON_DATA("proc_id", ::getpid());
		ADD_JSON_DATA("type", type);
		ADD_JSON_DATA("min", min);
		ADD_JSON_DATA("max", max);
		ADD_JSON_DATA("weight", weight);
		SEND_JSON();
		prime::util::send_message(ui_socket, json_string);

		std::unique_lock<std::mutex> lock(mon_disc_reg_m);
		mon_disc_reg_cv.wait(lock);

		mon_disc_return_m.lock();
		auto result = mon_disc_return;
		mon_disc_return_m.unlock();

		return result;
	}

	mon_cont_t rtm_interface::mon_cont_reg(mon_type_t type, prime::api::cont_t min, prime::api::cont_t max, prime::api::cont_t weight)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_MON_CONT_REG");
		ADD_JSON_DATA("proc_id", ::getpid());
		ADD_JSON_DATA("type", type);
		ADD_JSON_DATA("min", min);
		ADD_JSON_DATA("max", max);
		ADD_JSON_DATA("weight", weight);
		SEND_JSON();
		prime::util::send_message(ui_socket, json_string);

		std::unique_lock<std::mutex> lock(mon_cont_reg_m);
		mon_cont_reg_cv.wait(lock);

		mon_cont_return_m.lock();
		auto result = mon_cont_return;
		mon_cont_return_m.unlock();

		return result;
	}

	void rtm_interface::mon_disc_min(mon_disc_t mon, prime::api::disc_t min)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_DISC_MIN;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << min << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		prime::util::send_message(ui_socket, json_string);

		return;
	}

	void rtm_interface::mon_disc_max(mon_disc_t mon, prime::api::disc_t max)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_DISC_MAX;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << max << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		prime::util::send_message(ui_socket, json_string);

		return;
	}

	void rtm_interface::mon_disc_weight(mon_disc_t mon, prime::api::cont_t weight)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_DISC_WEIGHT;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << weight << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		prime::util::send_message(ui_socket, json_string);

		return;
	}

	void rtm_interface::mon_cont_min(mon_cont_t mon, prime::api::cont_t min)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_CONT_MIN;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << min << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		prime::util::send_message(ui_socket, json_string);

		return;
	}

	void rtm_interface::mon_cont_max(mon_cont_t mon, prime::api::cont_t max)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_CONT_MAX;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << max << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		prime::util::send_message(ui_socket, json_string);

		return;
	}

	void rtm_interface::mon_cont_weight(mon_cont_t mon, prime::api::cont_t weight)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_CONT_WEIGHT;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << weight << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}
		prime::util::send_message(ui_socket, json_string);

		return;
	}

	void rtm_interface::mon_disc_set(mon_disc_t mon, prime::api::disc_t val)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_DISC_SET;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << val << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		return;
	}

	void rtm_interface::mon_cont_set(mon_cont_t mon, prime::api::cont_t val)
	{
		// Five fields: Type, ID, Val, PID, TS
		char type = PRIME_API_APP_MON_CONT_SET;

		std::stringstream ss;
		ss << type << API_DELIMINATOR << mon.id << API_DELIMINATOR << val << API_DELIMINATOR << mon.proc_id << API_DELIMINATOR << util::get_timestamp();
		std::string json_string = ss.str();

		prime::util::send_message(socket, json_string);
		if(logger_en) {
			prime::util::send_message(logger_socket, json_string);
		}

		return;
	}

	void rtm_interface::mon_disc_dereg(mon_disc_t mon)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_MON_DISC_DEREG");
		boost::property_tree::ptree mon_node;
		mon_node.put("proc_id", mon.proc_id);
		mon_node.put("id", mon.id);
		mon_node.put("type", mon.type);
		mon_node.put("min", mon.min);
		mon_node.put("max", mon.max);
		mon_node.put("val", mon.val);
		mon_node.put("weight", mon.weight);
		data_node.add_child("mon", mon_node);
		SEND_JSON();
		prime::util::send_message(ui_socket, json_string);
	}

	void rtm_interface::mon_cont_dereg(mon_cont_t mon)
	{
		CREATE_JSON_ROOT("PRIME_API_APP_MON_CONT_DEREG");
		boost::property_tree::ptree mon_node;
		mon_node.put("proc_id", mon.proc_id);
		mon_node.put("id", mon.id);
		mon_node.put("type", mon.type);
		mon_node.put("min", mon.min);
		mon_node.put("max", mon.max);
		mon_node.put("val", mon.val);
		mon_node.put("weight", mon.weight);
		data_node.add_child("mon", mon_node);
		SEND_JSON();
		prime::util::send_message(ui_socket, json_string);
	}
	
	
	/* ---------------------------------- Struct Constructors --------------------------------- */
	ui_interface::ui_interface(pid_t proc_id,
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
		) :
		default_addrs(check_addrs(ui_addrs, proc_id)),
		proc_id(proc_id),
		socket(
			prime::uds::socket_layer_t::UI, 
			ui_addrs,
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(prime::uds::socket_layer_t::LOGGER, ui_addrs),
		logger_en(ui_addrs->logger_en),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		mon_disc_min_handler(mon_disc_min_handler),
		mon_disc_max_handler(mon_disc_max_handler),
		mon_disc_weight_handler(mon_disc_weight_handler),
		mon_cont_min_handler(mon_cont_min_handler),
		mon_cont_max_handler(mon_cont_max_handler),
		mon_cont_weight_handler(mon_cont_weight_handler),
		app_stop_handler(app_stop_handler)
	{
#ifdef API_DEBUG	
		std::cout << "UI Interface:" << std::endl;
		std::cout << "\tUI Socket: " << socket.get_endpoints() << std::endl;
		std::cout << "\tLogger Socket: " << logger_socket.get_endpoints() << std::endl;
#endif	
	}	
	
	
	/* ----------------------------------- UDS Constructors ----------------------------------- */
	ui_interface::ui_interface(pid_t proc_id,
		boost::function<void(pid_t)> app_reg_handler,
		boost::function<void(pid_t)> app_dereg_handler,
		boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_min_handler,
		boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_max_handler,
		boost::function<void(unsigned int, prime::api::disc_t)> mon_disc_weight_handler,
		boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_min_handler,
		boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_max_handler,
		boost::function<void(unsigned int, prime::api::cont_t)> mon_cont_weight_handler,
		boost::function<void(pid_t)> app_stop_handler,
		bool logger_en
		) :
		ui_interface(proc_id,
		app_reg_handler,
		app_dereg_handler,
		mon_disc_min_handler,
		mon_disc_max_handler,
		mon_disc_weight_handler,
		mon_cont_min_handler,
		mon_cont_max_handler,
		mon_cont_weight_handler,
		app_stop_handler,
		std::string("/tmp/app.ui.logger.") + std::to_string(proc_id) + std::string(".uds"),
		std::string("/tmp/logger.uds"),
		logger_en)
	{ }	
	
	ui_interface::ui_interface(pid_t proc_id,
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
		bool logger_en
		) :
		proc_id(proc_id),
		socket(
			std::string("/tmp/app.ui.") + std::to_string(proc_id) + std::string(".uds"),
			std::string("/tmp/ui.uds"),
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(logger_local_filename, logger_remote_filename),
		logger_en(logger_en),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		mon_disc_min_handler(mon_disc_min_handler),
		mon_disc_max_handler(mon_disc_max_handler),
		mon_disc_weight_handler(mon_disc_weight_handler),
		mon_cont_min_handler(mon_cont_min_handler),
		mon_cont_max_handler(mon_cont_max_handler),
		mon_cont_weight_handler(mon_cont_weight_handler),
		app_stop_handler(app_stop_handler)
	{ }	
	/* ---------------------------------------------------------------------------------------- */
	
	/* --------------------------------- Socket Constructors ---------------------------------- */
	ui_interface::ui_interface(pid_t proc_id,
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
		bool logger_en
		) :
		proc_id(proc_id),
		socket(
			std::string("/tmp/app.ui.") + std::to_string(proc_id) + std::string(".uds"),
			std::string("/tmp/ui.uds"),
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(logger_remote_address, logger_remote_port),
		logger_en(logger_en),
		app_reg_handler(app_reg_handler),
		app_dereg_handler(app_dereg_handler),
		mon_disc_min_handler(mon_disc_min_handler),
		mon_disc_max_handler(mon_disc_max_handler),
		mon_disc_weight_handler(mon_disc_weight_handler),
		mon_cont_min_handler(mon_cont_min_handler),
		mon_cont_max_handler(mon_cont_max_handler),
		mon_cont_weight_handler(mon_cont_weight_handler),
		app_stop_handler(app_stop_handler)
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

		if(!message_type.compare("PRIME_UI_APP_REG")) {
			pid_t proc_id = data.get<pid_t>("proc_id");
			app_reg_handler(proc_id);
		}
		else if(!message_type.compare("PRIME_UI_APP_DEREG")) {
			pid_t proc_id = data.get<pid_t>("proc_id");
			app_dereg_handler(proc_id);
		}
		else if(!message_type.compare("PRIME_UI_APP_MON_DISC_MIN")) {
			mon_disc_min_handler(data.get<unsigned int>("id"), data.get<prime::api::disc_t>("min"));
		}
		else if(!message_type.compare("PRIME_UI_APP_MON_DISC_MAX")) {
			mon_disc_max_handler(data.get<unsigned int>("id"), data.get<prime::api::disc_t>("max"));
		}
		else if(!message_type.compare("PRIME_UI_APP_MON_DISC_WEIGHT")) {
			mon_disc_weight_handler(data.get<unsigned int>("id"), data.get<prime::api::disc_t>("weight"));
		}
		else if(!message_type.compare("PRIME_UI_APP_MON_CONT_MIN")) {
			mon_cont_min_handler(data.get<unsigned int>("id"), data.get<prime::api::cont_t>("min"));
		}
		else if(!message_type.compare("PRIME_UI_APP_MON_CONT_MAX")) {
			mon_cont_max_handler(data.get<unsigned int>("id"), data.get<prime::api::cont_t>("max"));
		}
		else if(!message_type.compare("PRIME_UI_APP_MON_CONT_WEIGHT")) {
			mon_cont_weight_handler(data.get<unsigned int>("id"), data.get<prime::api::cont_t>("weight"));
		}
		else if(!message_type.compare("PRIME_UI_APP_STOP")) {
			pid_t proc_id = data.get<pid_t>("proc_id");
			app_stop_handler(proc_id);
		}
		else {
			std::cout << "UNKNOWN MESSAGE TYPE" << std::endl;
		}
	}

	void ui_interface::return_ui_app_start(void)
	{
		CREATE_JSON_ROOT("PRIME_UI_APP_RETURN_APP_START");
		ADD_JSON_DATA("proc_id", ::getpid());
		SEND_JSON();
	}

	void ui_interface::return_ui_app_stop(void)
	{
		CREATE_JSON_ROOT("PRIME_UI_APP_RETURN_APP_STOP");
		ADD_JSON_DATA("proc_id", ::getpid());
		SEND_JSON();
	}

	void ui_interface::ui_app_error(std::string message)
	{
		CREATE_JSON_ROOT("PRIME_UI_APP_ERROR");
		ADD_JSON_DATA("proc_id", ::getpid());
		ADD_JSON_DATA("msg", message);
		SEND_JSON();
	}

} } }
