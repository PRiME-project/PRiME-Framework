/*
 * Application template for the PRiME Framework
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
		};

		static int parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args, int argc, const char * argv[]);

		//**** App class constructors *****//
		app_template(prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args);
		~app_template();
	private:
		//**** Framework API interface objects *****//
		prime::api::app::rtm_interface rtm_api;

		//**** Framework Variables - do not edit *****//
		pid_t proc_id;

		//**** Framework Functions - Reg/dereg *****//
		void reg_rtm(void);
		void dereg_rtm(void);

		//**** Add Application Variables Here *****//



		//**** Add Application Function Prototypes Here *****//
		void app_run(void);


	};

	//**** Class constructor *****//
	app_template::app_template(prime::uds::socket_addrs_t* app_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::app_template::app_args_t* args) :
		proc_id(::getpid()),
		rtm_api(proc_id, app_addrs)
		//**** Add any class variable initialisation here *****//
		// e.g. ,frame_rate(args->frame_rate)
	{
		std::cout << "APP: Application Started" << std::endl;
		reg_rtm();
		std::cout << "APP: Application Registered" << std::endl;
	}

	app_template::~app_template()
	{
	}

	void app_template::reg_rtm(void)
	{
		// Register application with RTM
		rtm_api.reg(proc_id, (unsigned long int)COMPILE_UR_ID);

		//**** Add Registration of Application Knob and Monitor Here *****//


	}

	void app_template::dereg_rtm(void)
	{
		//**** Add Deregistration of Application Knobs and Monitors Here *****//


		// Deregister application with RTM
		rtm_api.dereg(proc_id);
	}

	void app_template::app_run()
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

