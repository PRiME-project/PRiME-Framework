/* This file is part of Codegen QLearning, an example RTM for the PRiME Framework.
 * 
 * Codegen QLearning is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Codegen QLearning is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Codegen QLearning.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by James Bantock, Sadegh Dalvandi, Domenico Balsamo, Charles Leech & Graeme Bragg
 */
 
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "uds.h"
#include "util.h"
#include "rtm_arch_utils.h"
#include "prime_api_rtm.h"
#include "codegen.hpp"

//#define DEBUGAPI
//#define DEBUG
//#define NOTRAIN
#define MAX_REG_VALUE 4294967295 //maximum value for the registers

namespace prime
{
	class rtm
	{
		public:
		struct rtm_args_t {
			double frame_time = 0.040;
			int ondemand = false;
			int max_cpu_cycle_training = 2000;
		};

		static int parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* rtm_app_addrs,
			prime::uds::socket_addrs_t* rtm_dev_addrs, prime::uds::socket_addrs_t* rtm_ui_addrs,
			prime::rtm::rtm_args_t* args, int argc, const char * argv[]);



		rtm(prime::uds::socket_addrs_t* rtm_app_addrs, prime::uds::socket_addrs_t* rtm_dev_addrs,
			prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args);
		~rtm();
		private:
		prime::api::rtm::app_interface app_api;
		prime::api::rtm::dev_interface dev_api;
		prime::api::rtm::ui_interface ui_api;

		std::mutex dev_knob_disc_m;
		std::mutex dev_knob_cont_m;
		std::mutex dev_mon_disc_m;
		std::mutex dev_mon_cont_m;
		std::vector<prime::api::dev::knob_disc_t> knobs_disc;
		std::vector<prime::api::dev::knob_cont_t> knobs_cont;
		std::vector<prime::api::dev::mon_disc_t> mons_disc;
		std::vector<prime::api::dev::mon_cont_t> mons_cont;

		std::mutex apps_m;
		std::vector<pid_t> apps;
		std::mutex app_knobs_disc_m;
		std::mutex app_knobs_cont_m;
		std::mutex app_mons_disc_m;
		std::mutex app_mons_cont_m;
		std::vector<prime::api::app::knob_disc_t> app_knobs_disc;
		std::vector<prime::api::app::knob_cont_t> app_knobs_cont;
		std::vector<prime::api::app::mon_disc_t> app_mons_disc;
		std::vector<prime::api::app::mon_cont_t> app_mons_cont;

		std::mutex ui_rtm_stop_m;
		std::condition_variable ui_rtm_stop_cv;

		prime::api::dev::knob_disc_t a15_frequency_knob;
		prime::api::dev::knob_disc_t a15_governor_knob;
		prime::api::dev::mon_disc_t cycle_count_4_mon;
		prime::api::dev::mon_disc_t cycle_count_5_mon;
        prime::api::dev::mon_disc_t cycle_count_6_mon;
        prime::api::dev::mon_disc_t cycle_count_7_mon;

		prime::api::dev::mon_cont_t power_mon;

		boost::thread rtm_thread;
		prime::codegen::qlearn_sc controller;
		unsigned int ondemand;
		double frame_time;
		int max_cpu_cycle_training;
		int frame_count = 0;
		float decode_time = 0;
		int decoded_frame_counter = 0;
		unsigned int freqChoice = 0;
		unsigned int currentCycle = 0;
		unsigned int max_cpu_cycle = 0;
		unsigned int min_cpu_cycle = 4294967295;
		int average_time = 0;
		unsigned int perf_mon_id = 0;
		float perf_mon_weight = 0;
		prime::api::cont_t perf_cont = 0;

		time_t timer;

		void ui_rtm_stop_handler(void);
		void app_reg_handler(pid_t proc_id, unsigned long int ur_id);
		void app_dereg_handler(pid_t proc_id);
		void knob_disc_reg_handler(pid_t proc_id, prime::api::app::knob_disc_t knob);
		void knob_cont_reg_handler(pid_t proc_id, prime::api::app::knob_cont_t knob);

		void knob_disc_dereg_handler(prime::api::app::knob_disc_t knob);
		void knob_cont_dereg_handler(prime::api::app::knob_cont_t knob);
		void mon_disc_reg_handler(pid_t proc_id, prime::api::app::mon_disc_t mon);
		void mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon);

		void mon_disc_dereg_handler(prime::api::app::mon_disc_t mon);
		void mon_cont_dereg_handler(prime::api::app::mon_cont_t mon);
		void app_weight_handler(pid_t proc_id, prime::api::cont_t weight);

		void knob_disc_min_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t min);
		void knob_disc_max_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t max);
		void knob_cont_min_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t min);
		void knob_cont_max_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t max);
		void mon_disc_min_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t min);
		void mon_disc_max_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t max);
		void mon_disc_weight_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t weight);
		void mon_disc_val_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t val);
		void mon_cont_min_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t min);
		void mon_cont_max_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t max);
		void mon_cont_weight_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t weight);
		void mon_cont_val_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t val);
		void run_rtm_loop(float d_t);
		void reg_dev();
		void dereg_dev();
		void qlearn_rtm(unsigned int cc);
		void filter_cpu_cycle(unsigned int cc);
	};

	rtm::rtm(
		prime::uds::socket_addrs_t* rtm_app_addrs,
		prime::uds::socket_addrs_t* rtm_dev_addrs,
		prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args
		) :
		app_api(
				boost::bind(&rtm::app_reg_handler, this, _1, _2),
				boost::bind(&rtm::app_dereg_handler, this, _1),
				boost::bind(&rtm::knob_disc_reg_handler, this, _1, _2),
				boost::bind(&rtm::knob_cont_reg_handler, this, _1, _2),
				boost::bind(&rtm::knob_disc_dereg_handler, this, _1),
				boost::bind(&rtm::knob_cont_dereg_handler, this, _1),
				boost::bind(&rtm::mon_disc_reg_handler, this, _1, _2),
				boost::bind(&rtm::mon_cont_reg_handler, this, _1, _2),
				boost::bind(&rtm::mon_disc_dereg_handler, this, _1),
				boost::bind(&rtm::mon_cont_dereg_handler, this, _1),
				boost::bind(&rtm::knob_disc_min_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::knob_disc_max_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::knob_cont_min_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::knob_cont_max_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_disc_min_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_disc_max_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_disc_weight_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_disc_val_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_cont_min_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_cont_max_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_cont_weight_change_handler, this, _1, _2, _3),
				boost::bind(&rtm::mon_cont_val_change_handler, this, _1, _2, _3),
				rtm_app_addrs
			),
		dev_api(rtm_dev_addrs),
		ui_api(
				boost::bind(&rtm::app_weight_handler, this, _1, _2),
				boost::bind(&rtm::ui_rtm_stop_handler, this),
				rtm_ui_addrs
			),
		ondemand(rtm_args->ondemand),
		frame_time(rtm_args->frame_time),
#ifdef NOTRAIN
		max_cpu_cycle_training(0)
#else
		max_cpu_cycle_training(rtm_args->max_cpu_cycle_training)
#endif
	{
		reg_dev();
		ui_api.return_ui_rtm_start();

		std::unique_lock<std::mutex> stop_lock(ui_rtm_stop_m);
		ui_rtm_stop_cv.wait(stop_lock);
		ui_api.return_ui_rtm_stop();
	}

	rtm::~rtm()	{}

	void rtm::ui_rtm_stop_handler(void)
	{
		dereg_dev();
		ui_rtm_stop_cv.notify_one();
	}

	void rtm::reg_dev()
	{
		// Register device knobs and monitors
		knobs_disc = dev_api.knob_disc_reg();
		knobs_cont = dev_api.knob_cont_reg();
		mons_disc = dev_api.mon_disc_reg();
		mons_cont = dev_api.mon_cont_reg();
		boost::property_tree::ptree dev_arch = dev_api.dev_arch_get();

		std::vector<unsigned int> fu_ids;
		std::vector<unsigned int> su_ids;

		//get all functional units ids
		fu_ids = prime::util::get_func_unit_ids(dev_arch);

		/**********************GET FREQUENCY KNOB*************************/
		//get all discrete knobs for 3nd functional unit (a15)
		std::vector<prime::api::dev::knob_disc_t> knobs_A15;
		knobs_A15 = prime::util::get_disc_knob_func_unit(dev_arch, knobs_disc, fu_ids.at(2));

		//search for freq knob in A15
		for(auto knob : knobs_A15){
			if(knob.type == prime::api::dev::PRIME_FREQ){
				// std::cout << "this is freq A15!" << std::endl;
				a15_frequency_knob = knob;
			}
		}

		/**********************GET CYCLE COUNTERS*************************/
		//get all cycle count monitors
		std::vector<prime::api::dev::mon_disc_t> cycle_count_mons;
		cycle_count_mons = prime::util::get_all_disc_mon(mons_disc,prime::api::dev::PRIME_CYCLES);

		//connect monitors to use on control loop
		cycle_count_4_mon = cycle_count_mons.at(4);
		cycle_count_5_mon = cycle_count_mons.at(5);
		cycle_count_6_mon = cycle_count_mons.at(6);
		cycle_count_7_mon = cycle_count_mons.at(7);

		/**********************GET GOVERNOR*******************************/
		//get governor from 3rd fuctional unit (a15)
		for(auto knob : knobs_A15){
			if(knob.type == prime::api::dev::PRIME_GOVERNOR){
				// std::cout << "this is governor A15!" << std::endl;
				a15_governor_knob = knob;
			}
		}

		dev_api.knob_disc_set(a15_governor_knob, 0);
	}

	void rtm::dereg_dev()
	{
		rtm_thread.interrupt();
		rtm_thread.join();

		// Deregister device knobs and monitors
		dev_api.knob_disc_dereg(knobs_disc);
		dev_api.knob_cont_dereg(knobs_cont);
		dev_api.mon_disc_dereg(mons_disc);
		dev_api.mon_cont_dereg(mons_cont);

#ifdef DEBUG
		std::cout << "RTM: Device Knobs and Monitors Deregistered" << std::endl;
#endif
	}

	void rtm::app_reg_handler(pid_t proc_id, unsigned long int ur_id)
	{
		// Add registered app to vector of applications
		apps_m.lock();
		apps.push_back(proc_id);

		if(ondemand)
		{
			dev_api.knob_disc_set(a15_governor_knob, 3);
		}

		// Reset RTM training stuff
		max_cpu_cycle = 0;
		min_cpu_cycle = 4294967295;
		frame_count = 0;
		frame_time=0;
		perf_cont = 0;


		//Set the frequency if it is passed to the RTM
		if(freqChoice != 0){
			dev_api.knob_disc_set(a15_frequency_knob, (freqChoice/100000)-2);
#ifdef DEBUGAPI
			std::cout << "\n\tFreqChoice: " << freqChoice << std::endl;
#endif
		}
		else{
			unsigned int freq_choice = 1500000;
			dev_api.knob_disc_set(a15_frequency_knob, (freq_choice/100000)-2);
#ifdef DEBUGAPI
			std::cout << "\n\tFreq_Choice: " << freq_choice << std::endl;
#endif
		}

		//Set affinity
		//We set the application affinity in a way to run the application
		//on core 4, this is only an example on how this can be done.
        cpu_set_t default_odroid_cpuset;
        CPU_ZERO(&default_odroid_cpuset);
		CPU_SET(5, &default_odroid_cpuset);
		prime::util::rtm_set_affinity(proc_id, std::vector<unsigned int>{5});

		//----------
		apps_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::app_dereg_handler(pid_t proc_id)
	{
#ifdef DEBUG
		std::cout << "Decoded frames: " << decoded_frame_counter <<std::endl;
#endif
		controller.printTable();

		decoded_frame_counter = 0;
		//controller.printTable();
		// Remove registered app from vector of applications
		apps_m.lock();
		apps.erase(std::remove_if(apps.begin(), apps.end(), [=](pid_t app) {return app == proc_id;}), apps.end());
		apps_m.unlock();
		//**** Add your code here *****//
	}

	void rtm::knob_disc_reg_handler(pid_t proc_id, prime::api::app::knob_disc_t knob)
	{
		// Add discrete application knob to vector of discrete knobs
		app_knobs_disc_m.lock();
		app_knobs_disc.push_back(knob);
		app_knobs_disc_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::knob_cont_reg_handler(pid_t proc_id, prime::api::app::knob_cont_t knob)
	{
		// Add continuous application knob to vector of continuous knobs
		app_knobs_cont_m.lock();
		app_knobs_cont.push_back(knob);
		app_knobs_cont_m.unlock();

		//**** Add your code here *****//
	}


	void rtm::knob_disc_dereg_handler(prime::api::app::knob_disc_t knob)
	{
		// Remove discrete application knob from vector of discrete knobs
		app_knobs_disc_m.lock();
		app_knobs_disc.erase(
		std::remove_if(
		app_knobs_disc.begin(),
		app_knobs_disc.end(),
		[=](prime::api::app::knob_disc_t app_knob)
		{return app_knob.id == knob.id && app_knob.proc_id == knob.proc_id;}),
		app_knobs_disc.end());
		app_knobs_disc_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::knob_cont_dereg_handler(prime::api::app::knob_cont_t knob)
	{
		// Remove continuous application knob from vector of continuous knobs
		app_knobs_cont_m.lock();
		app_knobs_cont.erase(
		std::remove_if(
		app_knobs_cont.begin(),
		app_knobs_cont.end(),
		[=](prime::api::app::knob_cont_t app_knob)
		{return app_knob.id == knob.id && app_knob.proc_id == knob.proc_id;}),
		app_knobs_cont.end());
		app_knobs_cont_m.unlock();

		//**** Add your code here *****//
	}

	/* ================================================================================== */
	/* ------------------------------ Knob Change Handlers ------------------------------ */
	/* ================================================================================== */

	// Fast-lane knob change functions
	void rtm::knob_disc_min_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t min)
	{
		app_knobs_disc_m.lock();
		//Updates knob minimum in app_knobs_disc
		for(auto& app_knob : app_knobs_disc){
			if((app_knob.id == id) && (app_knob.proc_id == proc_id)){
				app_knob.min = min;
			}
		}
		app_knobs_disc_m.unlock();
	}

	void rtm::knob_disc_max_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t max)
	{
		app_knobs_disc_m.lock();
		//Updates knob maximum in app_knobs_disc
		for(auto& app_knob : app_knobs_disc){
			if((app_knob.id == id) && (app_knob.proc_id == proc_id)){
				app_knob.max = max;
			}
		}
		app_knobs_disc_m.unlock();
	}

	void rtm::knob_cont_min_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t min)
	{
		app_knobs_cont_m.lock();
		//Updates knob minimum in app_knobs_disc
		for(auto& app_knob : app_knobs_cont){
			if((app_knob.id == id) && (app_knob.proc_id == proc_id)){
				app_knob.min = min;
			}
		}
		app_knobs_cont_m.unlock();
	}

	void rtm::knob_cont_max_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t max)
	{
		app_knobs_cont_m.lock();
		//Updates knob maximum in app_knobs_disc
		for(auto& app_knob : app_knobs_cont){
			if((app_knob.id == id) && (app_knob.proc_id == proc_id)){
				app_knob.max = max;
			}
		}
		app_knobs_cont_m.unlock();
	}

	/* ================================================================================== */



	/* ================================================================================== */
	/* ------------------------- Monitor Registration Handlers -------------------------- */
	/* ================================================================================== */


	void rtm::mon_disc_reg_handler(pid_t proc_id, prime::api::app::mon_disc_t mon)
	{
		//Called when a discrete monitor is registered
		app_mons_disc_m.lock();
		if(mon.type == prime::api::app::PRIME_PERF && mon.proc_id == proc_id)
		{
#ifdef DEBUG
			std::cout << "\n\t PRIME_PERF FOUND" << std::endl;
#endif
			if(mon.weight >= perf_mon_weight){
				perf_mon_id = mon.id;
				perf_mon_weight = mon.weight;
				if(mon.min == 0)
				{
					perf_cont = mon.max;
				}
				else if(mon.max == 0)
				{
					perf_cont = mon.min;
				}
				else
				{
					perf_cont = ((mon.max + mon.min) / 2);
				}
#ifdef DEBUG
				std::cout << "\n\t PRIME_PERF ID: " << perf_mon_id << std::endl;
				std::cout << "\n\t PRIME_PERF WEIGHT: " << perf_mon_weight << std::endl;
				std::cout << "\n\t PRIME_PERF PERF: " << perf_cont << std::endl;
#endif
			}
		}


		app_mons_disc.push_back(mon);
		app_mons_disc_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon)
	{
		//Called when a continuous monitor is registered
		app_mons_cont_m.lock();
		if(mon.type == prime::api::app::PRIME_PERF && mon.proc_id == proc_id)
		{
#ifdef DEBUG
			std::cout << "\n\t PRIME_PERF FOUND" << std::endl;
#endif
			if(mon.weight >= perf_mon_weight){
				perf_mon_id = mon.id;
				perf_mon_weight = mon.weight;
				if(mon.min == 0)
				{
					perf_cont = mon.max;
				}
				else if(mon.max == 0)
				{
					perf_cont = mon.min;
				}
				else
				{
					perf_cont = ((mon.max + mon.min) / 2);
				}
#ifdef DEBUG
				std::cout << "\n\t PRIME_PERF ID: " << perf_mon_id << std::endl;
				std::cout << "\n\t PRIME_PERF WEIGHT: " << perf_mon_weight << std::endl;
				std::cout << "\n\t PRIME_PERF PERF: " << perf_cont << std::endl;
#endif
			}
		}
		app_mons_cont.push_back(mon);
		app_mons_cont_m.unlock();

		//**** Add your code here *****//
	}





	void rtm::mon_disc_dereg_handler(prime::api::app::mon_disc_t mon)
	{
		// Remove application monitor from vector of discrete application
		app_mons_disc_m.lock();
		app_mons_disc.erase(
		std::remove_if(
		app_mons_disc.begin(), app_mons_disc.end(),
		[=](prime::api::app::mon_disc_t app_mon)
		{return app_mon.id == mon.id && app_mon.proc_id == mon.proc_id;}),
		app_mons_disc.end());
		app_mons_cont_m.unlock();
	}

	void rtm::mon_cont_dereg_handler(prime::api::app::mon_cont_t mon)
	{
		// Remove application monitor from vector of continuous application
		app_mons_cont_m.lock();
		app_mons_cont.erase(
		std::remove_if(
		app_mons_cont.begin(), app_mons_cont.end(),
		[=](prime::api::app::mon_cont_t app_mon)
		{return app_mon.id == mon.id && app_mon.proc_id == mon.proc_id;}),
		app_mons_cont.end());
		app_mons_cont_m.unlock();
	}

	/* ================================================================================== */


	/* ================================================================================== */
	/* ---------------------------- Monitor Change Handlers ----------------------------- */
	/* ================================================================================== */

	// Fast-lane monitor change functions
	void rtm::mon_disc_min_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t min)
	{
		app_mons_disc_m.lock();
		//Updates monitor minimum in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.min = min;

			}
		}
		app_mons_disc_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_disc_max_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t max)
	{
		app_mons_disc_m.lock();
		//Updates monitor maximum in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.max = max;

			}
		}
		app_mons_disc_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_disc_weight_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t weight)
	{
		app_mons_disc_m.lock();
		//Updates monitor weight in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.weight = weight;

			}
		}
		app_mons_disc_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_disc_val_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t val)
	{
		app_mons_disc_m.lock();
		//Updates monitor value in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.val = val;
			}
		}
		app_mons_disc_m.unlock();

		//**** Add your code here *****//
	}


	void rtm::mon_cont_min_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t min)
	{
		app_mons_cont_m.lock();
		//Updates monitor minimum in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.min = min;

			}
		}
		app_mons_cont_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_cont_max_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t max)
	{
		app_mons_cont_m.lock();
		//Updates monitor maximum in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.max = max;
			}
		}
		app_mons_cont_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_cont_weight_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t weight)
	{
		app_mons_cont_m.lock();
		//Updates monitor weight in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.weight = weight;

			}
		}
		app_mons_cont_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::mon_cont_val_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t val)
	{
		app_mons_cont_m.lock();

		//Updates monitor value in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id) && (app_mon.id == perf_mon_id)){
				app_mon.val = val;
				run_rtm_loop(app_mon.val);
			}
		}
		app_mons_cont_m.unlock();
	}

	void rtm::run_rtm_loop(float d_t)
	{

#ifdef NOTRAIN
				//Static table size
				if(frame_count == 0)
				{
					controller = prime::codegen::qlearn_sc((unsigned int)((perf_cont / 1000) * 1000000), 60, 2056664);
				}
#endif

				frame_count++;
				decoded_frame_counter++;
				if(!ondemand){
					decode_time = d_t;
					average_time += decode_time;

					//std::cout << "decode time, " << decode_time << std::endl;

					unsigned int freq_choice = 2000000;
					unsigned int cycle_count_5 = 0;

					int skip_phase = 9;
					int frame_rate = 10;
					int numberOfRows = 8;

					if(skip_phase > frame_count)
					{
						cycle_count_5 = (unsigned int) dev_api.mon_disc_get(cycle_count_5_mon);
					}

					if(frame_count % frame_rate == 0)
					{
						if(frame_count < max_cpu_cycle_training)
						{

							//Find the maximum cycle count within the first max_cpu_cycle_training frames
							cycle_count_5 = (unsigned int) dev_api.mon_disc_get(cycle_count_5_mon) / frame_rate;

							if(frame_count > 250 && cycle_count_5 > max_cpu_cycle)
							{
								max_cpu_cycle = cycle_count_5;
							}
							if(frame_count > 250 && cycle_count_5 < min_cpu_cycle)
							{
								min_cpu_cycle = cycle_count_5;
							}
						}
#ifndef NOTRAIN
						// Build a table each time.
						else if(frame_count == max_cpu_cycle_training)
						{
							unsigned int length = (unsigned int)((max_cpu_cycle - min_cpu_cycle)/numberOfRows);
							//Calculate the number of rows based on the max_cpu_cycle collected in max_cpu_cycle_training
							int rowNum = (max_cpu_cycle / length) * 3;
							//0.040 should be replace by the value comming from arguments/app monitor
							controller = prime::codegen::qlearn_sc((unsigned int)((perf_cont / 1000) * 1000000), rowNum, length);
#ifdef DEBUG
							std::cout << "min: " << min_cpu_cycle << " max: " << max_cpu_cycle << " rowNum: " << rowNum << " length: " << length << std::endl;
#endif
							average_time = 0;

						}
#endif
						else if(frame_count > max_cpu_cycle_training){
							//RTM starts here!
							average_time = average_time/frame_rate;
							cycle_count_5 = (unsigned int) dev_api.mon_disc_get(cycle_count_5_mon) / frame_rate;
							controller.run(freq_choice, cycle_count_5 , average_time);
							dev_api.knob_disc_set(a15_frequency_knob, (freq_choice/100000)-2);
							//std::cout << "frame_count," << frame_count << ", Freq_Choice, " << freq_choice << ", average_time, " << average_time << std::endl;
							average_time = 0;

#ifdef DEBUGAPI
							std::cout << "\n\tFreq_Choice: " << freq_choice << std::endl;
#endif
						}

					}

				} else {
					// This is a nasty hack to get current frequency through the logger.
					if(frame_count % 10 == 0)
					{
						std::string cpu_path = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq";
						std::string freq_str;
						std::ifstream ifs(cpu_path);
						if(ifs.is_open()) {
							getline(ifs, freq_str);
							ifs.close();
						}
						dev_api.knob_disc_set(a15_frequency_knob, (std::stoi(freq_str)/100000)-2);
					}
				}


	}

	// Old JSON Functions
	void rtm::app_weight_handler(pid_t proc_id, prime::api::cont_t weight) {}


	/* ================================================================================== */


	void rtm::qlearn_rtm(unsigned int cc)
	{
		unsigned int freq_choice = 2000000;
		//rtm::filter_cpu_cycle(cc);
		//controller.run(freq_choice, currentCycle);


	}

	void rtm::filter_cpu_cycle(unsigned int cc)
	{
		if(currentCycle == 0 || (cc <= 60000000 && cc >= 20000000))
		{
			currentCycle = cc;
		}

	}

	int rtm::parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* app_addrs,
			prime::uds::socket_addrs_t* dev_addrs, prime::uds::socket_addrs_t* ui_addrs,
			prime::rtm::rtm_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser(rtm_name + " RTM.","PRiME Project\n");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::Flag ondemand_gov(optional, "ondemand", "Use Ondemand rather than the Q-Learning algorithm - useful for assessing performance", {'o', "ondemand"});
		//args::ValueFlag<float> frame_time(optional, "frame_time", "Target frame time", {'f', "frametime"});
		args::ValueFlag<int> max_cpu_cycle_training(optional, "max_cpu_cycle_training", "Number of performance monitor updates to carry out for training", {'t', "traintime"});

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

		UTIL_ARGS_LOGGER_PROC_RTM();

		/*if(frame_time) {
			args->frame_time = args::get(frame_time);
		} */

		if(max_cpu_cycle_training) {
			args->max_cpu_cycle_training = args::get(max_cpu_cycle_training);
		}

		args->ondemand = ondemand_gov;

		return 0;
	}
}


int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t rtm_app_addrs, rtm_dev_addrs, rtm_ui_addrs;
	prime::rtm::rtm_args_t rtm_args;
	if(prime::rtm::parse_cli("Codegen Q-Learning", &rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args, argc, argv)) {
		return -1;
	}

	prime::rtm RTM(&rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args);

	return 0;
}
