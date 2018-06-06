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
 *		written by Charles Leech & Graeme Bragg
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
#include "sched.h"

#include "uds.h"
#include "util.h"
#include "prime_api_dev.h"
#include "args/args.hxx"

//#define DEBUG

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

#define CPU_TEMP_MIN	0
#define CPU_TEMP_MAX	80

#define GPU_TEMP_MIN	0
#define GPU_TEMP_MAX	80

namespace prime { namespace dev
{
	class odroid
	{
	public:
		odroid(prime::uds::socket_addrs_t *dev_rtm_addrs, prime::uds::socket_addrs_t *dev_ui_addrs, prime::api::dev::dev_args_t* dev_args);
		~odroid();

		static void signal_handler(int signum);
		static std::string get_governor(unsigned int idx);
		static void save_governors(void);

	private:
		prime::api::dev::rtm_interface rtm_api;
		prime::api::dev::ui_interface ui_api;
		prime::uds logger_socket;
		bool logger_en;

		std::string power_node_a7 = "/sys/bus/i2c/drivers/INA231/3-0045/";
		std::string power_node_a15 = "/sys/bus/i2c/drivers/INA231/3-0040/";
		std::string power_node_mem = "/sys/bus/i2c/drivers/INA231/3-0041/";
		std::string power_node_gpu = "/sys/bus/i2c/drivers/INA231/3-0044/";
		std::string temp_node = "/sys/devices/10060000.tmu/temp";

		//Handler functions
		void ui_dev_stop_handler(void);
		void cpu_freq_handler(prime::api::disc_t val, prime::api::disc_t core);
		void cpu_governor_handler(prime::api::disc_t val, prime::api::disc_t core);
		void gpu_freq_handler(prime::api::disc_t val);
		void gpu_freq_en_handler(prime::api::disc_t val);
		void pmc_control_handler(unsigned int core, unsigned int pmc, unsigned int event);
		prime::api::dev::mon_disc_ret_t cycle_count_handler(unsigned int core);
		prime::api::dev::mon_disc_ret_t pmc_handler(unsigned int core, unsigned int pmc);
		prime::api::dev::mon_cont_ret_t read_temp_a15_handler(int core);
		prime::api::dev::mon_cont_ret_t read_temp_gpu_handler(void);

		//Device functions
		void add_dev_knobs(void);
		void add_dev_mons(void);
		void control_loop(void);

		//Driver functions
		void set_governor(std::string governor, unsigned int idx);
		void set_affinity(unsigned int cpu);
		unsigned int read_cycle_count();
		unsigned long int pmc_get_supported_events(void);
		void enable_power(std::string filename);
		double read_power(std::string filename);
		prime::api::dev::mon_cont_ret_t  total_power(void);
		prime::api::dev::mon_cont_ret_t  a15_power(void);
		prime::api::dev::mon_cont_ret_t  a7_power(void);
		prime::api::dev::mon_cont_ret_t  gpu_power(void);
		prime::api::dev::mon_cont_ret_t  mem_power(void);

		bool log_power;
		boost::thread control_loop_thread;
        std::vector<std::pair<prime::api::dev::mon_cont_t, boost::function<prime::api::dev::mon_cont_ret_t(void)>>> power_mons;

        void log_disc_knobs(std::vector<std::pair<prime::api::dev::knob_disc_t, boost::function<void(prime::api::disc_t)>>> knobs_disc);
        void log_cont_knobs(std::vector<std::pair<prime::api::dev::knob_cont_t, boost::function<void(prime::api::cont_t)>>> knobs_cont);
        void log_disc_mons(std::vector<std::pair<prime::api::dev::mon_disc_t, boost::function<prime::api::dev::mon_disc_ret_t(void)>>> mons_disc);
        void log_cont_mons(std::vector<std::pair<prime::api::dev::mon_cont_t, boost::function<prime::api::dev::mon_cont_ret_t(void)>>> mons_cont);

		static std::string a7_governor;
		static std::string a15_governor;
	};

	odroid::odroid(prime::uds::socket_addrs_t *dev_rtm_addrs, prime::uds::socket_addrs_t *dev_ui_addrs, prime::api::dev::dev_args_t* dev_args) :
		logger_en(dev_rtm_addrs->logger_en),
		log_power(dev_args->power),
		rtm_api("../build/architecture_dev.json", dev_rtm_addrs), //provide device architecture to interface
		ui_api(boost::bind(&odroid::ui_dev_stop_handler, this), dev_ui_addrs),
		logger_socket(prime::uds::socket_layer_t::LOGGER, dev_rtm_addrs)	//TODO: deal with this better: uses same socket as API
	{
#ifdef DEBUG
		std::cout << "Init Device:" << std::endl;
		std::cout << "\tSave Governors" << std::endl;
#endif		
		//Get currently set governors
		save_governors();

#ifdef DEBUG
		std::cout << "\tEnable Power Sensors" << std::endl;
#endif	
		enable_power(power_node_a7);
		enable_power(power_node_a15);
		enable_power(power_node_mem);
		enable_power(power_node_gpu);

#ifdef DEBUG
		std::cout << "\tAdd Knobs" << std::endl;
#endif	
		add_dev_knobs();
#ifdef DEBUG
		std::cout << "\tAdd Monitors" << std::endl;
#endif			
		add_dev_mons();
		//rtm_api.print_architecture();

		auto mons_cont = rtm_api.get_cont_mons();
		for(auto& mon : mons_cont)
		{
            if(mon.first.type == prime::api::dev::PRIME_POW)
                power_mons.push_back(mon);
		}

#ifdef DEBUG
		std::cout << "\tDone" << std::endl;
#endif	
		ui_api.return_ui_dev_start();

		control_loop_thread = boost::thread(&odroid::control_loop, this);
		control_loop_thread.join();
	}

	odroid::~odroid()
	{
		set_governor(a7_governor, 4);
		set_governor(a15_governor, 4);
		gpu_freq_en_handler(1);
		ui_api.return_ui_dev_stop();
	}

	void odroid::ui_dev_stop_handler(void)
	{
		control_loop_thread.interrupt();
	}

	void odroid::add_dev_knobs()
	{
		unsigned int fu_id, su_id; //Functional unit ID and sub unit ID.

		// Add device knob to available knobs with event handler
#ifdef DEBUG
		std::cout << "\t\tGet Architecture" << std::endl;
#endif			
		boost::property_tree::ptree architecture = rtm_api.get_architecture();
        //rtm_api.print_architecture();

		/* ====================== Global ====================== */

		// none

#ifdef DEBUG
		std::cout << "\t\tPopulate A7" << std::endl;
#endif			
		/* ======================== A7 ======================== */
		fu_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.id");
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.knobs.id");
		boost::property_tree::ptree freq_knob_a7 = architecture.get_child("device.functional_units.cpu_a7.knobs.freq");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, freq_knob_a7.get<int>("id")),
			prime::api::dev::PRIME_FREQ, 0, 12, 12, 12,
			boost::bind(&odroid::cpu_freq_handler, this, _1, 0)
		);		// Frequency Control

		boost::property_tree::ptree governor_knob_a7 = architecture.get_child("device.functional_units.cpu_a7.knobs.governor");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, governor_knob_a7.get<int>("id")),
			prime::api::dev::PRIME_GOVERNOR, 0, 5, 5, 5,
			boost::bind(&odroid::cpu_governor_handler, this, _1, 0)
		);		// Governor Control
		//set_governor(std::string("userspace"), 0);

		/* ---------------------- Core 0 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu0.id");
		boost::property_tree::ptree cpu0_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a7.cpu0.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 0, 0, _1)
		);

		boost::property_tree::ptree cpu0_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a7.cpu0.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 0, 1, _1)
		);

		boost::property_tree::ptree cpu0_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a7.cpu0.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 0, 2, _1)
		);

		boost::property_tree::ptree cpu0_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a7.cpu0.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 0, 3, _1)
		);

		/* ---------------------- Core 1 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu1.id");
		boost::property_tree::ptree cpu1_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a7.cpu1.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 1, 0, _1)
		);

		boost::property_tree::ptree cpu1_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a7.cpu1.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 1, 1, _1)
		);

		boost::property_tree::ptree cpu1_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a7.cpu1.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 1, 2, _1)
		);

		boost::property_tree::ptree cpu1_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a7.cpu1.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 1, 3, _1)
		);

		/* ---------------------- Core 2 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu2.id");
		boost::property_tree::ptree cpu2_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a7.cpu2.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 2, 0, _1)
		);

		boost::property_tree::ptree cpu2_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a7.cpu2.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 2, 1, _1)
		);

		boost::property_tree::ptree cpu2_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a7.cpu2.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 2, 2, _1)
		);

		boost::property_tree::ptree cpu2_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a7.cpu2.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 2, 3, _1)
		);

		/* ---------------------- Core 3 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu3.id");
		boost::property_tree::ptree cpu3_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a7.cpu3.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 3, 0, _1)
		);

		boost::property_tree::ptree cpu3_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a7.cpu3.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 3, 1, _1)
		);

		boost::property_tree::ptree cpu3_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a7.cpu3.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 3, 2, _1)
		);

		boost::property_tree::ptree cpu3_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a7.cpu3.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 3, 3, _1)
		);


#ifdef DEBUG
		std::cout << "\t\tPopulate A15" << std::endl;
#endif			
		/* ======================== A15 ======================= */
		fu_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.id");
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.knobs.id");
		boost::property_tree::ptree freq_knob_a15 = architecture.get_child("device.functional_units.cpu_a15.knobs.freq");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, freq_knob_a15.get<int>("id")),
			prime::api::dev::PRIME_FREQ, 0, 18, 18, 18,
			boost::bind(&odroid::cpu_freq_handler, this, _1, 4)
		);	// Frequency change handler

		boost::property_tree::ptree governor_knob_a15 = architecture.get_child("device.functional_units.cpu_a15.knobs.governor");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, governor_knob_a15.get<int>("id")),
			prime::api::dev::PRIME_GOVERNOR, 0, 5, 5, 5,
			boost::bind(&odroid::cpu_governor_handler, this, _1, 4)
		);		// Governor Control
		//set_governor(std::string("userspace"), 4);

		/* ---------------------- Core 4 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu4.id");
		boost::property_tree::ptree cpu4_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 4, 0, _1)
		);

		boost::property_tree::ptree cpu4_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 4, 1, _1)
		);

		boost::property_tree::ptree cpu4_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 4, 2, _1)
		);

		boost::property_tree::ptree cpu4_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 4, 3, _1)
		);

		boost::property_tree::ptree cpu4_pmc_control_4_lev = architecture.get_child("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_4");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_control_4_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 4, 4, _1)
		);

		boost::property_tree::ptree cpu4_pmc_control_5_lev = architecture.get_child("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_5");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_control_5_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 4, 5, _1)
		);

		/* ---------------------- Core 5 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu5.id");
		boost::property_tree::ptree cpu5_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 5, 0, _1)
		);

		boost::property_tree::ptree cpu5_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 5, 1, _1)
		);

		boost::property_tree::ptree cpu5_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 5, 2, _1)
		);

		boost::property_tree::ptree cpu5_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 5, 3, _1)
		);

		boost::property_tree::ptree cpu5_pmc_control_4_lev = architecture.get_child("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_4");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_control_4_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 5, 4, _1)
		);

		boost::property_tree::ptree cpu5_pmc_control_5_lev = architecture.get_child("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_5");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_control_5_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 5, 5, _1)
		);

		/* ---------------------- Core 6 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu6.id");
		boost::property_tree::ptree cpu6_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 6, 0, _1)
		);

		boost::property_tree::ptree cpu6_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 6, 1, _1)
		);

		boost::property_tree::ptree cpu6_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 6, 2, _1)
		);

		boost::property_tree::ptree cpu6_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 6, 3, _1)
		);

		boost::property_tree::ptree cpu6_pmc_control_4_lev = architecture.get_child("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_4");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_control_4_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 6, 4, _1)
		);

		boost::property_tree::ptree cpu6_pmc_control_5_lev = architecture.get_child("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_5");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_control_5_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 6, 5, _1)
		);

		/* ---------------------- Core 7 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu7.id");
		boost::property_tree::ptree cpu7_pmc_control_0_lev = architecture.get_child("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_0");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_control_0_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 7, 0, _1)
		);

		boost::property_tree::ptree cpu7_pmc_control_1_lev = architecture.get_child("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_1");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_control_1_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 7, 1, _1)
		);

		boost::property_tree::ptree cpu7_pmc_control_2_lev = architecture.get_child("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_2");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_control_2_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 7, 2, _1)
		);

		boost::property_tree::ptree cpu7_pmc_control_3_lev = architecture.get_child("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_3");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_control_3_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 7, 3, _1)
		);

		boost::property_tree::ptree cpu7_pmc_control_4_lev = architecture.get_child("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_4");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_control_4_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 7, 4, _1)
		);

		boost::property_tree::ptree cpu7_pmc_control_5_lev = architecture.get_child("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_5");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_control_5_lev.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC_CNT, 0, 255, 0, 0,
			boost::bind(&odroid::pmc_control_handler, this, 7, 5, _1)
		);


#ifdef DEBUG
		std::cout << "\t\tPopulate GPU" << std::endl;
#endif			
		/* ======================== GPU ======================= */
		fu_id = architecture.get<unsigned int>("device.functional_units.gpu.id");
		su_id = architecture.get<unsigned int>("device.functional_units.gpu.knobs.id");
		boost::property_tree::ptree freq_knob_gpu = architecture.get_child("device.functional_units.gpu.knobs.freq");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, freq_knob_gpu.get<int>("id")),
			prime::api::dev::PRIME_FREQ, 0, 6, 6, 6,
			boost::bind(&odroid::gpu_freq_handler, this, _1)
		);		// Frequency Control

		boost::property_tree::ptree freq_en_knob_gpu = architecture.get_child("device.functional_units.gpu.knobs.freq_en");
		rtm_api.add_knob_disc(
			prime::util::set_id(fu_id, su_id, 0, freq_en_knob_gpu.get<int>("id")),
			prime::api::dev::PRIME_FREQ_EN, 0, 1, 1, 1,
			boost::bind(&odroid::gpu_freq_en_handler, this, _1)
		);		// Frequency Control Enable

	}

	void odroid::add_dev_mons()
	{
		unsigned int fu_id, su_id; //Functional unit ID and sub unit ID.

		// Add device monitors to available monitors with event handlers
		boost::property_tree::ptree architecture = rtm_api.get_architecture();

		/* ====================== Global ====================== */
		fu_id = architecture.get<unsigned int>("device.functional_units.global_monitors.id");
		su_id = architecture.get<unsigned int>("device.functional_units.global_monitors.mons.id");
		boost::property_tree::ptree soc_power_mon = architecture.get_child("device.functional_units.global_monitors.mons.power");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, soc_power_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, 0, 0,
			boost::bind(&odroid::total_power, this)
		);


		/* ======================== A7 ======================== */
		fu_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.id");
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.mons.id");
		boost::property_tree::ptree a7_power_mon = architecture.get_child("device.functional_units.cpu_a7.mons.power");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, a7_power_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, 0, 0,
			boost::bind(&odroid::a7_power, this)
		);

		/* ---------------------- Core 0 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu0.id");
		boost::property_tree::ptree cpu0_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a7.cpu0.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 0)
		);

		boost::property_tree::ptree cpu0_pmc_0_mon = architecture.get_child("device.functional_units.cpu_a7.cpu0.mons.pmc_0");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_0_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 0, 0)
		);

		boost::property_tree::ptree cpu0_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a7.cpu0.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 0, 1)
		);

		boost::property_tree::ptree cpu0_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a7.cpu0.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 0, 2)
		);

		boost::property_tree::ptree cpu0_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a7.cpu0.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu0_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 0, 3)
		);


		/* ---------------------- Core 1 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu1.id");
		boost::property_tree::ptree cpu1_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a7.cpu1.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 1)
		);

		boost::property_tree::ptree cpu1_pmc_0_mon = architecture.get_child("device.functional_units.cpu_a7.cpu1.mons.pmc_0");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_0_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 1, 0)
		);

		boost::property_tree::ptree cpu1_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a7.cpu1.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 1, 1)
		);

		boost::property_tree::ptree cpu1_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a7.cpu1.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 1, 2)
		);

		boost::property_tree::ptree cpu1_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a7.cpu1.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu1_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 1, 3)
		);

		/* ---------------------- Core 2 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu2.id");
		boost::property_tree::ptree cpu2_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a7.cpu2.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 2)
		);

		boost::property_tree::ptree cpu2_pmc_0_mon = architecture.get_child("device.functional_units.cpu_a7.cpu2.mons.pmc_0");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_0_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 2, 0)
		);

		boost::property_tree::ptree cpu2_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a7.cpu2.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 2, 1)
		);

		boost::property_tree::ptree cpu2_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a7.cpu2.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 2, 2)
		);

		boost::property_tree::ptree cpu2_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a7.cpu2.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu2_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 2, 3)
		);

		/* ---------------------- Core 3 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a7.cpu3.id");
		boost::property_tree::ptree cpu3_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a7.cpu3.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 3)
		);

		boost::property_tree::ptree cpu3_pmc_0_mon = architecture.get_child("device.functional_units.cpu_a7.cpu3.mons.pmc_0");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_0_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 3, 0)
		);

		boost::property_tree::ptree cpu3_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a7.cpu3.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 3, 1)
		);

		boost::property_tree::ptree cpu3_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a7.cpu3.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 3, 2)
		);

		boost::property_tree::ptree cpu3_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a7.cpu3.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu3_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 3, 3)
		);




		/* ======================== A15 ======================= */
		fu_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.id");
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.mons.id");
		boost::property_tree::ptree a15_power_mon = architecture.get_child("device.functional_units.cpu_a15.mons.power");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, a15_power_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, 0, 0,
			boost::bind(&odroid::a15_power, this)
		);

		/* ---------------------- Core 4 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu4.id");
		boost::property_tree::ptree cpu4_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 4)
		);

		boost::property_tree::ptree cpu4_pmc_0_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.pmc_0");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_0_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 4, 0)
		);

		boost::property_tree::ptree cpu4_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 4, 1)
		);

		boost::property_tree::ptree cpu4_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 4, 2)
		);

		boost::property_tree::ptree cpu4_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 4, 3)
		);

		boost::property_tree::ptree cpu4_pmc_4_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.pmc_4");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_4_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 4, 4)
		);

		boost::property_tree::ptree cpu4_pmc_5_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.pmc_5");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu4_pmc_5_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 4, 5)
		);

		boost::property_tree::ptree cpu4_temp_mon = architecture.get_child("device.functional_units.cpu_a15.cpu4.mons.temp");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, cpu4_temp_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_TEMP, 0, CPU_TEMP_MIN, CPU_TEMP_MAX,
			boost::bind(&odroid::read_temp_a15_handler, this, 4)
		);

		/* ---------------------- Core 5 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu5.id");
		boost::property_tree::ptree cpu5_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 5)
		);

		boost::property_tree::ptree cpu5_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 5, 1)
		);

		boost::property_tree::ptree cpu5_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 5, 2)
		);

		boost::property_tree::ptree cpu5_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 5, 3)
		);

		boost::property_tree::ptree cpu5_pmc_4_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.pmc_4");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_4_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 5, 4)
		);

		boost::property_tree::ptree cpu5_pmc_5_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.pmc_5");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu5_pmc_5_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 5, 5)
		);

		boost::property_tree::ptree cpu5_temp_mon = architecture.get_child("device.functional_units.cpu_a15.cpu5.mons.temp");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, cpu5_temp_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_TEMP, 0, CPU_TEMP_MIN, CPU_TEMP_MAX,
			boost::bind(&odroid::read_temp_a15_handler, this, 5)
		);

		/* ---------------------- Core 6 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu6.id");
		boost::property_tree::ptree cpu6_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 6)
		);

		boost::property_tree::ptree cpu6_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 6, 1)
		);

		boost::property_tree::ptree cpu6_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 6, 2)
		);

		boost::property_tree::ptree cpu6_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 6, 3)
		);

		boost::property_tree::ptree cpu6_pmc_4_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.pmc_4");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_4_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 6, 4)
		);

		boost::property_tree::ptree cpu6_pmc_5_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.pmc_5");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu6_pmc_5_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 6, 5)
		);

		boost::property_tree::ptree cpu6_temp_mon = architecture.get_child("device.functional_units.cpu_a15.cpu6.mons.temp");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, cpu6_temp_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_TEMP, 0, CPU_TEMP_MIN, CPU_TEMP_MAX,
			boost::bind(&odroid::read_temp_a15_handler, this, 6)
		);

		/* ---------------------- Core 7 ---------------------- */
		su_id = architecture.get<unsigned int>("device.functional_units.cpu_a15.cpu7.id");
		boost::property_tree::ptree cpu7_cycle_count_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.cycle_count");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_cycle_count_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_CYCLES, 0, 0, 0,
			boost::bind(&odroid::cycle_count_handler, this, 7)
		);

		boost::property_tree::ptree cpu7_pmc_1_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.pmc_1");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_1_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 7, 1)
		);

		boost::property_tree::ptree cpu7_pmc_2_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.pmc_2");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_2_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 7, 2)
		);

		boost::property_tree::ptree cpu7_pmc_3_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.pmc_3");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_3_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 7, 3)
		);

		boost::property_tree::ptree cpu7_pmc_4_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.pmc_4");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_4_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 7, 4)
		);

		boost::property_tree::ptree cpu7_pmc_5_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.pmc_5");
		rtm_api.add_mon_disc(
			prime::util::set_id(fu_id, su_id, 0, cpu7_pmc_5_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_PMC, 0, 0, 0,
			boost::bind(&odroid::pmc_handler, this, 7, 5)
		);

		boost::property_tree::ptree cpu7_temp_mon = architecture.get_child("device.functional_units.cpu_a15.cpu7.mons.temp");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, cpu7_temp_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_TEMP, 0, CPU_TEMP_MIN, CPU_TEMP_MAX,
			boost::bind(&odroid::read_temp_a15_handler, this, 7)
		);



		/* ======================== GPU ======================= */
		fu_id = architecture.get<unsigned int>("device.functional_units.gpu.id");
		su_id = architecture.get<unsigned int>("device.functional_units.gpu.mons.id");
		boost::property_tree::ptree gpu_power_mon = architecture.get_child("device.functional_units.gpu.mons.power");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, gpu_power_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, 0, 0,
			boost::bind(&odroid::gpu_power, this)
		);

		boost::property_tree::ptree gpu_temp_mon = architecture.get_child("device.functional_units.gpu.mons.temp");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, gpu_temp_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_TEMP, 0, GPU_TEMP_MIN, GPU_TEMP_MAX,
			boost::bind(&odroid::read_temp_gpu_handler, this)
		);


		/* ====================== Memory ====================== */
		fu_id = architecture.get<unsigned int>("device.functional_units.mem.id");
		su_id = architecture.get<unsigned int>("device.functional_units.mem.mons.id");
		boost::property_tree::ptree mem_power_mon = architecture.get_child("device.functional_units.mem.mons.power");
		rtm_api.add_mon_cont(
			prime::util::set_id(fu_id, su_id, 0, mem_power_mon.get<unsigned int>("id")),
			prime::api::dev::PRIME_POW, 0, 0, 0,
			boost::bind(&odroid::mem_power, this)
		);
	}


	void odroid::control_loop()
	{
		while(1) {
			if(log_power){
                log_cont_mons(power_mons);
            }

			try {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
			}
			catch (boost::thread_interrupted&) {
				return;
			}
		}
	}

	// Utility function to set CPU Governor
	void odroid::set_governor(std::string governor, unsigned int idx)
	{
		std::string path;

		if ((idx >= 0) && (idx <= 3)) {
			// A7
			path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
		} else if ((idx >= 4) && (idx <= 7)) {
			// A15
			path = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_governor";
		} else {
			// out of range
			return;
		}

		std::ofstream ofs(path);
		ofs << governor;
		ofs.close();
	}

	// Utility function to get CPU Governor
	std::string odroid::get_governor(unsigned int idx)
	{
		std::string path, governor;

		if ((idx >= 0) && (idx <= 3)) {
			// A7
			path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
		} else if ((idx >= 4) && (idx <= 7)) {
			// A15
			path = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_governor";
		} else {
			// out of range
			return "";
		}

		std::ifstream ifs(path);
		if(ifs.is_open()) {
			getline(ifs, governor);
			ifs.close();
		}

		return governor;
	}

	// Event handler when RTM sets frequency device knob
	void odroid::cpu_freq_handler(prime::api::disc_t val, prime::api::disc_t core)
	{
		unsigned int freq = 200000;		// Set the base freq.
		freq += (100000 * val);			// Set freq based on index.

		if((core < 0) || (core > 7)) {
			// Sanity check for out of range value
			return;
		}

		std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(core) + "/cpufreq/scaling_setspeed";
		//std::cout << path << "   " << freq << std::endl;
		std::ofstream ofs(path);
		ofs << freq;
		ofs.close();
	}

	// Event handler to set the CPU governor
	void odroid::cpu_governor_handler(prime::api::disc_t val, prime::api::disc_t core)
	{
		// 0 = userspace, 1 = powersave, 2 = conservative
		// 3 = ondemand, 4 = interactive, 5 = performance
		std::string governor;

		switch (val) {
			case 0:	governor = "userspace";		// Userspace Governor
					break;
			case 1:	governor = "powersave";		// Powersave Governor
					break;
			case 2:	governor = "conservative";	// Conservative Governor
					break;
			case 3:	governor = "ondemand";		// Ondemand Governor
					break;
			case 4:	governor = "interactive";	// Interactive Governor
					break;
			case 5:	governor = "performance";	// Performance Governor
					break;
			default: return;	// out of range.
		}

		set_governor(governor, core);
	}

	// Event handler to set the GPU Frequency
	void odroid::gpu_freq_handler(prime::api::disc_t val)
	{
		unsigned int freq;

		switch (val) {			// Convert index into freq.
			case 0:	freq = 177;
					break;
			case 1:	freq = 266;
					break;
			case 2:	freq = 350;
					break;
			case 3:	freq = 420;
					break;
			case 4: freq = 480;
					break;
			case 5: freq = 543;
					break;
			default: freq = 600;
		}

		std::string path = "/sys/class/misc/mali0/device/clock";
		std::ofstream ofs(path);
		ofs << freq;
		ofs.close();
	}

	// Event handler to enable/disable GPU frequency control.
	void odroid::gpu_freq_en_handler(prime::api::disc_t val)
	{
		// 0 = manual control. 1 (or any other value) = automatic DVFS.
		std::string path = "/sys/class/misc/mali0/device/clock";
		std::ofstream ofs(path);
		if(val) {
			// Set automatic control
			ofs << "1";
		} else {
			// Set manual control
			ofs << "0";
		}
		ofs.close();
	}

	// Utility function to set cpu affinity
	void odroid::set_affinity(unsigned int cpu)
	{
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(cpu, &cpuset);
		pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
		while(sched_getcpu() != cpu);
	}

	// Event handler when RTM gets cycle count cpu monitor. Returns the number of cycles since last call and resets counter.
	prime::api::dev::mon_disc_ret_t odroid::cycle_count_handler(unsigned int core)
	{
		unsigned int cur_ccount, ccount;
		prime::api::dev::mon_disc_ret_t ret;
		ccount = 0;

		set_affinity(core);		// Make sure we are on the correct core.

		// Get cycle count from PMC
		__asm volatile("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(ret.val));
		__asm volatile("MCR p15, 0, %0, c9, c13, 0\t\n": :"r"(ccount));
		
		ret.min = ret.max = 0;
		
		return ret;
	}

	// Event handler when RTM gets PMC cpu monitor. Returns the number of events since last call and resets counter.
	prime::api::dev::mon_disc_ret_t odroid::pmc_handler(unsigned int core, unsigned int pmc)
	{
		unsigned int cur_pcount, pcount;
		prime::api::dev::mon_disc_ret_t ret;
		pcount = 0;

		set_affinity(core);		// Make sure we are on the correct core.

		// Select the correct PMC in PMSELR
		__asm volatile("MCR p15, 0, %0, c9, c12, 5\t\n": :"r"(pmc));

		// Get the counter value from PMXEVCNTR
		__asm volatile("MRC p15, 0, %0, c9, c13, 2\t\n": "=r"(ret.val));
		__asm volatile("MCR p15, 0, %0, c9, c13, 2\t\n": :"r"(pcount));
		
		ret.min = ret.max = 0;
		
		return ret;
	}

	// Event handler when RTM sets PMC control knob
	void odroid::pmc_control_handler(unsigned int core, unsigned int pmc, unsigned int event)
	{
		set_affinity(core);		// Make sure we are on the correct core.

		// Select the correct PMC in PMSELR
		__asm volatile("MCR p15, 0, %0, c9, c12, 5\t\n": :"r"(pmc));


		// Set the event in PMXEVTYPER
		__asm volatile("MCR p15, 0, %0, c9, c13, 1\t\n": :"r"(event));
	}

	// Utility function to get the list of supported PMCs, may want return type as uint64_t?
	unsigned long int odroid::pmc_get_supported_events(void)
	{
		unsigned int pmc_tmp;
		uint64_t pmceid;

		// Read PMCEID1
		__asm volatile("MRC p15, 0, %0, c9, c12, 7\t\n": "=r"(pmc_tmp));
		pmceid = pmc_tmp;
		pmceid <<= 32;

		// Read PMCEID0
		__asm volatile("MRC p15, 0, %0, c9, c12, 6\t\n": "=r"(pmc_tmp));
		pmceid |= pmc_tmp;

		return pmceid;
	}


	// Utility function to enable power sensors
	void odroid::enable_power(std::string filename)
	{
		std::ofstream ofs(filename + "enable");
		ofs << "1";
		ofs.close();
	}

	// Utility function to read power sensors
	double odroid::read_power(std::string filename)
	{
		std::ifstream ifs(filename + "sensor_W");
		double power;
		ifs >> power;
		ifs.close();
		return power;
	}

	// Utility function to read power sensors
	prime::api::dev::mon_cont_ret_t odroid::total_power()
	{
		prime::api::dev::mon_cont_ret_t ret;
		ret.val = read_power(power_node_a7) + read_power(power_node_a15) + read_power(power_node_mem) + read_power(power_node_gpu);
		return ret;
	}

	// Utility function to read power sensors
	prime::api::dev::mon_cont_ret_t  odroid::a15_power()
	{
		prime::api::dev::mon_cont_ret_t ret;
		ret.val = read_power(power_node_a15);
		return ret;
	}
	// Utility function to read power sensors
	prime::api::dev::mon_cont_ret_t  odroid::a7_power()
	{
		prime::api::dev::mon_cont_ret_t ret;
		ret.val = read_power(power_node_a7);
		return ret;
	}

	// Placeholder Utility function to read GPU power sensors
	prime::api::dev::mon_cont_ret_t  odroid::gpu_power()
	{
		prime::api::dev::mon_cont_ret_t ret;
		ret.val = read_power(power_node_gpu);
		return ret;
	}

	// Placeholder Utility function to read Memory power sensors
	prime::api::dev::mon_cont_ret_t  odroid::mem_power()
	{
		prime::api::dev::mon_cont_ret_t ret;
		ret.val = read_power(power_node_mem);
		return ret;
	}


	// Utility function to read temperature sensor
	prime::api::dev::mon_cont_ret_t odroid::read_temp_a15_handler(int core)
	{
		prime::api::dev::mon_cont_ret_t ret;
		
		if ((core < 4) || (core > 7)) {
			// Error: a core we don't have temp for.
			ret.val = ret.min = ret.max = 0;
			return ret;
		} else {
			core = core - 3;
			std::ifstream ifs(temp_node);
			std::string name, tmp, temperature;
			for(int i=0;i<core;i++){
				ifs >> name;
				ifs >> tmp;
				ifs >> temperature;
			}
			ifs.close();
			ret.val = stof(temperature)/1000;
			ret.min = CPU_TEMP_MIN;
			ret.max = CPU_TEMP_MAX;
			
			return ret;
		}
	}


	prime::api::dev::mon_cont_ret_t odroid::read_temp_gpu_handler()
	{
		prime::api::dev::mon_cont_ret_t ret;
		
		std::ifstream ifs(temp_node);
		std::string name, tmp, temperature;
		for(int i=0;i<5;i++){
			ifs >> name;
			ifs >> tmp;
			ifs >> temperature;
		}
		ifs.close();
		ret.val = stof(temperature)/1000;
		ret.min = GPU_TEMP_MIN;
		ret.max = GPU_TEMP_MAX;
		
		return ret;
	}

    void odroid::log_disc_knobs(std::vector<std::pair<prime::api::dev::knob_disc_t, boost::function<void(prime::api::disc_t)>>> knobs_disc)
    {
		for(auto const &knob : knobs_disc) {
			CREATE_JSON_ROOT("PRIME_LOG_DEV_KNOB_DISC");
			ADD_JSON_DATA("id", knob.first.id);
			ADD_JSON_DATA("type", knob.first.type);
			ADD_JSON_DATA("min", knob.first.min);
			ADD_JSON_DATA("max", knob.first.max);
			ADD_JSON_DATA("val", knob.first.val);
			SEND_JSON(logger_socket);
		}
    }

    void odroid::log_cont_knobs(std::vector<std::pair<prime::api::dev::knob_cont_t, boost::function<void(prime::api::cont_t)>>> knobs_cont)
    {
		for(auto const &knob : knobs_cont) {
			CREATE_JSON_ROOT("PRIME_LOG_DEV_KNOB_CONT");
			ADD_JSON_DATA("id", knob.first.id);
			ADD_JSON_DATA("type", knob.first.type);
			ADD_JSON_DATA("min", knob.first.min);
			ADD_JSON_DATA("max", knob.first.max);
			ADD_JSON_DATA("val", knob.first.val);
			SEND_JSON(logger_socket);
		}
    }

    void odroid::log_disc_mons(std::vector<std::pair<prime::api::dev::mon_disc_t, boost::function<prime::api::dev::mon_disc_ret_t(void)>>> mons_disc)
    {
		for(auto const &mon : mons_disc) {
			prime::api::dev::mon_disc_ret_t mon_vals;
			
			// Get updated value and bounds
			mon_vals = mon.second();
			
			CREATE_JSON_ROOT("PRIME_LOG_DEV_MON_DISC");
			ADD_JSON_DATA("id", mon.first.id);
			ADD_JSON_DATA("val", mon_vals.val);
			ADD_JSON_DATA("min", mon_vals.min);
			ADD_JSON_DATA("max", mon_vals.max);
			ADD_JSON_DATA("type", mon.first.type);
			SEND_JSON(logger_socket);
		}
    }

    void odroid::log_cont_mons(std::vector<std::pair<prime::api::dev::mon_cont_t, boost::function<prime::api::dev::mon_cont_ret_t(void)>>> mons_cont)
    {
		for(auto const &mon : mons_cont) {
			prime::api::dev::mon_cont_ret_t mon_vals;
			
			// Get updated value and bounds
			mon_vals = mon.second();
			
			CREATE_JSON_ROOT("PRIME_LOG_DEV_MON_CONT");
			ADD_JSON_DATA("id", mon.first.id);
			ADD_JSON_DATA("val", mon_vals.val);
			ADD_JSON_DATA("min", mon_vals.min);
			ADD_JSON_DATA("max", mon_vals.max);
			ADD_JSON_DATA("type", mon.first.type);
			SEND_JSON(logger_socket);
		}
    }

    void odroid::save_governors(void)
    {
		a7_governor = get_governor(0);
		a15_governor = get_governor(4);
	}

	void odroid::signal_handler(int signum)
	{
		// Restore A7 Governor
		std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
		ofs << a7_governor;
		ofs.close();

		// Restore A15 Governor
		ofs.open("/sys/devices/system/cpu/cpu4/cpufreq/scaling_governor");
		ofs << a15_governor;
		ofs.close();

		// Restore GPU DVFS
		ofs.open("/sys/class/misc/mali0/device/clock");
		ofs << "1";
		ofs.close();

		exit(signum);
	}
} }

std::string prime::dev::odroid::a7_governor;
std::string prime::dev::odroid::a15_governor;

int main(int argc, const char * argv[])
{
#ifdef DEBUG
	std::cout << "Pre-Init Device:" << std::endl;
		std::cout << "\tSave Governors" << std::endl;
#endif		
	prime::dev::odroid::save_governors();

#ifdef DEBUG
		std::cout << "\tRegister Signal Handlers" << std::endl;
#endif		
	signal(SIGTERM, prime::dev::odroid::signal_handler);
	signal(SIGINT, prime::dev::odroid::signal_handler);

#ifdef DEBUG
		std::cout << "\tParse addresses & arguments" << std::endl;
#endif		
	prime::uds::socket_addrs_t dev_rtm_addrs, dev_ui_addrs;
	prime::api::dev::dev_args_t dev_args;
	if(prime::api::dev::parse_cli("Odroid XU3/4", &dev_rtm_addrs, &dev_ui_addrs, &dev_args, argc, argv)) {
		return -1;
	}
	//prime::util::parse_socket_addrs(&dev_addrs, argc, argv);

#ifdef DEBUG
		std::cout << "\tGo" << std::endl;
#endif	
	prime::dev::odroid dev(&dev_rtm_addrs, &dev_ui_addrs, &dev_args);
    return 0;
}
