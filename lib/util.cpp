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

#include <string>
#include <chrono>
#include <time.h>
#include <sched.h>
#include "util.h"
#include "uds.h"
#include "args/args.hxx"

namespace prime { namespace util
{
	void send_message(prime::uds& sock, std::string message)
	{
#ifdef DEBUG_API
		std::cout << message << std::endl;
#endif
		std::vector<char> raw(message.begin(), message.end());
		sock.send_message(raw);
	}

	unsigned long long get_timestamp()
	{
		auto now = std::chrono::system_clock::now();
 		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
		return duration.count();
	}

	unsigned int set_id(unsigned int fu_id, unsigned int level1_id, unsigned int level2_id, unsigned int knob_mon_id)
	{
		return (fu_id << 24) + (level1_id << 16) + (level2_id << 8) + knob_mon_id;
	}

	void get_id(unsigned int full_id, unsigned int& fu_id, unsigned int& level1_id, unsigned int& level2_id, unsigned int& knob_mon_id)
	{
		fu_id = full_id >> 24;
		level1_id = (full_id >> 16) && 0xFF;
		level2_id = (full_id >> 8) && 0xFF;
		knob_mon_id = full_id && 0xFF;
		return;
	}


	/* RTM Utility Functions. */
	void rtm_set_affinity(pid_t app_pid, std::vector<unsigned int> cpus)
	{
        cpu_set_t affinity_set, check_set;
        CPU_ZERO(&affinity_set);
        CPU_ZERO(&check_set);

		for(unsigned int cpu : cpus){
			CPU_SET(cpu, &affinity_set);
		}

		struct proc_tasks *app_tasks = proc_open_tasks(app_pid);
		pid_t app_tid;

		while (!proc_next_tid(app_tasks, &app_tid)) {
			sched_setaffinity(app_tid, sizeof(cpu_set_t), &affinity_set);
		}
		proc_close_tasks(app_tasks);

		//Check affinity was set correctly
		sched_getaffinity(app_pid, sizeof(cpu_set_t), &check_set);
		if(!CPU_EQUAL(&check_set, &affinity_set)){
			std::cout << "RTM: Could not change app pid: " << app_pid << " affinity." << std::endl;
		}
	}
	
	/*int parse_socket_addrs(prime::uds::socket_addrs_t* addrs, int argc, const char *argv[])
	{
		args::ArgumentParser parser("PRiME API Specific arguments","\n");
		parser.LongSeparator("=");
		args::HelpFlag help(parser, "help", "", {'h', "help"});

		args::Group optional(parser, "Optional Arguments:", args::Group::Validators::DontCare);
		//args::Flag logger_off(optional, "logger_off", "Disable logging of API messages and logger output.", {"lo", "logger_off"});
		
		//args::Group remote_logger(parser, "Remote logger, both fields required:", args::Group::Validators::And);
		//args::ValueFlag<std::string> logger_address(optional, "logger_address", "The remote address of the logger", {"la", "logger_address"});
		//args::ValueFlag<int> logger_port(optional, "logger_port", "The remote port for the logger", {"lp", "logger_port"});
		
		UTIL_LOGGER_PARAMS();
		
		try {
			parser.ParseCLI(argc, argv);
		} catch (args::Help) {
			//std::cout << parser;
			return -1;
		} catch (args::ParseError e) {
			//std::cerr << e.what() << std::endl;
			//std::cerr << parser;
			return -1;
		} catch (args::ValidationError e) {
			//std::cerr << e.what() << std::endl;
			//std::cerr << parser;
			return -1;
		}
		
		int ret = 0;
		
		if(logger_off) {
			addrs->logger_en = false;
			ret++;
		} else {
			addrs->logger_en = true;
		}
		
		if(logger_address) {
			addrs->logger_addr = true;
			addrs->remote_logger = true;
			addrs->logger_remote_address = args::get(logger_address);
			addrs->logger_port = args::get(logger_port);
			ret++;
		}
		
		return ret;
		
	}*/
} }
