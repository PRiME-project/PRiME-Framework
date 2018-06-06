/* This file is part of MRPI, an example RTM for the PRiME Framework.
 * 
 * MRPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MRPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MRPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Karunakar Basireddy
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
#include "mrpi_to_freq-v1.hpp"

namespace prime
{
class rtm
{
public:
    struct rtm_args_t
    {
        //double frame_time = 0.040;
        int ondemand = false;
        //int max_cpu_cycle_training = 2000;
    };



    static int parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* rtm_app_addrs,
                         prime::uds::socket_addrs_t* rtm_dev_addrs, prime::uds::socket_addrs_t* rtm_ui_addrs,
                         prime::rtm::rtm_args_t* args, int argc, const char * argv[]);



    rtm(prime::uds::socket_addrs_t* rtm_app_addrs, prime::uds::socket_addrs_t* rtm_dev_addrs,
        prime::uds::socket_addrs_t* rtm_ui_addrs, prime::rtm::rtm_args_t* rtm_args);



    //rtm(double frame_time);
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
    //	prime::api::dev::mon_disc_t cycle_count_4_mon;
    //	prime::api::dev::mon_disc_t cycle_count_5_mon;
    prime::api::dev::mon_cont_t power_mon;

    prime::api::dev::knob_disc_t L2Cache_RR_4_knob;
    prime::api::dev::knob_disc_t Instructions_4_knob;
    prime::api::dev::knob_disc_t L2Cache_RR_5_knob;
    prime::api::dev::knob_disc_t Instructions_5_knob;
    prime::api::dev::mon_disc_t L2Cache_RR_4_mon;
    prime::api::dev::mon_disc_t Instructions_4_mon;
    prime::api::dev::mon_disc_t L2Cache_RR_5_mon;
    prime::api::dev::mon_disc_t Instructions_5_mon;


    prime::api::dev::knob_disc_t L2Cache_RR_6_knob;
    prime::api::dev::knob_disc_t Instructions_6_knob;
    prime::api::dev::knob_disc_t L2Cache_RR_7_knob;
    prime::api::dev::knob_disc_t Instructions_7_knob;
    prime::api::dev::mon_disc_t L2Cache_RR_6_mon;
    prime::api::dev::mon_disc_t Instructions_6_mon;
    prime::api::dev::mon_disc_t L2Cache_RR_7_mon;
    prime::api::dev::mon_disc_t Instructions_7_mon;


    prime::api::dev::knob_disc_t a15_governor_knob;

    boost::thread rtm_thread;
    prime::rtm_pmc::rtm_pmcf controller;
    unsigned int ondemand;

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


    void reg_dev();
    void dereg_dev();
    void control_loop();

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
    //frame_time(rtm_args->frame_time),
    controller(/*(unsigned int)(frame_time * 1000000)*/)
{
    reg_dev();
    ui_api.return_ui_rtm_start();
    rtm_thread = boost::thread(&rtm::control_loop, this);

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

/* ================================================================================== */
/* -------------------------- Device Registration Handlers -------------------------- */
/* ================================================================================== */

void rtm::reg_dev()
{
    // Register device knobs and monitors
    knobs_disc = dev_api.knob_disc_reg();
    knobs_cont = dev_api.knob_cont_reg();
    mons_disc = dev_api.mon_disc_reg();
    mons_cont = dev_api.mon_cont_reg();
    boost::property_tree::ptree dev_arch = dev_api.dev_arch_get();
    unsigned int fu_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.id");
    unsigned int su_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.id");
    unsigned int Freq_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.freq.id");
    unsigned int Freq_full_id = prime::util::set_id(fu_id, su_id, 0, Freq_knob_id);



    // Iterate to match IDs of known knobs
    /*for(auto knob : knobs_disc)
    {
        if(knob.id == full_id) a15_frequency_knob = knob;
    }

    su_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.id");
    unsigned int mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.mons.cycle_count.id");
    unsigned int cycle_count_4_full_id = prime::util::set_id(fu_id, su_id, 0, mon_id);

    su_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.id");
    mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.mons.cycle_count.id");
    unsigned int cycle_count_5_full_id = prime::util::set_id(fu_id, su_id, 0, mon_id);*/


    unsigned int cpu4_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.id");
    unsigned int L2Cache_RR_4_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_0.id");
    unsigned int Instructions_4_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.knobs.pmc_control_1.id");
    unsigned int L2Cache_RR_4_knob_full_id = prime::util::set_id(fu_id, cpu4_id, 0, L2Cache_RR_4_knob_id);
    unsigned int Instructions_4_knob_full_id = prime::util::set_id(fu_id, cpu4_id, 0, Instructions_4_knob_id);

    unsigned int cpu5_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.id");
    unsigned int L2Cache_RR_5_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_2.id");
    unsigned int Instructions_5_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.knobs.pmc_control_1.id");
    unsigned int L2Cache_RR_5_knob_full_id = prime::util::set_id(fu_id, cpu5_id, 0, L2Cache_RR_5_knob_id);
    unsigned int Instructions_5_knob_full_id = prime::util::set_id(fu_id, cpu5_id, 0, Instructions_5_knob_id);

    unsigned int cpu6_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu6.id");
    unsigned int L2Cache_RR_6_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_0.id");
    unsigned int Instructions_6_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu6.knobs.pmc_control_1.id");
    unsigned int L2Cache_RR_6_knob_full_id = prime::util::set_id(fu_id, cpu6_id, 0, L2Cache_RR_6_knob_id);
    unsigned int Instructions_6_knob_full_id = prime::util::set_id(fu_id, cpu6_id, 0, Instructions_6_knob_id);

    unsigned int cpu7_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu7.id");
    unsigned int L2Cache_RR_7_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_0.id");
    unsigned int Instructions_7_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu7.knobs.pmc_control_1.id");
    unsigned int L2Cache_RR_7_knob_full_id = prime::util::set_id(fu_id, cpu7_id, 0, L2Cache_RR_7_knob_id);
    unsigned int Instructions_7_knob_full_id = prime::util::set_id(fu_id, cpu7_id, 0, Instructions_7_knob_id);



    fu_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.id");
    su_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.id");
    unsigned int gov_knob_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.knobs.governor.id");
    unsigned int gov_full_id = prime::util::set_id(fu_id, su_id, 0, gov_knob_id);




    for(auto knob : knobs_disc)
    {
        if(knob.id == L2Cache_RR_4_knob_full_id) L2Cache_RR_4_knob = knob;
        if(knob.id == L2Cache_RR_5_knob_full_id) L2Cache_RR_5_knob = knob;
        if(knob.id == L2Cache_RR_6_knob_full_id) L2Cache_RR_6_knob = knob;
        if(knob.id == L2Cache_RR_7_knob_full_id) L2Cache_RR_7_knob = knob;
        if(knob.id == Instructions_4_knob_full_id) Instructions_4_knob = knob;
        if(knob.id == Instructions_5_knob_full_id) Instructions_5_knob = knob;
	if(knob.id == Instructions_6_knob_full_id) Instructions_6_knob = knob;
        if(knob.id == Instructions_7_knob_full_id) Instructions_7_knob = knob;

        if(knob.id == Freq_full_id) a15_frequency_knob = knob;
        if(knob.id == gov_full_id) a15_governor_knob = knob;
    }

    dev_api.knob_disc_set(L2Cache_RR_4_knob, 0x17);
    dev_api.knob_disc_set(L2Cache_RR_5_knob, 0x17);
    dev_api.knob_disc_set(L2Cache_RR_6_knob, 0x17);
    dev_api.knob_disc_set(L2Cache_RR_7_knob, 0x17);
    dev_api.knob_disc_set(Instructions_4_knob, 0x08);
    dev_api.knob_disc_set(Instructions_5_knob, 0x08);
    dev_api.knob_disc_set(Instructions_6_knob, 0x08);
    dev_api.knob_disc_set(Instructions_7_knob, 0x08);

    unsigned int L2Cache_RR_4_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.mons.pmc_0.id");
    unsigned int L2Cache_RR_4_mon_full_id = prime::util::set_id(fu_id, cpu4_id, 0, L2Cache_RR_4_mon_id);
    unsigned int L2Cache_RR_5_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.mons.pmc_2.id");
    unsigned int L2Cache_RR_5_mon_full_id = prime::util::set_id(fu_id, cpu5_id, 0, L2Cache_RR_5_mon_id);
    unsigned int L2Cache_RR_6_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu6.mons.pmc_0.id");
    unsigned int L2Cache_RR_6_mon_full_id = prime::util::set_id(fu_id, cpu6_id, 0, L2Cache_RR_6_mon_id);
    unsigned int L2Cache_RR_7_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu7.mons.pmc_2.id");
    unsigned int L2Cache_RR_7_mon_full_id = prime::util::set_id(fu_id, cpu7_id, 0, L2Cache_RR_7_mon_id);


    unsigned int Instructions_4_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu4.mons.pmc_1.id");
    unsigned int Instructions_4_mon_full_id = prime::util::set_id(fu_id, cpu4_id, 0, Instructions_4_mon_id);
    unsigned int Instructions_5_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu5.mons.pmc_1.id");
    unsigned int Instructions_5_mon_full_id = prime::util::set_id(fu_id, cpu5_id, 0, Instructions_5_mon_id);
    unsigned int Instructions_6_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu6.mons.pmc_1.id");
    unsigned int Instructions_6_mon_full_id = prime::util::set_id(fu_id, cpu6_id, 0, Instructions_6_mon_id);
    unsigned int Instructions_7_mon_id = dev_arch.get<unsigned int>("device.functional_units.cpu_a15.cpu7.mons.pmc_1.id");
    unsigned int Instructions_7_mon_full_id = prime::util::set_id(fu_id, cpu7_id, 0, Instructions_7_mon_id);

    // Iterate to match IDs of known monitors
    for(auto mon : mons_disc)
    {
        if(mon.id == L2Cache_RR_4_mon_full_id) L2Cache_RR_4_mon = mon;
        if(mon.id == L2Cache_RR_5_mon_full_id) L2Cache_RR_5_mon = mon;
        if(mon.id == L2Cache_RR_6_mon_full_id) L2Cache_RR_6_mon = mon;
        if(mon.id == L2Cache_RR_7_mon_full_id) L2Cache_RR_7_mon = mon;
        if(mon.id == Instructions_4_mon_full_id) Instructions_4_mon = mon;
        if(mon.id == Instructions_5_mon_full_id) Instructions_5_mon = mon;
	if(mon.id == Instructions_6_mon_full_id) Instructions_6_mon = mon;
        if(mon.id == Instructions_7_mon_full_id) Instructions_7_mon = mon;
    }

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
    // Add application to vector of registered applications
    apps_m.lock();
    apps.push_back(proc_id);

    if(ondemand)
    {
        dev_api.knob_disc_set(a15_governor_knob, 3);
    }



    apps_m.unlock();

    //**** Add your code here *****//
}

void rtm::app_dereg_handler(pid_t proc_id)
{
    // Remove  application from vector of registered applications
    apps_m.lock();
    apps.erase(std::remove_if(apps.begin(), apps.end(), [=](pid_t app)
    {
        return app == proc_id;
    }), apps.end());
    apps_m.unlock();

    //**** Add your code here *****//
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
    {
        return app_knob.id == knob.id && app_knob.proc_id == knob.proc_id;
    }),
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
    {
        return app_knob.id == knob.id && app_knob.proc_id == knob.proc_id;
    }),
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
    for(auto& app_knob : app_knobs_disc)
    {
        if((app_knob.id == id) && (app_knob.proc_id == proc_id))
        {
            app_knob.min = min;
        }
    }
    app_knobs_disc_m.unlock();
}

void rtm::knob_disc_max_change_handler(pid_t proc_id, unsigned int id, prime::api::disc_t max)
{
    app_knobs_disc_m.lock();
    //Updates knob maximum in app_knobs_disc
    for(auto& app_knob : app_knobs_disc)
    {
        if((app_knob.id == id) && (app_knob.proc_id == proc_id))
        {
            app_knob.max = max;
        }
    }
    app_knobs_disc_m.unlock();
}

void rtm::knob_cont_min_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t min)
{
    app_knobs_cont_m.lock();
    //Updates knob minimum in app_knobs_disc
    for(auto& app_knob : app_knobs_cont)
    {
        if((app_knob.id == id) && (app_knob.proc_id == proc_id))
        {
            app_knob.min = min;
        }
    }
    app_knobs_cont_m.unlock();
}

void rtm::knob_cont_max_change_handler(pid_t proc_id, unsigned int id, prime::api::cont_t max)
{
    app_knobs_cont_m.lock();
    //Updates knob maximum in app_knobs_disc
    for(auto& app_knob : app_knobs_cont)
    {
        if((app_knob.id == id) && (app_knob.proc_id == proc_id))
        {
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
    {
        return app_mon.id == mon.id && app_mon.proc_id == mon.proc_id;
    }),
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
    {
        return app_mon.id == mon.id && app_mon.proc_id == mon.proc_id;
    }),
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
    for(auto& app_mon : app_mons_disc)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_disc)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_disc)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_disc)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_cont)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_cont)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_cont)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
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
    for(auto& app_mon : app_mons_cont)
    {
        if((app_mon.id == id) && (app_mon.proc_id == proc_id))
        {
            app_mon.val = val;
        }
    }
    app_mons_cont_m.unlock();

    //**** Add your code here *****//
}

// Old JSON Functions
void rtm::app_weight_handler(pid_t proc_id, prime::api::cont_t weight) {}


/* ================================================================================== */

void rtm::control_loop()
{
    bool freq_reset = false;
//        unsigned int mapping_idx = 0;

    //int Num_Apps=2;
    double MRPI;
    //int App_map=1;
//	 FILE *fptrm;

    dev_api.knob_disc_set(a15_governor_knob, 0);

    while(1)
    {

        //	dev_api.knob_disc_set(a15_governor_knob, 0);
        // Run RTM if apps registered > 0 and ondemand not chosen
        if(apps.size())
        {
            unsigned int New_Freq=200000;
            freq_reset = false;


// FILE *fptr2;
            //fptrm = fopen("MRPI_actual.txt","a");
            //std::cout << "Reached Here and got stuck"<<std::endl;*/
            // fprintf(fptr2,"%d\n",Num_Apps);
            // fclose(fptr2);

            /*MRPI Collection*/

            unsigned int L2Cache_RR_4 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_4_mon);
            //unsigned int L2Cache_RR_5 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_5_mon);
  	   //unsigned int L2Cache_RR_6 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_6_mon);
	   //unsigned int L2Cache_RR_7 = (unsigned int) dev_api.mon_disc_get(L2Cache_RR_7_mon);
            //std::cout << "L2Cache RR on core 4: " << L2Cache_RR_4 << std::endl;

            unsigned int Instructions_4 = (unsigned int) dev_api.mon_disc_get(Instructions_4_mon);
            //unsigned int Instructions_5 = (unsigned int) dev_api.mon_disc_get(Instructions_5_mon);
            //unsigned int Instructions_6 = (unsigned int) dev_api.mon_disc_get(Instructions_6_mon);
            //unsigned int Instructions_7 = (unsigned int) dev_api.mon_disc_get(Instructions_7_mon);
            //std::cout << "Instructions on core 4: " << Instructions_4 << std::endl;

            //  

            //std::cout << "L2Cache RR on core 5: " << L2Cache_RR_5 << std::endl;
            //std::cout << "Instructions on core 5: " << Instructions_5 << std::endl;

            double MRPI_4=(double) L2Cache_RR_4/Instructions_4;

            //double MRPI_5=(double) L2Cache_RR_5/Instructions_5;
            //double MRPI_6=(double) L2Cache_RR_6/Instructions_6;
            //double MRPI_7=(double) L2Cache_RR_7/Instructions_7;
            //std::cout << "MRPI on Core 4: " << MRPI_4 << std::endl;
           //std::cout << "MRPI on Core 5: " << MRPI_5 << std::endl;

            // fprintf(fptrm,"%f\n",MRPI_4);
            // fclose(fptrm);
            MRPI=MRPI_4;

            /*            if(MRPI_4>MRPI_5)
                        {
                            MRPI=MRPI_4;
                        }
			if(MRPI<MRPI_6)
			{
			MRPI=MRPI_6;
			}
			if(MRPI<MRPI_7)
			{
			MRPI=MRPI_7;
			}

*/

            //fprintf(fptrm,"%f\n",MRPI);
            // fclose(fptrm);
//            MRPI=MRPI_4;

            /* Run RTM providing MRPI and returning New_Freq*/

             controller.run(New_Freq, MRPI);

            //unsigned int freq_set = *New_Freq;

            // fprintf(fptr2,"%u-%u \n",MRPI,freq_choice);
            /*fprintf(fptr2,"%d\n",Num_Apps);
            fclose(fptr2);*/

	    std::cout <<"MRPI: "<< MRPI << "  " << "Frequency: " << New_Freq << std::endl;

            // Set A15 frequency using device knob

            dev_api.knob_disc_set(a15_frequency_knob, (New_Freq/100000)-2);


//	     dev_api.knob_disc_set(a15_frequency_knob, 10);

            apps_m.lock();

//		if(App_map==1)
            //          	{
            //int j=1;
	    int thread_set=1;
	    if(thread_set){
            for(auto app : apps)
            {
                /*              prime::util::rtm_set_affinity(app, std::vector<unsigned int> {(mapping_idx % 4) + 4});
                                std::cout << "App PID: " << app << " affinity changed to " << (mapping_idx % 4) + 4 << std::endl;*/
//                  int Num_App=(int)(apps.size());
                //                      if(Num_App==1)
                //                    {
                prime::util::rtm_set_affinity(app, std::vector<unsigned int> {4});
                //std::cout << "App PID: " << app << " affinity set to " << 4 << std::endl;
                //                }
                //                  else
                //              {
                //               if(app==apps[0])
                //            {
                //prime::util::rtm_set_affinity(app, std::vector<unsigned int> {4});
                //    std::cout << "App PID: " << app << " affinity set to " << 4 << std::endl;
                //          }
                //        else if(app==apps[1])
                //      {
                //   prime::util::rtm_set_affinity(app, std::vector<unsigned int> {5});
                // std::cout << "App PID: " << app << " affinity set to " << 5 << std::endl;
                //  }
                //App_map=2;
		thread_set=0;
            }
	    }
		
		
           std::this_thread::sleep_for(std::chrono::milliseconds(100));

            //  }
            //}

            apps_m.unlock();
//               mapping_idx++;
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




int rtm::parse_cli(std::string rtm_name, prime::uds::socket_addrs_t* app_addrs,
                   prime::uds::socket_addrs_t* dev_addrs, prime::uds::socket_addrs_t* ui_addrs,
                   prime::rtm::rtm_args_t* args, int argc, const char * argv[])
{
    args::ArgumentParser parser(rtm_name + " RTM.","PRiME Project\n");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
    args::Flag ondemand_gov(optional, "ondemand", "Use Ondemand rather than the PMC-Based algorithm - useful for assessing performance", {'o', "ondemand"});
    //args::ValueFlag<float> frame_time(optional, "frame_time", "Target frame time", {'f', "frametime"});
    //args::ValueFlag<int> max_cpu_cycle_training(optional, "max_cpu_cycle_training", "Number of performance monitor updates to carry out for training", {'t', "traintime"});

    UTIL_ARGS_LOGGER_PARAMS();

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return -1;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return -1;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return -1;
    }

    UTIL_ARGS_LOGGER_PROC_RTM();

    /*if(frame_time) {
    	args->frame_time = args::get(frame_time);
    }

    if(max_cpu_cycle_training) {
    	args->max_cpu_cycle_training = args::get(max_cpu_cycle_training);
    }*/

    args->ondemand = ondemand_gov;

    return 0;
}





}


int main(int argc, const char * argv[])
{
    prime::uds::socket_addrs_t rtm_app_addrs, rtm_dev_addrs, rtm_ui_addrs;
    prime::rtm::rtm_args_t rtm_args;
    if(prime::rtm::parse_cli("PMC-Based", &rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args, argc, argv))
    {
        return -1;
    }

    prime::rtm RTM(&rtm_app_addrs, &rtm_dev_addrs, &rtm_ui_addrs, &rtm_args);
    return 0;
}
