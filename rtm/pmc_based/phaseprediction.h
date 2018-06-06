/* This file is part of MRPI, an example RTM for the PRiME Framework.
 * 
 * MRPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MRPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MRPI.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Karunakar Basireddy
 */

/*Smoothing Parameter*/
#define GAMMA 0.6
#define COMPENSATE_ERROR 1
#define CONSIDER_FEEDBACK 1
/* prototypes*/


double Predict_MRPI_Next_Period(double predicted_MRPI, double actual_MRPI);

/*function definition: For predicting the MRPI of Ti+1 at Ti */
double Predict_MRPI_Next_Period(double predicted_MRPI, double actual_MRPI)
{
    predicted_MRPI = GAMMA*actual_MRPI + (1-GAMMA)*predicted_MRPI;
    return predicted_MRPI;
}

/*Prediction error*/

//double prediction_slack = 0;
//if(PERF_AVAILABLE) {
// prediction_error = actual_MRPI - predicted_MRPI;
// }

//unsigned long double n_compensated_MRPI_p = predicted_MRPI + COMPENSATE_ERROR*prediction_error;
