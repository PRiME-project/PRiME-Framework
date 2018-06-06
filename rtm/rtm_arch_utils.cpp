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

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "rtm_arch_utils.h"

namespace prime { namespace util
{
	void print_architecture(boost::property_tree::ptree dev_arch)
	{
		std::cout << "Device Name: " << dev_arch.get_child("device").get<std::string>("descriptor") << std::endl;

		//Parse Functional Units
		boost::property_tree::ptree funcUnitItemList;
		try{
			funcUnitItemList = dev_arch.get_child("device.functional_units");
			std::cout << "Functional Units:" << std::endl;
		}catch(boost::property_tree::ptree_bad_path &){
			std::cerr << "\t" << "No Functional Units" << std::endl;
		}

		for(auto funcUnitItem : funcUnitItemList)
		{
			boost::property_tree::ptree funcUnit = funcUnitItem.second;
			std::cout << "\t" << funcUnitItem.first << ": " << std::endl;

			//Parse Global MONITORS
			boost::property_tree::ptree funcUnitMonList;
			try{
				funcUnitMonList = funcUnit.get_child("mons");
			}catch(boost::property_tree::ptree_bad_path &){
				std::cerr << "\t\t" << "No Top-level Monitors Found" << std::endl;
			}
			for(auto funcUnitMon : funcUnitMonList)
			{
				if(funcUnitMon.first != "id"){
					boost::property_tree::ptree mon = funcUnitMon.second;
					std::cout << "\t" << "\t" << funcUnitMon.first << ": "  \
								<< "id: " << mon.get<std::string>("id") \
								<< ", type: " << mon.get<std::string>("type") << std::endl;
				}
			}

			//Parse Global KNOBS
			boost::property_tree::ptree funcUnitKnobList;
			try{
				funcUnitKnobList = funcUnit.get_child("knobs");
			}catch(boost::property_tree::ptree_bad_path &){
				std::cerr << "\t\t" << "No Top-level Knobs Found" << std::endl;
			}
			for(auto funcUnitKnob : funcUnitKnobList)
			{
				if(funcUnitKnob.first != "id"){
					boost::property_tree::ptree mon = funcUnitKnob.second;
					std::cout << "\t" << "\t" << funcUnitKnob.first << ": "  \
								<< "id: " << mon.get<std::string>("id") \
								<< ", type: " << mon.get<std::string>("type") << std::endl;
				}
			}

			//Parse Second-Level
			for(auto funcUnitSecondLevelItem : funcUnit)
			{
				//get anything besides id, knobs or mons
				if(funcUnitSecondLevelItem.first != "id" && funcUnitSecondLevelItem.first != "knobs" && funcUnitSecondLevelItem.first != "mons"){
					std::cout << "\t\t" << funcUnitSecondLevelItem.first << std::endl;


					//Parse Second-Level MONITORS
					boost::property_tree::ptree funcUnitSecondLevelMonList;
					boost::property_tree::ptree funcUnitSecondLevel = funcUnitSecondLevelItem.second;
					try{
						funcUnitSecondLevelMonList = funcUnitSecondLevel.get_child("mons");
					}catch(boost::property_tree::ptree_bad_path &){
						std::cerr << "\t\t" << "No Second-level Monitors Found" << std::endl;
					}
					for(auto funcUnitSecondLevelMon : funcUnitSecondLevelMonList)
					{
						if(funcUnitSecondLevelMon.first != "id"){
							boost::property_tree::ptree mon = funcUnitSecondLevelMon.second;
							std::cout << "\t\t\t" << funcUnitSecondLevelMon.first << ": "  \
										<< "id: " << mon.get<std::string>("id") \
										<< ", type: " << mon.get<std::string>("type") << std::endl;
						}
					}

					//Parse Second-Level KNOBS
					boost::property_tree::ptree funcUnitSecondLevelKnobList;
					try{
						funcUnitSecondLevelKnobList = funcUnitSecondLevel.get_child("knobs");
					}catch(boost::property_tree::ptree_bad_path &){
						std::cerr << "\t\t" << "No Second-level Knobs Found" << std::endl;
					}
					for(auto funcUnitSecondLevelKnob : funcUnitSecondLevelKnobList)
					{
						if(funcUnitSecondLevelKnob.first != "id"){
							boost::property_tree::ptree mon = funcUnitSecondLevelKnob.second;
							std::cout << "\t\t\t" << funcUnitSecondLevelKnob.first << ": "  \
										<< "id: " << mon.get<std::string>("id") \
										<< ", type: " << mon.get<std::string>("type") << std::endl;
						}
					}
				}
			}
		}
		return;
    }

	//returns a vector with all discrete monitors of a given type
	std::vector<prime::api::dev::mon_disc_t> get_all_disc_mon(std::vector<prime::api::dev::mon_disc_t> mons_disc, prime::api::dev::mon_type_t type)
	{
		std::vector<prime::api::dev::mon_disc_t> type_mons;

		// Iterate to match IDs of known discrete monitors
		for(auto mon : mons_disc){
			if(mon.type == type){
				type_mons.push_back(mon);
			}
		}
		return type_mons;
	}

	//returns a vector with all continuous monitors of a given type
	std::vector<prime::api::dev::mon_cont_t> get_all_cont_mon(std::vector<prime::api::dev::mon_cont_t> mons_cont, prime::api::dev::mon_type_t type)
	{
		std::vector<prime::api::dev::mon_cont_t> type_mons;

		// Iterate to match IDs of known continuous monitors
		for(auto mon : mons_cont){
			if(mon.type == type){
				type_mons.push_back(mon);
			}
		}
		return type_mons;
	}

	//returns a vector with all discrete knobs of a given type
	std::vector<prime::api::dev::knob_disc_t> get_all_disc_knob(std::vector<prime::api::dev::knob_disc_t> knobs_disc, prime::api::dev::knob_type_t type)
	{
		std::vector<prime::api::dev::knob_disc_t> type_knobs;

		std::string type_str;
		switch(type){
			case prime::api::dev::PRIME_VOLT:	type_str = "PRIME_VOLT"; break;
			case prime::api::dev::PRIME_FREQ:	type_str = "PRIME_FREQ"; break;
			case prime::api::dev::PRIME_EN:		type_str = "PRIME_EN"; break;
			case prime::api::dev::PRIME_PMC_CNT:type_str = "PRIME_PMC_CNT"; break;
		}

		// Iterate to match IDs of known discrete knob
		for(auto knob : knobs_disc){
			if(knob.type == type){
				type_knobs.push_back(knob);
#ifdef DEBUG
				std::cout << type_str << std::endl;
#endif // DEBUG
			}
		}
		return type_knobs;
	}

	//returns a vector with all continuous knobs of a given type
	std::vector<prime::api::dev::knob_cont_t> get_all_cont_knob(std::vector<prime::api::dev::knob_cont_t> knobs_cont, prime::api::dev::knob_type_t type)
	{
		std::vector<prime::api::dev::knob_cont_t> type_knobs;

		std::string type_str;
		switch(type){
			case prime::api::dev::PRIME_VOLT:	type_str = "PRIME_VOLT"; break;
			case prime::api::dev::PRIME_FREQ:	type_str = "PRIME_FREQ"; break;
			case prime::api::dev::PRIME_EN:		type_str = "PRIME_EN"; break;
			case prime::api::dev::PRIME_PMC_CNT:type_str = "PRIME_PMC_CNT"; break;
		}

		// Iterate to match IDs of known continuous knob
		for(auto knob : knobs_cont){
			if(knob.type == type){
				type_knobs.push_back(knob);
#ifdef DEBUG
				std::cout << type_str << std::endl;
#endif // DEBUG
			}
		}
		return type_knobs;
	}

	/*
	functions for functional units
	*/
	//return vector with functional units ids
	std::vector<unsigned int> get_func_unit_ids(boost::property_tree::ptree dev_arch)
	{
		std::vector<unsigned int> func_unit_ids;

		//Parse Functional Units
		boost::property_tree::ptree funcUnitItemList;
		try{
			funcUnitItemList = dev_arch.get_child("device.functional_units");
		}catch(boost::property_tree::ptree_bad_path &){
			std::cerr << "No Functional Units" << std::endl;
		}

		for(auto funcUnitItem : funcUnitItemList)
		{
			boost::property_tree::ptree funcUnit = funcUnitItem.second;
#ifdef DEBUG
			std::cout << funcUnitItem.first << ": " << funcUnit.get<unsigned int>("id") << std::endl;
#endif // DEBUG
			func_unit_ids.push_back(funcUnit.get<unsigned int>("id"));
		}
		return func_unit_ids;
	}

	//return vector with all ids from knobs and monitors for a given functional unit
	std::vector<unsigned int> get_mons_knobs_ids_func_unit(boost::property_tree::ptree dev_arch, unsigned int func_unit_id)
	{
		std::vector<unsigned int> ids;

		//Parse Functional Units
        boost::property_tree::ptree funcUnitItemList;
        try{
            funcUnitItemList = dev_arch.get_child("device.functional_units");
        }catch(boost::property_tree::ptree_bad_path &){
            // std::cerr << "No Functional Units" << std::endl;
            return ids;
        }

		boost::property_tree::ptree funcUnit;
		for(auto funcUnitItem : funcUnitItemList)
		{
			if(dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".id") == func_unit_id)
			{
				funcUnit = funcUnitItem.second;

				//Parse MONITORS
				boost::property_tree::ptree funcUnitMonList;
				try{
					funcUnitMonList = funcUnit.get_child("mons");
				}catch(boost::property_tree::ptree_bad_path &){
					// std::cerr << "No Monitors Found" << std::endl;
					// return ids;
				}

				for(auto funcUnitMon : funcUnitMonList)
				{
					if(funcUnitMon.first != "id"){
						unsigned int su_id = dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".mons.id");
						unsigned int mon_id = dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".mons." + funcUnitMon.first + ".id");

						ids.push_back(prime::util::set_id(func_unit_id, su_id, 0, mon_id));
#ifdef DEBUG
						std::cout << "device.functional_units." << funcUnitItem.first << ".mons." + funcUnitMon.first + ".id" << std::endl;
#endif // DEBUG
					}
				}

				//Parse KNOBS
				try{
					funcUnitMonList = funcUnit.get_child("knobs");
				}catch(boost::property_tree::ptree_bad_path &){
					// std::cerr << "No Knobs Found" << std::endl;
					return ids;
				}

				for(auto funcUnitMon : funcUnitMonList)
				{
					if(funcUnitMon.first != "id"){
						unsigned int su_id = dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".knobs.id");
						unsigned int mon_id = dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".knobs." + funcUnitMon.first + ".id");

						ids.push_back(prime::util::set_id(func_unit_id, su_id, 0, mon_id));
#ifdef DEBUG
						std::cout << "device.functional_units." << funcUnitItem.first << ".knobs." + funcUnitMon.first + ".id" << std::endl;
#endif // DEBUG
					}
				}

				return ids;
			}
		}
	}

	//return vector with all discrete monitors from functional unit
	std::vector<prime::api::dev::mon_disc_t> get_disc_mon_func_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::mon_disc_t> mons_disc, unsigned int func_unit_id)
    {
		std::vector<prime::api::dev::mon_disc_t> fu_disc_mon;

		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_func_unit(dev_arch, func_unit_id);

		//search for monitors
		for(auto id : mons_knobs_ids){
			for(auto mon : mons_disc){
				if(mon.id == id){
					fu_disc_mon.push_back(mon);
				}
			}
		}
		return fu_disc_mon;
	}

	//return vector with all continuous monitors from functional unit
	std::vector<prime::api::dev::mon_cont_t> get_cont_mon_func_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::mon_cont_t> mons_cont, unsigned int func_unit_id)
	{
		std::vector<prime::api::dev::mon_cont_t> fu_cont_mon;

		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_func_unit(dev_arch, func_unit_id);

		//search for monitors
		for(auto id : mons_knobs_ids){
			for(auto mon : mons_cont){
				if(mon.id == id){
					fu_cont_mon.push_back(mon);
				}
			}
		}
		return fu_cont_mon;
	}

	//return vector with all discrete knobs from functional unit
	std::vector<prime::api::dev::knob_disc_t> get_disc_knob_func_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::knob_disc_t> knobs_disc, unsigned int func_unit_id)
    {
		std::vector<prime::api::dev::knob_disc_t> fu_disc_knob;

		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_func_unit(dev_arch, func_unit_id);

		//search for monitors
		for(auto id : mons_knobs_ids){
			for(auto knob : knobs_disc){
				if(knob.id == id){
					fu_disc_knob.push_back(knob);
				}
			}
		}
		return fu_disc_knob;
	}

	//return vector with all continuous knobs from functional unit
	std::vector<prime::api::dev::knob_cont_t> get_cont_knob_func_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::knob_cont_t> knobs_cont, unsigned int func_unit_id)
    {
		std::vector<prime::api::dev::knob_cont_t> fu_cont_knob;

		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_func_unit(dev_arch, func_unit_id);

		//search for monitors
		for(auto id : mons_knobs_ids){
			for(auto knob : knobs_cont){
				if(knob.id == id){
					fu_cont_knob.push_back(knob);
				}
			}
		}
		return fu_cont_knob;
	}


	/*
	functions for sub-units
	*/
	//return vector with sub units ids from a given functional unit
	std::vector<unsigned int> get_sub_unit_ids(boost::property_tree::ptree dev_arch, unsigned int func_unit_id)
	{
		std::vector<unsigned int> ids;

		//Parse Functional Units
		boost::property_tree::ptree funcUnitItemList;
		try{
			funcUnitItemList = dev_arch.get_child("device.functional_units");
		}catch(boost::property_tree::ptree_bad_path &){
			return ids;
		}

		boost::property_tree::ptree funcUnit;
		for(auto funcUnitItem : funcUnitItemList)
		{
			if(dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".id") == func_unit_id){
				funcUnit = funcUnitItem.second;

				//Parse sub unit
				for(auto funcUnitSecondLevelItem : funcUnit)
				{
					//get anything besides id, knobs or mons
					if(funcUnitSecondLevelItem.first != "id" && funcUnitSecondLevelItem.first != "knobs" && funcUnitSecondLevelItem.first != "mons"){
						// std::cout << "device.functional_units." << funcUnitItem.first << "." << funcUnitSecondLevelItem.first << ".id"<< std::endl;
						ids.push_back(dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + "." + funcUnitSecondLevelItem.first + ".id"));
					}
				}
				return ids;
			}
		}
	}

	//return vector with all ids from knobs and monitors for a given functional unit
	std::vector<unsigned int> get_mons_knobs_ids_sub_unit(boost::property_tree::ptree dev_arch, unsigned int func_unit_id, unsigned int sub_unit_id)
	{
		std::vector<unsigned int> ids;

		//Parse Functional Units
		boost::property_tree::ptree funcUnitItemList;
		try{
			funcUnitItemList = dev_arch.get_child("device.functional_units");
		}catch(boost::property_tree::ptree_bad_path &){
			return ids;
		}

		boost::property_tree::ptree funcUnit;
		for(auto funcUnitItem : funcUnitItemList)
		{
			if(dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + ".id") == func_unit_id){
				funcUnit = funcUnitItem.second;

				//Parse sub unit
				for(auto funcUnitSecondLevelItem : funcUnit)
				{
					//get anything besides id, knobs or mons
					if(funcUnitSecondLevelItem.first != "id" && funcUnitSecondLevelItem.first != "knobs" && funcUnitSecondLevelItem.first != "mons"){
						if(dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + "." + funcUnitSecondLevelItem.first + ".id") == sub_unit_id){

							//Parse Second-Level MONITORS
							boost::property_tree::ptree funcUnitSecondLevelMonList;
							boost::property_tree::ptree funcUnitSecondLevel = funcUnitSecondLevelItem.second;
							try{
								funcUnitSecondLevelMonList = funcUnitSecondLevel.get_child("mons");
							}catch(boost::property_tree::ptree_bad_path &){
								// std::cerr << "\t\t" << "No Second-level Monitors Found" << std::endl;
							}
							for(auto funcUnitSecondLevelMon : funcUnitSecondLevelMonList)
							{
								if(funcUnitSecondLevelMon.first != "id"){
									unsigned int mon_id = dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + "." + funcUnitSecondLevelItem.first + ".mons." + funcUnitSecondLevelMon.first + ".id");
									// std::cout << "device.functional_units." << funcUnitItem.first << "." << funcUnitSecondLevelItem.first << ".mons." << funcUnitSecondLevelMon.first << ".id" << std::endl;

									ids.push_back(prime::util::set_id(func_unit_id, sub_unit_id, 0, mon_id));
								}
							}

							//Parse Second-Level KNOBS
							boost::property_tree::ptree funcUnitSecondLevelKnobList;
							try{
								funcUnitSecondLevelKnobList = funcUnitSecondLevel.get_child("knobs");
							}catch(boost::property_tree::ptree_bad_path &){
								// std::cerr << "\t\t" << "No Second-level Knobs Found" << std::endl;
								return ids;
							}
							for(auto funcUnitSecondLevelKnob : funcUnitSecondLevelKnobList)
							{
								if(funcUnitSecondLevelKnob.first != "id"){
									unsigned int knob_id = dev_arch.get<unsigned int>("device.functional_units." + funcUnitItem.first + "." + funcUnitSecondLevelItem.first + ".knobs." + funcUnitSecondLevelKnob.first + ".id");
									// std::cout << "device.functional_units." << funcUnitItem.first << "." << funcUnitSecondLevelItem.first << ".knobs." << funcUnitSecondLevelMon.first << ".id" << std::endl;

									ids.push_back(prime::util::set_id(func_unit_id, sub_unit_id, 0, knob_id));
								}
							}
							return ids;
						}
					}
				}
			}
		}
	}

	//return vector with all discrete monitors from sub unit
	std::vector<prime::api::dev::mon_disc_t> get_disc_mon_sub_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::mon_disc_t> mons_disc, unsigned int func_unit_id, unsigned int sub_unit_id)
	{
		std::vector<prime::api::dev::mon_disc_t> su_mon_disc;

		//get all mons and knobs for this su
		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_sub_unit(dev_arch, func_unit_id, sub_unit_id);

		//search for discrete monitors
		for(auto mon_knob_id : mons_knobs_ids){
			for(auto mon : mons_disc){
				if(mon.id == mon_knob_id){
					su_mon_disc.push_back(mon);
				}
			}
		}
		return su_mon_disc;
	}

	//return vector with all continuous monitors from sub unit
	std::vector<prime::api::dev::mon_cont_t> get_cont_mon_sub_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::mon_cont_t> mons_cont, unsigned int func_unit_id, unsigned int sub_unit_id)
	{
		std::vector<prime::api::dev::mon_cont_t> su_mon_cont;

		//get all mons and knobs for this su
		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_sub_unit(dev_arch, func_unit_id, sub_unit_id);

		//search for discrete monitors
		for(auto id : mons_knobs_ids){
			for(auto mon : mons_cont){
				if(mon.id == id){
					su_mon_cont.push_back(mon);
				}
			}
		}
		return su_mon_cont;
	}

	//return vector with all discrete knobs from sub unit
	std::vector<prime::api::dev::knob_disc_t> get_disc_knob_sub_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::knob_disc_t> knobs_disc, unsigned int func_unit_id, unsigned int sub_unit_id)
	{
		std::vector<prime::api::dev::knob_disc_t> su_knobs_disc;

		//get all mons and knobs for this su
		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_sub_unit(dev_arch, func_unit_id, sub_unit_id);

		//search for discrete monitors
		for(auto id : mons_knobs_ids){
			for(auto knob : knobs_disc){
				if(knob.id == id){
					su_knobs_disc.push_back(knob);
				}
			}
		}
		return su_knobs_disc;
	}

	//return vector with all continuous knobs from sub unit
	std::vector<prime::api::dev::knob_cont_t> get_cont_knob_sub_unit(boost::property_tree::ptree dev_arch,
        std::vector<prime::api::dev::knob_cont_t> knobs_cont, unsigned int func_unit_id, unsigned int sub_unit_id)
	{
		std::vector<prime::api::dev::knob_cont_t> su_knobs_cont;

		//get all mons and knobs for this su
		std::vector<unsigned int> mons_knobs_ids = get_mons_knobs_ids_sub_unit(dev_arch, func_unit_id, sub_unit_id);

		//search for discrete monitors
		for(auto id : mons_knobs_ids){
			for(auto knob : knobs_cont){
				if(knob.id == id){
					su_knobs_cont.push_back(knob);
				}
			}
		}
		return su_knobs_cont;
	}
} }
