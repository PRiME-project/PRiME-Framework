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

#include "phaseprediction.h"

/*Smoothing Parameter*/
#define GAMMA 0.6

/* prototypes*/


/*function definition: For predicting the MRPI of Ti+1 at Ti */
double Predict_MRPI_Next_Period(double predicted_MRPI, double actual_MRPI) {
   predicted_MRPI = GAMMA*actual_MRPI + (1-GAMMA)*predicted_MRPI;
   return predicted_MRPI;
}
