/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by James Davis & Graeme Bragg
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <climits>
#include <boost/bind.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cmath>
#include <csignal>
#include <algorithm>
#include <iterator>

#include "uds.h"
#include "util.h"
#include "prime_api_dev.h"
#include "sched.h"
#include "args/args.hxx"

#define CREATE_JSON_ROOT(type) \
    boost::property_tree::ptree root; \
    root.put("ts", prime::util::get_timestamp()); \
    root.put("type", type); \
    boost::property_tree::ptree data_node

#define ADD_JSON_DATA(key, value) \
    data_node.put(key, value)

#define SEND_JSON(socket_name) \
    root.add_child("data", data_node); \
    { std::stringstream ss; \
    write_json(ss, root); \
    std::string json_string = ss.str(); \
    prime::util::send_message(socket_name, json_string); }
	
// Min/max voltages taken from https://www.altera.com/documentation/mcn1422497163812.html

#define SOC_POW_MIN		0
#define SOC_POW_MAX		0
#define CPU_POW_MIN		0
#define CPU_POW_MAX		0
#define FPGA_POW_MIN	0
#define FPGA_POW_MAX	0
#define POW_MIN			0
#define POW_MAX			0

namespace prime { namespace dev
{
	class c5soc
	{
	public:
		c5soc(prime::uds::socket_addrs_t *dev_rtm_addrs, prime::uds::socket_addrs_t *dev_ui_addrs,  prime::api::dev::dev_args_t* dev_args);
		~c5soc();

	private:
		prime::api::dev::rtm_interface rtm_api;
		prime::api::dev::ui_interface ui_api;
		prime::uds ui_socket;
		prime::uds logger_socket;
		boost::thread loop_thread;
		
		bool logger_en;
		bool log_power;
		
		//Handler functions
		void ui_dev_stop_handler(void);
		
		//Device functions
		void add_dev_knobs(void);
		void add_dev_mons(void);
		void loop(void);
		
		//Driver functions
		//void set_gov(std::string gov);
		//void set_freq(unsigned int freq);
		void set_volt(std::string rail, prime::api::cont_t volt);
		prime::api::cont_t get_pow(std::string rail);
	};

	c5soc::c5soc(prime::uds::socket_addrs_t *dev_rtm_addrs, prime::uds::socket_addrs_t *dev_ui_addrs, prime::api::dev::dev_args_t* dev_args) :
		rtm_api("architecture_dev.json", dev_rtm_addrs), //provide device architecture to interface
		ui_api(boost::bind(&c5soc::ui_dev_stop_handler, this), dev_ui_addrs),
		ui_socket(prime::uds::socket_layer_t::UI, dev_ui_addrs),
		logger_socket(prime::uds::socket_layer_t::LOGGER, dev_rtm_addrs),	//TODO: deal with this better: uses same socket as API
		logger_en(dev_rtm_addrs->logger_en),
		log_power(dev_args->power)
	{
		//set_gov(std::string("userspace"));
		add_dev_knobs();
		add_dev_mons();
		//ui_api.return_ui_dev_start();
		
		loop_thread = boost::thread(&c5soc::loop, this);
		loop_thread.join();
	}

	c5soc::~c5soc()
	{
		//set_gov(std::string("interactive"));
		//ui_api.return_ui_dev_stop();
	}
	
	void c5soc::ui_dev_stop_handler(void)
	{
		loop_thread.interrupt();
	}

	void c5soc::add_dev_knobs(void)
	{
		boost::property_tree::ptree arch = rtm_api.get_architecture();
		
		// CPU frequency
		unsigned int fu_id = arch.get<unsigned int>("device.functional_units.cpu.id");
		/*rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.knobs.freq").get<int>("id")),
			prime::api::dev::PRIME_FREQ, 200000, 2000000, 2000000, 2000000, boost::bind(&c5soc::set_freq, this, _1)
		);*/
		
		// CPU 1.1V rail voltage
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.knobs.volt_1_1").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 1.07, 1.13, 1.1, 1.1, boost::bind(&c5soc::set_volt, this, std::string("HPS_1_1"), _1)
		);
		
		// CPU 1.5V rail voltage
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.knobs.volt_1_5").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 1.425, 1.575, 1.5, 1.5, boost::bind(&c5soc::set_volt, this, std::string("HPS_1_5"), _1)
		);
		
		// CPU 2.5V rail voltage
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.knobs.volt_2_5").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 2.375, 2.625, 2.5, 2.5, boost::bind(&c5soc::set_volt, this, std::string("HPS_2_5"), _1)
		);
		
		// CPU 3.3V rail voltage
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.knobs.volt_3_3").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 3.135, 3.465, 3.3, 3.3, boost::bind(&c5soc::set_volt, this, std::string("HPS_3_3"), _1)
		);
		
		// FPGA 1.1V rail voltage
		fu_id = arch.get<unsigned int>("device.functional_units.fpga.id");
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.knobs.volt_1_1").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 1.07, 1.13, 1.1, 1.1, boost::bind(&c5soc::set_volt, this, std::string("FPGA_1_1"), _1)
		);
		
		// FPGA 1.5V rail voltage
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.knobs.volt_1_5").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 1.425, 1.575, 1.5, 1.5, boost::bind(&c5soc::set_volt, this, std::string("FPGA_1_5"), _1)
		);
		
		// FPGA 2.5V rail voltage
		rtm_api.add_knob_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.knobs.volt_2_5").get<int>("id")),
			prime::api::dev::PRIME_VOLT, 2.375, 2.625, 2.5, 2.5, boost::bind(&c5soc::set_volt, this, std::string("FPGA_2_5"), _1)
		);
	}

	void c5soc::add_dev_mons(void)
	{
		boost::property_tree::ptree arch = rtm_api.get_architecture();
		
		// Total power
		unsigned int fu_id = arch.get<unsigned int>("device.functional_units.global_monitors.id");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.global_monitors.mons.pow").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, SOC_POW_MIN, SOC_POW_MIN,
			boost::bind(&c5soc::get_pow, this, std::string("soc"))
		);

        // CPU power
		fu_id = arch.get<unsigned int>("device.functional_units.cpu.id");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.mons.pow").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, CPU_POW_MIN, CPU_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("cpu"))
		);
		
		// CPU 1.1V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.mons.pow_1_1").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, CPU_POW_MIN, CPU_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("HPS_1_1"))
		);
		
        // CPU 1.5V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.mons.pow_1_5").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, CPU_POW_MIN, CPU_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("HPS_1_5"))
		);
		
		// CPU 2.5V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.mons.pow_2_5").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, CPU_POW_MIN, CPU_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("HPS_2_5"))
		);
		
		// CPU 3.3V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.cpu.mons.pow_3_3").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, CPU_POW_MIN, CPU_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("HPS_3_3"))
		);

        // FPGA total power
		fu_id = arch.get<unsigned int>("device.functional_units.fpga.id");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.mons.pow").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, FPGA_POW_MIN, FPGA_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("fpga"))
		);
		
		// FPGA 1.1V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.mons.pow_1_1").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, FPGA_POW_MIN, FPGA_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("FPGA_1_1"))
		);
		
        // FPGA 1.5V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.mons.pow_1_5").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, FPGA_POW_MIN, FPGA_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("FPGA_1_5"))
		);
		
		// FPGA 2.5V rail power
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, 0, 0, arch.get_child("device.functional_units.fpga.mons.pow_2_5").get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, FPGA_POW_MIN, FPGA_POW_MAX,
			boost::bind(&c5soc::get_pow, this, std::string("FPGA_2_5"))
		);
	}

	void c5soc::loop()
	{
		while(1) {
			try {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
			}
			catch(boost::thread_interrupted&) {
				return;
			}
		}
	}

	/*void c5soc::set_gov(std::string gov)
	{
		std::ofstream stream("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		stream << gov;
		stream.close();
	}*/

	/*void c5soc::set_freq(unsigned int freq)
	{
		std::ofstream stream("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
		stream << freq;
		stream.close();
	}*/
	
	void c5soc::set_volt(std::string rail, prime::api::cont_t volt)
	{
		std::system((std::string("../build/volreg ") + rail + std::string(" ") + std::to_string((float)volt)).c_str());
	}
	
	prime::api::dev::mon_cont_ret_t c5soc::get_pow(std::string rail)
	{
		prime::api::dev::mon_cont_ret_t ret;
		
		if(rail == std::string("soc"))
		{
			ret.val = get_pow(std::string("cpu")) + get_pow(std::string("fpga"));
			ret.min = SOC_POW_MIN;
			ret.max = SOC_POW_MAX;
			return ret;
		}
		else if(rail == std::string("cpu"))
		{
			ret.val = get_pow(std::string("HPS_1_1")) + get_pow(std::string("HPS_1_5")) + get_pow(std::string("HPS_2_5")) + get_pow(std::string("HPS_3_3"));
			ret.min = CPU_POW_MIN;
			ret.max = CPU_POW_MAX;
			return ret;
		}
		else if(rail == std::string("fpga"))
		{
			ret.val = get_pow(std::string("FPGA_1_1")) + get_pow(std::string("FPGA_1_5")) + get_pow(std::string("FPGA_2_5"));
			ret.min = FPGA_POW_MIN;
			ret.max = FPGA_POW_MAX;
			return ret;
		}
		else
		{
			FILE* handle = popen((std::string("../build/volreg ") + rail).c_str(), "r");
			char c_line[128];
			fgets(c_line, 128, handle);
			pclose(handle);
			
			std::string line(c_line);
			line.resize(line.length() - 3);					// Chop off " W\n"
			line = line.substr(line.rfind(" ") + 1);		// Get everything after last " "
			
			ret.val = (prime::api::cont_t)atof(line.c_str());
			ret.min = POW_MIN;
			ret.max = POW_MAX;
			return ret;
		}
	}

} }


int main(int argc, const char * argv[])
{
	
	prime::uds::socket_addrs_t dev_rtm_addrs, dev_ui_addrs;
	prime::api::dev::dev_args_t dev_args;
	if(prime::api::dev::parse_cli("Cyclone V SoC", &dev_rtm_addrs, &dev_ui_addrs, &dev_args, argc, argv)) {
		return -1;
	}
	
	prime::dev::c5soc dev(&dev_rtm_addrs, &dev_ui_addrs, &dev_args);
}
