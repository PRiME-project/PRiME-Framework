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

#ifndef JACOBI_H
#define JACOBI_H

#include <cstring> //strncmp
#include <unistd.h> //getpid()
#include <assert.h> //assert
#include <sys/time.h> //gettimeofday()
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstddef>
#include <string>
#include <time.h>
#include <math.h>
#include <ctime>
#include <chrono>

//POSIX Threads
#include <pthread.h>
#include <sched.h>

#include "oclUtil.h"

#include "jacobi_kernel.h"

namespace prime { namespace app
{
    #define CONVERGENCE_THRESHOLD 1.0e-12	// l2 norm threshold at which to declare solution found
    #define THROUGHPUT_THRESHOLD 100 // (Hz/FPS)

	//#define DEBUG_APP
	//#define DEBUG_APP_TIMING

	// KOCL enable/disable
#ifdef C5SOC
	//#define KOCL_EN
#endif

	// OpenCL for Altera Cyclone V:
#ifdef KOCL_EN
	#define AOCX "jacobi_kocl.aocx"
#else
    #define AOCX "jacobi.aocx"
#endif
    #define MAX_AOCX_SIZE 100000000

    // OpenCL for ODROID:
    #define PROG_PATH "../app/jacobi/assets/jacobi.cl"

    typedef struct {
        float* A;
        float* b;
        float* x;
        int idx;
        } jacobi_TD_float;

    typedef struct {
        double* A;
        double* b;
        double* x;
        int idx;
        } jacobi_TD_double;

    static void *jacobi_inner_float(void *thread_arg);
    static void *jacobi_inner_double(void *thread_arg);

}}
#endif
