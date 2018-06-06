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
   model.cpp - Linear Regression Model Code
  ---------------------------------------------------------------------------
   Author: Charles Leech
   Email: cl19g10 [at] ecs.soton.ac.uk

   Sources:
   https://www.gnu.org/software/gsl/manual/html_node/Multi_002dparameter-fitting.html
   https://rosettacode.org/wiki/Multiple_regression#C
  ---------------------------------------------------------------------------*/
#include "model.h"

namespace prime { namespace model
{
	LinearModel::LinearModel(unsigned int id, unsigned int predictors) :
		trained(false), id(id), num_predictors(predictors), rms_error(-1)
	{}

	LinearModel::~LinearModel(){}

	unsigned int LinearModel::get_id(void) { return id;	}

	double LinearModel::get_rms_error(void) { return rms_error;	}

	unsigned int LinearModel::get_num_samples(void) { return y_vals.size(); }

	unsigned int LinearModel::get_num_predictors(void) { return num_predictors; }

	void LinearModel::set_num_predictors(unsigned int num) { num_predictors = num; }

	void LinearModel::add_sample(double y, std::vector<double> x)
	{
		if(y_vals.size() > 255)
		{
			y_vals.erase(y_vals.begin(), y_vals.begin()+64);
			x_vals.erase(x_vals.begin(), x_vals.begin()+64);
		}
		y_vals.push_back(y);
		x_vals.push_back(x);
	}

	void LinearModel::clear_samples()
	{
		y_vals.clear();
		x_vals.clear();
		coefficients.clear();
		rms_error = -1;
	}

	//train model when enough samples are collected
	void LinearModel::train(void)
	{
		unsigned int num_samples = y_vals.size();
#ifdef DEBUG_MODEL
		//printf("MODEL: Training LR model\n");
		//printf("MODEL: Number of predictors = %d\n", num_predictors);
		//printf("MODEL: Number of samples = %d\n", num_samples);
#endif // DEBUG_MODEL

		gsl_matrix *X = gsl_matrix_alloc (num_samples, num_predictors+1); //allocate predictor matrix
		gsl_vector *Y = gsl_vector_alloc (num_samples); //allocate response vector
		gsl_vector *C = gsl_vector_alloc (num_predictors+1); //allocate coefficient vector
		gsl_matrix *COV = gsl_matrix_alloc (num_predictors+1, num_predictors+1); //allocate covariance matrix
		gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc (num_samples, num_predictors+1);

		for(unsigned int n = 0;  n < num_samples; n++)
		{
			gsl_matrix_set (X, n, 0, 1.0);
			for(unsigned int j = 0;  j < num_predictors; j++)
			{
#ifdef DEBUG_MODEL
				//printf("MODEL: x[%d] = %f\n", j, x_vals[n][j]);
#endif // DEBUG_MODEL
				gsl_matrix_set (X, n, j+1, x_vals[n][j]);  // set predictor vector values
			}
#ifdef DEBUG_MODEL
			//printf("MODEL:y = %f\n", y_vals[n]);
#endif // DEBUG_MODEL
			gsl_vector_set (Y, n, y_vals[n]);  // set response vector values
		}

		double chisq;
		gsl_multifit_linear (X, Y, C, COV, &chisq, work);


		//coefficients_m.lock();
		coefficients.clear();
		for(unsigned int j = 0;  j < num_predictors + 1; j++)
		{
			coefficients.push_back(gsl_vector_get(C,j));
		}
		//coefficients_m.unlock();

#ifdef DEBUG_MODEL
		std::cout << "MODEL: Linear model coefficients: ";
		for(unsigned int j = 0;  j < num_predictors + 1; j++)
		{
			std::cout << "c[" << j << "] = " << coefficients[j] << ", ";
		}
		std::cout << std::endl;
#endif // DEBUG_MODEL

		gsl_matrix_free (X);
		gsl_vector_free (Y);
		gsl_vector_free (C);
		gsl_matrix_free (COV);
		gsl_multifit_linear_free (work);

		//enhancement: added test to calc rms_error
		trained = true;
		return;
	}

	int LinearModel::predict(std::vector<double> x, double *pred_val)
	{
		if(coefficients.size() != x.size()+1)
			return -1;

		double y = coefficients[0];
		for(unsigned int j = 0; j < num_predictors; j++)
			y += coefficients[j+1] * x[j];
		*pred_val = y;
		return 0;
	}

	double LinearModel::calc_rms_error(void)
	{
		//TODO: calcuate rms error
		return rms_error;
	}
} }
