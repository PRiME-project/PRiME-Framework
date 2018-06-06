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
   regression.h - Runtime Manager Header
  ---------------------------------------------------------------------------
   Author: Charles Leech
   Email: cl19g10 [at] ecs.soton.ac.uk
  ---------------------------------------------------------------------------*/
#ifndef REGRESSION_H
#define REGRESSION_H

#include <iostream>
#include <sstream>
#include <string>
#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include <map>
#include <limits.h>
#include <cmath>

#include "prime_api_rtm.h"
#include "util.h"
#include "model.h"

//#define DEBUG_RTM

namespace prime
{
	class regression
	{
	public:
		regression(
			std::vector<prime::api::app::knob_disc_t> const &app_knobs_disc,
			std::vector<prime::api::app::knob_cont_t> const &app_knobs_cont,
			std::vector<prime::api::dev::knob_disc_t> const &dev_knobs_disc,
			std::vector<prime::api::dev::knob_cont_t> const &dev_knobs_cont,
			unsigned int num_training_samples);
		~regression();
		//variables

		//Functions
        void add_app_model(unsigned int mon_id);
		void add_dev_model(unsigned int mon_id);//, prime::api::dev::mon_type_t mon_type);

		void remove_app_model(unsigned int mon_id);
		void remove_dev_model(unsigned int mon_id);//, prime::api::dev::mon_type_t mon_type);

		void add_knob(void);

		void add_data(unsigned int mon_id, prime::api::cont_t mon_val);
		void check_app_mon_disc_bounds(prime::api::app::mon_disc_t app_mon);
		void check_app_mon_cont_bounds(prime::api::app::mon_cont_t app_mon);
		int optimise_mon(unsigned int opt_mon_id, unsigned int bound_mon_id, double bound_mon_min, std::vector<double> &knob_vals);
		int predict_mon(unsigned int mon_id, double *pred_val);

	private:
		//variables
		unsigned int num_monitors;
		unsigned int num_knobs;

		//std::vector<prime::api::app::mon_disc_t> app_mon_disc;
		//std::vector<prime::api::app::mon_cont_t> app_mon_cont;

		std::mutex knobs_disc_m;
		std::mutex knobs_cont_m;
		std::vector<prime::api::app::knob_disc_t> const &app_knobs_disc;
		std::vector<prime::api::app::knob_cont_t> const &app_knobs_cont;
		std::vector<prime::api::dev::knob_disc_t> const &dev_knobs_disc;
		std::vector<prime::api::dev::knob_cont_t> const &dev_knobs_cont;

		//std::mutex models_m;
		std::vector<model::LinearModel> models;
		//std::map<unsigned int, model::LinearModel> models;
		unsigned int min_samples;
		bool stop_training;

		//functions
        void train_model(model::LinearModel &model);
		void update_model(model::LinearModel &model, unsigned int num_pred, bool reset);
		model::LinearModel* get_model_ptr(unsigned int mon_id);
		bool check_bool_vector_nand(std::vector<bool> check_vector);

		std::vector<double> dydx(model::LinearModel* y, std::vector<double> x, std::vector<double> dx);
		int gradient_decent(model::LinearModel *opt_model, model::LinearModel *app_model, double app_min, std::vector<double> &knob_vals);

	};
}

#endif // REGRESSION_H
