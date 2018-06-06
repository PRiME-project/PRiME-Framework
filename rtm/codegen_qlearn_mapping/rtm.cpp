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
//#include <stdlib>
#include <thread>
#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "uds.h"
#include "util.h"
#include "prime_api_rtm.h"
#include "global_definitions.hpp"
#include "codegen.hpp"

namespace prime
{
	class rtm
	{
	public:
		rtm(double frame_time, int ondemand);
		~rtm();
	private:
		prime::api::rtm::app_interface app_api;
		prime::api::rtm::dev_interface dev_api;
		prime::api::rtm::ui_interface ui_api;
		prime::uds logger_socket;

		std::vector<prime::api::dev::knob_disc_t> knobs_disc;
		std::vector<prime::api::dev::knob_cont_t> knobs_cont;
		std::vector<prime::api::dev::mon_disc_t> mons_disc;
		std::vector<prime::api::dev::mon_cont_t> mons_cont;

		std::mutex apps_m;
		std::vector<pid_t> apps;
		std::mutex affinity_knobs_m;
		std::vector<prime::api::app::knob_disc_t> affinity_knobs;
		std::mutex app_mons_cont_m;
		std::vector<prime::api::app::mon_cont_t> app_mons_cont;

		std::mutex ui_dev_dereg_m;
		std::condition_variable ui_dev_dereg_cv;
		std::mutex ui_rtm_stop_m;
		std::condition_variable ui_rtm_stop_cv;

		prime::api::dev::knob_disc_t a15_frequency_knob;
		prime::api::dev::mon_disc_t cycle_count_4_mon;
		prime::api::dev::mon_disc_t cycle_count_5_mon;
		prime::api::dev::mon_cont_t temp_4_mon;
		prime::api::dev::mon_cont_t temp_5_mon;
		prime::api::dev::mon_cont_t power_mon;

		boost::thread rtm_thread;
		prime::codegen::qlearn_sc controller;
		unsigned int ondemand;

		void ui_rtm_stop_handler(void);
		void app_reg_handler(pid_t proc_id);
		void app_dereg_handler(pid_t proc_id);
		void knob_disc_reg_handler(pid_t proc_id, prime::api::app::knob_disc_t knob);
		void knob_cont_reg_handler(pid_t proc_id, prime::api::app::knob_cont_t knob);
		void knob_disc_change_handler(prime::api::app::knob_disc_t knob);
		void knob_cont_change_handler(prime::api::app::knob_cont_t knob);
		void knob_disc_dereg_handler(prime::api::app::knob_disc_t knob);
		void knob_cont_dereg_handler(prime::api::app::knob_cont_t knob);
		void mon_disc_reg_handler(pid_t proc_id, prime::api::app::mon_disc_t mon);
		void mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon);
		void mon_disc_change_handler(prime::api::app::mon_disc_t mon);
		void mon_cont_change_handler(prime::api::app::mon_cont_t mon);
		void mon_disc_dereg_handler(prime::api::app::mon_disc_t mon);
		void mon_cont_dereg_handler(prime::api::app::mon_cont_t mon);
		void app_weight_handler(pid_t proc_id, prime::api::cont_t weight);

		void reg_dev();
		void dereg_dev();
		void control_loop();
		void print_arch(boost::property_tree::ptree dev_arch);
	};

	rtm::rtm(double frame_time, int ondemand) :
		app_api(
			boost::bind(&rtm::app_reg_handler, this, _1),
			boost::bind(&rtm::app_dereg_handler, this, _1),
			boost::bind(&rtm::knob_disc_reg_handler, this, _1, _2),
			boost::bind(&rtm::knob_cont_reg_handler, this, _1, _2),
			boost::bind(&rtm::knob_disc_change_handler, this, _1),
			boost::bind(&rtm::knob_cont_change_handler, this, _1),
			boost::bind(&rtm::knob_disc_dereg_handler, this, _1),
			boost::bind(&rtm::knob_cont_dereg_handler, this, _1),
			boost::bind(&rtm::mon_disc_reg_handler, this, _1, _2),
			boost::bind(&rtm::mon_cont_reg_handler, this, _1, _2),
			boost::bind(&rtm::mon_disc_change_handler, this, _1),
			boost::bind(&rtm::mon_cont_change_handler, this, _1),
			boost::bind(&rtm::mon_disc_dereg_handler, this, _1),
			boost::bind(&rtm::mon_cont_dereg_handler, this, _1)
		),
		dev_api(),
		ui_api(
			boost::bind(&rtm::app_weight_handler, this, _1, _2),
			boost::bind(&rtm::dereg_dev, this),
			boost::bind(&rtm::ui_rtm_stop_handler, this)
		),
		logger_socket("/tmp/rtm.logger.uds", "/tmp/logger.uds"),
		ondemand(ondemand),
		controller((unsigned int)(frame_time * 1000000))
	{
		reg_dev();
		rtm_thread = boost::thread(&rtm::control_loop, this);

		std::unique_lock<std::mutex> dereg_lock(ui_dev_dereg_m);
		ui_dev_dereg_cv.wait(dereg_lock);

		rtm_thread.interrupt();
		rtm_thread.join();

		std::unique_lock<std::mutex> stop_lock(ui_rtm_stop_m);
		ui_rtm_stop_cv.wait(stop_lock);
		ui_api.return_ui_rtm_stop();
	}

	rtm::~rtm()	{}

	void rtm::ui_rtm_stop_handler(void)
	{
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

		unsigned int fu_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.id");
		unsigned int freq_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.freq.id");
		unsigned int full_id = dev_api.set_id(fu_id, 0, 0, freq_knob_id);

		// Iterate to match IDs of known knobs
		for(auto knob : knobs_disc){
			if(knob.id == full_id) a15_frequency_knob = knob;
		}

		unsigned int cpu4_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.sub_units.cpu4.id");
		unsigned int cpu5_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.sub_units.cpu5.id");
		unsigned int cycle_count_4_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.sub_units.cpu4.mons.cycle_count.id");
		unsigned int cycle_count_5_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.sub_units.cpu5.mons.cycle_count.id");
		unsigned int cycle_count_4_full_id = dev_api.set_id(fu_id, cpu4_id, 0, cycle_count_4_mon_id);
		unsigned int cycle_count_5_full_id = dev_api.set_id(fu_id, cpu5_id, 0, cycle_count_5_mon_id);

		// Iterate to match IDs of known monitors
		for(auto mon : mons_disc){
			if(mon.id == cycle_count_4_full_id) cycle_count_4_mon = mon;
			if(mon.id == cycle_count_5_full_id) cycle_count_5_mon = mon;
		}
		return;
	}

	void rtm::dereg_dev()
	{
		ui_dev_dereg_cv.notify_one();
		// Deregister device knobs and monitors
		dev_api.knob_disc_dereg(knobs_disc);
		dev_api.knob_cont_dereg(knobs_cont);
		dev_api.mon_disc_dereg(mons_disc);
		dev_api.mon_cont_dereg(mons_cont);
	}

	void rtm::control_loop()
	{
		bool freq_reset = false;
		while(1) {
			apps_m.lock();
			// Run RTM if apps registered > 0 and ondemand not chosen
			if(apps.size() && !ondemand) {
				unsigned int freq_choice = 2000000;
				unsigned int core_choice = 1;
				freq_reset = false;

				// Get cycle count values for both cores from device monitors
				unsigned int cycle_count_4 = (unsigned int) dev_api.mon_disc_get(cycle_count_4_mon);
				unsigned int cycle_count_5 = (unsigned int) dev_api.mon_disc_get(cycle_count_5_mon);

				// Choose maximum cycle count from both cores
				unsigned int cycle_count = (cycle_count_4 > cycle_count_5) ? cycle_count_4 : cycle_count_5;
				
				//Execution time ???????
				unsigned int exe_time = (rand() % 20 + 1) * 1000000; 
				
				// Run RTM providing cycle count and returning freq_choice
				controller.run(freq_choice, core_choice, cycle_count, exe_time);
                
				// Set A15 frequency using device knob
				dev_api.knob_disc_set(a15_frequency_knob, freq_choice);

				// Mapping 
				affinity_knobs_m.lock();
				for(auto aff_knob : affinity_knobs) {
						app_api.knob_disc_set(aff_knob, core_choice);
						aff_knob.val = core_choice;
				}
				affinity_knobs_m.unlock();
			}
			else {
				if(!freq_reset){
					dev_api.knob_disc_set(a15_frequency_knob, 200000);
					freq_reset = true;
				}
			}
			apps_m.unlock();

			try {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			}
			catch (boost::thread_interrupted&) {
				dev_api.knob_disc_set(a15_frequency_knob, 200000);
				return;
			}
		}
	}

	void rtm::app_reg_handler(pid_t proc_id)
	{
		// Add registered app to vector of applications
		apps_m.lock();
		apps.push_back(proc_id);
		apps_m.unlock();
	}

	void rtm::app_dereg_handler(pid_t proc_id)
	{
		// Remove registered app from vector of applications
		apps_m.lock();
		apps.erase(std::remove_if(apps.begin(), apps.end(), [=](pid_t app) {return app == proc_id;}), apps.end());
		apps_m.unlock();
	}

	void rtm::knob_disc_reg_handler(pid_t proc_id, prime::api::app::knob_disc_t knob)
	{
		// Add application affinity knob to vector of affinity knobs
		affinity_knobs_m.lock();
		if(knob.id == AFFINITY_BITMASK_ID) {
			affinity_knobs.push_back(knob);
		}
		affinity_knobs_m.unlock();
	}

	void rtm::knob_disc_dereg_handler(prime::api::app::knob_disc_t knob)
	{
		// Remove application affinity knob from vector of affinity knobs
		affinity_knobs_m.lock();
		if(knob.id == AFFINITY_BITMASK_ID) {
			affinity_knobs.erase(
				std::remove_if(
					affinity_knobs.begin(),
					affinity_knobs.end(),
					[=](prime::api::app::knob_disc_t aff_knob)
					{return aff_knob.id == knob.id && aff_knob.proc_id == knob.proc_id;}),
				affinity_knobs.end());
		}
		affinity_knobs_m.unlock();
	}

	// Unused event handlers
	void rtm::knob_cont_reg_handler(pid_t proc_id, prime::api::app::knob_cont_t knob) {}
	void rtm::knob_disc_change_handler(prime::api::app::knob_disc_t knob) {}
	void rtm::knob_cont_change_handler(prime::api::app::knob_cont_t knob) {}
	void rtm::knob_cont_dereg_handler(prime::api::app::knob_cont_t knob) {}
	void rtm::mon_disc_reg_handler(pid_t proc_id, prime::api::app::mon_disc_t mon) {}

	void rtm::mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon)
	{
		//Called when frame_decode_time_mon is registered
		app_mons_cont_m.lock();
		app_mons_cont.push_back(mon);
		app_mons_cont_m.unlock();
	}

	void rtm::mon_disc_change_handler(prime::api::app::mon_disc_t mon) {}
	void rtm::mon_cont_change_handler(prime::api::app::mon_cont_t mon)
	{
		app_mons_cont_m.lock();
		for(auto& app_mon : app_mons_cont){
			if(app_mon.id == mon.id){
				app_mon = mon;
			}
		}
		app_mons_cont_m.unlock();
	}
	void rtm::mon_disc_dereg_handler(prime::api::app::mon_disc_t mon) {}
	void rtm::mon_cont_dereg_handler(prime::api::app::mon_cont_t mon)
	{
		// Remove registered app monitor from vector of app_mons_cont
		app_mons_cont_m.lock();
		app_mons_cont.erase(
			std::remove_if(
				app_mons_cont.begin(), app_mons_cont.end(),
				[=](prime::api::app::mon_cont_t app_mon)
				{return app_mon.id == mon.id && app_mon.proc_id == mon.proc_id;}),
			app_mons_cont.end());
		app_mons_cont_m.unlock();
	}

	void rtm::app_weight_handler(pid_t proc_id, prime::api::cont_t weight) {}
}


int main(int argc, char * argv[])
{
	if(argc > 2)
		prime::rtm RTM(std::stod(argv[1], nullptr), std::stoi(argv[2], nullptr));
	else
		prime::rtm RTM(0.018, 0); //use default values if none are specified
	return 0;
}
