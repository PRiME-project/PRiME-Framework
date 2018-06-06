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

#include "prime_api_dev.h"
#include "uds.h"
#include "util.h"
#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/exception/all.hpp>
#include <algorithm>
#include <utility>

//#define DEBUG

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

namespace prime { namespace api { namespace dev
{
	int parse_cli(std::string device_name, prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::api::dev::dev_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser(device_name + " Device.","PRiME Project\n");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		//args::Flag arg_temp(optional, "temp", "Enable steady state temperature readings for each operating point", {'t', "temp"});
		args::Flag arg_power(optional, "power", "Log power at 4Hz intervals", {'p', "power"});
		
		UTIL_ARGS_LOGGER_PARAMS();

		try {
			parser.ParseCLI(argc, argv);
		} catch (args::Help) {
			std::cout << parser;
			return -1;
		} catch (args::ParseError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return -1;
		} catch (args::ValidationError e) {
			std::cerr << e.what() << std::endl;
			std::cerr << parser;
			return -1;
		}
		
		UTIL_ARGS_LOGGER_PROC();

		//args->temp = arg_temp;
		args->power = arg_power;

		return 0;
	}
	
	bool rtm_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs)
	{
#ifdef DEBUG
		std::cout << "Checking Addresses" << std::endl;
#endif
		bool ret = false;
		
		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/dev.rtm.logger.uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;
#ifdef DEBUG
			std::cout << "\tDefault Logger set" << std::endl;
#endif			
			ret = true;
		}
		
		if(!socket_addrs->api_addr) {
			//No API address present, use default.
			socket_addrs->api_local_address = "/tmp/dev.rtm.uds";
			socket_addrs->api_remote_address = "/tmp/rtm.dev.uds";
			socket_addrs->api_addr = true;
#ifdef DEBUG
			std::cout << "\tDefault API set" << std::endl;
#endif						
			ret = true;
		}
		
		/*if(!socket_addrs->ui_addr) {
			//No UI address present, use default.
			socket_addrs->ui_local_address = ;
			socket_addrs->ui_remote_address = ;
			socket_addrs->ui_addr = true;
#ifdef DEBUG
			std::cout << "\tDefault UI set" << std::endl;
#endif			
			ret = true;
		}*/
		
#ifdef DEBUG
			std::cout << "\tAddresses Updated?:" << ret << std::endl;
#endif					
		return ret;
	}
	
	bool ui_interface::check_addrs(prime::uds::socket_addrs_t *socket_addrs)
	{
		bool ret = false;
		if(!socket_addrs->logger_addr) {
			//No Logger address present, use default.
			socket_addrs->logger_local_address = "/tmp/dev.ui.logger.uds";
			socket_addrs->logger_remote_address = "/tmp/logger.uds";
			socket_addrs->logger_addr = true;
			ret = true;
		}
		
		/*if(!socket_addrs->api_addr) {
			//No API address present, use default.
			socket_addrs->api_local_address = "/tmp/dev.rtm.uds";
			socket_addrs->api_remote_address = "/tmp/rtm.dev.uds";
			socket_addrs->api_addr = true;
			ret = true;
		}*/
		
		if(!socket_addrs->ui_addr) {
			//No UI address present, use default.
			socket_addrs->ui_local_address = "/tmp/dev.ui.uds";
			socket_addrs->ui_remote_address = "/tmp/ui.uds";
			socket_addrs->ui_addr = true;
			ret = true;
		}
		return ret;
	}
	
	rtm_interface::rtm_interface(
			std::string archfilename, 
			prime::uds::socket_addrs_t *dev_addrs
		) :
		default_addrs(check_addrs(dev_addrs)),
		archfilename(archfilename),
		logger_en(dev_addrs->logger_en),
		
		socket(
			prime::uds::socket_layer_t::DEV_RTM, 
			dev_addrs,
			boost::bind(&rtm_interface::message_handler, this, _1)),
		logger_socket(prime::uds::socket_layer_t::LOGGER, dev_addrs)
	{
		try {
			boost::property_tree::read_json(archfilename, architecture);
		}
		catch(boost::exception &ex) {
			std::cerr << boost::diagnostic_information(ex);
		}
	}
	
	/* ----------------------------------- UDS Constructors ----------------------------------- */
	/* Default Constructor: Uses UDS for API and Logger with default endpoints. */
	rtm_interface::rtm_interface(std::string archfilename, bool logger_en) :
		rtm_interface(archfilename,
			"/tmp/dev.rtm.logger.uds",
			"/tmp/logger.uds",
			"/tmp/dev.rtm.uds",
			"/tmp/rtm.dev.uds",
			logger_en)
	{ }
	
	/* UDS API, UDS Logger: Specified Logger path. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en
		) :
		rtm_interface(archfilename,
			logger_local_filename,
			logger_remote_filename,
			"/tmp/dev.rtm.uds",
			"/tmp/rtm.dev.uds",
			logger_en)
	{ }
	
	/* UDS API, UDS Logger: Specified API path. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			bool api_fname,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) : 
		rtm_interface(archfilename, 
			"/tmp/dev.rtm.logger.uds",
			"/tmp/logger.uds",
			api_local_filename,
			api_remote_filename,
			logger_en)
	{ }
	
	/* UDS API, UDS Logger: Specified logger and API paths. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) : 
		archfilename(archfilename),
		logger_en(logger_en),
		socket(  
			api_local_filename,
			api_remote_filename,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		logger_socket(
			logger_local_filename,
			logger_remote_filename)
	{
		try {
			boost::property_tree::read_json(archfilename, architecture);
		}
		catch(boost::exception &ex) {
			std::cerr << boost::diagnostic_information(ex);
		}
	}
	/* ---------------------------------------------------------------------------------------- */
	
	/* --------------------------------- Socket Constructors ---------------------------------- */
	/* Default UDS API, Network Logger address & path. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en
		) :
		rtm_interface(archfilename,
			logger_remote_address,
			logger_remote_port,
			"/tmp/dev.rtm.uds",
			"/tmp/rtm.dev.uds",
			logger_en)
	{ }
		
	/* UDS API, Network Logger: Specified API path and logger address & port. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			std::string api_local_filename,
			std::string api_remote_filename,
			bool logger_en
		) : 
		archfilename(archfilename),
		logger_en(logger_en),
		socket(  
			api_local_filename,
			api_remote_filename,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		logger_socket(logger_remote_address, logger_remote_port)
	{
		try {
			boost::property_tree::read_json(archfilename, architecture);
		}
		catch(boost::exception &ex) {
			std::cerr << boost::diagnostic_information(ex);
		}
	}
	
	
	/* Network API, default UDS logger. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			unsigned int api_local_port,
			bool logger_en
		) :
		rtm_interface(archfilename,
			"/tmp/dev.rtm.logger.uds",
			"/tmp/logger.uds",
			api_local_port,
			logger_en)
	{ }
		
	/* Network API, UDS Logger: Specified logger path and API port. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			std::string logger_local_filename,
			std::string logger_remote_filename,
			unsigned int api_local_port,
			bool logger_en
		) :
		archfilename(archfilename),
		logger_en(logger_en),
		socket(api_local_port,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		logger_socket(logger_local_filename, logger_remote_filename)
	{
		try {
			boost::property_tree::read_json(archfilename, architecture);
		}
		catch(boost::exception &ex) {
			std::cerr << boost::diagnostic_information(ex);
		}
	}
	
	
	/* Network API & Logger. */
	rtm_interface::rtm_interface(
			std::string archfilename, 
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			unsigned int api_local_port,
			bool logger_en
		) :
		archfilename(archfilename),
		logger_en(logger_en),
		socket(api_local_port,
			boost::bind(&rtm_interface::message_handler, this, _1)),	
		logger_socket(logger_remote_address, logger_remote_port)
	{
		try {
			boost::property_tree::read_json(archfilename, architecture);
		}
		catch(boost::exception &ex) {
			std::cerr << boost::diagnostic_information(ex);
		}
	}
	/* ---------------------------------------------------------------------------------------- */

	rtm_interface::~rtm_interface()
	{
	}

	void rtm_interface::message_handler(std::vector<char> message)
	{


		if(message[0] != '{') { // Not json, process quickly
			std::string delim = API_DELIMINATOR;
			size_t position;
			std::string id, val, type;
			std::string message_string( (reinterpret_cast<char*>(message.data())) + 1 + delim.length());		// This is 4ms+ faster than the two lines below!

#ifdef DEBUG
			std::string ts;
			std::cout << "Message String: " <<  message_string << std::endl;
#endif
			switch((prime::api::rtm_dev_msg_t)message[0]) {
				case PRIME_API_DEV_KNOB_DISC_SET:
					// Set a discrete knob, 4 fields: Type, ID & Value (plus Timestamp)
					position = message_string.find(delim);
					id = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);
#ifdef DEBUG
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					ts = message_string.substr(0, position);
					std::cout << "PRIME_API_DEV_KNOB_DISC_SET\tts: " << ts << "\tid: " << id << "\tval: " << val << "\t¿\n" << std::endl;
#endif
					rtm_interface::knob_disc_set(std::stoul(id), std::stoul(val));
					break;

				case PRIME_API_DEV_KNOB_CONT_SET:
					// Set a continuous knob, 4 fields: Type, ID & Value (plus Timestamp)
					position = message_string.find(delim);
					id = message_string.substr(0, position);
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					val = message_string.substr(0, position);
#ifdef DEBUG
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					ts = message_string.substr(0, position);
					std::cout << "PRIME_API_DEV_KNOB_CONT_SET\tts: " << ts << "\tid: " << id << "\tval: " << val << "\t¿\n" << std::endl;
#endif
					rtm_interface::knob_cont_set(std::stoul(id), std::stof(val));
					break;

				case PRIME_API_DEV_MON_DISC_GET:
					// Get a discrete monitor, 3 fields: Type, ID (plus Timestamp)
					position = message_string.find(delim);
					id = message_string.substr(0, position);
#ifdef DEBUG
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					ts = message_string.substr(0, position);
					std::cout << "PRIME_API_DEV_MON_DISC_GET\tts: " << ts << "\tid: " << id << "\t¿\n" << std::endl;
#endif
					rtm_interface::return_mon_disc_get(std::stoul(id));
					break;

				case PRIME_API_DEV_MON_CONT_GET:
					// Get a continuous monitor, 3 fields: Type, ID (plus Timestamp)
					position = message_string.find(delim);
					id = message_string.substr(0, position);
#ifdef DEBUG
					message_string.erase(0, position+delim.length());
					position = message_string.find(delim);
					ts = message_string.substr(0, position);
					std::cout << "PRIME_API_DEV_MON_CONT_GET\tts: " << ts << "\tid: " << id << "\t¿\n" << std::endl;
#endif
					rtm_interface::return_mon_cont_get(std::stoul(id));
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

			if(!message_type.compare("PRIME_API_DEV_KNOB_DISC_SIZE")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_knob_disc_size();
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_CONT_SIZE")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_knob_cont_size();
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_DISC_REG")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_knob_disc_reg();
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_CONT_REG")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_knob_cont_reg();
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_DISC_SET")) {
				unsigned int id = root.get<unsigned int>("id");
				prime::api::disc_t val = root.get<prime::api::disc_t>("val");
				rtm_interface::knob_disc_set(id, val);
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_CONT_SET")) {
				unsigned int id = root.get<unsigned int>("id");
				prime::api::cont_t val = root.get<prime::api::cont_t>("val");
				rtm_interface::knob_cont_set(id, val);
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_DISC_DEREG")) {
			}
			else if(!message_type.compare("PRIME_API_DEV_KNOB_CONT_DEREG")) {
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_DISC_SIZE")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_mon_disc_size();
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_CONT_SIZE")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_mon_cont_size();
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_DISC_REG")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_mon_disc_reg();
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_CONT_REG")) {
				boost::property_tree::ptree data_node = root.get_child("data");
				rtm_interface::return_mon_cont_reg();
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_DISC_GET")) {
				unsigned int id = root.get<unsigned int>("id");
				rtm_interface::return_mon_disc_get(id);
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_CONT_GET")) {
				unsigned int id = root.get<unsigned int>("id");
				rtm_interface::return_mon_cont_get(id);
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_DISC_DEREG")) {
			}
			else if(!message_type.compare("PRIME_API_DEV_MON_CONT_DEREG")) {
			}
			else if(!message_type.compare("PRIME_API_DEV_ARCH_GET")) {
				rtm_interface::return_arch_get();
			}

			else {
				std::cout << "UNKNOWN MESSAGE TYPE" << std::endl;
			}
		}
	}

	void rtm_interface::add_knob_disc(
		unsigned int id,
		knob_type_t type,
		prime::api::disc_t min,
		prime::api::disc_t max,
		prime::api::disc_t val,
		prime::api::disc_t init,
		boost::function<void(prime::api::disc_t)> set_handler
	)
	{
		knob_disc_t knob;
		knob.id = id;
		knob.type = type;
		knob.min = min;
		knob.max = max;
		knob.val = val;
		knob.init = init;
		knobs_disc.push_back(std::make_pair(knob, set_handler));
	}

	void rtm_interface::remove_knob_disc(unsigned int id)
	{
		knobs_disc.erase(std::remove_if(knobs_disc.begin(), knobs_disc.end(), [id](const std::pair<knob_disc_t, boost::function<void(prime::api::disc_t)>> knob) {return knob.first.id == id;}), knobs_disc.end());
	}

	void rtm_interface::add_knob_cont(
		unsigned int id,
		knob_type_t type,
		prime::api::cont_t min,
		prime::api::cont_t max,
		prime::api::cont_t val,
		prime::api::cont_t init,
		boost::function<void(prime::api::cont_t)> set_handler
	)
	{
		knob_cont_t knob;
		knob.id = id;
		knob.type = type;
		knob.min = min;
		knob.max = max;
		knob.val = val;
		knob.init = init;
		knobs_cont.push_back(std::make_pair(knob, set_handler));
	}

	void rtm_interface::remove_knob_cont(unsigned int id)
	{
		knobs_cont.erase(std::remove_if(knobs_cont.begin(), knobs_cont.end(), [id](const std::pair<knob_cont_t, boost::function<void(prime::api::cont_t)>> knob) {return knob.first.id == id;}), knobs_cont.end());
	}

	void rtm_interface::add_mon_disc(
		unsigned int id,
		mon_type_t type,
		prime::api::disc_t val,
		prime::api::disc_t min,
		prime::api::disc_t max,
		boost::function<prime::api::dev::mon_disc_ret_t(void)> get_handler
	)
	{
		mon_disc_t mon;
		mon.id = id;
		mon.type = type;
		mon.val = val;
		mon.min = min;
		mon.max = max;
		mons_disc.push_back(std::make_pair(mon, get_handler));
	}

	void rtm_interface::remove_mon_disc(unsigned int id)
	{
		mons_disc.erase(std::remove_if(mons_disc.begin(), mons_disc.end(), [id](const std::pair<mon_disc_t, boost::function<prime::api::dev::mon_disc_ret_t(void)>> mon) {return mon.first.id == id;}), mons_disc.end());
	}

	void rtm_interface::add_mon_cont(
		unsigned int id,
		mon_type_t type,
		prime::api::cont_t val,
		prime::api::cont_t min,
		prime::api::cont_t max,
		boost::function<prime::api::dev::mon_cont_ret_t(void)> get_handler
	)
	{
		mon_cont_t mon;
		mon.id = id;
		mon.type = type;
		mon.val = val;
		mon.min = min;
		mon.max = max;
		mons_cont.push_back(std::make_pair(mon, get_handler));
	}

	void rtm_interface::remove_mon_cont(unsigned int id)
	{
		mons_cont.erase(std::remove_if(mons_cont.begin(), mons_cont.end(), [id](const std::pair<mon_cont_t, boost::function<prime::api::dev::mon_cont_ret_t(void)>> mon) {return mon.first.id == id;}), mons_cont.end());
	}

	void rtm_interface::return_knob_disc_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_KNOB_DISC_SIZE");
		ADD_JSON_DATA("", knobs_disc.size());
		SEND_JSON();
	}

	void rtm_interface::return_knob_cont_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_KNOB_CONT_SIZE");
		ADD_JSON_DATA("", knobs_cont.size());
		SEND_JSON();
	}

	void rtm_interface::return_knob_disc_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_KNOB_DISC_REG");
		for(auto knob : knobs_disc) {
			knob.second(knob.first.val);
			boost::property_tree::ptree knob_node;
			knob_node.put("id", knob.first.id);
			knob_node.put("type", knob.first.type);
			knob_node.put("min", knob.first.min);
			knob_node.put("max", knob.first.max);
			knob_node.put("val", knob.first.val);
			knob_node.put("init", knob.first.init);
			data_node.push_back(std::make_pair("", knob_node));
		}
		SEND_JSON();
	}

	void rtm_interface::return_knob_cont_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_KNOB_CONT_REG");
		for(auto knob : knobs_cont) {
			knob.second(knob.first.val);
			boost::property_tree::ptree knob_node;
			knob_node.put("id", knob.first.id);
			knob_node.put("type", knob.first.type);
			knob_node.put("min", knob.first.min);
			knob_node.put("max", knob.first.max);
			knob_node.put("val", knob.first.val);
			knob_node.put("init", knob.first.init);
			data_node.push_back(std::make_pair("", knob_node));
		}
		SEND_JSON();
	}

	void rtm_interface::knob_disc_set(unsigned int id, prime::api::disc_t val)
	{
		for(auto &knob : knobs_disc) {
			if(knob.first.id == id) {
				knob.first.val = val;
				knob.second(val);
			}
		}
	}

	void rtm_interface::knob_cont_set(unsigned int id, prime::api::cont_t val)
	{
		for(auto &knob : knobs_cont) {
			if(knob.first.id == id) {
				knob.first.val = val;
				knob.second(val);
			}
		}
	}

	prime::api::disc_t rtm_interface::mon_disc_get(unsigned int id)
	{
		mons_disc_m.lock();
		for(auto &mon : mons_disc) {
			if(mon.first.id == id) {
				prime::api::dev::mon_disc_ret_t vals = mon.second();
				mon.first.val = vals.val;
        			mons_disc_m.unlock();
				return vals.val;
			}
		}
        	mons_disc_m.unlock();
	}

	prime::api::cont_t rtm_interface::mon_cont_get(unsigned int id)
	{
		mons_cont_m.lock();
		for(auto &mon : mons_cont) {
			if(mon.first.id == id) {
				prime::api::dev::mon_cont_ret_t vals = mon.second();
				mon.first.val = vals.val;
        			mons_cont_m.unlock();
				return vals.val;
			}
		}
        	mons_cont_m.unlock();
	}
	void rtm_interface::return_mon_disc_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_MON_DISC_SIZE");
		ADD_JSON_DATA("", mons_disc.size());
		SEND_JSON();
	}

	void rtm_interface::return_mon_cont_size(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_MON_CONT_SIZE");
		ADD_JSON_DATA("", mons_cont.size());
		SEND_JSON();
	}

	void rtm_interface::return_mon_disc_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_MON_DISC_REG");
		mons_disc_m.lock();
		for(auto mon : mons_disc) {
			prime::api::dev::mon_disc_ret_t mon_vals;
			
			// Get updated value and bounds
			mon_vals = mon.second();
			mon.first.val = mon_vals.val;
			mon.first.min = mon_vals.min;
			mon.first.max = mon_vals.max;
			
			boost::property_tree::ptree mon_node;
			mon_node.put("id", mon.first.id);
			mon_node.put("type", mon.first.type);
			mon_node.put("val", mon.first.val);
			mon_node.put("min", mon.first.min);
			mon_node.put("max", mon.first.max);
			data_node.push_back(std::make_pair("", mon_node));
		}
		mons_disc_m.unlock();
		SEND_JSON();
	}

	void rtm_interface::return_mon_cont_reg(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_MON_CONT_REG");
		mons_cont_m.lock();
		for(auto mon : mons_cont) {
			prime::api::dev::mon_cont_ret_t mon_vals;
			
			// Get updated value and bounds
			mon_vals = mon.second();
			mon.first.val = mon_vals.val;
			mon.first.min = mon_vals.min;
			mon.first.max = mon_vals.max;
			
			boost::property_tree::ptree mon_node;
			mon_node.put("id", mon.first.id);
			mon_node.put("type", mon.first.type);
			mon_node.put("val", mon.first.val);
			mon_node.put("min", mon.first.min);
			mon_node.put("max", mon.first.max);
			data_node.push_back(std::make_pair("", mon_node));
		}
		mons_cont_m.unlock();
		SEND_JSON();
	}

	void rtm_interface::return_mon_disc_get(unsigned int id)
	{
		mons_disc_m.lock();
		for(auto mon : mons_disc) {
			if(mon.first.id == id) {
				char type = PRIME_API_DEV_RETURN_MON_DISC_GET;
				prime::api::dev::mon_disc_ret_t mon_vals;
				std::stringstream ss;
				
				// Get updated value and bounds
				mon_vals = mon.second();
				mon.first.val = mon_vals.val;
				mon.first.min = mon_vals.min;
				mon.first.max = mon_vals.max;
				
				mons_disc_m.unlock();

				// Create message
				ss << type << API_DELIMINATOR;
				ss << std::to_string((unsigned int)id) << API_DELIMINATOR;
				ss << std::to_string(mon.first.val) << API_DELIMINATOR;
				ss << std::to_string(mon.first.min) << API_DELIMINATOR;
				ss << std::to_string(mon.first.max) << API_DELIMINATOR;
				ss << prime::util::get_timestamp();
				std::string json_string = ss.str();

				// Send the Message
				prime::util::send_message(socket, json_string);
				if(logger_en) {
					prime::util::send_message(logger_socket, json_string);
				}

				return;
			}
		}
		mons_disc_m.unlock();
	}

	void rtm_interface::return_mon_cont_get(unsigned int id)
	{
		mons_cont_m.lock();
		for(auto mon : mons_cont) {
			if(mon.first.id == id) {
				char type = PRIME_API_DEV_RETURN_MON_CONT_GET;
				prime::api::dev::mon_cont_ret_t mon_vals;
				std::stringstream ss;
				
				// Get updated value and bounds
				mon_vals = mon.second();
				mon.first.val = mon_vals.val;
				mon.first.min = mon_vals.min;
				mon.first.max = mon_vals.max;
				
				mons_cont_m.unlock();

				// Create message
				ss << type << API_DELIMINATOR;
				ss << std::to_string((unsigned int)id) << API_DELIMINATOR;
				ss << std::to_string(mon.first.val) << API_DELIMINATOR;
				ss << std::to_string(mon.first.min) << API_DELIMINATOR;
				ss << std::to_string(mon.first.max) << API_DELIMINATOR;
				ss << prime::util::get_timestamp();
				std::string json_string = ss.str();

				// Send the Message
				prime::util::send_message(socket, json_string);
				if(logger_en) {
					prime::util::send_message(logger_socket, json_string);
				}

				return;
			}
		}
		mons_cont_m.unlock();
	}

	void rtm_interface::return_arch_get(void)
	{
		CREATE_JSON_ROOT("PRIME_API_DEV_RETURN_ARCH_GET");
		ADD_JSON_DATA("", archfilename);
		SEND_JSON();
	}

	void rtm_interface::print_architecture(void)
	{
		std::cout << "Device Name: " << architecture.get_child("device").get<std::string>("descriptor") << std::endl;

		//Parse Functional Units
		boost::property_tree::ptree funcUnitItemList;
		try{
			funcUnitItemList = architecture.get_child("device.functional_units");
			std::cout << "Functional Units:" << std::endl;
		}catch(boost::property_tree::ptree_bad_path &){
			std::cerr << "\t" << "No Functional Units" << std::endl;
		}
		for(auto funcUnitItem : funcUnitItemList)
		{
			boost::property_tree::ptree funcUnit = funcUnitItem.second;
			std::cout << "\t" << funcUnitItem.first << ": " << std::endl;

			//Parse Global Knobs
			boost::property_tree::ptree funcUnitKnobList;
			try{
				funcUnitKnobList = funcUnit.get_child("knobs");
				std::cerr << "\t" << "Top-level Knobs:" << std::endl;
			}catch(boost::property_tree::ptree_bad_path &){
				std::cerr << "\t" << "No Top-level Knobs Found" << std::endl;
			}

			for(auto funcUnitKnob : funcUnitKnobList)
			{
				if(funcUnitKnob.first != "id"){
					boost::property_tree::ptree knob = funcUnitKnob.second;
					std::cout << "\t" << "\t" << funcUnitKnob.first << ": " \
								<< "id: " << knob.get<std::string>("id") \
								<< ", type: " << knob.get<std::string>("type") << std::endl;
				}
			}

			//Parse Global Monitors
			boost::property_tree::ptree funcUnitMonList;
			try{
				funcUnitMonList = funcUnit.get_child("mons");
				std::cerr << "\t" << "Top-level Monitors:" << std::endl;
			}catch(boost::property_tree::ptree_bad_path &){
				std::cerr << "\t" << "No Top-level Monitors Found" << std::endl;
			}
			for(auto funcUnitMon : funcUnitMonList)
			{
				if(funcUnitMon.first != "id"){
					boost::property_tree::ptree mon = funcUnitMon.second;
					std::cout << "\t" << "\t" << funcUnitMon.first << ": "  \
								<< "id: " << mon.get<std::string>("id") \
								<< ", type: " << mon.get<std::string>("type") << std::endl;
				}
			}

			//Parse Sub Units
			boost::property_tree::ptree subUnitItemList;
			try{
				subUnitItemList = funcUnit.get_child("sub_units");
				std::cout << "\t" << "Sub Units:" << std::endl;
			}catch(boost::property_tree::ptree_bad_path &){
				std::cerr << "\t" << "No Sub Units Found" << std::endl;
			}
			for(auto subUnitItem : subUnitItemList)
			{
				boost::property_tree::ptree subUnit = subUnitItem.second;
				std::cout << "\t" << "\t" << subUnitItem.first << ": " << std::endl;
				//Parse Sub Unit Knobs
				boost::property_tree::ptree subUnitKnobList;
				try{
					subUnitKnobList = subUnit.get_child("knobs");
					std::cerr << "\t" << "\t" << "Sub Unit Knobs:" << std::endl;
				}catch(boost::property_tree::ptree_bad_path &){
					std::cerr << "\t" << "\t" << "No Sub Unit Knobs Found" << std::endl;
				}
				for(auto subUnitKnob : subUnitKnobList)
				{
					boost::property_tree::ptree knob = subUnitKnob.second;
					std::cout << "\t" << "\t" << "\t" << subUnitKnob.first << ":" \
								<< ", id: " << knob.get<std::string>("id") \
								<< ", type: " << knob.get<std::string>("type") << std::endl;
				}
				//Parse Sub Unit Monitors
				boost::property_tree::ptree subUnitMonList;
				try{
					subUnitMonList = subUnit.get_child("mons");
					std::cerr << "\t" << "\t" << "Sub Unit Monitors:" << std::endl;
				}catch(boost::property_tree::ptree_bad_path &){
					std::cerr << "\t" << "\t" << "No Sub Unit Monitors Found" << std::endl;
				}
				for(auto subUnitMon : subUnitMonList)
				{
					boost::property_tree::ptree mon = subUnitMon.second;
					std::cout << "\t" << "\t" << "\t" << subUnitMon.first << ":" \
								<< ", id: " << mon.get<std::string>("id") \
								<< ", type: " << mon.get<std::string>("type") << std::endl;
				}
			}
		}
		return;
	}

	/* ---------------------------------- Struct Constructors ---------------------------------- */
	ui_interface::ui_interface(
			boost::function<void(void)> dev_stop_handler,
			prime::uds::socket_addrs_t *dev_addrs
		) :
		default_addrs(check_addrs(dev_addrs)),
		//socket(
		//	std::string("/tmp/dev.ui.uds"),
		//	std::string("/tmp/ui.uds"),
		//	boost::bind(&ui_interface::message_handler, this, _1)),
		logger_en(dev_addrs->logger_en),
		socket(
			prime::uds::socket_layer_t::UI, 
			dev_addrs,
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(prime::uds::socket_layer_t::LOGGER, dev_addrs),	
		//logger_socket(logger_local_filename, logger_remote_filename),
		dev_stop_handler(dev_stop_handler)
	{ }

	/* ----------------------------------- UDS Constructors ----------------------------------- */
	ui_interface::ui_interface(
			boost::function<void(void)> dev_stop_handler, 
			bool logger_en
		) :
		ui_interface(dev_stop_handler,
			"/tmp/dev.ui.logger.uds",
			"/tmp/logger.uds",
			logger_en)
	{ }
	
	ui_interface::ui_interface(
			boost::function<void(void)> dev_stop_handler,
			std::string logger_local_filename,
			std::string logger_remote_filename,
			bool logger_en
		) :
		logger_en(logger_en),
		socket(
			std::string("/tmp/dev.ui.uds"),
			std::string("/tmp/ui.uds"),
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(logger_local_filename, logger_remote_filename),
		dev_stop_handler(dev_stop_handler)
	{ }
	/* ---------------------------------------------------------------------------------------- */
	
	/* --------------------------------- Socket Constructors ---------------------------------- */
	ui_interface::ui_interface(
			boost::function<void(void)> dev_stop_handler,
			std::string logger_remote_address,
			unsigned int logger_remote_port,
			bool logger_en
		) : 
		logger_en(logger_en),
		socket(
			std::string("/tmp/dev.ui.uds"),
			std::string("/tmp/ui.uds"),
			boost::bind(&ui_interface::message_handler, this, _1)),
		logger_socket(logger_remote_address, logger_remote_port),
		dev_stop_handler(dev_stop_handler)
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

		if(!message_type.compare("PRIME_UI_DEV_STOP")) {
			dev_stop_handler();
		}
		else {
			std::cout << "UNKNOWN MESSAGE TYPE" << std::endl;
		}
	}

	void ui_interface::return_ui_dev_start(void)
	{
		CREATE_JSON_ROOT("PRIME_UI_DEV_RETURN_DEV_START");
		SEND_JSON();
	}

	void ui_interface::return_ui_dev_stop(void)
	{
		CREATE_JSON_ROOT("PRIME_UI_DEV_RETURN_DEV_STOP");
		SEND_JSON();
	}

	void ui_interface::ui_dev_error(std::string message)
	{
		CREATE_JSON_ROOT("PRIME_UI_DEV_ERROR");
		ADD_JSON_DATA("msg", message);
		SEND_JSON();
	}

} } }
