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
#include <opencv2/opencv.hpp>
#include "opencv2/core/version.hpp"
#include "args/args.hxx"

#ifdef CV_VERSION_EPOCH
    #if CV_VERSION_EPOCH == 3
        #define CV_FRAME_COUNT CAP_PROP_FRAME_COUNT
        #define CV_POS_FRAMES CAP_PROP_POS_FRAMES
    #else
        #define CV_FRAME_COUNT CV_CAP_PROP_FRAME_COUNT
        #define CV_POS_FRAMES CV_CAP_PROP_POS_FRAMES
    #endif // CV_VERSION_EPOCH
#else
    #define CV_FRAME_COUNT CV_CAP_PROP_FRAME_COUNT
    #define CV_POS_FRAMES CV_CAP_PROP_POS_FRAMES
#endif // CV_VERSION_EPOCH

#define DEFAULT_FRAME_RATE 25		// 25 FPS = 40 ms

namespace prime { namespace app
{
	class video_decode
	{
	public:
		struct app_args_t {
			double frame_rate = DEFAULT_FRAME_RATE;
			int camera = 0;
			bool ui_en = true;
			std::string video_filename;
		};

		static int parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::video_decode::app_args_t* args, int argc, const char * argv[]);

		video_decode(prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::video_decode::app_args_t* args);
		~video_decode();
	private:
		std::string video_filename;
		int camera;
		double frame_rate;
		pid_t proc_id;
		boost::thread decode_thread;
		bool ui_en;

		prime::api::app::rtm_interface rtm_api;
		prime::api::app::ui_interface ui_api;

		std::mutex ui_app_reg_m;
		std::condition_variable ui_app_reg_cv;
		std::mutex ui_app_dereg_m;
		std::condition_variable ui_app_dereg_cv;
		std::mutex ui_app_stop_m;
		std::condition_variable ui_app_stop_cv;

		std::mutex app_mons_disc_m;
		std::mutex app_mons_cont_m;
		prime::api::app::mon_cont_t frame_rate_mon;

		cv::VideoCapture cap;
		cv::Mat frame, display_frame;

		void ui_stop(pid_t proc_id_ui);
		void reg_rtm(pid_t proc_id_ui);
		void dereg_rtm(pid_t proc_id_ui);

		void ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min);
		void ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max);
		void ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight);
		void ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min);
		void ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max);
		void ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight);

		void decode_video();
		int decode_frame();
	};

	video_decode::video_decode(prime::uds::socket_addrs_t* app_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::video_decode::app_args_t* args) :
		proc_id(::getpid()),
		rtm_api(proc_id, app_addrs),
		ui_api(proc_id,
			boost::bind(&video_decode::reg_rtm, this, _1),
			boost::bind(&video_decode::dereg_rtm, this, _1),
			boost::bind(&video_decode::ui_mon_disc_min_handler, this, _1, _2),
			boost::bind(&video_decode::ui_mon_disc_max_handler, this, _1, _2),
			boost::bind(&video_decode::ui_mon_disc_weight_handler, this, _1, _2),
			boost::bind(&video_decode::ui_mon_cont_min_handler, this, _1, _2),
			boost::bind(&video_decode::ui_mon_cont_max_handler, this, _1, _2),
			boost::bind(&video_decode::ui_mon_cont_weight_handler, this, _1, _2),
			boost::bind(&video_decode::ui_stop, this, _1),
			ui_addrs),
		video_filename(args->video_filename),
		camera(args->camera),
		frame_rate(args->frame_rate),
		ui_en(args->ui_en)
	{

		if(video_filename.compare(""))
		{
			cap.open(video_filename); //video file used
			if(!cap.isOpened())	{
				std::cout << "Could not open video file: " << video_filename << std::endl;
				ui_api.return_ui_app_stop();
				return;
			} else {
				std::cout << "Video file opened" <<std::endl;
			}

		} else
		{
			cap.open(camera); //camera ID used
			if(!cap.isOpened())	{
				std::cout << "Could not open camera ID: "<< camera << std::endl;
				ui_api.return_ui_app_stop();
				return;
			} else {
				std::cout << "Camera opened" <<std::endl;
			}
		}

		ui_api.return_ui_app_start();
		std::cout << "APP: Video Decode Application Started" << std::endl;
        if(!ui_en){
			reg_rtm(proc_id);
			std::cout << "APP: Video Decode Application Registered" << std::endl;
			decode_video();
		} else {
			std::unique_lock<std::mutex> reg_lock(ui_app_reg_m);
			ui_app_reg_cv.wait(reg_lock);

			std::cout << "APP: Video Decode Application Registered" << std::endl;
			decode_thread = boost::thread(&video_decode::decode_video, this);

			std::unique_lock<std::mutex> dereg_lock(ui_app_dereg_m);
			ui_app_dereg_cv.wait(dereg_lock);

			std::cout << "APP: Video Decode Application Deregistered" << std::endl;

			std::unique_lock<std::mutex> stop_lock(ui_app_stop_m);
			ui_app_stop_cv.wait(stop_lock);
		}
		std::cout << "APP: Video Decode Application Stopped" << std::endl;
	}

	video_decode::~video_decode()
	{
	}

	void video_decode::ui_stop(pid_t proc_id_ui)
	{
		ui_api.return_ui_app_stop();
		ui_app_stop_cv.notify_one();
	}

	void video_decode::reg_rtm(pid_t proc_id_ui)
	{
		// Register application with RTM
		if(proc_id_ui == proc_id)
			rtm_api.reg(proc_id, (unsigned long int)COMPILE_UR_ID);
		else{
			ui_api.ui_app_error(std::string("proc_ids do not match"));
		}

		// Register continuous monitor for frame decode time
		//type, min, max, weight
		frame_rate_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_PERF, 0.0, frame_rate, 0.75);

		ui_app_reg_cv.notify_one();
	}

	void video_decode::dereg_rtm(pid_t proc_id_ui)
	{
		decode_thread.interrupt();
		decode_thread.join();

		// Deregister frame decode knob
		rtm_api.mon_cont_dereg(frame_rate_mon);

		// Deregister application with RTM
		rtm_api.dereg(proc_id);

		ui_app_dereg_cv.notify_one();
	}

	void video_decode::ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min)
	{
		app_mons_disc_m.lock();
		//Match id to app monitor
		//prime::api::app::mon_disc_t mon;
		//rtm_api.mon_disc_min(mon, min);
		app_mons_disc_m.unlock();
	}

	void video_decode::ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max)
	{
		app_mons_disc_m.lock();
		//Match id to app monitor
		//prime::api::app::mon_disc_t mon;
		//rtm_api.mon_disc_max(mon, max);
		app_mons_disc_m.unlock();
	}

	void video_decode::ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight)
	{
		app_mons_disc_m.lock();
		//Match id to app monitor
		//prime::api::app::mon_disc_t mon;
		//rtm_api.mon_disc_weight(mon, weight);
		app_mons_disc_m.unlock();
	}

	void video_decode::ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min)
	{
		app_mons_cont_m.lock();
		if(frame_rate_mon.id == id){
			rtm_api.mon_cont_min(frame_rate_mon, min);
			frame_rate_mon.min = min;
		}
		app_mons_cont_m.unlock();
	}

	void video_decode::ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max)
	{
		app_mons_cont_m.lock();
		if(frame_rate_mon.id == id){
			rtm_api.mon_cont_max(frame_rate_mon, max);
			frame_rate_mon.max = max;
		}
		app_mons_cont_m.unlock();
	}

	void video_decode::ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight)
	{
		app_mons_cont_m.lock();
		if(frame_rate_mon.id == id){
			rtm_api.mon_cont_weight(frame_rate_mon, weight);
			frame_rate_mon.weight = weight;
		}
		app_mons_cont_m.unlock();
	}

	void video_decode::decode_video()
	{
		// Record video start time for checking duration
		int frame_count = cap.get(CV_FRAME_COUNT);

		while(1) // boost::thread can be interrupted by thread.interrupt so long as an interruption point is passed - e.g. sleep_until
		{
            //This will make the video loop back to the beginning
			if(cap.get(CV_POS_FRAMES) == frame_count)
				cap.set(CV_POS_FRAMES, 0);

			// Time individual frame decode
			auto frame_start_time = std::chrono::high_resolution_clock::now();
			if(decode_frame())
				break;
			auto frame_finish_time = std::chrono::high_resolution_clock::now();
			frame_rate_mon.val = 1000/std::chrono::duration_cast<std::chrono::milliseconds>(frame_finish_time - frame_start_time).count();

			// Update application monitor
			rtm_api.mon_cont_set(frame_rate_mon, frame_rate_mon.val);

			// If the frame is decoded faster than frame_decode_time_mon.max, sleep for the remaining interval (sleep_time = frame_decode_time_mon.max - frame_decode_time_mon.val)
			std::this_thread::sleep_until(frame_start_time + std::chrono::duration<double>(1/frame_rate_mon.max));

			try {
				 boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted&) {
				return;
			}
		}
	}

	int video_decode::decode_frame()
	{
		if(!cap.read(frame)) {
			std::cout << "cap.read() failed" << std::endl;
			ui_api.ui_app_error(std::string("cap.read() failed"));
			return -1;
		}
		return 0;
	}

	int video_decode::parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::video_decode::app_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Application: " + app_name,"PRiME Project\n");
		parser.LongSeparator("=");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

		args::Group required(parser, "All of these arguments are required:", args::Group::Validators::All);

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::Flag ui_off(optional, "ui_off", "Signal that the UI is not hosting the Application", {'u', "ui_off"});
		args::ValueFlag<double> frame_rate(optional, "frame_rate", "Target frame rate for decoding, in frames per second (default = 25)", {'r', "frame_rate"});
		args::ValueFlag<std::string> vfilename(optional, "filename", "Path of video file", {'f', "filename"});
		args::ValueFlag<int> vcamera(optional, "camera", "ID of camera object (default: 0)", {'c', "camera"});

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

		if(frame_rate){
			args->frame_rate = args::get(frame_rate);
		}
		if(vfilename){
			args->video_filename = args::get(vfilename);
			std::cout << "Using video file: " << args->video_filename << std::endl;
		}
		if(vcamera){
			args->camera = args::get(vcamera);
			std::cout << "Using camera ID: " << args->camera << std::endl;
		}
		if(ui_off)
			args->ui_en = false;

		return 0;
	}
} }

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t app_addrs, ui_addrs;
	prime::app::video_decode::app_args_t app_args;
	if(prime::app::video_decode::parse_cli("Video Decode", &app_addrs, &ui_addrs, &app_args, argc, argv)) {
		return -1;
	}

	prime::app::video_decode app(&app_addrs, &ui_addrs, &app_args);
}

