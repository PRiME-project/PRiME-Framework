/* This file is part of Thermal RTM, an example RTM for the PRiME Framework.
 * 
 * Thermal RTM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Thermal RTM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Thermal RTM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Eduardo Weber-Wachter
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
#include "thermal-rtm.hpp"
// extern "C" {
// #include "predictor.h"
// }
extern "C" {
#include "phaseprediction.h"
}
#include "rtm_arch_utils.h"
#include "args/args.hxx"
// #include "syst_Big.h"

#define DEBUG
#define DEBUG_DVFS

#ifdef DEBUG 
#define D(x) x
#else 
#define D(x)
#endif

#ifdef DEBUG_DVFS
#define DV(x) x
#else 
#define DV(x)
#endif

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

		std::mutex dev_knob_disc_m;
		std::mutex dev_knob_cont_m;
		std::mutex dev_mon_disc_m;
		std::mutex dev_mon_cont_m;
		std::vector<prime::api::dev::knob_disc_t> knobs_disc;
		std::vector<prime::api::dev::knob_cont_t> knobs_cont;
		std::vector<prime::api::dev::mon_disc_t> mons_disc;
		std::vector<prime::api::dev::mon_cont_t> mons_cont;

		//DPTM
		prime::api::dev::mon_disc_t L2Cache_RR_4_mon;
		prime::api::dev::mon_disc_t Instructions_4_mon;
		prime::api::dev::mon_disc_t L2Cache_RR_5_mon;
		prime::api::dev::mon_disc_t Instructions_5_mon;

		prime::api::dev::knob_disc_t L2Cache_RR_4_knob;
		prime::api::dev::knob_disc_t Instructions_4_knob;
		prime::api::dev::knob_disc_t L2Cache_RR_5_knob;
		prime::api::dev::knob_disc_t Instructions_5_knob;
		//DPTM

		//freq knobs
		std::vector<prime::api::dev::knob_disc_t> dev_knobs_gov;
		std::vector<prime::api::dev::knob_disc_t> dev_knobs_freq_en;
		//freq knobs

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

		std::mutex ui_dev_dereg_m;
		std::condition_variable ui_dev_dereg_cv;
		std::mutex ui_rtm_stop_m;
		std::condition_variable ui_rtm_stop_cv;

		prime::api::dev::knob_disc_t little_freq_knob;
		prime::api::dev::knob_disc_t big_freq_knob;

		prime::api::dev::knob_disc_t a15_frequency_knob;
		prime::api::dev::knob_disc_t pmc_cont_0_4_knob;
		prime::api::dev::knob_disc_t a15_governor_knob;
		
		prime::api::dev::mon_disc_t cycle_count_4_mon;
		prime::api::dev::mon_disc_t cycle_count_5_mon;
		prime::api::dev::mon_disc_t cycle_count_6_mon;
		prime::api::dev::mon_disc_t cycle_count_7_mon;
		prime::api::dev::mon_cont_t power_mon;

		boost::thread rtm_thread;

		//logger-dev
		std::mutex am_updated_m;
		std::vector<unsigned int> amc_updated;
		std::vector<unsigned int> amd_updated;

		boost::thread rtm_temperature_thread;
		std::mutex temp_bool_m;

		bool all_profiling_complete, end_profiling;
		bool cpu_profiling_complete, gpu_profiling_complete;
		//end logger-dev

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

		//Monitors of specific types
		std::vector<prime::api::dev::mon_cont_t> power_mons;
		std::vector<prime::api::dev::mon_cont_t> temp_mons;
		std::vector<prime::api::dev::mon_disc_t> cycle_count_mons;

		//Knobs of specific types
		std::vector<prime::api::dev::knob_cont_t> volt_knobs;
		std::vector<prime::api::dev::knob_disc_t> freq_knobs;


		/* ================================================================================== */
		/* -------------------------- DPTM -------------------------- */
		/* ================================================================================== */
			

			/////predictor
			double pred_temp, pred_temp_nt, power_l, power_b, power_gpu, power_mem;
			int New_Freq_L, New_Freq_B;

			int temp, prev_temp;

			double alphaC_b, alphaC_l;
			double leakage_b, leakage_l;

			double pred_error_1, pred_error_2;

			double prediction_error[BIG_FREQ_SIZE+1];
			///// END predictor


			int compute_max_freq(int Current_Freq_L, int Current_Freq_B, int freq_changed, int *Max_Freq_L, int *Max_Freq_B);

			double predicted_temp(double temp, double prev_temp, double power_b, double power_l, double power_gpu, double power_mem, int prev_freq_b, int prev_freq_l, int freq_b, int freq_l);

			void compute_alphaC(int prev_freq_l, int prev_freq_b);

			double repredict_temp(int prev_freq_b, int prev_freq_l, int freq_b, int freq_l);



			struct timeval start, end;

			/////adaptative dvfs big
			long double predicted_MRPI,actual_MRPI;
			// static double prediction_error; //DVFS_prediction_error
			long double DVFS_prediction_error;
			long double compensated_MRPI_Predicted, Perf_loss, IPS_New, IPS_Old,IPS_New_temp;
			int New_Freq;
			int Set_Frequency_Status, Check_No_Change;
			int Adapted_Freq;

			int idle;
			int freq_changed;

			int DVFS_Big(int *Current_Freq, int Temp_Max_Freq, int new_max_temp);
			int DVFS_LITTLE(int *Current_Freq, int Temp_Max_Freq);
			/////END adaptative dvfs big
			void run(unsigned int& New_Freq, double MRPI);

			/////adaptativeDVFS.c
			int Temp_Management_Number_Intervals;
			int Interval_Count;

			int Prev_LITTLE_Freq;
			int *Current_LITTLE_Freq;
			int *Max_LITTLE_Freq;

			int Prev_Big_Freq;
			int *Current_Big_Freq;
			int *Max_Big_Freq;
			/////adaptativeDVFS.c

		/* ================================================================================== */
		

		void reg_dev();
		void dereg_dev();
		void control_loop();

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
		temp_all_stable(false)
	{
		reg_dev();
		ui_api.return_ui_rtm_start();

		std::cout << "RTM: Starting thermal-rtm loop thread" << std::endl;
		// profile_thread = boost::thread(&rtm::profile_setup, this);
		rtm_thread = boost::thread(&rtm::control_loop, this);


		if(!ui_en){
			if(temp_en){
				rtm_temperature_thread.join();
			}
			rtm_thread.join();
		} else {
			std::unique_lock<std::mutex> stop_lock(ui_rtm_stop_m);
			ui_rtm_stop_cv.wait(stop_lock);
			ui_api.return_ui_rtm_stop();
		}
		std::cout << "RTM: Stopped" << std::endl;
	}
	//original thermal-rtm
	// {
	// 	reg_dev();
	// 	ui_api.return_ui_rtm_start();

	// 	rtm_thread = boost::thread(&rtm::control_loop, this);

	// 	std::unique_lock<std::mutex> stop_lock(ui_rtm_stop_m);
	// 	ui_rtm_stop_cv.wait(stop_lock);
	// 	ui_api.return_ui_rtm_stop();
	// }

	rtm::~rtm()	{}

	void rtm::ui_rtm_stop_handler(void)
	{
		rtm_thread.interrupt();
		rtm_thread.join();
		rtm_temperature_thread.interrupt();
		rtm_temperature_thread.join();
		std::cout << "RTM: Threads joined" << std::endl;
		dereg_dev();
		std::cout << "RTM: Device deregistered" << std::endl;
		ui_rtm_stop_cv.notify_one();
	}

/* ================================================================================== */
/* -------------------------- Device Registration Handlers -------------------------- */
/* ================================================================================== */

	void rtm::reg_dev()
	{

		/////////////////////////////
		// Register device knobs and monitors
		knobs_disc = dev_api.knob_disc_reg();
		knobs_cont = dev_api.knob_cont_reg();
		mons_disc = dev_api.mon_disc_reg();
		mons_cont = dev_api.mon_cont_reg();

		unsigned int global_power_mon_id;

		boost::property_tree::ptree dev_arch = dev_api.dev_arch_get();
		prime::util::print_architecture(dev_arch);


		//for DPTM

		/* ================================================================================== */
		/* -------------------------- DPTM -------------------------- */
		/* ================================================================================== */
			//TODO: use new utility functions

			// boost::property_tree::ptree dev_arch = dev_api.dev_arch_get();
			unsigned int fu_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.id");
			unsigned int su_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.id");
			unsigned int Freq_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.freq.id");
			unsigned int Freq_full_id = prime::util::set_id(fu_id, su_id, 0, Freq_knob_id);

			unsigned int cpu4_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.id");
			unsigned int L2Cache_RR_4_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_0.id");
			unsigned int Instructions_4_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_1.id");
			unsigned int L2Cache_RR_4_knob_full_id = prime::util::set_id(fu_id, cpu4_id, 0, L2Cache_RR_4_knob_id);
			unsigned int Instructions_4_knob_full_id = prime::util::set_id(fu_id, cpu4_id, 0, Instructions_4_knob_id);

			// unsigned int cpu5_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.id");
			// unsigned int L2Cache_RR_5_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_2.id");
			// unsigned int Instructions_5_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_1.id");
			// unsigned int L2Cache_RR_5_knob_full_id = prime::util::set_id(fu_id, cpu5_id, 0, L2Cache_RR_5_knob_id);
			// unsigned int Instructions_5_knob_full_id = prime::util::set_id(fu_id, cpu5_id, 0, Instructions_5_knob_id);

			fu_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.id");
			su_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.id");
			// unsigned int gov_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.governor.id");
			// unsigned int gov_full_id = prime::util::set_id(fu_id, su_id, 0, gov_knob_id);

			for(auto knob : knobs_disc)
			{
				if(knob.id == L2Cache_RR_4_knob_full_id) L2Cache_RR_4_knob = knob;
				// if(knob.id == L2Cache_RR_5_knob_full_id) L2Cache_RR_5_knob = knob;
				if(knob.id == Instructions_4_knob_full_id) Instructions_4_knob = knob;
				// if(knob.id == Instructions_5_knob_full_id) Instructions_5_knob = knob;
				// if(knob.id == Freq_full_id) a15_frequency_knob = knob;
				// if(knob.id == gov_full_id) a15_governor_knob = knob;
			}

			dev_api.knob_disc_set(L2Cache_RR_4_knob, 0x17);
			// dev_api.knob_disc_set(L2Cache_RR_5_knob, 0x17);
			dev_api.knob_disc_set(Instructions_4_knob, 0x08);
			// dev_api.knob_disc_set(Instructions_5_knob, 0x08);

			unsigned int L2Cache_RR_4_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.mons.pmc_0.id");
			unsigned int L2Cache_RR_4_mon_full_id = prime::util::set_id(fu_id, cpu4_id, 0, L2Cache_RR_4_mon_id);
			// unsigned int L2Cache_RR_5_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.mons.pmc_2.id");
			// unsigned int L2Cache_RR_5_mon_full_id = prime::util::set_id(fu_id, cpu5_id, 0, L2Cache_RR_5_mon_id);
			unsigned int Instructions_4_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.mons.pmc_1.id");
			unsigned int Instructions_4_mon_full_id = prime::util::set_id(fu_id, cpu4_id, 0, Instructions_4_mon_id);
			// unsigned int Instructions_5_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.mons.pmc_1.id");
			// unsigned int Instructions_5_mon_full_id = prime::util::set_id(fu_id, cpu5_id, 0, Instructions_5_mon_id);

			// Iterate to match IDs of known monitors
			for(auto mon : mons_disc)
			{
				if(mon.id == L2Cache_RR_4_mon_full_id) L2Cache_RR_4_mon = mon;
				// if(mon.id == L2Cache_RR_5_mon_full_id) L2Cache_RR_5_mon = mon;
				if(mon.id == Instructions_4_mon_full_id) Instructions_4_mon = mon;
				// if(mon.id == Instructions_5_mon_full_id) Instructions_5_mon = mon;
			}

			unsigned int L2Cache_RR_4 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_4_mon);
			std::cout << "L2Cache_RR_4: " << L2Cache_RR_4 << std::endl;
			unsigned int Instructions_4 = (unsigned int) dev_api.mon_disc_get(Instructions_4_mon);
			std::cout << "Instructions_4: " << Instructions_4 << std::endl;

			//end for DPTM
		/* ================================================================================== */

		std::vector<unsigned int> funcUnitIds;
		std::vector<unsigned int> subUnitIds;
		std::vector<prime::api::dev::mon_disc_t> su_disc_mons;
		std::vector<prime::api::dev::mon_cont_t> su_cont_mons;

		//get all cycle_count monitors for all sub units
		//get all fu ids
		funcUnitIds = prime::util::get_func_unit_ids(dev_arch);

		//get all sub units from all func units
		for(auto fu : funcUnitIds){
			subUnitIds = prime::util::get_sub_unit_ids(dev_arch, fu);
			for(auto su : subUnitIds){
				su_disc_mons = prime::util::get_disc_mon_sub_unit(dev_arch, mons_disc, fu, su);
				for(auto mon : su_disc_mons){
						std::cout << "disc mon: id: " << mon.id << ", type: " << mon.type << std::endl;
				}
				su_cont_mons = prime::util::get_cont_mon_sub_unit(dev_arch, mons_cont, fu, su);
				for(auto mon : su_cont_mons){
						std::cout << "cont mon: id: " << mon.id << ", type: " << mon.type << std::endl;
				}
			}
		}

		std::cout << "** Test Monitor Type Utility Functions **" << std::endl;

		//get all monitors
		cycle_count_mons = prime::util::get_all_disc_mon(mons_disc, prime::api::dev::PRIME_CYCLES);
		temp_mons = prime::util::get_all_cont_mon(mons_cont, prime::api::dev::PRIME_TEMP);
		power_mons = prime::util::get_all_cont_mon(mons_cont, prime::api::dev::PRIME_POW);

		std::cout << "PRIME_CYCLES Monitors: " << std::endl;
		for(auto mon : cycle_count_mons){
			std::cout << "id: " << mon.id << ", type: " << mon.type << std::endl;
		}

		std::cout << "PRIME_TEMP Monitors: " << std::endl;
		for(auto mon : temp_mons){
			std::cout << "id: " << mon.id << ", type: " << mon.type << std::endl;
		}

		std::cout << "PRIME_POW Monitors: " << std::endl;
		for(auto mon : power_mons){
			std::cout << "id: " << mon.id << ", type: " << mon.type << std::endl;
		}

		std::cout << "** Test Knob Type Utility Functions **" << std::endl;

		volt_knobs = prime::util::get_all_cont_knob(knobs_cont, prime::api::dev::PRIME_VOLT);
		freq_knobs = prime::util::get_all_disc_knob(knobs_disc, prime::api::dev::PRIME_FREQ);

		std::cout << "PRIME_VOLT Monitors: " << std::endl;
		for(auto knob : volt_knobs){
			std::cout << "id: " << knob.id << ", type: " << knob.type << ", min: " << knob.min << ", max: " << knob.max << std::endl;
		}

		std::cout << "PRIME_FREQ Monitors: " << std::endl;
		for(auto knob : freq_knobs){
			std::cout << "id: " << knob.id << ", type: " << knob.type << ", min: " << knob.min << ", max: " << knob.max << std::endl;
		}

		
		//FREQ KNOBS
		dev_knobs_gov = util::get_all_disc_knob(knobs_disc, api::dev::PRIME_GOVERNOR);
		dev_knobs_freq_en = util::get_all_disc_knob(knobs_disc, api::dev::PRIME_FREQ_EN);

		std::cout << "RTM: Setting Frequency Governors knobs to userspace" << std::endl;
		std::cout << "RTM: Number of Governor knobs: " << dev_knobs_gov.size() << std::endl;
		for(auto& dkg : dev_knobs_gov){
			// dev_knobs_gov_init_val.push_back(dkg.val);
			dev_api.knob_disc_set(dkg, 0); // 0 = userspace
			dkg.val = 0;
			std::cout << "RTM: Governor knob id: " << dkg.id << " to: " << dkg.val << std::endl;
		}

		std::cout << "RTM: Initialising CPU frequency knobs" << std::endl;
		for(auto& dfk : freq_knobs){
			dev_api.knob_disc_set(dfk, dfk.max);
			dfk.val = dfk.max;
			std::cout << "RTM: Frequency knob id: " << dfk.id << " to: " << dfk.val << std::endl;
		}

		little_freq_knob = freq_knobs[0];
		big_freq_knob = freq_knobs[1];
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

		std::cout << "RTM: Device Knobs and Monitors Deregistered" << std::endl;
	}
/* ================================================================================== */
	
	

/* ================================================================================== */
/* ---------------------------- App Registration Handlers --------------------------- */
/* ================================================================================== */

	void rtm::app_reg_handler(pid_t proc_id, unsigned long int ur_id)
	{
		// Add registered app to vector of applications
		apps_m.lock();
		apps.push_back(proc_id);
		apps_m.unlock();

		//**** Add your code here *****//
	}

	void rtm::app_dereg_handler(pid_t proc_id)
	{
		// Remove registered app from vector of applications
		apps_m.lock();
		apps.erase(std::remove_if(apps.begin(), apps.end(), [=](pid_t app) {return app == proc_id;}), apps.end());
		apps_m.unlock();

		//Stop profiling
		rtm_thread.interrupt();
		rtm_thread.join();
		rtm_temperature_thread.interrupt();
	}
/* ================================================================================== */
	
	

/* ================================================================================== */
/* --------------------------- Knob Registration Handlers --------------------------- */
/* ================================================================================== */

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
		
		//**** Add your code here *****//
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
		
		//**** Add your code here *****//
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
		
		//**** Add your code here *****//
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
		
		//**** Add your code here *****//
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

		//**** Add your code here *****//
	}

	void rtm::mon_cont_reg_handler(pid_t proc_id, prime::api::app::mon_cont_t mon)
	{
		//Called when a continuous monitor is registered
		app_mons_cont_m.lock();
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

				am_updated_m.lock();
				if(std::find(amd_updated.begin(), amd_updated.end(), id) == amd_updated.end()){
					amd_updated.push_back(id);
				}
				am_updated_m.unlock();
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
		
		//**** Add your code here *****//
	}
	
	// Old JSON Functions
	void rtm::app_weight_handler(pid_t proc_id, prime::api::cont_t weight) {}
	
	
/* ================================================================================== */
		
	double rtm::predicted_temp(double temp, double prev_temp, double power_b, double power_l, double power_gpu, double power_mem, int prev_freq_b, int prev_freq_l, int freq_b, int freq_l){
		
		pred_error_2 = pred_error_1;
		pred_error_1 = temp - pred_temp_nt;
				
		// std::cout << "pred_temp_nt: " << pred_temp_nt << std::endl;
		// std::cout << "pred_error_1: " << pred_error_1 << std::endl;
		// std::cout << "alphaC_b: " << alphaC_b << std::endl;
		// std::cout << "leakage_b: " << leakage_b << std::endl;
		// std::cout << "alphaC_l: " << alphaC_l << std::endl;
		// std::cout << "leakage_l: " << leakage_l << std::endl;

		pred_temp_nt = AR2*prev_temp + AR1*temp + POW_B*(alphaC_b*Frequency_Table_Big[freq_b]*pow(VDD_B[freq_b],2) + leakage_b) + POW_L*(alphaC_l*Frequency_Table_Little[freq_l]*pow(VDD_L[freq_l],2) + leakage_l) + POW_MEM*power_mem + POW_GPU*power_gpu;

		double local_pred_temp = INTERCEPT + pred_temp_nt + ARMA1*(pred_error_1 - INTERCEPT) + ARMA2*(pred_error_2 - INTERCEPT) + ERROR*prediction_error[freq_b];
			
		return local_pred_temp;	
	}

	void rtm::compute_alphaC(int prev_freq_l, int prev_freq_b){
		leakage_b = VDD_B[prev_freq_b]*C1_B*pow(temp,2)*exp(C2_B/temp);

		leakage_l = VDD_L[prev_freq_l]*C1_L*pow(temp,2)*exp(C2_L/temp);

		alphaC_b = (power_b - leakage_b)/(Frequency_Table_Big[prev_freq_b]*pow(VDD_B[prev_freq_b],2));
		alphaC_l = (power_l - leakage_l)/(Frequency_Table_Little[prev_freq_l]*pow(VDD_L[prev_freq_l],2));

	}

	double rtm::repredict_temp(int prev_freq_b, int prev_freq_l, int freq_b, int freq_l) {

		pred_temp = INTERCEPT + AR2*prev_temp + AR1*temp + POW_B*(alphaC_b*Frequency_Table_Big[freq_b]*pow(VDD_B[freq_b],2) + leakage_b) + POW_L*(alphaC_l*Frequency_Table_Little[freq_l]*pow(VDD_L[freq_l],2) + leakage_l) + POW_MEM*power_mem + POW_GPU*power_gpu +  ERROR*prediction_error[freq_b];

		return pred_temp;
	}

	int rtm::compute_max_freq(int Current_Freq_L, int Current_Freq_B, int freq_changed, int *Max_Freq_L, int *Max_Freq_B){

		// temp = (int) dev_api.mon_cont_get(temp_mons[2]);
		temp = (int) dev_api.mon_cont_get(temp_mons[0]);

			power_l = dev_api.mon_cont_get(power_mons[1]);
			power_b = dev_api.mon_cont_get(power_mons[2]);
			power_gpu = dev_api.mon_cont_get(power_mons[3]);
			power_mem = dev_api.mon_cont_get(power_mons[4]);

			New_Freq_L = 12;
			New_Freq_B = 18;

		if(temp > START_PREDICTION_TEMP){

			//Doesnt change prediction error if freq_changed from MRPI
			if(CONSIDER_TEMP_FEEDBACK && freq_changed == 0)
				prediction_error[New_Freq_B] = temp - pred_temp + prediction_error[New_Freq_B];
					
			New_Freq_L = 12;
			New_Freq_B = 18;

			compute_alphaC(Current_Freq_L, Current_Freq_B);

			// D(std::cout << "prev_temp:" << prev_temp << std::endl;)
			// D(std::cout << "temp:" << temp << std::endl;)

			pred_temp = predicted_temp(temp, prev_temp, power_b, power_l, power_gpu, power_mem, Current_Freq_B, Current_Freq_L, New_Freq_B, New_Freq_L);

			// D(std::cout << "temp: " << temp << std::endl;)
			// D(std::cout << "pred_temp: " << pred_temp << std::endl;)

			while(pred_temp > TEMP_TRESHOLD && New_Freq_B > 0) {
				New_Freq_B--;
				pred_temp = predicted_temp(temp, prev_temp, power_b, power_l, power_gpu, power_mem, Current_Freq_B, Current_Freq_L, New_Freq_B, New_Freq_L);
				// D(std::cout << "pred_temp: " << pred_temp << std::endl;)
				// D(std::cout << "pred_temp: " << pred_temp << std::endl;)
			}
			// D(std::cout << "---------------" << std::endl;)

			// if(CONSIDER_TEMP_FEEDBACK && TEMP_TRESHOLD - pred_temp >= 2 && New_Freq_B != BIG_FREQ_SIZE)
			// 	prediction_error[New_Freq_B + 1] = prediction_error[New_Freq_B];
		}
		else {
			New_Freq_L = 12;
			New_Freq_B = 18;
		}


		*Max_Freq_B=New_Freq_B;
		*Max_Freq_L=New_Freq_L;

		D(std::cout << "temp: " << temp;)
		D(std::cout << " pred_temp: " << pred_temp;)
		D(std::cout << " Max_Freq_B: " << *Max_Freq_B << std::endl;)

		prev_temp = temp;
		return 1;
	}

	
	// void rtm::control_loop()
	// {
	// 	unsigned int cycle_count[10], temp, pmc, pow, i, j;
	// 	unsigned long long timestamps[10];
		
	// 	i = 1;
	// 	j = 0;
		
	// 	/* ================================================================================== */
	// 	/* ------------------------- /////////////initialization ---------------------------- */
	// 	/* ================================================================================== */

	// 		// static int Temp_Management_Number_Intervals;
	// 		// static int Interval_Count;

	// 		// static int Prev_LITTLE_Freq;
	// 		// static int *Current_LITTLE_Freq;
	// 		// static int *Max_LITTLE_Freq;

	// 		// static int Prev_Big_Freq;
	// 		// static int *Current_Big_Freq;
	// 		// static int *Max_Big_Freq;

	// 		Current_LITTLE_Freq = (int *) malloc(sizeof(int));
	// 		Max_LITTLE_Freq = (int *) malloc(sizeof(int));

	// 		Current_Big_Freq = (int *) malloc(sizeof(int));
	// 		Max_Big_Freq = (int *) malloc(sizeof(int));

	// 		//malloc the two Max_Big_Freq

	// 		*Current_Big_Freq = BIG_FREQ_SIZE;
	// 		*Current_LITTLE_Freq = LITTLE_FREQ_SIZE;
	// 		*Max_Big_Freq = BIG_FREQ_SIZE;
	// 		*Max_LITTLE_Freq = LITTLE_FREQ_SIZE;
	// 		Prev_Big_Freq = BIG_FREQ_SIZE;
	// 		Prev_LITTLE_Freq = LITTLE_FREQ_SIZE;

	// 		for (int k = 0; k < BIG_FREQ_SIZE+1; k++){
	// 			prediction_error[k]=0;
	// 		}

	// 		pred_error_1 = 0;
	// 		pred_error_2 = 0;
	// 		pred_temp_nt = 0;


	// 		// void initialise_Big(){
	// 		Check_No_Change=0;
	// 		predicted_MRPI = 0;
	// 		IPS_Old = 0;
	// 		// prediction_error=0;
	// 		DVFS_prediction_error=0;
	// 		idle = 0;
	// 		//end  void initialise_Big(){

	// 		temp = (int) dev_api.mon_cont_get(temp_mons[2]);
	// 		prev_temp = temp;
			
	// 		if(CONSIDER_MRPI_POWER_MANAGEMENT && CONSIDER_TEMP_MANAGEMENT)
	// 			Temp_Management_Number_Intervals = (int) TEMP_MANAGEMENT_PERIOD/MRPI_SAMPLE_PERIOD;
	// 		else
	// 			Temp_Management_Number_Intervals = 1;

	// 		Interval_Count = 0;

	// 		//period of runing RTM
	// 		int sample_period;
	// 		if(CONSIDER_MRPI_POWER_MANAGEMENT)
	// 			sample_period = MRPI_SAMPLE_PERIOD;
	// 		else if(CONSIDER_TEMP_MANAGEMENT)
	// 			sample_period = TEMP_MANAGEMENT_PERIOD;
	// 		else
	// 			return;
	// 		struct timespec tim;
	// 		tim.tv_sec = (int) sample_period / 1000000000;
	// 		tim.tv_nsec = (int) sample_period % 1000000000;
	// 		//

	// 		std::cout << "Temp_Management_Number_Intervals:" << Temp_Management_Number_Intervals << std::endl;
	// 		std::cout << "sample_period:" << sample_period << std::endl;
	// 		std::cout << "tim.tv_sec:" << tim.tv_sec << std::endl;
	// 		std::cout << "tim.tv_nsec:" << tim.tv_nsec << std::endl;
	// 		std::cout << "prev_temp:" << prev_temp << std::endl;
	// 		std::cout << "temp:" << temp << std::endl;
	// 		std::cout << "=============" << std::endl;
	// 	/* ================================================================================== */

	// 	int new_max_temp = 0;
		
	// 	// big_freq_knob.val = dev_api.knob_disc_get(big_freq_knob);
	// 	// std::cout << "RTM: get big_freq_knob: " << big_freq_knob.val << std::endl;

	// 	// dev_api.knob_disc_set(big_freq_knob, 18);
	// 	// big_freq_knob.val = 18;
	// 	// std::cout << "RTM: big_freq_knob: " << big_freq_knob.val << std::endl;
	// 	// boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));

	// 	// dev_api.knob_disc_set(little_freq_knob, 12);
	// 	// little_freq_knob.val = 12;
	// 	// std::cout << "RTM: little_freq_knob: " << little_freq_knob.val << std::endl;
	// 	// boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
	// 	// for (int i = 0; i < 18; ++i){
	// 	// 	dev_api.knob_disc_set(big_freq_knob, 18-i);
	// 	// 	big_freq_knob.val = 18-i;
	// 	// 	std::cout << "RTM: big_freq_knob: " << big_freq_knob.val << std::endl;
	// 	// 	dev_api.mon_cont_get(temp_mons[2]);
	// 	// 	boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
	// 	// }

	// 	// dev_api.knob_disc_set(a15_governor_knob, 0); // 0 = userspace

	// 	// dev_api.knob_disc_set(a15_frequency_knob, 12);
	// 	// dev_api.knob_disc_set(little_freq_knob, 12);
	// 	// dev_api.knob_disc_set(big_freq_knob, 18);

	// 	unsigned long long timestamp = util::get_timestamp();

	// 	//while loop that encapsulate the power and temperature management
	// 	while(1) {
	// 		// std::cout << "temp cycle:" << util::get_timestamp() - timestamp << std::endl;
	// 		// timestamp = util::get_timestamp();

	// 		Prev_Big_Freq = big_freq_knob.val;
	// 		Prev_LITTLE_Freq = *Current_LITTLE_Freq;

	// 		if(CONSIDER_TEMP_MANAGEMENT && Interval_Count == 0){
	// 			// 	gettimeofday(&start, NULL);
	// 			compute_max_freq(little_freq_knob.val, big_freq_knob.val, freq_changed, Max_LITTLE_Freq, Max_Big_Freq);
	// 			// 	gettimeofday(&end, NULL);
	// 			Interval_Count = Temp_Management_Number_Intervals;
	// 			// 	freq_changed = 0;
	// 			new_max_temp = 1;
	// 		}

	// 		if(CONSIDER_MRPI_POWER_MANAGEMENT){
				
	// 			// D(std::cout << "big_freq_knob.val: " << big_freq_knob.val << std::endl;)
	// 			// D(std::cout << "Max_Big_Freq: " << *Max_Big_Freq << std::endl;)
	// 			// D(std::cout << "new_max_temp: " << new_max_temp << std::endl;)
	// 			int res;
	// 			res  = DVFS_Big(&big_freq_knob.val, *Max_Big_Freq, new_max_temp);
	// 				// DVFS_LITTLE(little_freq_knob.val, *Max_LITTLE_Freq);
	// 			if(res == 1){
	// 				freq_changed = 1;
	// 				// D(std::cout << "freq Big: " << big_freq_knob.val << std::endl;)
	// 				// D(std::cout << "actual_MRPI: " << actual_MRPI << std::endl;)
	// 				// D(std::cout << "IPS_New: " << IPS_New << std::endl;)
	// 			}

	// 			// 	//repredict the future temp based on the new frequencies
	// 				if(new_max_temp) {
	// 					repredict_temp(Prev_Big_Freq, Prev_LITTLE_Freq, *Current_Big_Freq, *Current_LITTLE_Freq);
	// 					new_max_temp = 0;
	// 				}
	// 		}
	// 		else {
	// 			// 	*Current_Big_Freq = *Max_Big_Freq;
	// 			// 	*Current_LITTLE_Freq = *Max_LITTLE_Freq;
	// 			// 	// modify_frequency(Frequency_Table_Big[*Current_Big_Freq], Frequency_Table_Little[*Current_LITTLE_Freq]);
	// 		}

	// 		Interval_Count--;

	// 		try {
	// 			// boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
	// 			// nanosleep(&tim, NULL);
	// 			std::this_thread::sleep_for(std::chrono::milliseconds(200));
	// 		}
	// 		catch (boost::thread_interrupted&) {
	// 			dev_api.knob_disc_set(a15_frequency_knob, 0);
	// 			std::cout << "RTM Control Loop Interrupted" << std::endl;

	// 			///DPTM
	// 			//Terminate PMUS and free memory of MRPI management
	// 			if(CONSIDER_MRPI_POWER_MANAGEMENT){
	// 				// finish_Big();
	// 				// finish_LITTLE();
	// 			}

	// 			//Free memory
	// 			free(Current_LITTLE_Freq);
	// 			Current_LITTLE_Freq = NULL;
	// 			free(Max_LITTLE_Freq);
	// 			Max_LITTLE_Freq = NULL;
	// 			free(Current_Big_Freq);
	// 			Current_Big_Freq = NULL;
	// 			free(Max_Big_Freq);
	// 			Max_Big_Freq = NULL;

	// 			///DPTM

	// 			return;
	// 		}
	// 	}
	// }


	void rtm::control_loop()
	{
		bool freq_reset = false;
		//   unsigned int mapping_idx = 0;

		//int Num_Apps=2;
		double MRPI;
		//int App_map=1;
		//	 FILE *fptrm;

		// dev_api.knob_disc_set(a15_governor_knob, 0);

		unsigned long long timestamp = util::get_timestamp();

		/////for temp predictor
		int *Max_Big_Freq;
		int *Max_LITTLE_Freq;
		Max_Big_Freq = (int *) malloc(sizeof(int));
		Max_LITTLE_Freq = (int *) malloc(sizeof(int));
		*Max_Big_Freq = BIG_FREQ_SIZE;
		*Max_LITTLE_Freq = LITTLE_FREQ_SIZE;
		if(CONSIDER_MRPI_POWER_MANAGEMENT && CONSIDER_TEMP_MANAGEMENT)
			Temp_Management_Number_Intervals = (int) TEMP_MANAGEMENT_PERIOD/MRPI_SAMPLE_PERIOD;
		else
			Temp_Management_Number_Intervals = 1;

		Interval_Count = 0;
		DVFS_prediction_error=0;
		idle = 0;

		temp = (int) dev_api.mon_cont_get(temp_mons[0]);
		prev_temp = temp;
		/////for temp predictor


		while(1)
		{
			// std::cout << "temp cycle:" << util::get_timestamp() - timestamp << std::endl;
			// timestamp = util::get_timestamp();
			
			//	dev_api.knob_disc_set(a15_governor_knob, 0);
			// Run RTM if apps registered > 0 and ondemand not chosen
			if(apps.size())
			{
				unsigned int New_Freq=200000;
				freq_reset = false;


				unsigned int L2Cache_RR_4 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_4_mon);

				unsigned int Instructions_4 = (unsigned int) dev_api.mon_disc_get(Instructions_4_mon);

				double MRPI_4=(double) L2Cache_RR_4/Instructions_4;

				MRPI=MRPI_4;

				run(New_Freq, MRPI);

				New_Freq = (New_Freq/100000)-2;
				std::cout <<"MRPI:- "<< MRPI << "Frequency:" << New_Freq << std::endl;


			if(CONSIDER_TEMP_MANAGEMENT && Interval_Count == 0){
				// 	gettimeofday(&start, NULL);
				compute_max_freq(little_freq_knob.val, big_freq_knob.val, freq_changed, Max_LITTLE_Freq, Max_Big_Freq);
				// 	gettimeofday(&end, NULL);
				Interval_Count = Temp_Management_Number_Intervals;
				// 	freq_changed = 0;
				// new_max_temp = 1;

				std::cout <<"MAX freq: "<< *Max_Big_Freq << std::endl;

			}
			Interval_Count--;

			if(New_Freq <= *Max_Big_Freq) {
				/*Set Operating Frequency of the Cores (KHz)*/
				if (New_Freq!=big_freq_knob.val){
					// Set_Frequency_Status = cpufreq_set_frequency(5,Frequency_Table_Big[New_Freq]);
					dev_api.knob_disc_set(big_freq_knob, New_Freq);
					big_freq_knob.val = New_Freq;
					freq_changed = 1;
				}
			}
			else {
				// Set_Frequency_Status= cpufreq_set_frequency(5,Frequency_Table_Big[Temp_Max_Freq]);
				dev_api.knob_disc_set(big_freq_knob, *Max_Big_Freq);
				big_freq_knob.val = *Max_Big_Freq;
				if(*Max_Big_Freq != big_freq_knob.val)
					freq_changed = 1;

			}

			// dev_api.knob_disc_set(big_freq_knob, New_Freq);

				apps_m.lock();
				int thread_set=1;
				if(thread_set){
					for(auto app : apps)
					{
						prime::util::rtm_set_affinity(app, std::vector<unsigned int> {4});
					  	thread_set=0;
					}
				}


				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				apps_m.unlock();
			}
			else
			{
				if(!freq_reset)
				{
					dev_api.knob_disc_set(a15_frequency_knob, 0);
					freq_reset = true;
					//std::cout<< "I am here" <<std::endl;
				}
			}

			try
			{
				boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
			}
			catch (boost::thread_interrupted&)
			{
				dev_api.knob_disc_set(a15_frequency_knob, 0);
				std::cout << "RTM Control Loop Interrupted" << std::endl;
				return;
			}
		}
	}


	int rtm::DVFS_Big(int *Current_Freq, int Temp_Max_Freq, int new_max_temp)
	{

		struct timespec tim2,tim3;
		freq_changed = 0;
		Check_No_Change=0;

		/*Adaptively changing the SAMPLE_PERIOD by checking last N frequencies of operation*/

		// if(Check_No_Change<SCALING_STOP_LENGTH && idle==0){
		if(Check_No_Change<SCALING_STOP_LENGTH){

			/*get perf data before workload prediction starts*/
			// actual_MRPI=Get_PMC_Data_Big();
			// IPS_New = Instructions_Big/MRPI_SAMPLE_PERIOD;

			//
			unsigned int L2Cache_RR_4 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_4_mon);
			unsigned int Instructions_4 = (unsigned int) dev_api.mon_disc_get(Instructions_4_mon);
			
			double actual_MRPI=(double) L2Cache_RR_4/Instructions_4;
			//IPS_New = Instructions_4/MRPI_SAMPLE_PERIOD;

			// D(std::cout << "actual_MRPI: " << actual_MRPI << std::endl;)
			// D(std::cout << "IPS_New: " << IPS_New << std::endl;)

			//

			/* Prediction error */
			if(CONSIDER_FEEDBACK) {
				// DVFS_prediction_error = actual_MRPI - predicted_MRPI;
				DVFS_prediction_error = actual_MRPI;
			}

			/*prediction starts from here*/
			predicted_MRPI=Predict_MRPI_Next_Period(predicted_MRPI,actual_MRPI);

			compensated_MRPI_Predicted = predicted_MRPI + COMPENSATE_ERROR*DVFS_prediction_error;

			// DV(std::cout << "predicted_MRPI: " << predicted_MRPI;)
			// DV(std::cout << " compensated: " << compensated_MRPI_Predicted;)
			// DV(std::cout << " Freq: " << *Current_Freq << std::endl;)

			/* Phase to Frequency Mapping : Selecting appropriate frequency of operation */

			// if(compensated_MRPI_Predicted>0.036){
			// 	New_Freq = 9;
			// }
			// else if(compensated_MRPI_Predicted>0.033 && compensated_MRPI_Predicted<=0.036){
			// 	New_Freq = 10;    }
			// else if(compensated_MRPI_Predicted>0.03 && compensated_MRPI_Predicted<=0.033){
			// 	New_Freq = 11;    }
			// else if(compensated_MRPI_Predicted>0.027 && compensated_MRPI_Predicted<=0.03){
			// 	New_Freq = 12;    }
			// else if(compensated_MRPI_Predicted>0.024 && compensated_MRPI_Predicted<=0.027){
			// 	New_Freq = 13;    }
			// else if(compensated_MRPI_Predicted>0.021 && compensated_MRPI_Predicted<=0.024){
			// 	New_Freq = 14;    }
			// else if(compensated_MRPI_Predicted>0.018 && compensated_MRPI_Predicted<=0.021){
			// 	New_Freq = 15;    }
			// else if(compensated_MRPI_Predicted>0.015 && compensated_MRPI_Predicted<=0.018){
			// 	New_Freq = 16;    }
			// else if(compensated_MRPI_Predicted>0.012 && compensated_MRPI_Predicted<=0.015){
			// 	New_Freq = 17;    }
			// else
			// 	New_Freq = 18;
			if (compensated_MRPI_Predicted>0.01)
			{
				New_Freq=4;
			}
			else if (compensated_MRPI_Predicted>0.009)
			{
				New_Freq=5;
			}
			else if (compensated_MRPI_Predicted>0.0083 && compensated_MRPI_Predicted<=0.009)
			{
				New_Freq=6;
			}
			else if (compensated_MRPI_Predicted>0.0074 && compensated_MRPI_Predicted<=0.0083)
			{
				New_Freq=7;
			}
			else if (compensated_MRPI_Predicted>0.0063 && compensated_MRPI_Predicted<=0.0074)
			{
				New_Freq=8;
			}
			else if (compensated_MRPI_Predicted>0.0057 && compensated_MRPI_Predicted<=0.0063)
			{
				New_Freq=9;
			}
			else if (compensated_MRPI_Predicted>0.0034 && compensated_MRPI_Predicted<=0.0057)
			{
				New_Freq=10;
			}
			else if (compensated_MRPI_Predicted>0.002 && compensated_MRPI_Predicted<=0.0034)
			{
				New_Freq=11;
			}
			else if (compensated_MRPI_Predicted>0.0015 && compensated_MRPI_Predicted<=0.002)
			{
				New_Freq=12;
			}
			else if (compensated_MRPI_Predicted>0.001 && compensated_MRPI_Predicted<=0.0015)
			{
				New_Freq=13;
			}
			else if (compensated_MRPI_Predicted>0.0095 && compensated_MRPI_Predicted<=0.001)
			{
				New_Freq=14;
			}
			else if (compensated_MRPI_Predicted>0.0009 && compensated_MRPI_Predicted<=0.0095)
			{
				New_Freq=15;
			}
			else if (compensated_MRPI_Predicted>0.00085 && compensated_MRPI_Predicted<=0.0009)
			{
				New_Freq=16;
			}
			else if (compensated_MRPI_Predicted>0.0008 && compensated_MRPI_Predicted<=0.00085)
			{
				New_Freq=17;
			}
			else
			{
				New_Freq=18;
			}
					// New_Freq=New_Freq-2;





			if(New_Freq <= Temp_Max_Freq) {
				/*Set Operating Frequency of the Cores (KHz)*/
				if (New_Freq!=*Current_Freq){
					// Set_Frequency_Status = cpufreq_set_frequency(5,Frequency_Table_Big[New_Freq]);
					dev_api.knob_disc_set(big_freq_knob, New_Freq);
					big_freq_knob.val = New_Freq;
					*Current_Freq=New_Freq;
					freq_changed = 1;
					// Check_No_Change--;
					// if(Check_No_Change==-1*(SCALING_STOP_LENGTH))
					// 	Check_No_Change=0;
				}
				// else{
				// 	Check_No_Change++;
				// }

				/*Workload modelling and performanace tuning*/
				// IPS_New_temp+=IPS_New*0.01;

				// if(IPS_New_temp<IPS_Old){
				// 	Perf_loss=(IPS_Old-IPS_New)/IPS_Old;
				// 	/* compensate the performance loss by stepping up the core frequency*/
				// 	Adapted_Freq = Frequency_Table_Big[New_Freq]+Perf_loss*2000000;
				// 	IPS_Old=IPS_New+Perf_loss*IPS_Old;

				// 	if(Adapted_Freq <= Frequency_Table_Big[Temp_Max_Freq]){
				// 		// Set_Frequency_Status= cpufreq_set_frequency(5,Adapted_Freq);
				// 		dev_api.knob_disc_set(big_freq_knob, Adapted_Freq);
				// 		big_freq_knob.val = Adapted_Freq;
				// 	}
				// }
			}
			else {
				// Set_Frequency_Status= cpufreq_set_frequency(5,Frequency_Table_Big[Temp_Max_Freq]);
				dev_api.knob_disc_set(big_freq_knob, Temp_Max_Freq);
				big_freq_knob.val = Temp_Max_Freq;
				if(Temp_Max_Freq != *Current_Freq)
					freq_changed = 1;

				*Current_Freq = Temp_Max_Freq;
			}
		// }
		// else if(idle > 0) {
		// 	if(*Current_Freq > Temp_Max_Freq) {
		// 		// cpufreq_set_frequency(5,Frequency_Table_Big[Temp_Max_Freq]);
		// 		dev_api.knob_disc_set(big_freq_knob, Temp_Max_Freq);
		// 		big_freq_knob.val = Temp_Max_Freq;
		// 		*Current_Freq = Temp_Max_Freq;
		// 		freq_changed = 1;
		// 	}
		// 	idle--;
		}
		// Stop monitoring if last N (SCALING_STOP_LENGTH) frequencies are same
		else{
			long double temp1;
			temp1=(long double)(MRPI_SAMPLE_PERIOD/1000000000);
			temp1=temp1*SCALING_STOP_LENGTH;
			tim2.tv_sec=(int)(temp1);
			if(tim2.tv_sec==0)
			{
				tim2.tv_nsec=(int)(3*MRPI_SAMPLE_PERIOD);
			}
			else
			{
				tim2.tv_nsec=0;
			}
			nanosleep(&tim2,&tim3);
			Check_No_Change=0;
		
			// idle = 3;
			// Check_No_Change=0;

			// if(*Current_Freq > Temp_Max_Freq) {
			// 	// cpufreq_set_frequency(5,Frequency_Table_Big[Temp_Max_Freq]);
			// 	dev_api.knob_disc_set(big_freq_knob, Temp_Max_Freq);
			// 	big_freq_knob.val = Temp_Max_Freq;
			// 	*Current_Freq = Temp_Max_Freq;
			// 	freq_changed = 1;
			// }
		}

		DV(std::cout << "MRPI: " << predicted_MRPI;)
		DV(std::cout << " Freq: " << *Current_Freq << std::endl;)

		if(new_max_temp)
			return 0;
		else
			return freq_changed;
	}

	void rtm::run(unsigned int& New_Freq, double MRPI)
	{
		//struct timeval  tv1, tv2;
		//gettimeofday(&tv1, NULL);
		static double predicted_MRPI,actual_MRPI;
		double prediction_error=0;
		static double compensated_MRPI_Predicted, Perf_loss, IPS_New, IPS_Old=0,IPS_New_temp;
		static unsigned int Current_Freq;
		// const unsigned int MRPI_Frequency_Table_Big[]= {1100000,1200000,1300000,1400000,1500000,1600000,1700000,1800000,1900000,2000000};
		//Modified for video decode app
		//const unsigned int MRPI_Frequency_Table_Big[]= {1400000,1500000,1600000,1700000,1800000,1900000,2000000};
		const unsigned int MRPI_Frequency_Table_Big[]= {600000,700000,800000,900000,1000000,1100000,1200000,1300000,1400000,1500000,1600000,1700000,1800000,1900000,2000000};

		static int Set_Frequency_Status, Check_No_Change=0;
		/*defining required time parameters*/
		struct timespec tim2,tim3;
		// tim.tv_sec = 0;
		// tim.tv_nsec = SAMPLE_PERIOD;*/
		predicted_MRPI = 0;
		/*get current core operating frequency*/
		// Current_Freq = cpufreq_get_freq_hardware(5);
		//if(Current_Freq!=200000)
		//  Set_Frequency_Status = cpufreq_set_frequency(5,2000000);

		//FILE *fptr;
		// FILE *fptrf;
		// fptr = fopen("MRPI_B.txt","a");
		// fptrf = fopen("Freq_B.txt","a");
		//PMU_initialize();*/
		//while(1)
		//{
		/*Adaptively changing the SAMPLE_PERIOD by checking last N frequencies of operation*/
		//struct timeval  tv1, tv2;
		//gettimeofday(&tv1, NULL);
		if(Check_No_Change<SCALING_STOP_LENGTH)
		{
			actual_MRPI=MRPI;
			//actual_MRPI=Get_PMC_Data_Big();
			//printf("MRPI=%Lf\n",actual_MRPI); //actual_MRPI = L2D_Cache_Refill/Instructions
			// IPS_New = Instructions/SAMPLE_PERIOD;

			/* Prediction error */
			if(CONSIDER_FEEDBACK)
			{
				prediction_error = actual_MRPI - predicted_MRPI;
			}
			/*prediction starts from here*/
			//printf("B_prediction=%Lf\n",predicted_MRPI);
			predicted_MRPI=Predict_MRPI_Next_Period(predicted_MRPI,actual_MRPI);
			//std::cout<<"Predicted_MRPI"<<predicted_MRPI<<std::endl;
			/*prediction error calculation and feedback*/
			/*
					if(CONSIDER_FEEDBACK){
						prediction_error = actual_MRPI - predicted_MRPI;
					//  printf("Error=%Lf\n",prediction_error);
					}*/
			compensated_MRPI_Predicted = predicted_MRPI + COMPENSATE_ERROR*prediction_error;


			//Modified for video decode app
						if (compensated_MRPI_Predicted>0.01)
						{
							New_Freq=MRPI_Frequency_Table_Big[0];
						}
						else if (compensated_MRPI_Predicted>0.009)
						{
							New_Freq=MRPI_Frequency_Table_Big[1];
						}
						else if (compensated_MRPI_Predicted>0.0083 && compensated_MRPI_Predicted<=0.009)
						{
							New_Freq=MRPI_Frequency_Table_Big[2];
						}
						else if (compensated_MRPI_Predicted>0.0074 && compensated_MRPI_Predicted<=0.0083)
						{
							New_Freq=MRPI_Frequency_Table_Big[3];
						}
						else if (compensated_MRPI_Predicted>0.0063 && compensated_MRPI_Predicted<=0.0074)
						{
							New_Freq=MRPI_Frequency_Table_Big[4];
						}
						else if (compensated_MRPI_Predicted>0.0057 && compensated_MRPI_Predicted<=0.0063)
						{
							New_Freq=MRPI_Frequency_Table_Big[5];
						}
						else if (compensated_MRPI_Predicted>0.0034 && compensated_MRPI_Predicted<=0.0057)
						{
							New_Freq=MRPI_Frequency_Table_Big[6];
						}
						else if (compensated_MRPI_Predicted>0.002 && compensated_MRPI_Predicted<=0.0034)
						{
							New_Freq=MRPI_Frequency_Table_Big[7];
						}
					else if (compensated_MRPI_Predicted>0.0015 && compensated_MRPI_Predicted<=0.002)
						{
							New_Freq=MRPI_Frequency_Table_Big[8];
						}
					else if (compensated_MRPI_Predicted>0.001 && compensated_MRPI_Predicted<=0.0015)
						{
							New_Freq=MRPI_Frequency_Table_Big[9];
						}
					else if (compensated_MRPI_Predicted>0.0095 && compensated_MRPI_Predicted<=0.001)
						{
							New_Freq=MRPI_Frequency_Table_Big[10];
						}
					else if (compensated_MRPI_Predicted>0.0009 && compensated_MRPI_Predicted<=0.0095)
					{
					New_Freq=MRPI_Frequency_Table_Big[11];
					}
					else if (compensated_MRPI_Predicted>0.00085 && compensated_MRPI_Predicted<=0.0009)
					{
					New_Freq=MRPI_Frequency_Table_Big[12];
					}
					else if (compensated_MRPI_Predicted>0.0008 && compensated_MRPI_Predicted<=0.00085)
					{
					New_Freq=MRPI_Frequency_Table_Big[13];
					}
						else
						{
							New_Freq=MRPI_Frequency_Table_Big[14];
						}
			
		}

		/*Stop monitoring if last N (SCALING_STOP_LENGTH) frequencies are same*/

		else
		{
			long double temp1;
			temp1=(long double)(SAMPLE_PERIOD/1000000000);
			temp1=temp1*SCALING_STOP_LENGTH;
			tim2.tv_sec=(int)(temp1);
			if(tim2.tv_sec==0)
			{
				tim2.tv_nsec=(int)(3*SAMPLE_PERIOD);
			}
			else
			{
				tim2.tv_nsec=0;
			}
			nanosleep(&tim2,&tim3);
			Check_No_Change=0;
		}
	}


	int rtm::parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* app_addrs, 
			prime::uds::socket_addrs_t* dev_addrs, prime::uds::socket_addrs_t* ui_addrs, 
			prime::rtm::rtm_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Thermal Runtime Manager.","PRiME Project\n");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
		// args::Group required(parser, "All of these arguments are required:", args::Group::Validators::All);
		// args::ValueFlag<int> arg_num_km(required, "num_km", "Number of application knobs and monitors", {'n', "num_km"});

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		// args::Flag arg_temp(optional, "temp", "Enable steady state temperature readings for each operating point", {'t', "temp"});
		// args::Flag arg_power(optional, "power", "Enable power reading for each operating point", {'p', "power"});
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

		// args->num_km = args::get(arg_num_km);
		// args->temp_en = arg_temp;
		// args->power_en = arg_power;

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
