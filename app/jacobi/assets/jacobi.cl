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

#ifndef C5SOC
	#include "../app/jacobi/jacobi_kernel.h"
	#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#else
	#include "../jacobi_kernel.h"
#endif

/* Kernel jacobi_float()
Computes Jacobi solution of Ax = b in single-precision floating point
A: pointer to (N x N) matrix A
b: pointer to (N x 1) vector b
x: pointer to (N x 1) vector x */

#ifdef C5SOC
	__attribute__((reqd_work_group_size(N, 1, 1)))
	__attribute__((max_work_group_size(N)))
#endif
__kernel void jacobi_float(__global const float* restrict A, 
							__global const float* restrict b,
							__global float* restrict x) 
{
#ifdef C5SOC
	int i = get_global_id(0);
#else
	int m = get_global_id(0);
	int k = get_global_id(1);
	int i = k*MWGS + m;
#endif

//	__local float x_local[N];
	
//	x_local[i] = x[i];
	
//	barrier(CLK_LOCAL_MEM_FENCE);
	
	float temp = 0.0;
	
	int j;
#ifdef C5SOC
	#pragma unroll UNROLL_LEVEL
#endif
	for(j = 0; j < N; j++) {					// temp = sum over i != j of A[i,j]*x[j]
		if(i != j) {
//			temp += A[i*N + j]*x_local[j];
			temp += A[i*N + j]*x[j];
		}
	}
	
	x[i] = (b[i] - temp)/A[i*N + i];			// x[i] = (b[i] - temp)/A[i,i]
	
//	barrier(CLK_GLOBAL_MEM_FENCE);

}

/* Kernel jacobi_double()
Computes Jacobi solution of Ax = b in double-precision floating point
A: pointer to (N x N) matrix A
b: pointer to (N x 1) vector b
x: pointer to (N x 1) vector x */

#ifdef C5SOC
	__attribute__((reqd_work_group_size(N, 1, 1)))
	__attribute__((max_work_group_size(N)))
#endif
__kernel void jacobi_double(__global const double* restrict A, 
							__global const double* restrict b, 
							__global double* restrict x)
{
#ifdef C5SOC
	int i = get_global_id(0);
#else
	int m = get_global_id(0);
	int k = get_global_id(1);
	int i = k*MWGS + m;
#endif
	
//	__local double x_local[N];
	
//	x_local[i] = x[i];
	
//	barrier(CLK_LOCAL_MEM_FENCE);
	
	double temp = 0.0;
	
	int j;
#ifdef C5SOC
	#pragma unroll UNROLL_LEVEL
#endif
	for(j = 0; j < N; j++) {					// temp = sum over i != j of A[i,j]*x[j]
		if(i != j) {
//			temp += A[i*N + j]*x_local[j];
			temp += A[i*N + j]*x[j];
		}
	}
	
	x[i] = (b[i] - temp)/A[i*N + i];			// x[i] = (b[i] - temp)/A[i,i]
	
//	barrier(CLK_GLOBAL_MEM_FENCE);

}
