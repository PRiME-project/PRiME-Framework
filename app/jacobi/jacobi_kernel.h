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


#ifndef JACOBI_KERNEL_H
#define JACOBI_KERNEL_H

#define N 256				// Problem size: A = (N x N), b = (N x 1)
#define UNROLL_LEVEL 8

#ifdef C5SOC
#define MWGS N
#else
#define MWGS 256
#endif

#define NUM_PTHREADS 8		//Block threading level for pthreads implementation
	
// Moved to CMakeLists
// Device selection. Comment out for Odroid
//#define C5SOC

#endif
