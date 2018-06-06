/* This file is part of LRGD, an example RTM for the PRiME Framework.
 * 
 * LRGD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * LRGD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LRGD.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Charles Leech
 */

/*---------------------------------------------------------------------------
   model.h - Linear Regression Model Header
  ---------------------------------------------------------------------------
   Author: Charles Leech
   Email: cl19g10 [at] ecs.soton.ac.uk
  ---------------------------------------------------------------------------*/
#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <deque>
#include <mutex>
#include <limits>
#include <gsl/gsl_multifit.h> // -lgsl -lgslcblas

//#define DEBUG_MODEL

namespace prime { namespace model
{
	class LinearModel
	{
	public:
		LinearModel(unsigned int id, unsigned int predictors);
		~LinearModel();
		bool trained;

		double get_rms_error(void);
		unsigned int get_id(void);
		unsigned int get_num_samples(void);
		unsigned int get_num_predictors(void);
		void set_num_predictors(unsigned int);
		void clear_samples();

		void add_sample(double y, std::vector<double> x);
		void train(void);
		int predict(std::vector<double> x, double *pred_val);
		double calc_rms_error(void);

	private:
		unsigned int id;
		unsigned int num_predictors;
		double rms_error;

		std::deque<double> y_vals;
		std::deque<std::vector<double> > x_vals;

		//std::mutex coefficients_m;
		std::vector<double> coefficients;

	};
} }

#endif // MODEL_H
