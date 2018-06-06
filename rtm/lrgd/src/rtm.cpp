/* This file is part of LRGD, an example RTM for the PRiME Framework.
 * 
 * LRGD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * LRGD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LRGD.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Charles Leech
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
#include <algorithm>
#include "uds.h"
#include "util.h"
#include "prime_api_rtm.h"
#include "rtm_arch_utils.h"
#include "args/args.hxx"
#include "regression.h"

#define NUM_CORES 8

namespace prime
{
	class rtm
	{
	public:
		struct rtm_args_t {
			bool ui_en = true;
		};

		typedef struct {
			std::vector<prime::api::cont_t> dev_pow_vals;
			unsigned int log_count;
			unsigned int app_mon_id;
		} mon_pow_log;

		static int parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* rtm_app_addrs,
			prime::uds::socket_addrs_t* rtm_dev_addrs, prime::uds::socket_addrs_t* rtm_ui_addrs,
			prime::rtm::rtm_args_t* args, int argc, const char * argv[]);

		rtm(prime::uds::socket_addrs_t* rtm_app_addrs, prime::uds::socket_addrs_t* rtm_dev_addrs,
			prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args);
		~rtm();
	private:
		//**** Variables ****//
		bool ui_en;

		prime::api::rtm::app_interface app_api;
		prime::api::rtm::dev_interface dev_api;
		prime::api::rtm::ui_interface ui_api;

		std::mutex dev_knobs_disc_m;
		std::mutex dev_knobs_cont_m;
		std::mutex dev_mons_disc_m;
		std::mutex dev_mons_cont_m;
		std::vector<prime::api::dev::knob_disc_t> dev_knobs_disc;
		std::vector<prime::api::dev::knob_cont_t> dev_knobs_cont;
		std::vector<prime::api::dev::mon_disc_t> dev_mons_disc;
		std::vector<prime::api::dev::mon_cont_t> dev_mons_cont;

		std::vector<prime::api::dev::knob_cont_t> dev_knobs_cont_dummy;

		std::mutex dev_knobs_freq_m;
		std::vector<prime::api::dev::knob_disc_t> dev_knobs_freq;
		std::vector<prime::api::dev::knob_disc_t> cpu_knobs_freq;
		std::vector<prime::api::dev::knob_disc_t> gpu_knobs_freq;

		std::vector<prime::api::dev::knob_disc_t> dev_knobs_gov;
		std::vector<prime::api::disc_t> dev_knobs_gov_init_val;
		std::vector<prime::api::dev::knob_disc_t> dev_knobs_freq_en;
		std::mutex dev_mons_temp_m;
		std::vector<prime::api::dev::mon_cont_t> dev_mons_temp;
		std::mutex dev_mons_pow_m;
		std::vector<prime::api::dev::mon_cont_t> dev_mons_power;
		std::vector<unsigned int> dev_models_trained;
		std::vector<unsigned int> app_models_trained;

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

		std::vector<prime::api::app::knob_disc_t> app_knobs_disc_par;
		std::vector<prime::api::app::knob_cont_t> app_knobs_cont_par;

		std::vector<mon_pow_log> app_mons_pow_log;

		std::mutex ui_rtm_stop_m;
		std::condition_variable ui_rtm_stop_cv;

		prime::regression rtm_lr;
		std::mutex avg_power_m, knobs_set_m;
		boost::thread power_log_thread;
		bool knobs_set, mon_set;

		//**** Functions ****//
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

		int parse_cli(int argc, const char *argv[]);
		void reg_dev(void);
		void dereg_dev(void);
		void randomise_model_knobs(void);
		void per_app_mon_power_logger(void);
		void global_power_logger(void);

		void opt_app_mon_disc_min(prime::api::app::mon_disc_t &app_mon);
		void opt_app_mon_disc_max(prime::api::app::mon_disc_t &app_mon);
		void opt_app_mon_cont_min(prime::api::app::mon_cont_t &app_mon);
		void opt_app_mon_cont_max(prime::api::app::mon_cont_t &app_mon);
		void set_model_knobs(std::vector<double> &knob_vals);
		std::vector<unsigned int> cores_to_affinity(prime::api::app::knob_disc_t knob);
	};

	rtm::rtm(prime::uds::socket_addrs_t* rtm_app_addrs, prime::uds::socket_addrs_t* rtm_dev_addrs,
			prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args) :
		ui_en(rtm_args->ui_en),
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
		rtm_lr(app_knobs_disc_par, app_knobs_cont_par, dev_knobs_freq, dev_knobs_cont_dummy, 20),
		knobs_set(true)
	{
		// move the RTM to core 0 so it has less impact on power consumption
		prime::util::rtm_set_affinity(getpid(), std::vector<unsigned int>{0});

		reg_dev();
		for(auto &dmp : dev_mons_power)
		{
			rtm_lr.add_dev_model(dmp.id);
			std::cout << "RTM: Created device model for cont monitor ID: " << dmp.id << std::endl;
		}

		ui_api.return_ui_rtm_start();

		//power_log_thread = boost::thread(&rtm::per_app_mon_power_logger, this);
		power_log_thread = boost::thread(&rtm::global_power_logger, this);

		if(!ui_en){
			power_log_thread.join();
		} else {
			std::cout << "RTM: Waiting before rtm stop lock" << std::endl;
			std::unique_lock<std::mutex> stop_lock(ui_rtm_stop_m);
			ui_rtm_stop_cv.wait(stop_lock);
			ui_api.return_ui_rtm_stop();
		}
		std::cout << "RTM: Stopped" << std::endl;
	}

	rtm::~rtm()	{}

	void rtm::ui_rtm_stop_handler(void)
	{
		power_log_thread.interrupt();
		dereg_dev();
		std::cout << "RTM: Device deregistered" << std::endl;
		ui_rtm_stop_cv.notify_one();
	}

	void rtm::reg_dev()
	{
		std::cout << "RTM: Registering device knobs & monitors" << std::endl;
		// Register device knobs and monitors
		dev_knobs_disc = dev_api.knob_disc_reg();
		dev_knobs_cont = dev_api.knob_cont_reg();
		dev_mons_disc = dev_api.mon_disc_reg();
		dev_mons_cont = dev_api.mon_cont_reg();

		std::cout << "RTM: Filtering out temperature monitors" << std::endl;
		for(auto dcm : dev_mons_cont){
			if(dcm.type == api::dev::PRIME_TEMP){
				dev_mons_temp.push_back(dcm);
				std::cout << "RTM: Found PRIME_TEMP monitor" << std::endl;
			}
			if(dcm.type == api::dev::PRIME_POW){
				dev_mons_power.push_back(dcm);
				std::cout << "RTM: Found PRIME_POW monitor" << std::endl;
			}
		}

		std::cout << "RTM: Filtering out frequency knobs" << std::endl;
		boost::property_tree::ptree arch = dev_api.dev_arch_get();
		auto num_fu = util::get_func_unit_ids(arch);

		for(auto fu_id : num_fu){
			auto dev_knobs_fu = util::get_disc_knob_func_unit(arch, dev_knobs_disc, fu_id);
			if(util::get_all_disc_knob(dev_knobs_fu, api::dev::PRIME_GOVERNOR).size())
			{
				for(auto ddk : dev_knobs_fu){
					if(ddk.type == api::dev::PRIME_FREQ){
						if(fu_id < 3){
							dev_knobs_freq.push_back(ddk);
							rtm_lr.add_knob();
						}
						cpu_knobs_freq.push_back(ddk);
						std::cout << "RTM: Found PRIME_FREQ knob under PRIME_GOVERNOR" << std::endl;
					}
				}
			}
			else if(util::get_all_disc_knob(dev_knobs_fu, api::dev::PRIME_FREQ_EN).size())
			{
				for(auto ddk : dev_knobs_fu){
					if(ddk.type == api::dev::PRIME_FREQ){
						//dev_knobs_freq.push_back(ddk);
						gpu_knobs_freq.push_back(ddk);
						std::cout << "RTM: Found PRIME_FREQ knob under PRIME_FREQ_EN" << std::endl;
					}
				}
			}
		}

		dev_knobs_gov = util::get_all_disc_knob(dev_knobs_disc, api::dev::PRIME_GOVERNOR);
		dev_knobs_freq_en = util::get_all_disc_knob(dev_knobs_disc, api::dev::PRIME_FREQ_EN);

		std::cout << "RTM: Setting Frequency Governors knobs to userspace" << std::endl;
		std::cout << "RTM: Number of Governor knobs: " << dev_knobs_gov.size() << std::endl;
		for(auto& dkg : dev_knobs_gov){
			dev_knobs_gov_init_val.push_back(dkg.val);
			dev_api.knob_disc_set(dkg, 0); // 0 = userspace
			dkg.val = 0;
			std::cout << "RTM: Governor knob id: " << dkg.id << " to: " << dkg.val << std::endl;
		}

		std::cout << "RTM: Initialising CPU frequency knobs" << std::endl;
		for(auto& dfk : cpu_knobs_freq){
			dev_api.knob_disc_set(dfk, dfk.max);
			dfk.val = dfk.max;
			std::cout << "RTM: Frequency knob id: " << dfk.id << " to: " << dfk.val << std::endl;
		}
	}

	void rtm::dereg_dev()
	{
		// Deregister device knobs and monitors
		dev_api.knob_disc_dereg(dev_knobs_disc);
		dev_api.knob_cont_dereg(dev_knobs_cont);
		dev_api.mon_disc_dereg(dev_mons_disc);
		dev_api.mon_cont_dereg(dev_mons_cont);
	}

	void rtm::per_app_mon_power_logger(void)
	{
		while(1)
		{
			while(app_mons_pow_log.size()) //log power if there is any app monitors to log it for
			{
				avg_power_m.lock();
				for(auto &app_log : app_mons_pow_log)
				{
					for(unsigned int dmp_idx = 0; dmp_idx < dev_mons_power.size(); dmp_idx++)
					{
						app_log.dev_pow_vals[dmp_idx] += dev_api.mon_cont_get(dev_mons_power[dmp_idx]); //read them so they are sent to the logger
					}
					//increment number of pow logs for app
					app_log.log_count++;
				}
				avg_power_m.unlock();

				try { boost::this_thread::sleep_for(boost::chrono::milliseconds(25)); }
				catch (boost::thread_interrupted&) {
					std::cout << "RTM: Power logging loop interrupted" << std::endl;
					return;
				}
			}
			try { boost::this_thread::sleep_for(boost::chrono::microseconds(1)); }
			catch (boost::thread_interrupted&) {
				std::cout << "RTM: Power logging loop interrupted" << std::endl;
				return;
			}
		}
	}

	void rtm::global_power_logger(void)
	{
        std::vector<prime::api::cont_t> dev_pow_vals = std::vector<prime::api::cont_t>(dev_mons_power.size(), 0);
        unsigned int log_count = 0;
        auto log_time = std::chrono::high_resolution_clock::now();

		while(1)
		{
			if((log_time + std::chrono::milliseconds(200)) < std::chrono::high_resolution_clock::now())
			{
				avg_power_m.lock();
				std::vector<prime::api::cont_t> avg_powers(dev_pow_vals.begin(), dev_pow_vals.end());
				std::transform(avg_powers.begin(), avg_powers.end(), avg_powers.begin(), std::bind2nd(std::divides<prime::api::cont_t>(), log_count));

				//reset logging variables
				std::fill(dev_pow_vals.begin(), dev_pow_vals.end(), 0);
				log_count = 0;
				avg_power_m.unlock();

				//TODO: Improve to build models with all dev monitors

//				for(unsigned int dmp_idx = 0; dmp_idx < dev_mons_power.size(); dmp_idx++)
//				{
//					rtm_lr.add_data(dev_mons_power[dmp_idx].id, avg_powers[dmp_idx]);
//
//					double pred_mon;
//					if(!rtm_lr.predict_mon(dev_mons_power[dmp_idx].id, &pred_mon)) {
//						std::cout << "Predicting power monitor ID: " << dev_mons_power[dmp_idx].id << ", val: " << pred_mon << std::endl;
//					}
//				}
				rtm_lr.add_data(dev_mons_power[0].id, avg_powers[0]);
#ifdef DEBUG_RTM
				double pred_mon;
				if(!rtm_lr.predict_mon(dev_mons_power[0].id, &pred_mon)) {
//					std::cout << "Power monitor update: ID = " << dev_mons_power[0].id << ", val = " << avg_powers[0] << ", prediction = " << pred_mon << std::endl;
				}
#endif // DEBUG_RTM

				log_time = std::chrono::high_resolution_clock::now();
			}

			avg_power_m.lock();
			for(unsigned int dmp_idx = 0; dmp_idx < dev_mons_power.size(); dmp_idx++)
			{
				dev_pow_vals[dmp_idx] += dev_api.mon_cont_get(dev_mons_power[dmp_idx]);
			}
			log_count++;
			avg_power_m.unlock();

			try {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(25));
			}
			catch (boost::thread_interrupted&) {
				std::cout << "RTM: Power logging loop interrupted" << std::endl;
				return;
			}
		}
	}

/* ================================================================================== */
/* ----------------------- Application Registration Handlers ------------------------ */
/* ================================================================================== */

	void rtm::app_reg_handler(pid_t proc_id, unsigned long int ur_id)
	{
		// Add registered app to vector of applications
		apps_m.lock();
		apps.push_back(proc_id);

		prime::api::app::knob_disc_t app_affinity_knob = {proc_id, app_api.get_unique_knob_id(),
			 (prime::api::app::knob_type_t)prime::api::app::PRIME_AFF, 1, NUM_CORES, 8};
		app_knobs_disc_par.push_back(app_affinity_knob);
		rtm_lr.add_knob();

		apps_m.unlock();
	}

	void rtm::app_dereg_handler(pid_t proc_id)
	{
		// Remove registered app from vector of applications
		apps_m.lock();
		apps.erase(std::remove_if(apps.begin(), apps.end(), [=](pid_t app) {return app == proc_id;}), apps.end());
		apps_m.unlock();
	}

/* ================================================================================== */
/* --------------------------- Knob Registration Handlers --------------------------- */
/* ================================================================================== */

	void rtm::knob_disc_reg_handler(pid_t proc_id, prime::api::app::knob_disc_t knob)
	{
		// Add discrete application knob to vector of discrete knobs
		app_knobs_disc_m.lock();
		if(knob.max == prime::api::PRIME_DISC_MAX) {
			knob.max = knob.val;
		}
		app_knobs_disc.push_back(knob);

		//TODO: extend to work other knob types
		if(knob.type == prime::api::app::PRIME_PAR)
		{
			app_knobs_disc_par.push_back(knob);
            rtm_lr.add_knob();
		}
		if(knob.type == prime::api::app::PRIME_DEV_SEL)
		{
			app_api.knob_disc_set(knob, knob.min);
		}
		app_knobs_disc_m.unlock();
	}

	void rtm::knob_cont_reg_handler(pid_t proc_id, prime::api::app::knob_cont_t knob)
	{
		// Add continuous application knob to vector of continuous knobs
		app_knobs_cont_m.lock();
		app_knobs_cont.push_back(knob);

		//TODO: test without if statement
		if(knob.type == prime::api::app::PRIME_PAR)
			app_knobs_cont_par.push_back(knob);
            rtm_lr.add_knob();

		app_knobs_cont_m.unlock();
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
	}

/* ================================================================================== */



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
		app_mons_disc.push_back(mon);

		rtm_lr.add_app_model(mon.id);
		std::cout << "RTM: Created application model for disc monitor ID: " << mon.id << std::endl;

//		avg_power_m.lock();
//		mon_pow_log app_log = mon_pow_log{std::vector<prime::api::cont_t>(dev_mons_power.size(), 0),0,mon.id};
//		app_mons_pow_log.push_back(mon_pow_log{std::vector<prime::api::cont_t>(dev_mons_power.size(), 0),0,mon.id});
//		avg_power_m.unlock();

		app_mons_disc_m.unlock();
	}

	void rtm::mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon)
	{
		//Called when a continuous monitor is registered
		app_mons_cont_m.lock();
		app_mons_cont.push_back(mon);

		rtm_lr.add_app_model(mon.id);
		std::cout << "RTM: Created application model for cont monitor ID: " << mon.id << std::endl;

//		avg_power_m.lock();
//		mon_pow_log app_log = mon_pow_log{std::vector<prime::api::cont_t>(dev_mons_power.size(), 0),0,mon.id};
//		app_mons_pow_log.push_back(mon_pow_log{std::vector<prime::api::cont_t>(dev_mons_power.size(), 0),0,mon.id});
//		avg_power_m.unlock();

		app_mons_cont_m.unlock();
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

		rtm_lr.remove_app_model(mon.id);

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

		rtm_lr.remove_app_model(mon.id);

		app_mons_cont_m.unlock();
	}

/* ================================================================================== */


/* ================================================================================== */
/* ---------------------------- Monitor Change Handlers ----------------------------- */
/* ================================================================================== */

	// Old JSON Functions
	void rtm::app_weight_handler(pid_t proc_id, prime::api::cont_t weight)
	{
		//**** Add your code here *****//
	}


	// Fast-lane monitor change functions
	void rtm::mon_disc_min_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t min)
	{
		app_mons_disc_m.lock();
		//Updates monitor minimum in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id))
			{
				app_mon.min = min;
				std::cout << "RTM: Application discrete monitor minimum updated. ID: " << id << ", min: " << min << std::endl;
				opt_app_mon_disc_min(app_mon);
			}
		}
		app_mons_disc_m.unlock();
	}

	void rtm::mon_disc_max_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t max)
	{
		app_mons_disc_m.lock();
		//Updates monitor maximum in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id))
			{
				app_mon.max = max;
				std::cout << "RTM: Application discrete monitor maximum updated. ID: " << id << ", max: " << max << std::endl;
				opt_app_mon_disc_max(app_mon);
			}
		}
		app_mons_disc_m.unlock();

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

		//TODO:
	}

	void rtm::mon_disc_val_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t val)
	{
		app_mons_disc_m.lock();
		//Updates monitor value in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.val = val;
				break;
			}
		}
		app_mons_disc_m.unlock();
	}


	void rtm::mon_cont_min_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t min)
	{
		app_mons_cont_m.lock();
		//Updates monitor minimum in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.min = min;

				std::cout << "RTM: Application continuous monitor minimum updated. ID: " << id << ", min: " << min << std::endl;
				opt_app_mon_cont_min(app_mon);
			}
		}
		app_mons_cont_m.unlock();
	}

	void rtm::mon_cont_max_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t max)
	{
		app_mons_cont_m.lock();
		//Updates monitor maximum in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.max = max;

				std::cout << "RTM: Application continuous monitor maximum updated. ID: " << id << ", max: " << max << std::endl;
				opt_app_mon_cont_max(app_mon);
			}
		}
		app_mons_cont_m.unlock();

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

		//TODO:
	}

	void rtm::mon_cont_val_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t val)
	{
		app_mons_cont_m.lock();
		//Update monitor value in rtm's copy of app_mons_cont
		prime::api::app::mon_cont_t *app_mon_ref;
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.val = val;
				app_mon_ref = &app_mon;
				break;
			}
		}

//		//Get a reference to the power log for this app monitor
//		mon_pow_log *this_pow_log;
//		for(auto& app_log : app_mons_pow_log){
//			if((app_log.mon_id == id)){
//				this_pow_log = &app_log;
//				break;
//			}
//		}
		app_mons_cont_m.unlock();

		if(app_mon_ref == nullptr){
			std::cout << "RTM: Error: app cont mon not found. ID: " << id << std::endl;
		}

//		//Collect average power measurements for last period
//		avg_power_m.lock();
//		std::vector<prime::api::cont_t> avg_powers(this_pow_log.dev_pow_vals->begin(), this_pow_log.dev_pow_vals->end());
//		std::transform(avg_powers.begin(), avg_powers.end(), avg_powers.begin(), _1 / dev_mons_power_count);
//
//		//reset logging variables
//		std::fill(this_pow_log.dev_pow_vals->begin(), this_pow_log.dev_pow_vals->end(), 0);
//		this_pow_log.log_count = 0;
//
//		avg_power_m.unlock();
//
//		//Need to update to target power models within app monitor models (needs heirarchical models)
//		for(auto& dcm : dev_cont_mon){
//			rtm_lr.add_data(dcm->id, avg_powers->val);
//		}


		//Only record sample if it occured after knobs were changed and more 1 monitor update occured
		knobs_set_m.lock();
		if(!knobs_set){
			knobs_set_m.unlock();
			return;
		} else {
			if (!mon_set){
				mon_set = true;
				knobs_set_m.unlock();
				return;
			}
		}
		knobs_set_m.unlock();

		//Update app model with new data from monitor
		rtm_lr.add_data(app_mon_ref->id, app_mon_ref->val);
		//randomise knobs if model is not trained
		double pred_mon;
		if(rtm_lr.predict_mon(app_mon_ref->id, &pred_mon)) {
			if(app_mon_ref->type == prime::api::app::PRIME_PERF)
				randomise_model_knobs();
		} else {
#ifdef DEBUG_RTM
			std::cout << "RTM: cont monitor update: ID: " << app_mon_ref->id << ", val: " << app_mon_ref->val << ", prediction: " << pred_mon << std::endl;
#endif // DEBUG_RTM

			//Optimise once after model trained for the first time.
			for(auto& amt_id : app_models_trained){
				if(amt_id == app_mon_ref->id){
					return;
				}
			}
			if(app_mon_ref->type == prime::api::app::PRIME_PERF)
			{
				opt_app_mon_cont_min(*app_mon_ref);
				app_models_trained.push_back(app_mon_ref->id);
			}
		}
	}

	void rtm::opt_app_mon_disc_min(prime::api::app::mon_disc_t &app_mon)
	{

		std::cout << "RTM: Optimising device power under app monitor min bound. ID: " << app_mon.id << ", min: " << app_mon.min << std::endl;

		std::vector<double> knob_vals;
		if(!rtm_lr.optimise_mon(dev_mons_power[0].id, app_mon.id, app_mon.min, knob_vals))
		{
			set_model_knobs(knob_vals);
		}
		else
		{
			app_models_trained.erase(
				std::remove_if(
					app_models_trained.begin(), app_models_trained.end(),
					[=](unsigned int amt_id)
					{return amt_id == app_mon.id;}),
				app_models_trained.end());
		}
	}

	void rtm::opt_app_mon_disc_max(prime::api::app::mon_disc_t &app_mon)
	{
		std::cout << "RTM: Optimising device power under app monitor max bound. ID: " << app_mon.id << ", max: " << app_mon.max << std::endl;

		std::vector<double> knob_vals;
		//TODO: implement gradient descent for optimising to a max
		if(!rtm_lr.optimise_mon(dev_mons_power[0].id, app_mon.id, app_mon.max, knob_vals))
		{
			set_model_knobs(knob_vals);
		}
		else
		{
			app_models_trained.erase(
				std::remove_if(
					app_models_trained.begin(), app_models_trained.end(),
					[=](unsigned int amt_id)
					{return amt_id == app_mon.id;}),
				app_models_trained.end());
		}
	}

	void rtm::opt_app_mon_cont_min(prime::api::app::mon_cont_t &app_mon)
	{
		std::cout << "RTM: Optimising device power under app monitor min bound. ID: " << app_mon.id << ", min: " << app_mon.min << std::endl;

		std::vector<double> knob_vals;
		if(!rtm_lr.optimise_mon(dev_mons_power[0].id, app_mon.id, app_mon.min, knob_vals))
		{
			set_model_knobs(knob_vals);
		}
		else
		{
			//app_models_trained.pop_back();
			app_models_trained.erase(
				std::remove_if(
					app_models_trained.begin(), app_models_trained.end(),
					[=](unsigned int amt_id)
					{return amt_id == app_mon.id;}),
				app_models_trained.end());
		}
	}

	void rtm::opt_app_mon_cont_max(prime::api::app::mon_cont_t &app_mon)
	{
		std::cout << "RTM: Optimising device power under app monitor max bound. ID: " << app_mon.id << ", max: " << app_mon.max << std::endl;

		std::vector<double> knob_vals;
		//TODO: implement gradient descent for optimising to a max
		if(!rtm_lr.optimise_mon(dev_mons_power[0].id, app_mon.id, app_mon.max, knob_vals))
		{
			set_model_knobs(knob_vals);
		}
		else
		{
			app_models_trained.erase(
				std::remove_if(
					app_models_trained.begin(), app_models_trained.end(),
					[=](unsigned int amt_id)
					{return amt_id == app_mon.id;}),
				app_models_trained.end());
		}
	}

	void rtm::randomise_model_knobs(void)
	{
		knobs_set_m.lock();
		knobs_set = false;
		knobs_set_m.unlock();

		app_knobs_disc_m.lock();
		for(auto& knob : app_knobs_disc_par) {
			if(knob.type == prime::api::app::PRIME_PAR){
				knob.val = knob.min + static_cast <int> (rand()) /( static_cast <int> (RAND_MAX/(knob.max - knob.min)));
				app_api.knob_disc_set(knob, knob.val);
			}
			else if(knob.type == prime::api::app::PRIME_AFF){
				knob.val = knob.min + static_cast <int> (rand()) /( static_cast <int> (RAND_MAX/(knob.max - knob.min)));
				util::rtm_set_affinity(knob.proc_id, cores_to_affinity(knob));
				//Hack to send affinity to logger:
				dev_api.knob_disc_set(prime::api::dev::knob_disc_t{666}, knob.val);
			}
		}
		app_knobs_disc_m.unlock();

		app_knobs_cont_m.lock();
		for(auto& knob : app_knobs_cont) {
			if(knob.type == prime::api::app::PRIME_PAR){
				knob.val = knob.min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(knob.max - knob.min)));
				app_api.knob_cont_set(knob, knob.val);
			}
		}
		app_knobs_cont_m.unlock();

		dev_knobs_disc_m.lock();
		for(auto& knob : dev_knobs_freq) {
			if(knob.type == prime::api::dev::PRIME_FREQ){
				knob.val = knob.min + static_cast <int> (rand()) /( static_cast <int> (RAND_MAX/(knob.max - knob.min)));
				dev_api.knob_disc_set(knob, knob.val);
			}
		}
		dev_knobs_disc_m.unlock();

		dev_knobs_cont_m.lock();
		for(auto& knob : dev_knobs_cont) {
			if(knob.type == prime::api::dev::PRIME_FREQ){
				knob.val = knob.min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(knob.max - knob.min)));
				dev_api.knob_cont_set(knob, knob.val);
			}
		}
		dev_knobs_cont_m.unlock();

		knobs_set_m.lock();
		knobs_set = true;
		mon_set = false;
		knobs_set_m.unlock();
	}

	void rtm::set_model_knobs(std::vector<double> &knob_vals)
	{
		int kvs_idx = 0;
		app_knobs_disc_m.lock();
		for(auto& knob : app_knobs_disc_par){
			knob.val = std::ceil(knob_vals[kvs_idx]);
			if(knob.type == prime::api::app::PRIME_AFF){
				util::rtm_set_affinity(knob.proc_id, cores_to_affinity(knob));
				//Hack to send affinity to logger:
				dev_api.knob_disc_set(prime::api::dev::knob_disc_t{666}, knob.val);
			} else {
				app_api.knob_disc_set(knob, knob.val);
			}
			kvs_idx++;
		}
		app_knobs_disc_m.unlock();
		app_knobs_cont_m.lock();
		for(auto& knob : app_knobs_cont_par){
			knob.val = knob_vals[kvs_idx];
			app_api.knob_cont_set(knob, knob.val);
			kvs_idx++;
		}
		app_knobs_cont_m.unlock();
		dev_knobs_disc_m.lock();
		for(auto& knob : dev_knobs_freq){
			knob.val = std::ceil(knob_vals[kvs_idx]);
			dev_api.knob_disc_set(knob, knob.val);
			kvs_idx++;
		}
		dev_knobs_disc_m.unlock();
		dev_knobs_cont_m.lock();
		for(auto& knob : dev_knobs_cont_dummy){
			knob.val = knob_vals[kvs_idx];
			dev_api.knob_cont_set(knob, knob.val);
			kvs_idx++;
		}
		dev_knobs_cont_m.unlock();
		return;
	}

	std::vector<unsigned int> rtm::cores_to_affinity(prime::api::app::knob_disc_t knob)
	{
		unsigned int idle_cores = knob.max - knob.val;
		std::vector<unsigned int> affinity_mask;

		while(idle_cores < knob.max)
		{
			affinity_mask.push_back(idle_cores);
			idle_cores++;
		}
		return affinity_mask;
	}

/* ================================================================================== */

	int rtm::parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* app_addrs,
			prime::uds::socket_addrs_t* dev_addrs, prime::uds::socket_addrs_t* ui_addrs,
			prime::rtm::rtm_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Linear Regression Runtime Manager.","PRiME Project\n");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
		args::Group required(parser, "All of these arguments are required:", args::Group::Validators::All);

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::Flag ui_off(optional, "ui_off", "Signal that the UI is not hosting the Application", {'u', "ui_off"});

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

		if(ui_off)
			args->ui_en = false;

		return 0;
	}
}

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t rtm_app_addrs, rtm_dev_addrs, rtm_ui_addrs;
	prime::rtm::rtm_args_t rtm_args;
	if(prime::rtm::parse_cli("Linear Regression", &rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args, argc, argv)) {
		return -1;
	}

	prime::rtm RTM(&rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args);

	return 0;
}
