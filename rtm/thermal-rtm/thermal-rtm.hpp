/* This file is part of Thermal RTM, an example RTM for the PRiME Framework.
 * 
 * Thermal RTM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Thermal RTM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Thermal RTM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Eduardo Weber-Wachter
 */

#ifndef ADAPTIVEDVFS_H_
#define ADAPTIVEDVFS_H_


#define CONSIDER_TEMP_MANAGEMENT 1
#define CONSIDER_MRPI_POWER_MANAGEMENT 1

#define BIG_FREQ_SIZE 18
#define LITTLE_FREQ_SIZE 12



/*
 *  MRPI POWER MANAGEMENT CONSTANTS
 *
 *
 */
#define SCALING_STOP_LENGTH 10
#define MRPI_SAMPLE_PERIOD 90000000L
#define SAMPLE_PERIOD 100000000L


/*
 *  TEMPERATURE MANAGEMENT CONSTANTS
 *
 */
#define CONSIDER_TEMP_FEEDBACK 1
#define TEMP_MANAGEMENT_PERIOD 1000000000L
#define START_PREDICTION_TEMP 0
#define TEMP_TRESHOLD 65

//Regression coefficients : proposed model
#define INTERCEPT 24.7760
#define AR1 0.2862
#define AR2 0.1354
#define POW_B 4.4316
#define POW_L 0
#define POW_GPU 0
#define POW_MEM 6.2712
#define ERROR 1
#define ARMA1 0.2337
#define ARMA2 0.1424

//Regression of past paper : predictive ...  Also remove consider temp feedback
//#define INTERCEPT 26.1628
//#define AR1 0.4017
//#define AR2 0
//#define POW_B 4.33422
//#define POW_L 0
//#define POW_GPU 0
//#define POW_MEM 6.1134
//#define ERROR 0
//#define ARMA1 0
//#define ARMA2 0


//Leakage power coefficients
#define C1_L 0.0000058256141184
#define C1_B 0.000062699677130684
#define C2_L 96.44072958
#define C2_B -67.49750985

// int Frequency_Table_Big[BIG_FREQ_SIZE+1];
int Frequency_Table_Big[]= {200000,300000,400000,500000,600000,700000,800000,900000,1000000,1100000,1200000,1300000,1400000,1500000,1600000,1700000,1800000,1900000,2000000};
// double VDD_B[BIG_FREQ_SIZE+1];

// int Frequency_Table_Little[LITTLE_FREQ_SIZE+1];
int Frequency_Table_Little[]= {200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000, 1000000, 1100000, 1200000, 1300000, 1400000};
// double VDD_L[LITTLE_FREQ_SIZE+1];

double VDD_B[] = {0.913750, 0.913750, 0.913750, 0.913750, 0.913750, 0.913750, 0.913750, 0.921250, 0.950000, 0.975000, 1.000000, 1.025000, 1.037500, 1.058750, 1.093750, 1.131250, 1.170000, 1.221250 ,1.297500};
double VDD_L[] = {0.913750, 0.913750, 0.913750, 0.913750, 0.925000, 0.962500, 1.000000, 1.037500, 1.072500, 1.108750, 1.150000, 1.198750, 1.247500};


#endif