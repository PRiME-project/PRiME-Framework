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

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include "uds.h"
#include "args/args.hxx"

extern "C"
{
#include "procutils.h"
}

#define UTIL_ARGS_LOGGER_PARAMS() \
		args::Flag logger_off(optional, "logger_off", "Disable logging of API messages and logger output.", {"lo", "logger_off"}); \
		args::ValueFlag<std::string> logger_address(optional, "logger_address", "The remote address of the logger", {"la", "logger_address"}); \
		args::ValueFlag<int> logger_port(optional, "logger_port", "The remote port for the logger", {"lp", "logger_port"}); 

#define UTIL_ARGS_LOGGER_PROC() \
		if(logger_off) { \
			api_addrs->logger_en = false; \
			ui_addrs->logger_en = false; \
		} \
		if(logger_address) { \
			api_addrs->logger_addr = true; \
			api_addrs->remote_logger = true; \
			api_addrs->logger_remote_address = args::get(logger_address); \
			api_addrs->logger_port = args::get(logger_port); \
			\
			ui_addrs->logger_addr = true; \
			ui_addrs->remote_logger = true; \
			ui_addrs->logger_remote_address = args::get(logger_address); \
			ui_addrs->logger_port = args::get(logger_port); \
		}


#define UTIL_ARGS_LOGGER_PROC_RTM() \
		if(logger_off) { \
			app_addrs->logger_en = false; \
			dev_addrs->logger_en = false; \
			ui_addrs->logger_en = false; \
		} \
		if(logger_address) { \
			app_addrs->logger_addr = true; \
			app_addrs->remote_logger = true; \
			app_addrs->logger_remote_address = args::get(logger_address); \
			app_addrs->logger_port = args::get(logger_port); \
			\
			dev_addrs->logger_addr = true; \
			dev_addrs->remote_logger = true; \
			dev_addrs->logger_remote_address = args::get(logger_address); \
			dev_addrs->logger_port = args::get(logger_port); \
			\
			ui_addrs->logger_addr = true; \
			ui_addrs->remote_logger = true; \
			ui_addrs->logger_remote_address = args::get(logger_address); \
			ui_addrs->logger_port = args::get(logger_port);	\
		}

namespace prime { namespace util
{    
    void send_message(prime::uds& sock, std::string message);
    unsigned long long get_timestamp();
   	unsigned int set_id(unsigned int fu_id, unsigned int level1_id, unsigned int level2_id, unsigned int knob_mon_id);
    void get_id(unsigned int full_id, unsigned int& fu_id, unsigned int& level1_id, unsigned int& level2_id, unsigned int& knob_mon_id);


    /* RTM Utility Functions. */
	void rtm_set_affinity(pid_t app_pid, std::vector<unsigned int> cpus);

	//int parse_socket_addrs(prime::uds::socket_addrs_t* addrs, int argc, const char *argv[]);
} }

#endif
