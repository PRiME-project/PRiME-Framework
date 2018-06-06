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

#define KNOB_LARGE_RANGE 8
#define KNOB_STEPS 10
#define NUM_APP_REPEATS 5
#define TEMPERATURE_STABILITY_THRESHOLD 2
#define TEMPERATURE_STABILITY_PERIOD 2500

namespace prime
{
	class rtm
	{
	public:
		struct rtm_args_t {
			int num_km;
			bool temp_en = false;
			bool power_en = false;
			bool ui_en = true;
		};

		static int parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* app_addrs,
			prime::uds::socket_addrs_t* dev_addrs, prime::uds::socket_addrs_t* ui_addrs,
			prime::rtm::rtm_args_t* args, int argc, const char * argv[]);

		rtm(prime::uds::socket_addrs_t* rtm_app_addrs, prime::uds::socket_addrs_t* rtm_dev_addrs,
			prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args);
		~rtm();
	private:
		//**** Variables ****//
		unsigned int num_km;
		bool ui_en;
		bool temp_all_stable, temp_en;
		bool power_en;

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
		std::vector<prime::api::app::knob_disc_t> app_dev_sel_knobs;
		bool dev_sel_knob_found;
		std::vector<unsigned int> app_knobs_disc_inc_val;

		std::mutex am_updated_m;
		std::vector<unsigned int> amc_updated;
		std::vector<unsigned int> amd_updated;

		std::mutex ui_rtm_stop_m;
		std::condition_variable ui_rtm_stop_cv;

		boost::thread profile_thread;
		boost::thread rtm_temperature_thread;
		std::mutex temp_bool_m;

		bool all_profiling_complete, end_profiling;
		bool cpu_profiling_complete, gpu_profiling_complete;
		bool dev_c5soc;

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

		void reg_dev(void);
		void dereg_dev(void);
		void profile_loop(void);
		void profile_setup(void);
		void set_cpu_knobs(void);
		void set_gpu_knobs(void);
		void temperature_watcher(void);
		void set_freq_ens(unsigned int set_val);
		void reset_freq_govs(void);
	};

	rtm::rtm(prime::uds::socket_addrs_t* rtm_app_addrs, prime::uds::socket_addrs_t* rtm_dev_addrs,
			prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args) :
		num_km(rtm_args->num_km),
		ui_en(rtm_args->ui_en),
		temp_en(rtm_args->temp_en),
		power_en(rtm_args->power_en),
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
		cpu_profiling_complete(false),
		gpu_profiling_complete(false),
		all_profiling_complete(false),
		end_profiling(false),
		temp_all_stable(false),
		dev_c5soc(false)
	{
		reg_dev();
		ui_api.return_ui_rtm_start();

		if(temp_en){
			std::cout << "RTM: Temperature Recording Enabled" << std::endl;
			std::cout << "RTM: Starting temperature watcher thread" << std::endl;
			rtm_temperature_thread = boost::thread(&rtm::temperature_watcher, this);
		} else {
			std::cout << "RTM: Temperature Recording Disabled" << std::endl;
		}

		if(power_en){
			std::cout << "RTM: Power Recording Enabled" << std::endl;
		} else {
			std::cout << "RTM: Power Recording Disabled" << std::endl;
		}

		std::cout << "RTM: Starting profile loop thread" << std::endl;
		profile_thread = boost::thread(&rtm::profile_setup, this);


		if(!ui_en){
			if(temp_en){
				rtm_temperature_thread.join();
			}
			profile_thread.join();
		} else {
			std::unique_lock<std::mutex> stop_lock(ui_rtm_stop_m);
			ui_rtm_stop_cv.wait(stop_lock);
			ui_api.return_ui_rtm_stop();
		}
		std::cout << "RTM: Stopped" << std::endl;
	}

	rtm::~rtm()	{}

	void rtm::ui_rtm_stop_handler(void)
	{
		profile_thread.interrupt();
		profile_thread.join();
		rtm_temperature_thread.interrupt();
		rtm_temperature_thread.join();
		std::cout << "RTM: Threads joined" << std::endl;
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

		std::string device_name = arch.get<std::string>("device.descriptor");
		std::cout << "RTM: Device descriptor is: " << device_name << std::endl;

		if(!device_name.compare("c5soc")){
			dev_c5soc = true;
		}


		auto num_fu = util::get_func_unit_ids(arch);

		for(auto fu_id : num_fu){
			auto dev_knobs_fu = util::get_disc_knob_func_unit(arch, dev_knobs_disc, fu_id);
			if(util::get_all_disc_knob(dev_knobs_fu, api::dev::PRIME_GOVERNOR).size())
			{
				for(auto ddk : dev_knobs_fu){
					if(ddk.type == api::dev::PRIME_FREQ){
						dev_knobs_freq.push_back(ddk);
						cpu_knobs_freq.push_back(ddk);
						std::cout << "RTM: Found PRIME_FREQ knob under PRIME_GOVERNOR" << std::endl;
						if(fu_id == 2)
							gpu_knobs_freq.push_back(ddk);
					}
				}
			}
			else if(util::get_all_disc_knob(dev_knobs_fu, api::dev::PRIME_FREQ_EN).size())
			{
				for(auto ddk : dev_knobs_fu){
					if(ddk.type == api::dev::PRIME_FREQ){
						dev_knobs_freq.push_back(ddk);
						gpu_knobs_freq.push_back(ddk);
						std::cout << "RTM: Found PRIME_FREQ knob under PRIME_FREQ_EN" << std::endl;
					}
				}
			}
			else
			{
				for(auto ddk : dev_knobs_fu){
					if(ddk.type == api::dev::PRIME_FREQ){
						dev_knobs_freq.push_back(ddk);
						std::cout << "RTM: Found PRIME_FREQ knob" << std::endl;
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

	void rtm::temperature_watcher(void)
	{
		std::vector<prime::api::cont_t> prev_temp(dev_mons_temp.size(), 0);
		std::vector<prime::api::cont_t> curr_temp(dev_mons_temp.size(), 0);
		std::vector<bool> temp_stable(dev_mons_temp.size(), false);
		int num_temp_mons = dev_mons_temp.size();
		bool loop_check_temp;
        int j = 0;

		while(1)
		{
			loop_check_temp = true;
			for(int i = 0; i < num_temp_mons; i++)
			{
				//check that temp reached steady state for each sensor
				dev_mons_temp_m.lock();
				curr_temp[i] = dev_api.mon_cont_get(dev_mons_temp[i]);
				dev_mons_temp_m.unlock();
				temp_stable[i] = abs(curr_temp[i] - prev_temp[i]) < TEMPERATURE_STABILITY_THRESHOLD ? true : false;
				//print out for debug
				//std::cout << "RTM: temp_mon " << i << " stable? " << temp_stable[i] << " diff = " << curr_temp[i] - prev_temp[i] << std::endl;
				prev_temp[i] = curr_temp[i];
				loop_check_temp &= temp_stable[i];
			}
            temp_bool_m.lock();
            if(loop_check_temp){
                temp_all_stable = true;
				//std::cout << "RTM: All temperature monitors are stable" << std::endl;
			}
			else{
                temp_all_stable = false;
			}
            temp_bool_m.unlock();
			try {
				boost::this_thread::sleep_for(boost::chrono::milliseconds(TEMPERATURE_STABILITY_PERIOD));
			}
			catch (boost::thread_interrupted&) {
				std::cout << "RTM: temperature watcher loop interrupted" << std::endl;
				return;
			}
		}
	}

	void rtm::profile_setup(void)
	{
        std::cout << "RTM: Waiting for application with " << num_km << " knobs and monitors to register" << std::endl;
        //block until app k/m fully registered
		while((app_knobs_disc.size() + app_knobs_cont.size() + app_mons_disc.size() + app_mons_cont.size()) < num_km)
		{
			try {
				boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted&) {
				std::cout << "RTM: Profiling loop interrupted" << std::endl;
				return;
			}
		}
		std::cout << "RTM: Detected application registered" << std::endl;
		std::cout << "RTM: Setting application affinity" << std::endl;
		if(apps.size())
		{
			//ODROID:
			//prime::util::rtm_set_affinity(apps[apps.size()-1], std::vector<unsigned int>{0,1,2,3,4,5,6,7}); //allow apps to use all the cpus
			//C5SOC:
			//prime::util::rtm_set_affinity(apps[apps.size()-1], std::vector<unsigned int>{0,1}); //allow apps to use all the cpus
		} else {
			std::cout << "RTM: Error: Trying to set affinity with no applications registered" << std::endl;
		}

		std::cout << "RTM: Resetting app knobs" << std::endl;
		for(auto& adk : app_knobs_disc)
		{
			if(adk.max == prime::api::PRIME_DISC_MAX) //only profile up to the init value of unbounded app knobs
			{
				adk.max = adk.val;
				std::cout << "RTM: Profiling unbounded adk "<< adk.id << " to " << adk.val << std::endl;
			}
			app_knobs_disc_inc_val.push_back(ceil((float)(adk.max - adk.min)/KNOB_STEPS));
			std::cout << "RTM: Incrementing adk " << adk.id << " by " << *(app_knobs_disc_inc_val.end()-1) << std::endl;

			app_api.knob_disc_set(adk, adk.min);
			std::cout << "RTM: Reset adk " << adk.id << " to: " << adk.min << std::endl;
			adk.val = adk.min;

			if(adk.type == prime::api::app::PRIME_DEV_SEL)
			{
				if(dev_c5soc){
					app_api.knob_disc_set(adk, adk.max);
				}

                std::cout << "RTM: Moving PRIME_DEV_SEL app knob" << std::endl;
                app_dev_sel_knobs.push_back(adk);
                dev_sel_knob_found = true;
                app_knobs_disc.erase(std::remove_if(
                        app_knobs_disc.begin(), app_knobs_disc.end(),
                        [=](prime::api::app::knob_disc_t app_knob) {return app_knob.id == adk.id;}),
                    app_knobs_disc.end());
			}
		}
		for(auto& ack : app_knobs_cont){
			if(ack.max == prime::api::PRIME_CONT_MAX) //only profile up to the init value of unbounded app knobs
				ack.max = ack.val;
			app_api.knob_cont_set(ack, ack.min);
			std::cout << "RTM: Reset ack " << ack.id << " to: " << ack.min << std::endl;
			ack.val = ack.min;
		}

		std::cout << "RTM: Profiling started" << std::endl;

        temp_bool_m.lock();
        temp_all_stable = false;
        temp_bool_m.unlock();

        profile_loop();
    }

	void rtm::profile_loop(void)
	{
        //Time dealy to ensure power measurements are taken for each operating point
        auto op_profile_time = std::chrono::high_resolution_clock::now();

		while(!end_profiling)
		{
			if(temp_en){
			//Block until all temp mon are stable
				while(!temp_all_stable){
					boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
				}
			}

			// Make app update all its monitors more than once so we know that the results were based on the new knobs
			for(int app_loops = 0; app_loops < NUM_APP_REPEATS+1; app_loops++)
			{
				am_updated_m.lock();
				amd_updated.clear();
				amc_updated.clear();
				am_updated_m.unlock();
				//Wait until all app monitors updated through mon_cont_change_handler
				while(app_mons_cont.size() + app_mons_disc.size() - amc_updated.size() - amd_updated.size())
				{
					try {
                        //boost::this_thread::interruption_point();
						boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
                    }
                    catch (boost::thread_interrupted&) {
                        std::cout << "RTM: Profiling loop interrupted" << std::endl;
                        return;
                    }
				}
			}
#ifdef DEBUG
			std::cout << "RTM: All app monitors updated" << std::endl;
#endif // DEBUG
			// If all the above happened faster than 256 milliseconds,
			// then wait to allow power measurement for this operating point
            std::this_thread::sleep_until(op_profile_time + std::chrono::duration<double>(256/1000));

			if(power_en)
			{
				dev_mons_pow_m.lock();
				for(auto dcm : dev_mons_power){
					dev_api.mon_cont_get(dcm); //read them so they are sent to the logger
				}
				dev_mons_pow_m.unlock();
			}

			if(dev_c5soc){
				set_gpu_knobs();
			} else {
				if(all_profiling_complete){
					reset_freq_govs();
					end_profiling = true; //should only get here once all operating points has been tried
					cpu_profiling_complete = false;
					gpu_profiling_complete = false;
				} else if(!cpu_profiling_complete){
					set_cpu_knobs();
				} else if(!gpu_profiling_complete){
					set_gpu_knobs();
				}
            }

            op_profile_time = std::chrono::high_resolution_clock::now();

            temp_bool_m.lock();
            temp_all_stable = false;
            temp_bool_m.unlock();

			try {
				boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted&) {
				std::cout << "RTM: Profiling loop interrupted" << std::endl;
				return;
			}
		}
		std::cout << "RTM: Profiling Complete" << std::endl;
	}

	void rtm::set_cpu_knobs(void)
	{
		//std::cout << "RTM: Setting CPU knobs" << std::endl;

		//Profile disc app knobs
		int adk_idx = 0;
		app_knobs_disc_m.lock();
		for(auto& adk : app_knobs_disc)
		{
			if(adk.val < adk.max)
			{
				//std::cout << "RTM: Setting adk..." << std::endl;
				if(adk.val < (adk.max - app_knobs_disc_inc_val[adk_idx]))
				{
					adk.val += app_knobs_disc_inc_val[adk_idx];
				}else{
					adk.val = adk.max;
				}
				app_api.knob_disc_set(adk, adk.val);
				std::cout << "RTM: Set adk " << adk.id << " to: " << adk.val << " (max = " << adk.max << ")" << std::endl;
				app_knobs_disc_m.unlock();
				return;
			}
			else {
				//std::cout << "RTM: Resetting adk..." << std::endl;
                app_api.knob_disc_set(adk, adk.min);
                adk.val = adk.min;
                std::cout << "RTM: Reset adk " << adk.id << " to: " << adk.val << " (max = " << adk.max << ")" << std::endl;
			}
			adk_idx++;
		}
		app_knobs_disc_m.unlock();

		//Profile disc app knobs
		app_knobs_cont_m.lock();
		for(auto& ack : app_knobs_cont){
			if(ack.val < ack.max){
				ack.val++;
				//std::cout << "RTM: Setting ack..." << std::endl;
				app_api.knob_cont_set(ack, ack.val);
				std::cout << "RTM: Set ack " << ack.id << " to: " << ack.val << " (max = " << ack.max << ")" << std::endl;
				app_knobs_cont_m.unlock();
				return;
			}
			else {
				//std::cout << "RTM: Setting ack..." << std::endl;
                app_api.knob_cont_set(ack, ack.min);
                ack.val = ack.min;
                std::cout << "RTM: Reset ack " << ack.id << " to: " << ack.val << " (max = " << ack.max << ")" << std::endl;
			}
		}
		app_knobs_cont_m.unlock();

		//Profile disc device knobs (only freq for odroid & not interested in cont ones)
		dev_knobs_disc_m.lock();
		for(auto& dkf : cpu_knobs_freq){
			if(dkf.val > dkf.min){
				dkf.val -= 2;
				//std::cout << "RTM: Setting dkf..." << std::endl;
                dev_api.knob_disc_set(dkf, dkf.val);
				dev_knobs_disc_m.unlock();
				std::cout << "RTM: Set dkf " << dkf.id << " to: " << dkf.val << " (max = " << dkf.max << ")" << std::endl;
				return;
			}
			else {
				//std::cout << "RTM: Resetting dkf..." << std::endl;
                dev_api.knob_disc_set(dkf, dkf.max);
                dkf.val = dkf.max;
                std::cout << "RTM: Reset dkf " << dkf.id << " to: " << dkf.val << " (max = " << dkf.max << ")" << std::endl;
			}
		}
		dev_knobs_disc_m.unlock();

		cpu_profiling_complete = true;

		reset_freq_govs();
		set_freq_ens(0);

		//Set jacobi DEV_SEL knob to GPU
		if(app_dev_sel_knobs.size() == 1){
            app_api.knob_disc_set(app_dev_sel_knobs[0], app_dev_sel_knobs[0].max);
            app_dev_sel_knobs[0].val = app_dev_sel_knobs[0].max;
			std::cout << "RTM: Set dev_sel_knob " << app_dev_sel_knobs[0].id << " to: " << app_dev_sel_knobs[0].val << " (max = " << app_dev_sel_knobs[0].max << ")" << std::endl;
        } else {
			std::cout << "RTM: No DEV_SEL knobs." << std::endl;
			all_profiling_complete = true;
        }

		sleep(1); //Delay to allow app to cycle once on GPU
	}

	void rtm::reset_freq_govs(void)
	{
		//Switch to GPU profling - need to generalise this better
		//Change cpu governors back to interactive
		std::cout << "RTM: Setting Frequency Governors knobs to interactive" << std::endl;
		int init_val_idx = 0;
		for(auto& dkg : dev_knobs_gov){
			dev_api.knob_disc_set(dkg, dev_knobs_gov_init_val[init_val_idx]);
			dkg.val = dev_knobs_gov_init_val[init_val_idx]; // 4 = interactive
			std::cout << "RTM: Governor knob id: " << dkg.id << " to: " << dkg.val << std::endl;
		}
	}

	void rtm::set_freq_ens(unsigned int set_val)
	{
		//Turn on control of GPU frequency
		std::cout << "RTM: Enabling frequency control on any non-Governor FUs" << std::endl;
		for(auto& dkfe : dev_knobs_freq_en){
			dev_api.knob_disc_set(dkfe, set_val); // 0 = dvfs disabled, i.e. userspace freq control enabled
			dkfe.val = 0;
			std::cout << "RTM: Set frequency enable knob id: " << dkfe.id << " to: " << dkfe.val << std::endl;
		}
		std::cout << "RTM: Initialising GPU frequency knobs" << std::endl;
		for(auto& dfk : gpu_knobs_freq){
			dev_api.knob_disc_set(dfk, dfk.max);
			dfk.val = dfk.max;
			std::cout << "RTM: Set frequency knob id: " << dfk.id << " to: " << dfk.val << std::endl;
		}
	}

	void rtm::set_gpu_knobs(void)
	{
		//std::cout << "RTM: Setting GPU knobs" << std::endl;

		//Profile disc app knobs
		int adk_idx = 0;
		app_knobs_disc_m.lock();
		for(auto& adk : app_knobs_disc){
			if(adk.val < adk.max)
			{
				//std::cout << "RTM: Setting adk..." << std::endl;
				if(adk.val < (adk.max - app_knobs_disc_inc_val[adk_idx]))
				{
					adk.val += app_knobs_disc_inc_val[adk_idx];
				}else{
					adk.val = adk.max;
				}
				app_api.knob_disc_set(adk, adk.val);
				std::cout << "RTM: Set adk " << adk.id << " to: " << adk.val << " (max = " << adk.max << ")" << std::endl;
				app_knobs_disc_m.unlock();
				return;
			}
			else {
				//std::cout << "RTM: Resetting adk..." << std::endl;
                app_api.knob_disc_set(adk, adk.min);
                adk.val = adk.min;
                std::cout << "RTM: Reset adk " << adk.id << " to: " << adk.min << " (max = " << adk.max << ")" << std::endl;
			}
			adk_idx++;
		}
		app_knobs_disc_m.unlock();

		//Profile disc app knobs
		app_knobs_cont_m.lock();
		for(auto& ack : app_knobs_cont){
			if(ack.val < ack.max){
				ack.val++;
				//std::cout << "RTM: Setting ack..." << std::endl;
				app_api.knob_cont_set(ack, ack.val);
				std::cout << "RTM: Set ack " << ack.id << " to: " << ack.val << " (max = " << ack.max << ")" << std::endl;
				app_knobs_cont_m.unlock();
				return;
			}
			else {
				//std::cout << "RTM: Setting ack..." << std::endl;
                app_api.knob_cont_set(ack, ack.min);
                ack.val = ack.min;
                std::cout << "RTM: Reset ack " << ack.id << " to: " << ack.min << " (max = " << ack.max << ")" << std::endl;
			}
		}
		app_knobs_cont_m.unlock();

		//Profile disc device knobs (only freq for odroid & not interested in cont ones)
		dev_knobs_disc_m.lock();
		for(auto& dkf : gpu_knobs_freq){
			if(dkf.val > dkf.min){
				dkf.val--;
				//std::cout << "RTM: Setting dkf..." << std::endl;
                dev_api.knob_disc_set(dkf, dkf.val);
				dev_knobs_disc_m.unlock();
				std::cout << "RTM: Set dkf " << dkf.id << " to: " << dkf.val << " (max = " << dkf.max << ")" << std::endl;
				return;
			}
			else {
				//std::cout << "RTM: Resetting dkf..." << std::endl;
                dev_api.knob_disc_set(dkf, dkf.max);
                dkf.val = dkf.max;
                std::cout << "RTM: Reset dkf " << dkf.id << " to: " << dkf.val << " (max = " << dkf.max << ")" << std::endl;
			}
		}
		dev_knobs_disc_m.unlock();

		gpu_profiling_complete = true;
		all_profiling_complete = true;

		set_freq_ens(1);
	}


/* ================================================================================== */
/* ----------------------- Application Registration Handlers ------------------------ */
/* ================================================================================== */

	void rtm::app_reg_handler(pid_t proc_id, unsigned long int ur_id)
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

		//Stop profiling
		profile_thread.interrupt();
		profile_thread.join();
		rtm_temperature_thread.interrupt();
	}


/* ================================================================================== */
/* --------------------------- Knob Registration Handlers --------------------------- */
/* ================================================================================== */

	void rtm::knob_disc_reg_handler(pid_t proc_id, prime::api::app::knob_disc_t knob)
	{
		// Add discrete application knob to vector of discrete knobs
		app_knobs_disc_m.lock();
		app_knobs_disc.push_back(knob);
		app_knobs_disc_m.unlock();
	}

	void rtm::knob_cont_reg_handler(pid_t proc_id, prime::api::app::knob_cont_t knob)
	{
		// Add continuous application knob to vector of continuous knobs
		app_knobs_cont_m.lock();
		app_knobs_cont.push_back(knob);
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
		app_mons_disc_m.unlock();
	}

	void rtm::mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon)
	{
		//Called when a continuous monitor is registered
		app_mons_cont_m.lock();
		app_mons_cont.push_back(mon);
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
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.min = min;
			}
		}
		app_mons_disc_m.unlock();
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
	}

	void rtm::mon_disc_val_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t val)
	{
		app_mons_disc_m.lock();
		//Updates monitor value in app_mons_disc
		for(auto& app_mon : app_mons_disc){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.val = val;

				am_updated_m.lock();
				if(std::find(amd_updated.begin(), amd_updated.end(), id) == amd_updated.end()){
					amd_updated.push_back(id);
				}
				am_updated_m.unlock();
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
	}

	void rtm::mon_cont_val_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t val)
	{
		app_mons_cont_m.lock();
		//Updates monitor value in app_mons_cont
		for(auto& app_mon : app_mons_cont){
			if((app_mon.id == id) && (app_mon.proc_id == proc_id)){
				app_mon.val = val;

				am_updated_m.lock();
				if(std::find(amc_updated.begin(), amc_updated.end(), id) == amc_updated.end()){
					amc_updated.push_back(id);
				}
				am_updated_m.unlock();
			}
		}
		app_mons_cont_m.unlock();
	}

/* ================================================================================== */

	int rtm::parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* app_addrs,
			prime::uds::socket_addrs_t* dev_addrs, prime::uds::socket_addrs_t* ui_addrs,
			prime::rtm::rtm_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Profiling Runtime Manager.","PRiME Project\n");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
		args::Group required(parser, "All of these arguments are required:", args::Group::Validators::All);
		args::ValueFlag<int> arg_num_km(required, "num_km", "Number of application knobs and monitors", {'n', "num_km"});

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::Flag arg_temp(optional, "temp", "Enable steady state temperature readings for each operating point", {'t', "temp"});
		args::Flag arg_power(optional, "power", "Enable power reading for each operating point", {'p', "power"});
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

		args->num_km = args::get(arg_num_km);
		args->temp_en = arg_temp;
		args->power_en = arg_power;

		if(ui_off)
			args->ui_en = false;

		return 0;
	}
}

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t rtm_app_addrs, rtm_dev_addrs, rtm_ui_addrs;
	prime::rtm::rtm_args_t rtm_args;
	if(prime::rtm::parse_cli("Profiler", &rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args, argc, argv)) {
		return -1;
	}

	prime::rtm RTM(&rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args);

	return 0;
}
