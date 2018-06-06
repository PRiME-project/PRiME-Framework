/*
 * Application template for the PRiME Framework with UI hooks
 *
 * No copyright is claimed.  This code is in the public domain; do with
 * it what you wish.
 *
 * Written by Charles Leech
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <list>
#include "uds.h"
#include "util.h"
#include "prime_api_app.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include "app_random.h"
#include "args/args.hxx"
// User Header Files

// User Defines

namespace prime { namespace app
{
	//**** Rename class to application's name *****//
	class app_template
	{
	public:
		struct app_args_t {
			//**** Add application's input arguments here *****//
			bool ui_en;
		};

		static int parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args, int argc, const char * argv[]);

		//**** App class constructors *****//
		app_template(prime::uds::socket_addrs_t* api_addrs,	prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args);
		~app_template();
	private:
		//**** Framework API interface objects *****//
		prime::api::app::rtm_interface rtm_api;
		prime::api::app::ui_interface ui_api;

		//**** Framework Variables - do not edit *****//
		pid_t proc_id;
		bool ui_en;
		std::mutex ui_app_reg_m;
		std::condition_variable ui_app_reg_cv;
		std::mutex ui_app_dereg_m;
		std::condition_variable ui_app_dereg_cv;
		std::mutex ui_app_stop_m;
		std::condition_variable ui_app_stop_cv;
		std::mutex app_mons_disc_m;
		std::mutex app_mons_cont_m;

		//**** Framework Functions - Reg/dereg *****//
		void ui_stop(pid_t proc_id_ui);
		void reg_rtm(pid_t proc_id_ui);
		void dereg_rtm(pid_t proc_id_ui);

		//**** Framework Functions - UI Handler Functions *****//
		void ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min);
		void ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max);
		void ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight);
		void ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min);
		void ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max);
		void ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight);

		//**** Add Application Variables Here *****//
		boost::thread app_thread;


		//**** Add Application Function Prototypes Here *****//
		void app_run();


	};

	//**** Class constructor *****//
	app_template::app_template(prime::uds::socket_addrs_t* app_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args) :
		proc_id(::getpid()),
		rtm_api(proc_id, app_addrs),
		ui_api(proc_id,
			boost::bind(&app_template::reg_rtm, this, _1),
			boost::bind(&app_template::dereg_rtm, this, _1),
			boost::bind(&app_template::ui_mon_disc_min_handler, this, _1, _2),
			boost::bind(&app_template::ui_mon_disc_max_handler, this, _1, _2),
			boost::bind(&app_template::ui_mon_disc_weight_handler, this, _1, _2),
			boost::bind(&app_template::ui_mon_cont_min_handler, this, _1, _2),
			boost::bind(&app_template::ui_mon_cont_max_handler, this, _1, _2),
			boost::bind(&app_template::ui_mon_cont_weight_handler, this, _1, _2),
			boost::bind(&app_template::ui_stop, this, _1),
			ui_addrs),
		ui_en(args->ui_en)
		//**** Add any class variable initialisation here *****//
		// e.g. ,frame_rate(args->frame_rate)
	{

		//**** Framework code - example UI interaction process *****//
		ui_api.return_ui_app_start();
		std::cout << "APP: Application Started" << std::endl;
        if(!ui_en){
			reg_rtm(proc_id);
			std::cout << "APP: Application Registered" << std::endl;
		} else {
			std::unique_lock<std::mutex> reg_lock(ui_app_reg_m);
			ui_app_reg_cv.wait(reg_lock);
			std::cout << "APP: Application Registered" << std::endl;

			app_thread = boost::thread(&app_template::app_run, this);

			std::unique_lock<std::mutex> dereg_lock(ui_app_dereg_m);
			ui_app_dereg_cv.wait(dereg_lock);

			std::cout << "APP: Application Deregistered" << std::endl;

			std::unique_lock<std::mutex> stop_lock(ui_app_stop_m);
			ui_app_stop_cv.wait(stop_lock);
		}
		std::cout << "APP: Application Stopped" << std::endl;
	}

	app_template::~app_template()
	{
	}

	void app_template::ui_stop(pid_t proc_id_ui)
	{
		ui_api.return_ui_app_stop();
		ui_app_stop_cv.notify_one();
	}

	void app_template::reg_rtm(pid_t proc_id_ui)
	{
		// Register application with RTM
		if(proc_id_ui == proc_id)
			rtm_api.reg(proc_id, (unsigned long int)COMPILE_UR_ID);
		else{
			ui_api.ui_app_error(std::string("proc_ids do not match"));
		}

		//**** Add Application Knob and Monitor Registration Here *****//


		ui_app_reg_cv.notify_one();
	}

	void app_template::dereg_rtm(pid_t proc_id_ui)
	{
		//**** Add Application Knob and Monitor Deregistration Here *****//


		// Deregister application with RTM
		rtm_api.dereg(proc_id);

		ui_app_dereg_cv.notify_one();
	}

	void app_template::ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min)
	{
		app_mons_disc_m.lock();

		//**** Add code here if app has a disc monitor  *****//

		app_mons_disc_m.unlock();
	}

	void app_template::ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max)
	{
		app_mons_disc_m.lock();

		//**** Add code here if app has a disc monitor  *****//

		app_mons_disc_m.unlock();
	}

	void app_template::ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight)
	{
		app_mons_disc_m.lock();

		//**** Add code here if app has a disc monitor  *****//
		app_mons_disc_m.unlock();
	}

	void app_template::ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min)
	{
		app_mons_cont_m.lock();

		//**** Add code here if app has a cont monitor  *****//

		app_mons_cont_m.unlock();
	}

	void app_template::ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max)
	{
		app_mons_cont_m.lock();

		//**** Add code here if app has a cont monitor  *****//

		app_mons_cont_m.unlock();
	}

	void app_template::ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight)
	{
		app_mons_cont_m.lock();

		//**** Add code here if app has a cont monitor  *****//

		app_mons_cont_m.unlock();
	}

	void app_template::app_run(void)
	{

		//**** Add/call main application code here *****//

	}

	int app_template::parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Application: " + app_name,"PRiME Project\n");
		parser.LongSeparator("=");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

		args::Group required(parser, "All of these arguments are required:", args::Group::Validators::All);

		//**** Add required CLI arguments here  *****//


		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::Flag ui_off(optional, "ui_off", "Signal that the UI is not hosting the Application", {'u', "ui_off"});

		//**** Add optiopnal CLI arguments here  *****//
		// e.g:
		// args::ValueFlag<double> frame_rate(optional, "frame_rate", "Target frame rate for decoding, in frames per second (default = 25)", {'r', "frame_rate"});

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

		UTIL_ARGS_LOGGER_PROC();

		if(ui_off)
			args->ui_en = false;

		//**** Add parising of CLI arguments here  *****//


		return 0;
	}
} }

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t app_addrs, ui_addrs;
	prime::app::app_template::app_args_t app_args;
	if(prime::app::app_template::parse_cli("Template Application", &app_addrs, &ui_addrs, &app_args, argc, argv)) {
		return -1;
	}

	prime::app::app_template app(&app_addrs, &ui_addrs, &app_args);
}

