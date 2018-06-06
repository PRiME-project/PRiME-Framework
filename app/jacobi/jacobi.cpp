/* This file is part of Jacobi, an example application for the PRiME Framework.
 * 
 * Jacobi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Jacobi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Jacobi.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by James Davis, Charles Leech & Graeme Bragg
 */

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "uds.h"
#include "util.h"
#include "prime_api_app.h"
#include "jacobi.h"
#include "app_random.h"
#include "args/args.hxx"
#include <random>
#include <limits>
#include <omp.h>
#include <sys/sysinfo.h>

#ifdef KOCL_EN
	#include "KOCL.h"
#endif

namespace prime { namespace app
{
	class jacobi
	{
	public:
		struct app_args_t {
			bool ui_en = true;
		};

		static int parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::jacobi::app_args_t* args, int argc, const char * argv[]);

		jacobi(prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::jacobi::app_args_t* args);
		~jacobi();
	private:
        pid_t proc_id;
		bool ui_en;

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

		prime::api::app::knob_disc_t iters_knob;
		prime::api::app::knob_disc_t prec_knob;
		prime::api::app::knob_disc_t dev_knob;

		prime::api::app::mon_cont_t error_mon;
		prime::api::app::mon_cont_t throughput_mon;

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

        //app-specific variables
        cl_int ret;
        cl_context context;
        cl_program program;
        cl_command_queue queue;
        cl_kernel kernel_double, kernel_float;
        cl_mem A_double_buffer, b_double_buffer, x_double_buffer;
        cl_mem A_float_buffer, b_float_buffer, x_float_buffer;

        unsigned int oclDevices;
        double* A, *b, *x;
        float *A_float, *b_float, *x_float;
        size_t global_work_size[2], local_work_size;
        cl_event kernel_event;

        //app-specific functions (+ those in header file)
		void jacobi_setup(void);
		void jacobi_run(void);
        void jacobi_float_cpu(float* A, float* b, float* x);
        void jacobi_double_cpu(double* A, double* b, double* x);
        double rand_in_range(double min, double max);
        double l2norm_sq(double* A, double* b, double* x);
        void setupOpenCL_ALTERA(cl_context *context, cl_program *program, cl_command_queue *comm_queue);
        void setupOpenCL_ODROID(cl_context *context, cl_program *program, cl_command_queue *comm_queue);
		int parse_cli(int argc, const char *argv[]);

#ifdef KOCL_EN
		KOCL_t* kocl;
		prime::api::app::mon_cont_t kocl_mon_tmp;
		std::vector<prime::api::app::mon_cont_t> kocl_mons;
#endif
	};

	jacobi::jacobi(prime::uds::socket_addrs_t* api_addrs, prime::uds::socket_addrs_t* ui_addrs, prime::app::jacobi::app_args_t* args) :
		proc_id(::getpid()),
		rtm_api(proc_id, api_addrs),
		ui_api(proc_id,
			boost::bind(&jacobi::reg_rtm, this, _1),
			boost::bind(&jacobi::dereg_rtm, this, _1),
			boost::bind(&jacobi::ui_mon_disc_min_handler, this, _1, _2),
			boost::bind(&jacobi::ui_mon_disc_max_handler, this, _1, _2),
			boost::bind(&jacobi::ui_mon_disc_weight_handler, this, _1, _2),
			boost::bind(&jacobi::ui_mon_cont_min_handler, this, _1, _2),
			boost::bind(&jacobi::ui_mon_cont_max_handler, this, _1, _2),
			boost::bind(&jacobi::ui_mon_cont_weight_handler, this, _1, _2),
			boost::bind(&jacobi::ui_stop, this, _1),
			ui_addrs),
		ui_en(args->ui_en)
	{

        jacobi_setup();
#ifdef KOCL_EN
		kocl = KOCL_init(1.0);
#endif
		std::cout << "APP: Jacobi Setup Complete" << std::endl;
		ui_api.return_ui_app_start();
		std::cout << "APP: Jacobi Started" << std::endl;

        if(!ui_en){
            reg_rtm(proc_id);
			std::cout << "APP: Jacobi Registered" << std::endl;
			std::cout << "APP: Jacobi Running" << std::endl;
            jacobi_run();
        } else {
            std::unique_lock<std::mutex> reg_lock(ui_app_reg_m);
            ui_app_reg_cv.wait(reg_lock);
			std::cout << "APP: Jacobi Registered" << std::endl;

			std::cout << "APP: Jacobi Running" << std::endl;
			run_thread = boost::thread(&jacobi::jacobi_run, this);

			std::unique_lock<std::mutex> dereg_lock(ui_app_dereg_m);
			ui_app_dereg_cv.wait(dereg_lock);

			std::cout << "APP: jacobi deregistered" << std::endl;

			std::unique_lock<std::mutex> stop_lock(ui_app_stop_m);
			ui_app_stop_cv.wait(stop_lock);
			std::cout << "APP: jacobi stopped" << std::endl;
			ui_api.return_ui_app_stop();
		}
	}

	jacobi::~jacobi()
	{
        // Clean up
        ret = clReleaseKernel(kernel_float);
        ret = clReleaseKernel(kernel_double);
        assert(ret == CL_SUCCESS);

        ret = clReleaseCommandQueue(queue);
        assert(ret == CL_SUCCESS);

        ret = clReleaseMemObject(A_float_buffer);
        ret = clReleaseMemObject(b_float_buffer);
        ret = clReleaseMemObject(x_float_buffer);
        ret = clReleaseMemObject(A_double_buffer);
        ret = clReleaseMemObject(b_double_buffer);
        ret = clReleaseMemObject(x_double_buffer);
        assert(ret == CL_SUCCESS);

        ret = clReleaseProgram(program);
        assert(ret == CL_SUCCESS);

        ret = clReleaseContext(context);
        assert(ret == CL_SUCCESS);

#ifdef KOCL_EN
		KOCL_del(kocl);
#endif
	}

	void jacobi::ui_stop(pid_t proc_id_ui)
	{
		ui_app_stop_cv.notify_one();
	}

	void jacobi::reg_rtm(pid_t proc_id_ui)
	{
		// Register application with RTM
		if(proc_id_ui == proc_id)
			rtm_api.reg(proc_id, (unsigned long int)COMPILE_UR_ID);
		else{
            ui_api.ui_app_error(std::string("proc_ids do not match"));
            return;
		}

        // Set up application knob for controlling number of iterations
        iters_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_ITR, 1, prime::api::PRIME_DISC_MAX, 10);
        app_disc_knobs.push_back(&iters_knob);

        // Set up application knob for controlling kernel precision. Two discrete options: 0 = float, 1 = double
        prec_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_PREC, 0, 1, 1);
        app_disc_knobs.push_back(&prec_knob);

        if(oclDevices){
            // Set up application knob for selecting device used to execute kernels. Two discrete options: 0 = CPU, 1 = GPU
            dev_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_DEV_SEL, 0, 1, 1);
        }else{
            // Set up application knob so it can still be queried but only pass one option.
            dev_knob = rtm_api.knob_disc_reg(prime::api::app::PRIME_DEV_SEL, 0, 0, 0);
        }

        // Set up application monitor for observing error. Required bound is (PRIME_CONT_MIN, CONVERGENCE_THRESHOLD]
		error_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_ERR, prime::api::PRIME_CONT_MIN, CONVERGENCE_THRESHOLD, 1.0);
        app_cont_mons.push_back(&error_mon);

        // Set up application monitor for observing throughput. Required bound is [THROUGHPUT_THRESHOLD, PRIME_CONT_MAX)
		throughput_mon = rtm_api.mon_cont_reg(prime::api::app::PRIME_PERF, THROUGHPUT_THRESHOLD, prime::api::PRIME_CONT_MAX, 1.0);
        app_cont_mons.push_back(&throughput_mon);

		ui_app_reg_cv.notify_one();
	}

	void jacobi::dereg_rtm(pid_t proc_id_ui)
	{
		run_thread.interrupt();
		run_thread.join();
		std::cout << "APP: jacobi_run joined" << std::endl;

		// Deregister knobs
		rtm_api.knob_disc_dereg(iters_knob);
		rtm_api.knob_disc_dereg(prec_knob);
        if(oclDevices)
            rtm_api.knob_disc_dereg(dev_knob);

		// Deregister monitors
		rtm_api.mon_cont_dereg(error_mon);
		rtm_api.mon_cont_dereg(throughput_mon);

#ifdef KOCL_EN
		// Deregister KOCL monitors
		if(!kocl_mons.empty())
			for(prime::api::app::mon_cont_t mon : kocl_mons)
				rtm_api.mon_cont_dereg(mon);
#endif

		// Deregister application with RTM
		rtm_api.dereg(proc_id);

		ui_app_dereg_cv.notify_one();
	}

	void jacobi::jacobi_setup(void)
	{
        cl_platform_id* platforms;
        cl_uint deviceCount;
		cl_device_id* devices;

        // get all platforms
        platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id));
        clGetPlatformIDs(1, platforms, NULL);

		// get all devices
		clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

        oclDevices = openCLDeviceInfo();
        if(oclDevices)
        {
            /* From Charlie: There is scope to merge the common parts of these 2 setup functions.
            I only use this method for speed. Only real difference is the mechanism for reading the program source (WithBinary vs FromSource)
            setup_ODROID comes from SM application code & uses ARM OCL Utils.
             */
#ifdef C5SOC
			setupOpenCL_ALTERA(&context, &program, &queue);
#else
			setupOpenCL_ODROID(&context, &program, &queue);
#endif

            // Create kernels
            kernel_double = clCreateKernel(program, "jacobi_double", &ret);
            kernel_float = clCreateKernel(program, "jacobi_float", &ret);
            assert(ret == CL_SUCCESS);

            A_double_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, N*N*sizeof(cl_double), NULL, &ret);
            b_double_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, N*sizeof(cl_double), NULL, &ret);
            x_double_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, N*sizeof(cl_double), NULL, &ret);
            A_float_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, N*N*sizeof(cl_float), NULL, &ret);
            b_float_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, N*sizeof(cl_float), NULL, &ret);
            x_float_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, N*sizeof(cl_float), NULL, &ret);
            checkSuccess(ret);
            assert(ret == CL_SUCCESS);

            // Set kernel arguments
            ret = clSetKernelArg(kernel_double, 0, sizeof(cl_mem), &A_double_buffer);
            ret = clSetKernelArg(kernel_double, 1, sizeof(cl_mem), &b_double_buffer);
            ret = clSetKernelArg(kernel_double, 2, sizeof(cl_mem), &x_double_buffer);
            ret = clSetKernelArg(kernel_float, 0, sizeof(cl_mem), &A_float_buffer);
            ret = clSetKernelArg(kernel_float, 1, sizeof(cl_mem), &b_float_buffer);
            ret = clSetKernelArg(kernel_float, 2, sizeof(cl_mem), &x_float_buffer);
            checkSuccess(ret);
            assert(ret == CL_SUCCESS);


			global_work_size[0] = MWGS;
			global_work_size[1] = N/MWGS;
			local_work_size = MWGS;
        }

        // Seed random number generator
        srand(time(NULL));

        // Create memory objects
        A = (double*)malloc(N*N*sizeof(double));
        b = (double*)malloc(N*sizeof(double));
        x = (double*)malloc(N*sizeof(double));
        A_float = (float*)malloc(N*N*sizeof(float));
        b_float = (float*)malloc(N*sizeof(float));
        x_float = (float*)malloc(N*sizeof(float));
	}

	void jacobi::jacobi_run()
	{
		//Variables for data initialisation
        int i, j;

        unsigned long long start, end;
        prime::api::cont_t throughput;
        double norm_sq, log_norm_sq;	// Square of L2 norm
        float throughput_threshold_min = THROUGHPUT_THRESHOLD;
        float error_threshold_max = CONVERGENCE_THRESHOLD;

		std::random_device rand_dev;
		std::default_random_engine gen_eng(rand_dev());
		const unsigned int maxRandNum = std::numeric_limits<unsigned int>::max();
		std::uniform_int_distribution<unsigned int> uniform_dist(0, maxRandNum);

		omp_set_num_threads(16);

		std::cout << "APP: Jacobi application setup complete, beginning continuous loop.\n" << std::endl;

		while(1)
		{
		
#ifdef DEBUG_APP_TIMING
            start = prime::util::get_timestamp();
#endif
            // Create initial input data. Make A diagonally dominant by keeping non-diagonal elements of A in range [0, N] and diagonal elements in range [N^2, N^3]. Initial guess of x is always zero-vector
            // Re-seed data for each iteration
            #pragma omp parallel for private(i, j)
			for(i = 0; i < N; i++) {
				for(j = 0; j < N; j++) {
					//A[i*N + j] = rand_in_range(0.0, (double)N);  //i != j
					A[i*N + j] = N*(double)uniform_dist(gen_eng)/maxRandNum;  //i != j
				}
				//b[i] = rand_in_range(0.0, (double)(N*N));
				b[i] = N*N*(double)uniform_dist(gen_eng)/maxRandNum;
				x[i] = 0.0;
			}
            #pragma omp parallel for private(i)
			for(i = 0; i < N; i++) {
				//A[i*N + i] = rand_in_range((double)(N*N), (double)(N*(double)(N*N))); //i = j
				A[i*N + i] = N*N + ((double)(N*N)*(N - 1))*(double)uniform_dist(gen_eng)/maxRandNum; //i = j
            }

            // Perturb input data. Keep A diagonally dominant. Reset x
            #pragma omp parallel for private(i, j)
            for(i = 0; i < N; i++) {
                for(j = 0; j < N; j++) {
					//A[i*N + j] += rand_in_range(0.0, 1.0);
					A[i*N + j] += (double)uniform_dist(gen_eng)/maxRandNum;
                }
                //b[i] += rand_in_range(0.0, (double)N);
                b[i] += N*(double)uniform_dist(gen_eng)/maxRandNum;
                x[i] = 0.0;
            }
			//Make A diagonally dominant here.
            #pragma omp parallel for private(i)
            for(i = 0; i < N; i++) {
				//A[i*N + i] += rand_in_range((double)N, (double)(N*N));
				A[i*N + i] += N + (N*(N - 1))*(double)uniform_dist(gen_eng)/maxRandNum;
            }

#ifdef DEBUG_APP_TIMING
            end = prime::util::get_timestamp();
			std::cout << "APP: Time to set up data: " << (double)(end - start)/1000000 << " s" << std::endl;
#endif

            // Fetch RTM's desired knob values
            iters_knob.val = rtm_api.knob_disc_get(iters_knob);
            prec_knob.val = rtm_api.knob_disc_get(prec_knob);
            dev_knob.val = rtm_api.knob_disc_get(dev_knob);
#ifdef KOCL_EN
			// If KOCL enabled and model building, force use of FPGA and exercise both kernels
			if(!KOCL_built(kocl) && kocl_mons.empty()) {
				dev_knob.val = 1;
				prec_knob.val = rand() % 2;
			}
#endif

#ifdef DEBUG_APP
			std::cout << "APP: Knobs: ";
			std::cout << "I:" << iters_knob.val << ", ";
			std::cout << "P:" << (prec_knob.val ? "double" : "float") << ", ";
			std::cout << "D:" << (dev_knob.val ? "GPU" : "CPU") << ", ";
			std::cout << std::endl;
#endif

            // Capture start time
            start = prime::util::get_timestamp();

            // If operating on floats, cast from doubles
            if(!prec_knob.val) {
                for(i = 0; i < N; i++) {
                    for(j = 0; j < N; j++) {
                        A_float[i*N + j] = (float)A[i*N + j];
                    }
                    b_float[i] = (float)b[i];
                    x_float[i] = (float)x[i];
                }
            }
            if(!dev_knob.val) {
                // CPU
                for(i = 0; i < iters_knob.val; i++) {
                    if (!prec_knob.val) { jacobi_float_cpu(A_float, b_float, x_float); }
                    else { jacobi_double_cpu(A, b, x); }
                }
            }
            else {
                // GPU/FPGA
                // Write to memory objects
                if(!prec_knob.val) {
                    ret = clEnqueueWriteBuffer(queue, A_float_buffer, CL_TRUE, 0, N*N*sizeof(float), A_float, 0, NULL, NULL);
                    ret = clEnqueueWriteBuffer(queue, b_float_buffer, CL_TRUE, 0, N*sizeof(float), b_float, 0, NULL, NULL);
                    ret = clEnqueueWriteBuffer(queue, x_float_buffer, CL_TRUE, 0, N*sizeof(float), x_float, 0, NULL, NULL);
                    checkSuccess(ret);
                    assert(ret == CL_SUCCESS);
                }
                else {
                    ret = clEnqueueWriteBuffer(queue, A_double_buffer, CL_TRUE, 0, N*N*sizeof(double), A, 0, NULL, NULL);
                    ret = clEnqueueWriteBuffer(queue, b_double_buffer, CL_TRUE, 0, N*sizeof(double), b, 0, NULL, NULL);
                    ret = clEnqueueWriteBuffer(queue, x_double_buffer, CL_TRUE, 0, N*sizeof(double), x, 0, NULL, NULL);
                    checkSuccess(ret);
                    assert(ret == CL_SUCCESS);
                }

                // Run kernel. Attach event to final one to signify that output memory transfer can commence once completed
                for(i = 0; i < iters_knob.val; i++) {
                    //printf("Enqueuing Kernels...\n");
                    if(!prec_knob.val) {
#ifdef C5SOC
						ret = clEnqueueNDRangeKernel(queue, kernel_float, (cl_uint)1, NULL, &global_work_size[0], &local_work_size, 0, NULL, &kernel_event);
#else
                        ret = clEnqueueNDRangeKernel(queue, kernel_float, (cl_uint)2, NULL, global_work_size, NULL, 0, NULL, &kernel_event);
#endif
                    }
                    else {
#ifdef C5SOC
						ret = clEnqueueNDRangeKernel(queue, kernel_double, (cl_uint)1, NULL, &global_work_size[0], &local_work_size, 0, NULL, &kernel_event);
#else
                        ret = clEnqueueNDRangeKernel(queue, kernel_double, (cl_uint)2, NULL, global_work_size, NULL, 0, NULL, &kernel_event);
#endif
                    }
                    checkSuccess(ret);
                    assert(ret == CL_SUCCESS);
                }

                // Read from memory objects
                if(!prec_knob.val) {
                    ret = clEnqueueReadBuffer(queue, x_float_buffer, CL_TRUE, 0, N*sizeof(float), x_float, 1, &kernel_event, NULL);
                }
                else {
                    ret = clEnqueueReadBuffer(queue, x_double_buffer, CL_TRUE, 0, N*sizeof(double), x, 1, &kernel_event, NULL);
                }
				checkSuccess(ret);
                assert(ret == CL_SUCCESS);
            }

            // If operating on floats, cast back to doubles
            if(!prec_knob.val) {
                for(i = 0; i < N; i++) {
                    x[i] = (double)x_float[i];
                }
            }

            // Calculate elapsed time. Update RTM with currently achieved throughput (cycles per second)
            end  = prime::util::get_timestamp();
            throughput = (prime::api::cont_t)1000000/(end - start);
            rtm_api.mon_cont_set(throughput_mon, throughput);
            throughput_mon.val = throughput;

#ifdef DEBUG_APP_TIMING
            start = prime::util::get_timestamp();
#endif
            // Calculate square of L2 norm. Update RTM with currently achieved error
            norm_sq = l2norm_sq(A, b, x);
            //log_norm_sq = log(norm_sq);

#ifdef DEBUG_APP_TIMING
            end = prime::util::get_timestamp();
			std::cout << "APP: Time to calculate square of L2 norm: " << (double)(end - start)/1000000 << " s" << std::endl;
#endif
            rtm_api.mon_cont_set(error_mon, norm_sq);
			error_mon.val = (prime::api::cont_t)norm_sq;

#ifdef DEBUG_APP
			printf("APP: Jacobi Throughput: %10.4f (solves per second)\n", throughput_mon.val);
			printf("APP: Jacobi Error:      %10.4e (L2 norm square)\n", error_mon.val);
#endif

#ifdef KOCL_EN
			if(KOCL_built(kocl)) {
				if(kocl_mons.empty()){
					// Register KOCL monitors
					std::cout << "Register KOCL monitors" << std::endl;
					for(int mon_num = 0; mon_num < 3; mon_num++)
					{
						kocl_mon_tmp = rtm_api.mon_cont_reg(prime::api::app::PRIME_POW, prime::api::PRIME_CONT_MIN, prime::api::PRIME_CONT_MAX, 1.0);
						kocl_mons.push_back(kocl_mon_tmp);
						std::cout << "Registered KOCL Monitor: ID: " << kocl_mons.back().id << ", min: " << kocl_mons.back().min << ", max: " << kocl_mons.back().max << std::endl;
					}
				}
				std::cout << "KOCL Power Monitors: " << std::endl;
				// Update KOCL monitors
				char kernel_name[64];
				strcpy(kernel_name, "jacobi_float");
				prime::api::cont_t pow = (prime::api::cont_t)KOCL_get(kocl, kernel_name);
				rtm_api.mon_cont_set(kocl_mons[0], pow);
				kocl_mons[0].val = pow;
				std::cout << kernel_name << ": " << pow << ", ";

				strcpy(kernel_name, "jacobi_double");
				pow = (prime::api::cont_t)KOCL_get(kocl, kernel_name);
				rtm_api.mon_cont_set(kocl_mons[1], pow);
				kocl_mons[1].val = pow;
				std::cout << kernel_name << ": " << pow << ", ";

				strcpy(kernel_name, "static");
				pow = (prime::api::cont_t)KOCL_get(kocl, kernel_name);
				rtm_api.mon_cont_set(kocl_mons[2], pow);
				kocl_mons[2].val = pow;
				std::cout << kernel_name << ": " << pow << ", ";
				std::cout << std::endl;
			}
#endif

			try {
				boost::this_thread::interruption_point();
			}
			catch (boost::thread_interrupted&) {
				std::cout << "APP: Jacobi Execution Loop Interrupted" << std::endl;
				return;
			}
        }
        return;
	}

    void jacobi::jacobi_float_cpu(float* A, float* b, float* x)
    {
        //printf("JACOBI: Using float repr.\n");
        void *status;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_t jacobi_thread_float[NUM_PTHREADS];
        jacobi_TD_float jacobi_TD_Array_float[NUM_PTHREADS];

        //printf("JACOBI: Creating threads.\n");

        for (int idx = 0; idx < NUM_PTHREADS; idx++)
        {
            jacobi_TD_Array_float[idx] = {A, b, x, idx};
            pthread_create(&jacobi_thread_float[idx], &attr, jacobi_inner_float, (void *)&jacobi_TD_Array_float[idx]);
        }
        //printf("JACOBI: Threads created.\n");

        for (int idx = 0; idx < NUM_PTHREADS; idx++)
        {
            pthread_join(jacobi_thread_float[idx], &status);
        }
        //printf("JACOBI: Threads joined.\n");

        return;
    }

    void jacobi::jacobi_double_cpu(double* A, double* b, double* x)
    {
        void *status;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_t jacobi_thread_double[NUM_PTHREADS];
        jacobi_TD_double jacobi_TD_Array_double[NUM_PTHREADS];

        for (int idx = 0; idx < NUM_PTHREADS; idx++)
        {
            jacobi_TD_Array_double[idx] = {A, b, x, idx};
            pthread_create(&jacobi_thread_double[idx], &attr, jacobi_inner_double, (void *)&jacobi_TD_Array_double[idx]);
        }

        for (int idx = 0; idx < NUM_PTHREADS; idx++)
        {
            pthread_join(jacobi_thread_double[idx], &status);
        }

        return;
    }

    //repr = 0, float
    void *jacobi_inner_float(void *thread_arg)
    {
        jacobi_TD_float *t_data;
        t_data = (jacobi_TD_float *) thread_arg;

        float *A = t_data->A;
        float *b = t_data->b;
        float *x = t_data->x;
        int idx = t_data->idx;

        for(int i = idx*(N/NUM_PTHREADS); i < (idx + 1)*(N/NUM_PTHREADS); i++)
        {
			float temp = 0.0;

			for(int j = 0; j < N; j++) {
				if(i != j) {
					temp = temp + A[i*N+j]*x[j];
				}
			}
			x[i] = (b[i] - temp)/A[i*N+i];
		}
        return (void *)0;
    }

    //repr = 1, double
    void *jacobi_inner_double(void *thread_arg)
    {
        jacobi_TD_double *t_data;
        t_data = (jacobi_TD_double *) thread_arg;

        double *A = t_data->A;
        double *b = t_data->b;
        double *x = t_data->x;
        int idx = t_data->idx;

        for(int i = idx*(N/NUM_PTHREADS); i < (idx + 1)*(N/NUM_PTHREADS); i++)
        {
			double temp = 0;

			for(int j = 0; j < N; j++) {
				if(i != j) {
					temp = temp + A[i*N+j]*x[j];
				}
			}
			x[i] = (b[i] - temp)/A[i*N+i];
		}
        return (void *)0;
    }

    //Random data
    double jacobi::rand_in_range(double min, double max)
    {
        return min + (max - min)*(double)rand()/RAND_MAX;
    }

    //L2 norm square error
    double jacobi::l2norm_sq(double* A, double* b, double* x)
    {
        int i;
        int j;
        double norm = 0.0;
        double temp;

        for(i = 0; i < N; i++) {
            temp = 0.0;
            for(j = 0; j < N; j++) {
                temp += A[i*N + j]*x[j];
            }
            norm += (temp - b[i])*(temp - b[i]);
        }

        return norm;
    }

    void jacobi::setupOpenCL_ALTERA(cl_context *context, cl_program *program, cl_command_queue *queue)
    {
        int ret;

        // Create platform
        cl_uint num_platforms;
        ret = clGetPlatformIDs(0, NULL, &num_platforms);
        assert(ret == CL_SUCCESS);

        cl_platform_id* platform_id = (cl_platform_id*)malloc(sizeof(cl_platform_id)*num_platforms);
        cl_platform_id platform = NULL;
        ret = clGetPlatformIDs(num_platforms, platform_id, &num_platforms);
        assert(ret == CL_SUCCESS);

        char name[100];
        for(int k = 0; k < (int)num_platforms; k++){
            clGetPlatformInfo(platform_id[k], CL_PLATFORM_VENDOR, sizeof(name), name, NULL);
            platform = platform_id[k];
            if(!strcmp(name, "Altera Corporation")) break;
        }
        assert(!strcmp(name, "Altera Corporation"));

        // Create context
        cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
        *context = clCreateContextFromType(props, CL_DEVICE_TYPE_ACCELERATOR, NULL, NULL, &ret);
        assert(ret == CL_SUCCESS);

        // Create device
        size_t dev_list_size;
        ret = clGetContextInfo(*context, CL_CONTEXT_DEVICES, 0, NULL, &dev_list_size);
        assert(ret == CL_SUCCESS);
        cl_device_id* devices = (cl_device_id*)malloc(dev_list_size);
        ret = clGetContextInfo(*context, CL_CONTEXT_DEVICES, dev_list_size, devices, NULL);
        assert(ret == CL_SUCCESS);
        cl_device_id device = devices[0];

        // Create program
        FILE* handle = fopen(AOCX, "r");
        char* buffer = (char*)malloc(MAX_AOCX_SIZE);
        size_t size = fread(buffer, 1, MAX_AOCX_SIZE, handle);
        fclose(handle);
        *program = clCreateProgramWithBinary(*context, 1, &device, (const size_t*)&size, (const unsigned char**)&buffer, NULL, &ret);

        assert(ret == CL_SUCCESS);
        ret = clBuildProgram(*program, 1, &device, NULL, NULL, NULL);
        assert(ret == CL_SUCCESS);

        // Create command queue
        *queue = clCreateCommandQueue(*context, device, 0, &ret);
        assert(ret == CL_SUCCESS);

        return;
    }

    void jacobi::setupOpenCL_ODROID(cl_context *context, cl_program *program, cl_command_queue *queue)
    {
        if (!createContext(context))
        {
            cleanUpOpenCL(*context, *queue, *program, NULL, NULL, 0);
            cerr << "JACOBI: Failed to create an OpenCL context. " << __FILE__ << ":"<< __LINE__ << endl;
        }

        cl_device_id device;
        if (!createCommandQueue(*context, queue, &device))
        {
            cleanUpOpenCL(*context, *queue, *program, NULL, NULL, 0);
            cerr << "JACOBI: Failed to create the OpenCL command queue. " << __FILE__ << ":"<< __LINE__ << endl;
        }

        if (!createProgram(*context, device, PROG_PATH, program))
        {
            cleanUpOpenCL(*context, *queue, *program, NULL, NULL, 0);
            cerr << "JACOBI: Failed to create OpenCL program." << __FILE__ << ":"<< __LINE__ << endl;
        }
        return;
    }

	void jacobi::ui_mon_disc_min_handler(unsigned int id, prime::api::disc_t min)
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

	void jacobi::ui_mon_disc_max_handler(unsigned int id, prime::api::disc_t max)
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

	void jacobi::ui_mon_disc_weight_handler(unsigned int id, prime::api::disc_t weight)
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

	void jacobi::ui_mon_cont_min_handler(unsigned int id, prime::api::cont_t min)
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

	void jacobi::ui_mon_cont_max_handler(unsigned int id, prime::api::cont_t max)
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

	void jacobi::ui_mon_cont_weight_handler(unsigned int id, prime::api::cont_t weight)
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

	int jacobi::parse_cli(std::string app_name, prime::uds::socket_addrs_t* api_addrs,
			prime::uds::socket_addrs_t* ui_addrs, prime::app::jacobi::app_args_t* args, int argc, const char * argv[])
	{
		args::ArgumentParser parser("Application: " + app_name,"PRiME Project\n");
		parser.LongSeparator("=");
		args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

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

		UTIL_ARGS_LOGGER_PROC();

		if(ui_off) {
			args->ui_en = false;
		}

		return 0;
	}
} }

int main(int argc, const char * argv[])
{
	prime::uds::socket_addrs_t app_addrs, ui_addrs;
	prime::app::jacobi::app_args_t app_args;
	if(prime::app::jacobi::parse_cli("Jacobi", &app_addrs, &ui_addrs, &app_args, argc, argv)) {
		return -1;
	}
	prime::app::jacobi app(&app_addrs, &ui_addrs, &app_args);
}

