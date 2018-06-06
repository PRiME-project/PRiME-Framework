/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017, 2018 University of Southampton
 *		written by Charles Leech
 */
 
#ifndef PRIME_RTM_ARCH_UTIL_H
#define PRIME_RTM_ARCH_UTIL_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include "util.h"
#include "prime_api_t.h"
#include "prime_api_dev_t.h"

namespace prime { namespace util
{
	void print_architecture(boost::property_tree::ptree dev_arch);

	//Get all the discrete monitors of a specific type
	std::vector<prime::api::dev::mon_disc_t> get_all_disc_mon(std::vector<prime::api::dev::mon_disc_t> mons_disc, prime::api::dev::mon_type_t type);
	//Get all the continuous monitors of a specific type
	std::vector<prime::api::dev::mon_cont_t> get_all_cont_mon(std::vector<prime::api::dev::mon_cont_t> mons_cont, prime::api::dev::mon_type_t type);
	//Get all the discrete knobs of a specific type
	std::vector<prime::api::dev::knob_disc_t> get_all_disc_knob(std::vector<prime::api::dev::knob_disc_t> knobs_disc, prime::api::dev::knob_type_t type);
	//Get all the continuous knobs of a specific type
	std::vector<prime::api::dev::knob_cont_t> get_all_cont_knob(std::vector<prime::api::dev::knob_cont_t> knobs_cont, prime::api::dev::knob_type_t type);

	//Get all the ids of the functional units
	std::vector<unsigned int> get_func_unit_ids(boost::property_tree::ptree dev_arch);
	//Get all the ids of the knobs and monitors of a functional unit
	std::vector<unsigned int> get_mons_knobs_ids_func_unit(boost::property_tree::ptree dev_arch, unsigned int func_unit_id);

	//Get all the discrete monitors of a functional unit
	std::vector<prime::api::dev::mon_disc_t> get_disc_mon_func_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::mon_disc_t> mons_disc, unsigned int func_unit_id);
	//Get all the continuous monitors of a functional unit
	std::vector<prime::api::dev::mon_cont_t> get_cont_mon_func_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::mon_cont_t> mons_cont, unsigned int func_unit_id);
	//Get all the discrete knobs of a functional unit
	std::vector<prime::api::dev::knob_disc_t> get_disc_knob_func_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::knob_disc_t> knobs_disc, unsigned int func_unit_id);
	//Get all the continuous knobs of a functional unit
	std::vector<prime::api::dev::knob_cont_t> get_cont_knob_func_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::knob_cont_t> knobs_cont, unsigned int func_unit_id);

	//Get all the ids of the sub units in a functional unit
	std::vector<unsigned int> get_sub_unit_ids(boost::property_tree::ptree dev_arch, unsigned int func_unit_id);
	//Get all the ids of the knobs and monitors of a sub unit in a functional unit
	std::vector<unsigned int> get_mons_knobs_ids_sub_unit(boost::property_tree::ptree dev_arch, unsigned int func_unit_id, unsigned int sub_unit_id);

    //Get all the discrete monitors of a sub unit in a functional unit
	std::vector<prime::api::dev::mon_disc_t> get_disc_mon_sub_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::mon_disc_t> mons_disc, unsigned int func_unit_id, unsigned int sub_unit_id);
    //Get all the continuous monitors of a sub unit in a functional unit
	std::vector<prime::api::dev::mon_cont_t> get_cont_mon_sub_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::mon_cont_t> mons_cont, unsigned int func_unit_id, unsigned int sub_unit_id);
	//Get all the discrete knobs of a sub unit in a functional unit
	std::vector<prime::api::dev::knob_disc_t> get_disc_knob_sub_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::knob_disc_t> knobs_disc, unsigned int func_unit_id, unsigned int sub_unit_id);
	//Get all the continuous knobs of a sub unit in a functional unit
	std::vector<prime::api::dev::knob_cont_t> get_cont_knob_sub_unit(boost::property_tree::ptree dev_arch, std::vector<prime::api::dev::knob_cont_t> knobs_cont, unsigned int func_unit_id, unsigned int sub_unit_id);

} }
#endif
