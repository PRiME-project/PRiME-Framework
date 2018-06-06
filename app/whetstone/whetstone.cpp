/*
 * C Converted Whetstone Double Precision Benchmark
 *		Version 1.2	22 March 1998
 *
 *	(c) Copyright 1998 Painter Engineering, Inc.
 *		All Rights Reserved.
 *
 *		Permission is granted to use, duplicate, and
 *		publish this text and program as long as it
 *		includes this entire comment block and limited
 *		rights reference.
 *
 * Converted by Rich Painter, Painter Engineering, Inc. based on the
 * www.netlib.org benchmark/whetstoned version obtained 16 March 1998.
 *
 * A novel approach was used here to keep the look and feel of the
 * FORTRAN version.  Altering the FORTRAN-based array indices,
 * starting at element 1, to start at element 0 for C, would require
 * numerous changes, including decrementing the variable indices by 1.
 * Instead, the array E1[] was declared 1 element larger in C.  This
 * allows the FORTRAN index range to function without any literal or
 * variable indices changes.  The array element E1[0] is simply never
 * used and does not alter the benchmark results.
 *
 * The major FORTRAN comment blocks were retained to minimize
 * differences between versions.  Modules N5 and N12, like in the
 * FORTRAN version, have been eliminated here.
 *
 * An optional command-line argument has been provided [-c] to
 * offer continuous repetition of the entire benchmark.
 * An optional argument for setting an alternate LOOP count is also
 * provided.  Define PRINTOUT to cause the POUT() function to print
 * outputs at various stages.  Final timing measurements should be
 * made with the PRINTOUT undefined.
 *
 * Questions and comments may be directed to the author at
 *			r.painter@ieee.org
 */
 /* Modified in July 2017 to remove "GOTO"s and to include in the
  * PRiME Framework by Charles Leech and Graeme Bragg, PRiME Project,
  * University of Southampton.
  *
  *
  */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
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

/* map the FORTRAN math functions, etc. to the C versions */
#define DSIN	sin
#define DCOS	cos
#define DATAN	atan
#define DLOG	log
#define DEXP	exp
#define DSQRT	sqrt

#define DEFAULT_THREADS	4		// The default number of threads to start
#define MAX_THREADS		256		// Maximum number of threads that can be run
#define MONITOR_UPDATE	500		// How often the performance monitor is updated
#define WHETSTONE_LOOP	1000	// Whetstone loop value

//#define THREAD_CONTROL		// Comment this out if individual thread control is not needed
//#define APP_DEBUG				// Uncomment this to turn on debug printing.

namespace prime { namespace app
{
	class whetstone
	{
	public:
		struct app_args_t {
			int threads = DEFAULT_THREADS;
			bool ui_en = true;
		};
		
		static int parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs, 
			prime::uds::socket_addrs_t* ui_addrs, prime::app::whetstone::app_args_t* args, int argc, const char * argv[]);
	
		whetstone(prime::uds::socket_addrs_t* app_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::whetstone::app_args_t* args);
		~whetstone();
	private:
		pid_t proc_id;
		
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
		prime::api::app::mon_cont_t performance_mon;

		std::atomic<bool> app_run;

		void ui_stop(pid_t proc_id_ui);
		void reg_rtm(pid_t proc_id_ui);
		void dereg_rtm(pid_t proc_id_ui);

		void ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min);
		void ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max);
		void ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight);
		void ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min);
		void ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max);
		void ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight);


		/* COMMON T,T1,T2,E1(4),J,K,L */
		typedef struct {
			int thread_num;

			double T,T1,T2,E1[5];
			int J,K,L;
			std::atomic<bool> thread_run;
			std::atomic<float> kips;
		} thread_data_t;


		/* Whetstone Utility Function Prototypes. */
		unsigned long long get_timestamp(void);
		void PA(double E[], int *J, double *T, double *T2);
		void P0(double E[], int *J, int *K, int *L);
		void P3(double X, double Y, double *Z, double *T, double *T2);


		/* Thread Stuff. */
		thread_data_t thread_data[MAX_THREADS];
		std::thread threads_array[MAX_THREADS];
		std::thread main_thread;
		int num_threads;
		bool ui_en;

		void master(void);
		void worker(void *data_ptr);
	};

	whetstone::whetstone(prime::uds::socket_addrs_t* app_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::whetstone::app_args_t* args) :
		proc_id(::getpid()),
		rtm_api(proc_id, app_addrs),
		ui_api(proc_id,
			boost::bind(&whetstone::reg_rtm, this, _1),
			boost::bind(&whetstone::dereg_rtm, this, _1),
			boost::bind(&whetstone::ui_mon_disc_min_handler, this, _1, _2),
			boost::bind(&whetstone::ui_mon_disc_max_handler, this, _1, _2),
			boost::bind(&whetstone::ui_mon_disc_weight_handler, this, _1, _2),
			boost::bind(&whetstone::ui_mon_cont_min_handler, this, _1, _2),
			boost::bind(&whetstone::ui_mon_cont_max_handler, this, _1, _2),
			boost::bind(&whetstone::ui_mon_cont_weight_handler, this, _1, _2),
			boost::bind(&whetstone::ui_stop, this, _1),
			ui_addrs),
		num_threads(args->threads),
		ui_en(args->ui_en)
	{
		ui_api.return_ui_app_start();
		std::cout << "APP: Whetstone Benchmark Started" << std::endl;

        if(!ui_en){
            reg_rtm(proc_id);
			std::cout << "APP: Whetstone Benchmark Registered" << std::endl;
			app_run = true;
            master();
        } else {
            std::unique_lock<std::mutex> reg_lock(ui_app_reg_m);
            ui_app_reg_cv.wait(reg_lock);

			std::cout << "APP: Whetstone Benchmark Registered" << std::endl;

			// Start "main" thread.
			app_run = true;
			main_thread = std::thread(&whetstone::master, this);

			std::unique_lock<std::mutex> dereg_lock(ui_app_dereg_m);
			ui_app_dereg_cv.wait(dereg_lock);
			std::cout << "APP: Whetstone Benchmark Deregistered" << std::endl;

			std::unique_lock<std::mutex> stop_lock(ui_app_stop_m);
			ui_app_stop_cv.wait(stop_lock);
			std::cout << "APP: Whetstone Benchmark Stopped" << std::endl;
			ui_api.return_ui_app_stop();
		}
	}

	whetstone::~whetstone()
	{
	}

	void whetstone::ui_stop(pid_t proc_id_ui)
	{
		ui_app_stop_cv.notify_one();
	}

	void whetstone::reg_rtm(pid_t proc_id_ui)
	{
		// Register application with RTM
		if(proc_id_ui == proc_id)
			rtm_api.reg(proc_id, (unsigned long int)COMPILE_UR_ID);
		else{
			ui_api.ui_app_error(std::string("proc_ids do not match"));
		}

		// Register continuous monitor for spin time of loader thread
		performance_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_PERF, 0.0, 100000.0, 1.0);

		ui_app_reg_cv.notify_one();
	}

	void whetstone::dereg_rtm(pid_t proc_id_ui)
	{
		// Mark app to die and join "main" thread
		app_run = false;
		main_thread.join();

		// Deregister frame decode knob
		rtm_api.mon_cont_dereg(performance_mon);

		// Deregister application with RTM
		rtm_api.dereg(proc_id);

		ui_app_dereg_cv.notify_one();
	}

	void whetstone::ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min)
	{
		app_mons_disc_m.lock();
		//Match id to app monitor
		//prime::api::app::mon_disc_t mon;
		//rtm_api.mon_disc_min(mon, min);
		app_mons_disc_m.unlock();
	}

	void whetstone::ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max)
	{
		app_mons_disc_m.lock();
		//Match id to app monitor
		//prime::api::app::mon_disc_t mon;
		//rtm_api.mon_disc_max(mon, max);
		app_mons_disc_m.unlock();
	}

	void whetstone::ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight)
	{
		app_mons_disc_m.lock();
		//Match id to app monitor
		//prime::api::app::mon_disc_t mon;
		//rtm_api.mon_disc_weight(mon, weight);
		app_mons_disc_m.unlock();
	}

	void whetstone::ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min)
	{
		app_mons_cont_m.lock();
		if(performance_mon.id == id){
			rtm_api.mon_cont_min(performance_mon, min);
			performance_mon.min = min;
		}
		app_mons_cont_m.unlock();
	}

	void whetstone::ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max)
	{
		app_mons_cont_m.lock();
		if(performance_mon.id == id){
			rtm_api.mon_cont_max(performance_mon, max);
			performance_mon.max = max;
		}
		app_mons_cont_m.unlock();
	}

	void whetstone::ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight)
	{
		app_mons_cont_m.lock();
		if(performance_mon.id == id){
			rtm_api.mon_cont_weight(performance_mon, weight);
			performance_mon.weight = weight;
		}
		app_mons_cont_m.unlock();
	}

	unsigned long long whetstone::get_timestamp()
	{
		auto now = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
		return duration.count();
	}

	void whetstone::PA(double E[], int *J, double *T, double *T2)
	{
		*J = 0;

		while(*J < 6){
			E[1] = ( E[1] + E[2] + E[3] - E[4]) * *T;
			E[2] = ( E[1] + E[2] - E[3] + E[4]) * *T;
			E[3] = ( E[1] - E[2] + E[3] + E[4]) * *T;
			E[4] = (-E[1] + E[2] + E[3] + E[4]) / *T2;
			*J += 1;
		}
	}

	void whetstone::P0(double E[], int *J, int *K, int *L)
	{
		E[*J] = E[*K];
		E[*K] = E[*L];
		E[*L] = E[*J];
	}

	void whetstone::P3(double X, double Y, double *Z, double *T, double *T2)
	{
		double X1, Y1;

		X1 = X;
		Y1 = Y;
		X1 = *T * (X1 + Y1);
		Y1 = *T * (X1 + Y1);
		*Z  = (X1 + Y1) / *T2;
	}


	void whetstone::master(void)
	{
		int idx;
		float perf;
#ifdef APP_DEBUG
		std::cout << "Master thread started." << std::endl;
#endif
		// Start loader threads
		for(idx = 0; idx<num_threads; idx++) {
			thread_data[idx].thread_num = idx;
			thread_data[idx].kips = 0;
			thread_data[idx].thread_run = true;

			//start the thread
			threads_array[idx] = std::thread(&whetstone::worker, this, (void*)&thread_data[idx]);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_UPDATE));

		while(app_run.load()) {
			perf = 0;
			for(idx = 0; idx<num_threads; idx++) {				// Get the performance of each thread.
				perf += thread_data[idx].kips.load();
			}
			rtm_api.mon_cont_set(performance_mon, perf);	// Update the monitor
#ifdef APP_DEBUG
			std::cout << "Combined Performance = " << perf << " KIPS over " << num_threads << " threads." << std::endl;
#endif

			std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_UPDATE));
		}



		for(idx = 0; idx<num_threads; idx++) {
#ifdef THREAD_CONTROL
			thread_data[idx].thread_run = false;	//signal each individual thread
#endif
			threads_array[idx].join();				// Sequentially join loader threads
		}
#ifdef APP_DEBUG
		std::cout << "Master thread terminated." << std::endl;
#endif
	}

	void whetstone::worker(void *data_ptr)
	{
		thread_data_t *t_data = (thread_data_t *) data_ptr;
		int thread_thread_num = t_data->thread_num;

		double *T = &t_data->T;
		double *T1 = &t_data->T1;
		double *T2 = &t_data->T2;
		double *E1 = t_data->E1;
		int *J = &t_data->J;
		int *K = &t_data->K;
		int *L = &t_data->L;

		/* used in the FORTRAN version */
		long I;
		long N1, N2, N3, N4, N6, N7, N8, N9, N10, N11;
		double X1,X2,X3,X4,X,Y,Z;
		long LOOP = WHETSTONE_LOOP;

		double m_hash[12];
		int loop_idx = 0;

		/* added for this version */
		unsigned long long startsec, finisec;

#ifdef APP_DEBUG
		std::cout << "Worker thread " << thread_thread_num << " started." << std::endl;
#endif

#ifdef THREAD_CONTROL
		while(t_data->thread_run.load())
#else
		while(app_run.load())
#endif
		{
		/*
		C	Start benchmark timing at this point.
		*/
			startsec = get_timestamp();
		/*
		C	The actual benchmark starts here.
		*/
			*T  = .499975;
			*T1 = 0.50025;
			*T2 = 2.0;

			N1  = 0;
			N2  = 12 * LOOP;
			N3  = 14 * LOOP;
			N4  = 345 * LOOP;
			N6  = 210 * LOOP;
			N7  = 32 * LOOP;
			N8  = 899 * LOOP;
			N9  = 616 * LOOP;
			N10 = 0;
			N11 = 93 * LOOP;
		/*
		C	Module 1: Simple identifiers
		*/
			X1  =  1.0;
			X2  = -1.0;
			X3  = -1.0;
			X4  = -1.0;

			for (I = 1; I <= N1; I++) {
				X1 = (X1 + X2 + X3 - X4) * *T;
				X2 = (X1 + X2 - X3 + X4) * *T;
				X3 = (X1 - X2 + X3 + X4) * *T;
				X4 = (-X1+ X2 + X3 + X4) * *T;
			}

		/*
		C	Module 2: Array elements
		*/
			E1[1] =  1.0;
			E1[2] = -1.0;
			E1[3] = -1.0;
			E1[4] = -1.0;

			for (I = 1; I <= N2; I++) {
				E1[1] = ( E1[1] + E1[2] + E1[3] - E1[4]) * *T;
				E1[2] = ( E1[1] + E1[2] - E1[3] + E1[4]) * *T;
				E1[3] = ( E1[1] - E1[2] + E1[3] + E1[4]) * *T;
				E1[4] = (-E1[1] + E1[2] + E1[3] + E1[4]) * *T;
			}

		/*
		C	Module 3: Array as parameter
		*/
			for (I = 1; I <= N3; I++)
				PA(E1, J, T, T2);

		/*
		C	Module 4: Conditional jumps
		*/
			*J = 1;
			for (I = 1; I <= N4; I++) {
				if (*J == 1)
					*J = 2;
				else
					*J = 3;

				if (*J > 2)
					*J = 0;
				else
					*J = 1;

				if (*J < 1)
					*J = 1;
				else
					*J = 0;
			}

		/*
		C	Module 5: Omitted
		C 	Module 6: Integer arithmetic
		*/

			*J = 1;
			*K = 2;
			*L = 3;

			for (I = 1; I <= N6; I++) {
				*J = *J * (*K-*J) * (*L-*K);
				*K = *L * *K - (*L-*J) * *K;
				*L = (*L-*K) * (*K+*J);
				E1[*L-1] = *J + *K + *L;
				E1[*K-1] = *J * *K * *L;
			}

		/*
		C	Module 7: Trigonometric functions
		*/
			X = 0.5;
			Y = 0.5;

			for (I = 1; I <= N7; I++) {
				X = *T * DATAN(*T2*DSIN(X)*DCOS(X)/(DCOS(X+Y)+DCOS(X-Y)-1.0));
				Y = *T * DATAN(*T2*DSIN(Y)*DCOS(Y)/(DCOS(X+Y)+DCOS(X-Y)-1.0));
			}

		/*
		C	Module 8: Procedure calls
		*/
			X = 1.0;
			Y = 1.0;
			Z = 1.0;

			for (I = 1; I <= N8; I++)
				P3(X,Y,&Z, T, T2);

		/*
		C	Module 9: Array references
		*/
			*J = 1;
			*K = 2;
			*L = 3;
			E1[1] = 1.0;
			E1[2] = 2.0;
			E1[3] = 3.0;

			for (I = 1; I <= N9; I++)
				P0(E1, J, K, L);

		/*
		C	Module 10: Integer arithmetic
		*/
			*J = 2;
			*K = 3;

			for (I = 1; I <= N10; I++) {
				*J = *J + *K;
				*K = *J + *K;
				*J = *K - *J;
				*K = *K - *J - *J;
			}

		/*
		C	Module 11: Standard functions
		*/
			X = 0.75;

			for (I = 1; I <= N11; I++)
				X = DSQRT(DEXP(DLOG(X)/(*T1)));


		/*
		C      Stop benchmark timing at this point.
		*/
			finisec = get_timestamp();
			loop_idx++;
		/*
		C----------------------------------------------------------------
		C      Performance in Whetstone KIP's per second is given by
		C
		C	(100*LOOP*II)/TIME
		C
		C      where TIME is in seconds.
		C--------------------------------------------------------------------
		*/
			t_data->kips = (100.0*LOOP)/(float)(finisec-startsec);
		}
#ifdef APP_DEBUG
		std::cout << "Worker thread " << thread_thread_num << " terminated." << std::endl;
#endif
	}
	
	int whetstone::parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs, 
			prime::uds::socket_addrs_t* ui_addrs, prime::app::whetstone::app_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Application: " + app_name,"PRiME Project\n");
		parser.LongSeparator("=");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

		args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
		args::ValueFlag<int> threads(optional, "threads", "Number of threads to use", {'t', "num_threads"});
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
		
		UTIL_ARGS_LOGGER_PROC();

		if(threads){
			args->threads = args::get(threads);
		}
		if(ui_off) {
			args->ui_en = false;
		}

		return 0;
	}
} }

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t app_addrs, ui_addrs;
	prime::app::whetstone::app_args_t app_args;
	if(prime::app::whetstone::parse_cli("Whetstone", &app_addrs, &ui_addrs, &app_args, argc, argv)) {
		return -1;
	}
	
	prime::app::whetstone app(&app_addrs, &ui_addrs, &app_args);
	
	return 0;
}


