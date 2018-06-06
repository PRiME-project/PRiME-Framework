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
   regression.cpp - Runtime Manager Class
  ---------------------------------------------------------------------------
   Author: Charles Leech
   Email: cl19g10 [at] ecs.soton.ac.uk
  ---------------------------------------------------------------------------*/
#include "regression.h"
#include "model.h"

namespace prime
{
	regression::regression(
		std::vector<prime::api::app::knob_disc_t> const &app_knobs_disc,
		std::vector<prime::api::app::knob_cont_t> const &app_knobs_cont,
		std::vector<prime::api::dev::knob_disc_t> const &dev_knobs_disc,
		std::vector<prime::api::dev::knob_cont_t> const &dev_knobs_cont,
		unsigned int num_training_samples) :
			app_knobs_disc(app_knobs_disc),
			app_knobs_cont(app_knobs_cont),
			dev_knobs_disc(dev_knobs_disc),
			dev_knobs_cont(dev_knobs_cont),
			num_monitors(0),
			num_knobs(0),
			min_samples(num_training_samples),
			stop_training(false)
	{
		srand(time(NULL));
		std::cout << "LRGD: Regression class initialised." << std::endl;
		return;
	}

	regression::~regression()
	{
		std::cout << "LRGD: Shutting down Regression class." << std::endl;
	}

	void regression::add_app_model(unsigned int mon_id)
	{
		models.push_back(model::LinearModel(mon_id, num_knobs));
		num_monitors++;
    }

	void regression::add_dev_model(unsigned int mon_id)//, prime::api::dev::mon_type_t mon_type)
	{
		models.push_back(model::LinearModel(mon_id, num_knobs));
		num_monitors++;
    }

	void regression::remove_app_model(unsigned int mon_id)
	{
		models.erase(std::remove_if(models.begin(), models.end(),
				[=](model::LinearModel model){return model.get_id() == mon_id;}),
			models.end());
    }

	void regression::remove_dev_model(unsigned int mon_id)//, prime::api::dev::mon_type_t mon_type)
	{
		models.erase(std::remove_if(models.begin(), models.end(),
				[=](model::LinearModel model){return model.get_id() == mon_id;}),
			models.end());
    }

	void regression::add_knob(void)
	{
		num_knobs++;
		for(auto &model : models)
			update_model(model, num_knobs, true);
    }

    void regression::update_model(model::LinearModel &model, unsigned int num_pred, bool reset)
	{
		model.set_num_predictors(num_pred);
		if(reset)
			model.clear_samples();
    }

	void regression::add_data(unsigned int mon_id, prime::api::cont_t mon_val)
	{
#ifdef DEBUG_RTM
		if(mon_id)
			std::cout << "LRGD: add_data: mon id = " << mon_id << ", val = " << mon_val << std::endl;
#endif //DEBUG_RTM
		for(auto& model : models)
		{
			if(model.get_id() == mon_id)
			{
				std::vector<double> knob_vals;
				for(auto knob : app_knobs_disc){ knob_vals.push_back((double)knob.val);	}
				for(auto knob : app_knobs_cont){ knob_vals.push_back((double)knob.val);	}
				for(auto knob : dev_knobs_disc){ knob_vals.push_back((double)knob.val);	}
				for(auto knob : dev_knobs_cont){ knob_vals.push_back((double)knob.val);	}

#ifdef DEBUG_RTM
				if(mon_id){
					std::cout << "LRGD: Knob vals: ";
					for(auto kv : knob_vals)
						std::cout << kv << ", ";
					std::cout <<  std::endl;
				}
#endif //DEBUG_RTM
				//models_m.lock();
				model.add_sample((double)mon_val, knob_vals);
				//models_m.unlock();

				int curr_samples = model.get_num_samples();
#ifdef DEBUG_RTM
				if(mon_id)
					std::cout << "LRGD: Current number of samples = " << curr_samples << std::endl;
#endif //DEBUG_RTM
				if((!(curr_samples % min_samples) && curr_samples > 0) && !stop_training){
					train_model(model);
				}
				return;
			}
		}
    }

    void regression::train_model(model::LinearModel &model)
	{
        auto start_time = util::get_timestamp();
		//models_m.lock();
		model.train();
		//models_m.unlock();
        float multifit_linear_time = (float)(util::get_timestamp() - start_time)/1000;
#ifdef DEBUG_RTM
        std::cout << "LRGD: Trained model ID: " << model.get_id() << " in " << multifit_linear_time << " ms"  <<  std::endl;
#endif //DEBUG_RTM
	}

	//call when mon val updated by app
	void regression::check_app_mon_disc_bounds(prime::api::app::mon_disc_t app_mon) {}

	//call when mon val updated by app
	void regression::check_app_mon_cont_bounds(prime::api::app::mon_cont_t app_mon)	{}

	//test whether all elements are false
	bool regression::check_bool_vector_nand(std::vector<bool> check_vector)
	{
		bool check_sum = true;
		for(auto elem : check_vector)
			check_sum &= elem;
		return !check_sum;
	}

	model::LinearModel* regression::get_model_ptr(unsigned int mon_id)
	{
		for(auto &model : models)
		{
			if(model.get_id() == mon_id){
				if(!model.trained){
					std::cout << "LRGD: Error: Model for monitor ID: " << mon_id << " must be trained before it can be optimised." << std::endl;
					return nullptr;
				}
				return &model;
			}
		}
		std::cout << "LRGD: Error: Model for monitor ID: " << mon_id << " could not be found." << std::endl;
		return nullptr;
	}

	//call to optimise device monitor - use app mon bounds (min/max) as constraints
	int regression::optimise_mon(unsigned int opt_mon_id, unsigned int bound_mon_id, double bound_mon_min, std::vector<double> &knob_vals)
	{
		//find model of dev monitor to optimise
		model::LinearModel *opt_model = get_model_ptr(opt_mon_id);
		if(opt_model == nullptr){
			return -1;
		}

		//find model of app monitor used for constraints
		model::LinearModel *bounding_model = get_model_ptr(bound_mon_id);
		if(bounding_model == nullptr){
			return -1;
		}

		//do gradient decent
#ifdef DEBUG_RTM
		std::cout << "LRGD: Performing gradient descent..." << std::endl;
#endif //DEBUG_RTM
		return gradient_decent(opt_model, bounding_model, bound_mon_min, knob_vals);
	}

	//find rate of change of app mon wrt knobs
	//r-code: c(fy(x+dx[,1])-fy(x),fy(x+dx[,2])-fy(x))/diag(dx)
	std::vector<double> regression::dydx(model::LinearModel* y, std::vector<double> x, std::vector<double> dx)
	{
		if(y->get_num_predictors() != x.size()){
			std::cout << "LRGD: INFO: Number of model predictors does not match number of knobs" << std::endl;
		}

		std::vector<double> fy;
		std::vector<double> x_dx(x);
		double fy_dx, fy_x;
		for(unsigned int i = 0; i < x.size(); i++)
		{
			x_dx[i] = x[i] - dx[i];

			y->predict(x, &fy_x);
			y->predict(x_dx, &fy_dx);

			//fy.push_back((fy_dx - fy_x)/dx[i]);  //fy(x+dx[,1])-fy(x)
			fy.push_back((fy_x - fy_dx)/dx[i]);  //fy(x+dx[,1])-fy(x)

#ifdef DEBUG_RTM
			std::cout << "LRGD: x[" << i << "] = " << x[i] << ", x_dx[" << i << "] = " << x_dx[i] << std::endl;
			std::cout << "LRGD: fy_dx = " << fy_dx << ", fy_x = " << fy_x << std::endl;
			std::cout << "LRGD: fy[" << i << "] = " << fy[i] << std::endl << std::endl;
#endif // DEBUG_RTM

			x_dx[i] += dx[i];
		}
		if(fy.size() != x.size()){
			std::cout << "LRGD: INFO: number of dydx predicted values does not match number of knobs" << std::endl;
		}
		return fy;
	}

	//gradient decent loop - return a set of knob values that meet the bounds of an app monitor and optimise a device monitor
	//knobs set in regression class
	int regression::gradient_decent(model::LinearModel *opt_model, model::LinearModel *bounding_model, double bound, std::vector<double> &knob_vals)
	{
		std::vector<double> knob_mins;
		std::vector<double> knob_maxs;

		for(auto knob : app_knobs_disc){
			knob_mins.push_back(knob.min);
			knob_maxs.push_back(knob.max);
		}
		for(auto knob : app_knobs_cont){
			knob_mins.push_back(knob.min);
			knob_maxs.push_back(knob.max);
		}
		for(auto knob : dev_knobs_disc){
			knob_mins.push_back(knob.min);
			knob_maxs.push_back(knob.max);
		}
		for(auto knob : dev_knobs_cont){
			knob_mins.push_back(knob.min);
			knob_maxs.push_back(knob.max);
		}
		//initialise knob values to their minimum
		knob_vals = std::vector<double>(knob_maxs);// x = c(0,0);

		double app_kv_max, app_kv;
		bounding_model->predict(knob_maxs, &app_kv_max);
		bounding_model->predict(knob_mins, &app_kv);
		std::vector<double> alpha(knob_vals.size(),100); // a=(pred_perf(x+1)-pred_perf(x))/2;
		double beta = 0.5;
		double opt_mon_val, bound_mon_val;

		std::vector<double> prev_dpdx (knob_vals.size(),1);
		std::vector<double> dpdx (knob_vals.size(),0);
		std::vector<double> xn(knob_vals.size(), 0);
		std::vector<bool> kv_done(knob_vals.size(), false);

		std::vector<double> dx (knob_vals.size(), 1.0); // dx= matrix(c(1e-3,0,0,1e-3),nrow=2,ncol=2);
		for(unsigned int dx_idx = 0; dx_idx < dx.size(); dx_idx++)
		{
			dx[dx_idx] = (knob_maxs[dx_idx] - knob_mins[dx_idx])/1e3; //emulating dx= matrix(c(1e-3,0,0,1e-3),nrow=2,ncol=2);
		}

		std::vector<double> bound_mon_steps, opt_mon_steps;//, alpha_steps; //y1_lst=NULL, pwr_lst=NULL
		std::vector<std::vector<double> > knob_vals_steps; //xc_lst=NULL; xf_lst=NULL;

		while(check_bool_vector_nand(kv_done))
		{
			//dpdx = dydx(bounding_model, knob_vals, dx); //dpdx = dydx(pred_perf,x,dx)
			dpdx = dydx(opt_model, knob_vals, dx); //dpdx = dydx(pred_pwr,x,dx)

#ifdef DEBUG_RTM
			std::cout << "LRGD: alpha: ";
			for(auto a_xn : alpha){
				std::cout << a_xn << ", ";
			}
			std::cout << std::endl;
			std::cout << "LRGD: dpdx: ";
			for(auto idx : dpdx){
				std::cout << idx << ", "; // print(dpdx)
			}
			std::cout << std::endl;
#endif // DEBUG_RTM

			if(bounding_model->predict(knob_vals, &bound_mon_val))
				std::cout << "LRGD: Bound model prediction error" << std::endl;
#ifdef DEBUG_RTM
			else
				std::cout << "LRGD: Max bounding_model value = " << bound_mon_val << std::endl;
#endif // DEBUG_RTM

			if(bound_mon_val < bound)
				break; // Max knobs still doesn't meet bound - nothing can be done!

			while(check_bool_vector_nand(kv_done))
			{
					for(unsigned int kvs_idx = 0; kvs_idx < knob_vals.size(); kvs_idx++)
					{
						//xn = x+dpdx*alpha
						//xn[kvs_idx] = knob_vals[kvs_idx] + (dpdx[kvs_idx] * alpha);
						// all(xn>0&xn<1) // check knob's val is in its bounds
						//knob_bounds_met = knob_bounds_met && ((knob_mins[kvs_idx] <= xn[kvs_idx]) && (knob_maxs[kvs_idx] >= xn[kvs_idx]));
						if(!kv_done[kvs_idx])
						{
							xn[kvs_idx] = knob_vals[kvs_idx] - abs(dpdx[kvs_idx] * alpha[kvs_idx]); // increment knob x

							while((knob_mins[kvs_idx] > xn[kvs_idx]) || (knob_maxs[kvs_idx] < xn[kvs_idx])) //check knob bound
							{
								alpha[kvs_idx] = alpha[kvs_idx] * beta;	//if outside bounds, reduce step size
								xn[kvs_idx] = knob_vals[kvs_idx] - abs(dpdx[kvs_idx] * alpha[kvs_idx]); //and reduce xn
							}

							if(xn[kvs_idx] - knob_vals[kvs_idx] == 0) //if alpha has not diminished
							{
								kv_done[kvs_idx] = true;
#ifdef DEBUG_RTM
								std::cout << "LRGD: kvs_idx " << kvs_idx << " done." << std::endl;
#endif // DEBUG_RTM
							}
						}
					}
#ifdef DEBUG_RTM
				std::cout << "LRGD: xn: ";
				for(unsigned int kvs_idx = 0; kvs_idx < knob_vals.size(); kvs_idx++){
					std::cout << xn[kvs_idx] << ", "; //print(xn)
				}
				std::cout << std::endl;
#endif // DEBUG_RTM

				if(bounding_model->predict(xn, &bound_mon_val)){
					std::cout << "LRGD: INFO: Cannot make model prediction." << std::endl;
				}
				if(bound_mon_val > bound){
					knob_vals = xn;
					break;
				}
				for(auto& a_xn : alpha){
					a_xn = a_xn * beta; //shrink step size
				}
#ifdef DEBUG_RTM
					std::cout << "LRGD: bound_mon_val = " << bound_mon_val << std::endl;
					std::cout << "LRGD: bound = " << bound << std::endl;
					for(auto& a_xn : alpha) std::cout << "LRGD: alpha = " << a_xn << std::endl;
#endif // DEBUG_RTM
			}
			for(unsigned int idx = 0; idx < prev_dpdx.size(); idx++)
			{
				if((prev_dpdx[idx] * dpdx[idx]) < 0)
					alpha[idx] = alpha[idx] * beta; //shrink step size
			}
			prev_dpdx = dpdx;
		}
//		if(!bound_mon_steps.size()){
//#ifdef DEBUG_RTM
//			if(opt_model->predict(xn, &bound_mon_val)){
//				std::cout << "LRGD: INFO: Cannot make model prediction." << std::endl;
//			}
//			std::cout << "LRGD: Optimisation Result not Found - setting maximum knob values." << std::endl;
//			std::cout << "LRGD: Optimised Monitor Value = " << opt_mon_val << ", Bounding Monitor Value = " << bound_mon_val << std::endl;
//			std::cout << "LRGD: Optimal Knob Values: ";
//			int idx = 0;
//			for(auto val : knob_vals){
//				std::cout << "x[" << idx << "]: " << val << ", ";
//				idx++;
//			}
//			std::cout << std::endl;
//#endif // DEBUG_RTM
//			return -1;
//		}
//		else{
//#ifdef DEBUG_RTM
			//std::cout << "LRGD: Optimisation Result Found:" << std::endl;
			std::cout << "LRGD: Optimised Monitor Value = " << opt_mon_val << ", Bounding Monitor Value = " << bound_mon_val << std::endl;
			std::cout << "LRGD: Optimal Knob Values: ";
			int idx = 0;
			for(auto val : knob_vals){
				std::cout << "x[" <<idx << "]: " << val << ", ";
				idx++;
			}
			std::cout << std::endl;
			return 0;
//#endif // DEBUG_RTM
//		}
	}

	//gradient ascent loop - return a set of knob values that meet the bounds of an app monitor and optimise a device monitor
	//knobs set in regression class
//	int regression::gradient_ascent(model::LinearModel *opt_model, model::LinearModel *bounding_model, double bound, std::vector<double> &knob_vals)
//	{
//		std::vector<double> knob_mins;
//		std::vector<double> knob_maxs;
//
//		for(auto knob : app_knobs_disc){
//			knob_mins.push_back(knob.min);
//			knob_maxs.push_back(knob.max);
//		}
//		for(auto knob : app_knobs_cont){
//			knob_mins.push_back(knob.min);
//			knob_maxs.push_back(knob.max);
//		}
//		for(auto knob : dev_knobs_disc){
//			knob_mins.push_back(knob.min);
//			knob_maxs.push_back(knob.max);
//		}
//		for(auto knob : dev_knobs_cont){
//			knob_mins.push_back(knob.min);
//			knob_maxs.push_back(knob.max);
//		}
//		//initialise knob values to their minimum
//		knob_vals = std::vector<double>(knob_mins);// x = c(0,0);
//
//		double app_kv_max, app_kv;
//		bounding_model->predict(knob_maxs, &app_kv_max);
//		bounding_model->predict(knob_vals, &app_kv);
//		std::vector<double> alpha(knob_vals.size(),(app_kv_max - app_kv)/10); // a=(pred_perf(x+1)-pred_perf(x))/2;
//		double beta = 0.5;
//		double conv = 1e-3; // conv=c(1e-3,1e-3);
//		double opt_mon_val, bound_mon_val;
//		bool done = false;
//
//		std::vector<double> prev_dpdx (knob_vals.size(),1);
//		std::vector<double> dpdx (knob_vals.size(),0);
//		std::vector<double> xn(knob_vals.size(), 0);
//
//		std::vector<double> dx (knob_vals.size(), 1.0); // dx= matrix(c(1e-3,0,0,1e-3),nrow=2,ncol=2);
//		for(unsigned int dx_idx = 0; dx_idx < dx.size(); dx_idx++)
//		{
//			dx[dx_idx] = (knob_maxs[dx_idx] - knob_mins[dx_idx])/1e3; //emulating dx= matrix(c(1e-3,0,0,1e-3),nrow=2,ncol=2);
//#ifdef DEBUG_RTM
////			std::cout << "dx[" << dx_idx << "]: " << dx[dx_idx] << ", ";
//#endif // DEBUG_RTM
//		}
//#ifdef DEBUG_RTM
//		std::cout << std::endl;
//#endif // DEBUG_RTM
//
//		std::vector<double> bound_mon_steps, opt_mon_steps;//, alpha_steps; //y1_lst=NULL, pwr_lst=NULL
//		std::vector<std::vector<double> > knob_vals_steps; //xc_lst=NULL; xf_lst=NULL;
//
//		while(1)
//		{
//			dpdx = dydx(bounding_model, knob_vals, dx); //dpdx = dydx(pred_perf,x,dx)
//			//dpdx = dydx(opt_model, knob_vals, dx); //dpdx = dydx(pred_pwr,x,dx)
//
//			for(auto idx : dpdx){
//				if(idx < 0){
//					std::cout << "LRGD: INFO: Negative gradient in model" << std::endl;
//					std::cout << "LRGD: INFO: Retraining model and waiting for more training samples to improve the model." << std::endl;
//					update_model(*bounding_model, num_knobs, true);
//					return -1;
//				}
//			}
//
//#ifdef DEBUG_RTM
//			std::cout << "LRGD: alpha = " << alpha[0] << std::endl;
//			std::cout << "LRGD: dpdx: ";
//			for(auto idx : dpdx){
//				std::cout << idx << ", "; // print(dpdx)
//			}
//			std::cout << std::endl;
//#endif // DEBUG_RTM
//
//			if(opt_model->predict(knob_vals, &opt_mon_val))
//				std::cout << "LRGD: Opt model prediction error" << std::endl;
//			if(bounding_model->predict(knob_vals, &bound_mon_val))
//				std::cout << "LRGD: Bound model prediction error" << std::endl;
//
//			while(1)
//			{
//				//bool knob_bounds_met = false;
//				//while(!knob_bounds_met)
//				//{
//					//knob_bounds_met = true;
//					for(unsigned int kvs_idx = 0; kvs_idx < knob_vals.size(); kvs_idx++)
//					{
//						//xn = x+dpdx*alpha
//						//xn[kvs_idx] = knob_vals[kvs_idx] + (dpdx[kvs_idx] * alpha);
//						// all(xn>0&xn<1) // check knob's val is in its bounds
//						//knob_bounds_met = knob_bounds_met && ((knob_mins[kvs_idx] <= xn[kvs_idx]) && (knob_maxs[kvs_idx] >= xn[kvs_idx]));
//						if(xn[kvs_idx] - knob_vals[kvs_idx]) //if alpha has not diminished
//						{
//							xn[kvs_idx] = knob_vals[kvs_idx] + (dpdx[kvs_idx] * alpha[kvs_idx]); // increment knob x
//
//							while((knob_mins[kvs_idx] > xn[kvs_idx]) && (knob_maxs[kvs_idx] < xn[kvs_idx])) //check knob bound
//							{
//								alpha[kvs_idx] = alpha[kvs_idx] * beta;	//if outside bounds, reduce step size
//								xn[kvs_idx] = knob_vals[kvs_idx] + (dpdx[kvs_idx] * alpha[kvs_idx]); //and reduce xn
//							}
//						}
//					}
//					//if(!knob_bounds_met){
//					//	alpha = alpha * beta; //shrink learning rate if xn is out of range
//					//}
//				//}
//#ifdef DEBUG_RTM
//				for(unsigned int kvs_idx = 0; kvs_idx < knob_vals.size(); kvs_idx++){
//					std::cout << "LRGD: xn[" << kvs_idx << "] = " << xn[kvs_idx] << std::endl; //print(xn)
//				}
//#endif // DEBUG_RTM
//				bool knobs_conv = true;
//				for(unsigned int kvs_idx = 0; kvs_idx < knob_vals.size(); kvs_idx++)
//				{
//					//all(abs(xn-x)<conv)
//					knobs_conv = knobs_conv && (std::fabs(xn[kvs_idx] - knob_vals[kvs_idx]) < conv);
//				}
//				if(knobs_conv){
//#ifdef DEBUG_RTM
//					std::cout << "LRGD: Convergence criterion met" << std::endl;
//#endif // DEBUG_RTM
//					done = true;
//					break; //all delta x within convergence threshold - finished
//				}
//				if(bounding_model->predict(xn, &bound_mon_val))
//					std::cout << "LRGD: INFO: Cannot make model prediction." << std::endl;
//
//				if(bound_mon_val > bound){
//					//alpha = alpha * beta;
//					for(auto& a_xn : alpha){
//						a_xn = a_xn * beta; //shrink learning rate if perf is greater than perf_req (make step towards it but don't overshoot)
//					}
//#ifdef DEBUG_RTM
//					std::cout << "LRGD: bound_mon_val = " << bound_mon_val << std::endl;
//					std::cout << "LRGD: bound = " << bound << std::endl;
//					for(a_xn : alpha) std::cout << "LRGD: alpha = " << a_xn << std::endl;
//#endif // DEBUG_RTM
//				}
//				else {
//					//alpha_steps.push_back(alpha);
//
//					opt_model->predict(knob_vals, &opt_mon_val);
//					opt_mon_steps.push_back(opt_mon_val); // pwr_lst=c(pwr_lst,pwr)
//
//					bounding_model->predict(knob_vals, &bound_mon_val);
//					bound_mon_steps.push_back(bound_mon_val); // perf_lst=c(perf_lst,perf)
//
//					knob_vals_steps.push_back(knob_vals); // xf_lst = c(xf_lst, x[1]);	xc_lst = c(xc_lst, x[2]);
//					knob_vals = xn;
//#ifdef DEBUG_RTM
//					std::cout << "LRGD: Opt Mon Val   = " << opt_mon_val << std::endl;
//					std::cout << "LRGD: Bound Mon Val = " << bound_mon_val << std::endl;
//					std::cout << "LRGD: knob vals: ";
//					for(auto val : knob_vals)
//					{
//						std::cout << val << ", ";
//					}
//					std::cout << std::endl;
//#endif // DEBUG_RTM
//					break;
//				}
//			}
//			double vec_sum = 0;
//			for(unsigned int idx = 0; idx < prev_dpdx.size(); idx++){
//				vec_sum += prev_dpdx[idx] * dpdx[idx];
//			}
//			if(vec_sum < 0) {
//				//alpha = alpha * beta;
//				for(auto& a_xn : alpha){
//					a_xn = a_xn * beta;
//				}
//			}
//			prev_dpdx = dpdx;
//			if(done)
//				break;
//		}
//		if(!bound_mon_steps.size()){
//#ifdef DEBUG_RTM
//			std::cout << "LRGD: Optimisation Result not Found - setting minimum knob values." << std::endl;
//			std::cout << "LRGD: Optimised Monitor Value = " << opt_mon_val << ", Bounding Monitor Value = " << bound_mon_val << std::endl;
//			knob_vals = knob_mins;
//#endif // DEBUG_RTM
//			return -1;
//		}
//		else{
////#ifdef DEBUG_RTM
//			//std::cout << "LRGD: Optimisation Result Found:" << std::endl;
//			std::cout << "LRGD: Optimised Monitor Value = " << opt_mon_val << ", Bounding Monitor Value = " << bound_mon_val << std::endl;
//
//			int idx = 0;
//			std::cout << "LRGD: Optimal Knob Values: ";
//			for(auto val : knob_vals){
//				std::cout << "x[" <<idx << "]: " << val << ", ";
//				idx++;
//			}
//			std::cout << std::endl;
//			return 0;
////#endif // DEBUG_RTM
//		}
//	}
//

	int regression::predict_mon(unsigned int mon_id, double *pred_val)
	{
		for(auto model : models)
		{
			if(model.get_id() == mon_id)
			{
				std::vector<double> knob_vals;
				for(auto knob : app_knobs_disc){ knob_vals.push_back(knob.val);	}
				for(auto knob : app_knobs_cont){ knob_vals.push_back(knob.val);	}
				for(auto knob : dev_knobs_disc){ knob_vals.push_back(knob.val);	}
				for(auto knob : dev_knobs_cont){ knob_vals.push_back(knob.val);	}

				if(model.predict(knob_vals, pred_val)){
					return -1;
				} else {
					return 0;
				}
			}
		}
		return -1;
	}
}
