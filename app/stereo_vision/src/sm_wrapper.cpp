#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "uds.h"
#include "util.h"
#include "prime_api_app.h"
#include "app_random.h"
#include "args/args.hxx"
#include <numeric>

#include "StereoMatch.h"
#include "ComFunc.h"

//#define BAD_PIX_THRESHOLD 50
//#define AVG_ERR_THRESHOLD 100
#define THROUGHPUT_THRESHOLD 0.5 // (Hz/FPS)

namespace prime { namespace app
{
	class stereo_match
	{
	public:
		struct app_args_t {
			bool ui_en = true;
			bool display_on = false;
			bool video = false;
			bool recalibrate = false;
            int num_sgbm_threads = MAX_CPU_THREADS;
		};

		static int parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::stereo_match::app_args_t* args, int argc, const char * argv[]);

		stereo_match(prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::stereo_match::app_args_t* args);
		~stereo_match();
	private:
		pid_t proc_id;
		bool ui_en, display_on, video, recalibrate;
		int num_sgbm_threads;

        //framework-related variables
		prime::api::app::rtm_interface rtm_api;
		prime::api::app::ui_interface ui_api;

		std::mutex ui_app_reg_m;
		std::condition_variable ui_app_reg_cv;
		std::mutex ui_app_dereg_m;
		std::condition_variable ui_app_dereg_cv;
		std::mutex ui_app_stop_m;
		std::condition_variable ui_app_stop_cv;

		std::mutex app_knobs_disc_m;
		std::mutex app_knobs_cont_m;
		std::mutex app_mons_disc_m;
		std::mutex app_mons_cont_m;
		std::vector<prime::api::app::knob_disc_t*> app_disc_knobs;
		std::vector<prime::api::app::knob_cont_t*> app_cont_knobs;
		std::vector<prime::api::app::mon_disc_t*> app_disc_mons;
		std::vector<prime::api::app::mon_cont_t*> app_cont_mons;

		boost::thread run_thread;

        //framework-related functions
		void ui_stop(pid_t proc_id_ui);
		void reg_rtm(pid_t proc_id_ui);
		void dereg_rtm(pid_t proc_id_ui);

		void ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min);
		void ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max);
		void ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight);
		void ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min);
		void ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max);
		void ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight);

//		prime::api::app::knob_disc_t sm_algorithm_knob;
//		prime::api::app::knob_disc_t comp_mode_knob;
//		prime::api::app::knob_disc_t img_type_knob;
//		prime::api::app::knob_disc_t dev_knob;
//
//		prime::api::app::mon_cont_t bad_pixel_error_mon;
//		prime::api::app::mon_cont_t avg_error_mon;
		prime::api::app::mon_cont_t throughput_mon;

		bool end_de;

		void stereo_match_run(void);
		int parse_cli(int argc, const char *argv[]);
	};

	stereo_match::stereo_match(prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::stereo_match::app_args_t* args) :
		proc_id(::getpid()),
		rtm_api(proc_id, api_addrs),
		ui_api(proc_id,
			boost::bind(&stereo_match::reg_rtm, this, _1),
			boost::bind(&stereo_match::dereg_rtm, this, _1),
			boost::bind(&stereo_match::ui_mon_disc_min_handler, this, _1, _2),
			boost::bind(&stereo_match::ui_mon_disc_max_handler, this, _1, _2),
			boost::bind(&stereo_match::ui_mon_disc_weight_handler, this, _1, _2),
			boost::bind(&stereo_match::ui_mon_cont_min_handler, this, _1, _2),
			boost::bind(&stereo_match::ui_mon_cont_max_handler, this, _1, _2),
			boost::bind(&stereo_match::ui_mon_cont_weight_handler, this, _1, _2),
			boost::bind(&stereo_match::ui_stop, this, _1),
			ui_addrs),
		ui_en(args->ui_en),
		display_on(args->display_on),
		video(args->video),
		recalibrate(args->recalibrate),
		num_sgbm_threads(args->num_sgbm_threads),
        end_de(false)
	{
		ui_api.return_ui_app_start();
		std::cout << "APP: Stereo Matching Started" << std::endl;

		if(!ui_en){
            reg_rtm(proc_id);
            std::cout << "APP: Stereo Matching Registered" << std::endl;
            std::cout << "APP: Stereo Matching Running" << std::endl;
            stereo_match_run();
        } else {
            std::unique_lock<std::mutex> reg_lock(ui_app_reg_m);
            ui_app_reg_cv.wait(reg_lock);
            std::cout << "APP: Stereo Matching Registered" << std::endl;

            std::cout << "APP: Stereo Matching Running" << std::endl;
            run_thread = boost::thread(&stereo_match::stereo_match_run, this);

            std::unique_lock<std::mutex> dereg_lock(ui_app_dereg_m);
            ui_app_dereg_cv.wait(dereg_lock);
            std::cout << "APP: Stereo Matching Deregistered" << std::endl;

            std::unique_lock<std::mutex> stop_lock(ui_app_stop_m);
            ui_app_stop_cv.wait(stop_lock);
            std::cout << "APP: Stereo Matching Stopped" << std::endl;
            ui_api.return_ui_app_stop();
        }
	}

	stereo_match::~stereo_match()
	{

	}

	void stereo_match::ui_stop(pid_t proc_id_ui)
	{
		ui_app_stop_cv.notify_one();
	}

	void stereo_match::reg_rtm(pid_t proc_id_ui)
	{
		// Register application with RTM
		if(proc_id_ui == proc_id)
			rtm_api.reg(proc_id, (unsigned long int)COMPILE_UR_ID);
		else{
            ui_api.ui_app_error(std::string("proc_ids do not match"));
            return;
		}

		std::cout << "APP: Registering Application Knobs" << std::endl;

        // Set up application knob for controlling SGBM algorithm mode. Three discrete options: 0 = MODE_HH, 1 = MODE_SGBM, 2 = MODE_SGBM_3WAY
        //comp_mode_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_GEN, 0, 2, 0);
        //app_disc_knobs.push_back(&comp_mode_knob);

        // Set up application knob for controlling image data type. Two discrete options: 0 = CV_8U, 1 = CV_32F
//        img_type_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_PREC, 0, 1, 1);
//        app_disc_knobs.push_back(&img_type_knob);

        // Set up application knob for controlling the SM algorithm to use. Two discrete options: 0 = STEREO_SGBM, 1 = STEREO_GIF
//        sm_algorithm_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_GEN, STEREO_SGBM, STEREO_GIF, STEREO_SGBM);
//        app_disc_knobs.push_back(&sm_algorithm_knob);

//        if(nOpenCLDev){
//            // Set up application knob for selecting device used to execute app. Two discrete options: 0 = CPU/OCV_DE, 1 = GPU/OCL_DE
//            dev_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_DEV_SEL, 0, 1, 1);
//        }else{
//            // Set up application knob so it can still be queried but only pass one option.
//            dev_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_DEV_SEL, 0, 0, 0);
//        }

		std::cout << "APP: Registering Application Monitors" << std::endl;
//        // Set up application monitor for observing avgerage pixel error. Required bound is (PRIME_CONT_MIN, BAD_PIX_THRESHOLD]
//		bad_pixel_error_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_ERR, prime::api::PRIME_CONT_MIN, BAD_PIX_THRESHOLD, 1.0);
//        app_cont_mons.push_back(&bad_pixel_error_mon);
//
//        // Set up application monitor for observing avgerage pixel error. Required bound is (PRIME_CONT_MIN, AVG_ERR_THRESHOLD]
//		avg_error_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_ERR, prime::api::PRIME_CONT_MIN, AVG_ERR_THRESHOLD, 1.0);
//        app_cont_mons.push_back(&avg_error_mon);

        // Set up application monitor for observing throughput. Required bound is [THROUGHPUT_THRESHOLD, PRIME_CONT_MAX)
		throughput_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_PERF, THROUGHPUT_THRESHOLD, prime::api::PRIME_CONT_MAX, 1.0);
        app_cont_mons.push_back(&throughput_mon);

		ui_app_reg_cv.notify_one();
	}

	void stereo_match::dereg_rtm(pid_t proc_id_ui)
	{
		end_de = true;
		run_thread.join();
		std::cout << "APP: stereo_match_run joined" << std::endl;

		// Deregister knobs
//		rtm_api.knob_disc_dereg(sm_algorithm_knob);
//		//rtm_api.knob_disc_dereg(comp_mode_knob);
//		rtm_api.knob_disc_dereg(img_type_knob);
//		rtm_api.knob_disc_dereg(dev_knob);

		// Deregister monitors
		//rtm_api.mon_cont_dereg(bad_pixel_error_mon);
		//rtm_api.mon_cont_dereg(avg_error_mon);
		rtm_api.mon_cont_dereg(throughput_mon);

		// Deregister application with RTM
		rtm_api.dereg(proc_id);

		ui_app_dereg_cv.notify_one();
	}

	void stereo_match::stereo_match_run(void)
	{
		std::cout << "APP: Starting Stereo Matching Application" << std::endl;
		int nOpenCLDev = openCLdevicepoll();
		StereoMatch sm = StereoMatch(nOpenCLDev, display_on, video, recalibrate);
		std::cout << "APP: Stereo Matching Application Started" << std::endl;

		if(display_on){
			cv::namedWindow("InputOutput", CV_WINDOW_AUTOSIZE);
			cv::resizeWindow("InputOutput", sm.display_container.cols, sm.display_container.rows);
		}

        std::thread sgbm_threads[num_sgbm_threads];
        std::mutex dispMap_m;
        std::mutex cap_m;

        for(int thread_id = 0; thread_id < num_sgbm_threads; thread_id++)
        {
            sgbm_threads[thread_id] = std::thread(&StereoMatch::sgbm_thread, std::addressof(sm),  std::ref(cap_m),  std::ref(dispMap_m), std::ref(end_de));
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        auto step_time = std::chrono::high_resolution_clock::now();

        char key = ' ';
        double frame_rate = 0;
		while(!end_de && (key != 'q'))
		{

			if(display_on)
			{
				cv::imshow("InputOutput", sm.display_container);

				switch(key)
				{
					case '=':
					{
						throughput_mon.min += 0.1;
						rtm_api.mon_cont_min(throughput_mon, throughput_mon.min);
						std::cout << "Increased minimum performance to " << throughput_mon.min << std::endl;
						break;
					}
					case '-':
					{
						if(throughput_mon.min > 0.1)
						{
							throughput_mon.min -= 0.1;
							rtm_api.mon_cont_min(throughput_mon, throughput_mon.min);
							std::cout << "Decrease minimum performance to " << throughput_mon.min << std::endl;
						} else{
							std::cout << "Cannot decrease minimum performance below " << throughput_mon.min << std::endl;
						}
						break;
					}
				}
				key = cv::waitKey(1);
            }

            auto now_time = (std::chrono::high_resolution_clock::now() - step_time).count();
            if(now_time > 2000000000)
            {
                dispMap_m.lock();

                if(sm.frame_rates.size())
                {
					//std::cout << "Frame times: ";
					for(double ft : sm.frame_rates){
						frame_rate += ft;
					//	std::cout << ft << ", ";
					}
					//std::cout << std::endl;

					frame_rate = frame_rate/sm.frame_rates.size();
					sm.frame_rates.clear();

					//std::cout << "Average frame rate: " << frame_rate << std::endl;

					throughput_mon.val = (prime::api::cont_t)frame_rate;
					rtm_api.mon_cont_set(throughput_mon, throughput_mon.val);
					frame_rate = 0;
                }
				dispMap_m.unlock();
				step_time = std::chrono::high_resolution_clock::now();
            }
		}
		end_de = true;
        std::cout << "APP: Quit signal sent\n" << std::endl;

        for(int thread_id = 0; thread_id < num_sgbm_threads; thread_id++)
        {
            sgbm_threads[thread_id].join();
        }

	}

	void stereo_match::ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min)
	{
		app_mons_disc_m.lock();
		for(auto &mon_disc : app_disc_mons) {
			if(mon_disc->id == id) {
                rtm_api.mon_disc_min(*mon_disc, min);
				mon_disc->min = min;
			}
		}
		app_mons_disc_m.unlock();
	}

	void stereo_match::ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max)
	{
		app_mons_disc_m.lock();
		for(auto &mon_disc : app_disc_mons) {
			if(mon_disc->id == id) {
                rtm_api.mon_disc_max(*mon_disc, max);
				mon_disc->max = max;
			}
		}
		app_mons_disc_m.unlock();
	}

	void stereo_match::ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight)
	{
		app_mons_disc_m.lock();
		for(auto &mon_disc : app_disc_mons) {
			if(mon_disc->id == id) {
                rtm_api.mon_disc_weight(*mon_disc, weight);
				mon_disc->weight = weight;
			}
		}
		app_mons_disc_m.unlock();
	}

	void stereo_match::ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min)
	{
		app_mons_cont_m.lock();
		for(auto &mon_cont : app_cont_mons) {
			if(mon_cont->id == id) {
                rtm_api.mon_cont_min(*mon_cont, min);
				mon_cont->min = min;
			}
		}
		app_mons_cont_m.unlock();
	}

	void stereo_match::ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max)
	{
		app_mons_cont_m.lock();
		for(auto &mon_cont : app_cont_mons) {
			if(mon_cont->id == id) {
                rtm_api.mon_cont_max(*mon_cont, max);
				mon_cont->max = max;
			}
		}
		app_mons_cont_m.unlock();
	}

	void stereo_match::ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight)
	{
		app_mons_cont_m.lock();
		for(auto &mon_cont : app_cont_mons) {
			if(mon_cont->id == id) {
                rtm_api.mon_cont_weight(*mon_cont, weight);
				mon_cont->weight = weight;
			}
		}
		app_mons_cont_m.unlock();
	}

	int stereo_match::parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::stereo_match::app_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Application: " + app_name,"PRiME Project\n");
		parser.LongSeparator("=");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::Flag arg_ui_en(optional, "UI Off", "Signal that the UI is not hosting the Application", {'u', "ui_off"});
		args::Flag arg_display(optional, "display", "Display the input images, the disparity map (and the ground truth for test iamges).", {'d', "display"});
		args::Flag arg_video(optional, "video", "Get input images from the camera instead of test images.", {'v', "video"});
		args::Flag arg_recalibrate(optional, "recalibrate", "Set to recalibrate the camera using chessboard images in the data folder.", {'r', "recalibrate"});

		args::ValueFlag<int> arg_nt_sgbm(optional, "Num Threads", "Number of parallel threads to run in SGBM mode.", {'n', "nt"});

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

		if(arg_ui_en) {
			args->ui_en = false;
		}
		if(arg_display) {
			args->display_on = true;
		}
		if(arg_video) {
			args->video = true;
		}
		if(arg_recalibrate) {
			args->recalibrate = true;
		}
		if(arg_nt_sgbm) {
			args->num_sgbm_threads = args::get(arg_nt_sgbm);
		}

		return 0;
	}

} }

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t app_addrs, ui_addrs;
	prime::app::stereo_match::app_args_t app_args;
	if(prime::app::stereo_match::parse_cli("Stereo Matching", &app_addrs, &ui_addrs, &app_args, argc, argv)) {
		return -1;
	}
	prime::app::stereo_match app(&app_addrs, &ui_addrs, &app_args);
}

