#!/usr/bin/python3
############################################################################
"""
analysis_pareto.py

This file is part of the PRiME Framework Logger.

The PRiME Framework Logger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The PRiME Framework Logger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the PRiME Framework Logger.  If not, see <http://www.gnu.org/licenses/>.

Copyright 2017, 2018 University of Southampton

Created by Charles Leech on 2017-09-03.

"""
############################################################################
import numpy as np
import matplotlib
matplotlib.use("pdf")
import matplotlib.pyplot as plt
import time
import sys
import re
from csv import reader

from monitor_OPs import mon_ops
from to_precision import *

MILLION = 1000000

############################################################################
#### Read in messages from logger file
############################################################################
try:
	log_file = str(sys.argv[1])
except:
	print("Specify a filename as the first argument")
	exit(0)

try:
	with open(log_file, "r") as f:
		messages = list(reader(f))
except OSError:
	print("Invalid filename specified")
	exit(0)

try:
	num_lines = int(sys.argv[2])
	print("Parsing first %s lines of %s" %(num_lines, log_file))
except:
	num_lines = len(messages)
	print("Parsing all lines of %s" %(log_file))
	
############################################################################
#### Set up data tables and timescales
############################################################################
table_api_types = dict()
ts_only_types = ["PRIME_API_DEV_RETURN_KNOB_DISC_REG","PRIME_API_DEV_RETURN_KNOB_CONT_REG", \
					"PRIME_API_DEV_RETURN_MON_DISC_REG","PRIME_API_DEV_RETURN_MON_CONT_REG", \
					"PRIME_API_DEV_RETURN_ARCH_GET"]

start_time = float()
end_time = float()

for field in messages[0]:
	name, val = field.split(":")
	if name in ["ts"]:
		start_time = float(val)
		break

for field in messages[num_lines-1]:
	name, val = field.split(":")
	if name in ["ts"]:
		end_time = float(val)
		break

duration = (end_time - start_time)/MILLION

############################################################################
#### Parse message one by one
############################################################################
for msg in messages[:num_lines]:
	name, api_type = msg.pop(0).split(":")
	
	msg_end = msg.pop(-1)
	msg_names = []
	msg_vals = []
	 
	for field in msg:
		name, val = field.split(":")
		
		if api_type not in ts_only_types:
			if name in ["ts"]:
				msg_names.append((str(name),np.dtype("f8")))
				msg_vals.append((float(val) - start_time)/MILLION)
			elif name in ["id","proc_id"]:
				msg_names.append((str(name),np.dtype("i8")))
				msg_vals.append(val)
			elif name in ["type","name"]:
				msg_names.append((str(name),np.dtype("U25")))
				msg_vals.append(val)
			else:
				msg_names.append((str(name),np.dtype("f8")))
				msg_vals.append(val)
		else:
			if name == "ts":
				msg_names = [(str(name),np.dtype("f8"))]
				msg_vals = [(float(val) - start_time)/MILLION]
				break
	
	############################################################################
	#### Split messages by api type
	############################################################################
	if api_type not in table_api_types:
		table_api_types[api_type] = np.array(tuple(msg_vals), dtype=np.dtype(msg_names))
	else:
		table_api_types[api_type] = np.append(table_api_types[api_type], np.array(tuple(msg_vals), dtype=np.dtype(msg_names)))


############################################################################
#### Device Temperature monitors
############################################################################
try:
	dev_mon_cont_updates = table_api_types["PRIME_API_DEV_RETURN_MON_CONT_GET"]
except:
	print("Warning: No device monitor values read by RTM (PRIME_API_DEV_RETURN_MON_CONT_GET). Some device trade-offs may not be plotted.")
else:
	dev_mon_temp_updates = dev_mon_cont_updates[dev_mon_cont_updates["type"] == "PRIME_TEMP"]
	if(len(dev_mon_temp_updates)):
		dev_mon_temp = dict()
		for idx in np.unique(dev_mon_temp_updates["id"]):
			dev_mon_temp[idx] = dev_mon_temp_updates[dev_mon_temp_updates["id"] == idx]
	
			#remove outliers - replace with previous value
			if(len(dev_mon_temp[idx][dev_mon_temp[idx]['val'] < 20])):
				print("Removed Temp Outlier(s)")
				np.copyto(dev_mon_temp[idx], np.roll(dev_mon_temp[idx], -1), where=dev_mon_temp[idx]['val'] < 20)

	
	dev_mon_pow_updates = dev_mon_cont_updates[dev_mon_cont_updates["type"] == "PRIME_POW"]
	if(len(dev_mon_pow_updates)):
		dev_mon_pow = dict()
		for idx in np.unique(dev_mon_pow_updates["id"]):
			dev_mon_pow[idx] = dev_mon_pow_updates[dev_mon_pow_updates["id"] == idx]
	
			#remove outliers - replace with previous value
			if(len(dev_mon_pow[idx][dev_mon_pow[idx]['val'] > 20])):
				print("Removed Power Outlier(s)")
				np.copyto(dev_mon_pow[idx], np.roll(dev_mon_pow[idx], -1), where=dev_mon_pow[idx]['val'] < 20)

	############################################################################
	#### Device Power monitors
	############################################################################
	else:
		try:
			dev_log_mon_cont = table_api_types["PRIME_LOG_DEV_MON_CONT"]
		except:
			print("Warning: No device monitor values logged directly from the device (PRIME_LOG_DEV_MON_CONT). Some device trade-offs may not be plotted.")
		else:
			dev_log_mon_pow = dev_log_mon_cont[dev_log_mon_cont["type"] == "PRIME_POW"]
			dev_mon_pow = dict()
			for idx in np.unique(dev_log_mon_pow["id"]):
				dev_mon_pow[idx] = dev_log_mon_pow[dev_log_mon_pow["id"] == idx]
	
				#remove outliers - replace with previous value
				if(len(dev_mon_pow[idx][dev_mon_pow[idx]['val'] > 20])):
					print("Removed Power Outlier(s)")
					np.copyto(dev_mon_pow[idx], np.roll(dev_mon_pow[idx], -1), where=dev_mon_pow[idx]['val'] > 20)
					

knob_arrays = dict()
############################################################################
#### Device Frequency Knobs
############################################################################
try:
	dev_knob_disc_updates = table_api_types["PRIME_API_DEV_KNOB_DISC_SET"]
	dev_knob_freq_updates = dev_knob_disc_updates[dev_knob_disc_updates["type"] == "PRIME_FREQ"]

	dev_knob_freq = dict()
	for idx in np.unique(dev_knob_freq_updates["id"]):
		dev_knob_freq[idx] = dev_knob_freq_updates[dev_knob_freq_updates["id"] == idx]
		knob_arrays[idx] = dev_knob_freq[idx]
except KeyError:
	print("No Device Knobs")
############################################################################
#### App Knobs
############################################################################
try:
	#val is contained in return message from KNOB_DISC_GET
	app_knob_disc_updates = table_api_types["PRIME_API_APP_RETURN_KNOB_DISC_GET"]
	
	app_knob_disc_updates = app_knob_disc_updates[app_knob_disc_updates["type"] != "PRIME_DEV_SEL"] 
	
	app_knob_disc = dict()
	for idx in np.unique(app_knob_disc_updates["id"]):
		app_knob_disc[idx] = app_knob_disc_updates[app_knob_disc_updates["id"] == idx]
		# [0::4] slices to select every 4th set of app knob gets, starting with the 1st	
except KeyError:
	pass
else:
	#Remove repeats
	app_knob_disc_temp = app_knob_disc

	for idx in app_knob_disc:
		iter_idx = 0
		while(iter_idx < len(app_knob_disc_temp[idx])):
			while((iter_idx + 1 < len(app_knob_disc_temp[idx])) and (app_knob_disc_temp[idx][iter_idx]["val"] == app_knob_disc_temp[idx][iter_idx+1]["val"])):
				app_knob_disc_temp[idx] = np.delete(app_knob_disc_temp[idx], iter_idx+1)
			iter_idx += 1
	app_knob_disc = app_knob_disc_temp
	
	for idx in app_knob_disc:
		knob_arrays[idx] = app_knob_disc[idx]

############################################################################
#### App Monitors
############################################################################
app_mon_cont_updates = table_api_types["PRIME_API_APP_MON_CONT_SET"]

app_mon_cont = dict()
app_mon_cont_mean = dict()
for idx in np.unique(app_mon_cont_updates["id"]):
	app_mon_cont[idx] = app_mon_cont_updates[app_mon_cont_updates["id"] == idx]
	#app_mon_cont_mean[idx] = app_mon_cont[idx][0::4]
	#app_mon_cont_mean[idx]["val"] = (app_mon_cont[idx][1::4]["val"] + app_mon_cont[idx][2::4]["val"] + app_mon_cont[idx][3::4]["val"])/3
	#app_mon_cont[idx] = app_mon_cont_mean[idx]
	
	if app_mon_cont[idx][0]["type"] == "PRIME_ERR":
		app_mon_err = app_mon_cont[idx]
	elif app_mon_cont[idx][0]["type"] == "PRIME_PERF":
		app_mon_perf = app_mon_cont[idx]

############################################################################
#### Create Operating Points
############################################################################
op_app = dict()
op_temp = dict()
op_pow = dict()

print("\nStarting Array Lengths:")

for k_idx in knob_arrays:
	print("knob[" + str(k_idx) + "]: len = " + str(len(knob_arrays[k_idx])))
for m_idx in app_mon_cont:
	print("app_mon[" + str(m_idx) + "]: len = " + str(len(app_mon_cont[m_idx])))

#For each app monitor, i.e. error, perf, etc
for acm_idx in app_mon_cont:
	print("\nacm_idx: " + str(acm_idx) + " (" + app_mon_cont[acm_idx][0]["type"] + ")")
	
	# Create OPs by associating knob values to each mon entry - done inside mon_ops
	op_app[acm_idx] = mon_ops(app_mon_cont[acm_idx], knob_arrays)
	#op_app[acm_idx] = mon_ops(app_mon_cont[acm_idx], app_knob_disc, dev_knob_freq)
	
	print("Associated Knobs to Mons")
	print("New Array Lengths:")
	
	for k_idx in op_app[acm_idx].op_knobs:
		print("op_knobs[" + str(k_idx) + "]: len = " + str(len(op_app[acm_idx].op_knobs[k_idx])))
	print("mon[" + str(acm_idx) + "]: len = " + str(len(op_app[acm_idx].mon)))
	
	op_app[acm_idx].avg_repeats()
		
	print("Averaged Mon Repeats")
	print("New Array Lengths:")
	
	for k_idx in op_app[acm_idx].op_knobs:
		print("op_knobs[" + str(k_idx) + "]: len = " + str(len(op_app[acm_idx].op_knobs[k_idx])))
		#print(op_app[acm_idx].op_knobs[k_idx])
	print("mon[" + str(acm_idx) + "]: len = " + str(len(op_app[acm_idx].mon)))
		
if(len(dev_mon_pow_updates)):
	#For dev pow monitors
	for m_idx in dev_mon_pow:
		op_pow[m_idx] = mon_ops(dev_mon_pow[m_idx], knob_arrays)
		op_pow[m_idx].avg_repeats()
		print("op_pow[" + str(m_idx) + "]: len = " + str(len(op_pow[m_idx].mon)))

if(len(dev_mon_temp_updates)):
	#For dev temp monitors
	for m_idx in dev_mon_temp:
		op_temp[m_idx] = mon_ops(dev_mon_temp[m_idx], knob_arrays)
		op_temp[m_idx].pick_last()
		#print("op_temp[m_idx]: " + str(m_idx) + ", len: " + str(len(op_temp[m_idx].mon)))
		#print("\nop_temp[m_idx].mon[\"val\"]: " + str(op_temp[m_idx].mon["val"]))

	#average temp mons
	for m_idx in op_temp:
		op_temp_avg = op_temp[m_idx].mon.copy()
		
	op_temp_avg["val"] = 0
	op_temp_avg["id"] = -1
		
	for m_idx in op_temp:
		op_temp_avg["val"] += op_temp[m_idx].mon["val"]
	op_temp_avg["val"] = op_temp_avg["val"]/len(op_temp)

print("mon types:")
for m_idx in op_app:
	print("\t" + str(op_app[m_idx].mon["type"][0]))
	if op_app[m_idx].mon["type"][0] == "PRIME_PERF":
		op_app_perf = op_app[m_idx].mon
		perf_idx = m_idx
	if op_app[m_idx].mon["type"][0] == "PRIME_ERR":
		op_app_err = op_app[m_idx].mon

############################################################################
#### Pareto fronts
############################################################################
from pareto_front_2D import simple_cull, dominates

subplt = dict()
############################################################################
#### Power-Performance Pareto front
############################################################################
try:
	print("Power-Performance Pareto front:")

	fig = plt.figure(figsize=(6, 2))
	plot_num = 0

	#set these to change the source data
	power = op_pow[0].mon["val"] # monitor for total power has ID 0 for odroid and cyclone 5 
	perf = op_app_perf["val"]

	#### 2D Scatter Pareto Front
	inputPoints = np.column_stack((power, 1/perf)).tolist()
	paretoPoints, dominatedPoints = simple_cull(inputPoints, dominates)

	dp = np.array(list(dominatedPoints))
	pp = np.array(list(paretoPoints))

	subplt[plot_num] = fig.add_subplot(111)
	subplt[plot_num].scatter(power, perf, color='r', s=3)

	#subplt[plot_num].scatter(dp[:,0],1/dp[:,1], color='g', s=3)
	subplt[plot_num].scatter(pp[:,0],1/pp[:,1], s=50, marker='x', edgecolors='none')
	subplt[plot_num].set_ylim(ymin=0)
	#subplt[plot_num].set_xlim(xmin=0)

	#pp.view('float64,float64').sort(order=['f0'], axis=0)
	#subplt[plot_num].plot(pp[:,0],pp[:,1], label="")
	subplt[plot_num].set_xlabel("Power (W)", fontsize=9)
	
	label_text_perf = "Application performance\n(cycles per second)"
	if "jacobi" in re.split("[_./]", log_file):
		label_text_perf = "Application performance\n(solves per second)"
	subplt[plot_num].set_ylabel(label_text_perf, fontsize=9)
	
	subplt[plot_num].tick_params(axis='both', which='major', labelsize=10)
	subplt[plot_num].tick_params(axis='both', which='minor', labelsize=8)
	plot_num+=1


	figure_filename = log_file[:-4] + "_pareto_front_pow_perf" + ".pdf"
	fig.savefig(figure_filename,bbox_inches="tight")
	print("\tPlotted")

except NameError:
	print("\tEither no power or no performance monitor was found")

############################################################################
#### Error-Performance Pareto front
############################################################################
try:
	print("Error-Performance Pareto front:")

	#set these to change the source data
	error = op_app_err["val"]
	perf = np.round(op_app_perf["val"],4)

	fig = plt.figure(figsize=(6, 2))
	plot_num = 0
	#### 2D Scatter Pareto Front
	inputPoints = np.column_stack((error, 1/perf)).tolist()
	paretoPoints, dominatedPoints = simple_cull(inputPoints, dominates)

	dp = np.array(list(dominatedPoints))
	pp = np.array(list(paretoPoints))
	pp.view('float64,float64').sort(order=['f0'], axis=0)
	pp[:,1] = np.round(1/pp[:,1],4)
	print(pp)

	subplt[plot_num] = fig.add_subplot(111)
	subplt[plot_num].scatter(error, perf, color='r', s=3)
	#subplt[plot_num].scatter(dp[:,0],dp[:,1], color='g', s=3)
	subplt[plot_num].scatter(pp[:,0],pp[:,1], s=50, marker='x', edgecolors='none')

	subplt[plot_num].set_ylim(ymin=0)
	subplt[plot_num].set_xlim(xmin=1e-20)
	subplt[plot_num].set_xscale('log')
	for label in subplt[plot_num].xaxis.get_ticklabels()[::2]:
		label.set_visible(False)
	
	subplt[plot_num].set_xlabel("App error", fontsize=9)
	subplt[plot_num].set_ylabel(label_text_perf, fontsize=9)
	
	subplt[plot_num].tick_params(axis='both', which='major', labelsize=10)
	subplt[plot_num].tick_params(axis='both', which='minor', labelsize=8)
	plot_num+=1

	figure_filename = log_file[:-4] + "_pareto_front_error_perf" + ".pdf"
	fig.savefig(figure_filename,bbox_inches="tight")
	print("\tPlotted")
except NameError:
	print("\tEither no error or no performance monitor was found")


############################################################################
#### Temp-Performance Pareto front
############################################################################
try:
	print("Temp-Performance Pareto front:")

	fig = plt.figure(figsize=(6, 2))
	plot_num = 0

	#set these to change the source data
	temp = op_temp_avg["val"]
	perf =  op_app_perf["val"]

	#### 2D Scatter Pareto Front
	inputPoints = np.column_stack((temp, 1/perf)).tolist()
	paretoPoints, dominatedPoints = simple_cull(inputPoints, dominates)

	dp = np.array(list(dominatedPoints))
	pp = np.array(list(paretoPoints))

	subplt[plot_num] = fig.add_subplot(111)
	subplt[plot_num].scatter(temp, perf, color='r', s=3)

	#subplt[plot_num].scatter(dp[:,0],dp[:,1], color='g', s=3)
	subplt[plot_num].scatter(pp[:,0],1/pp[:,1], s=50, marker='x', edgecolors='none')
	subplt[plot_num].set_ylim(ymin=0)

	#pp.view('float64,float64').sort(order=['f0'], axis=0)
	#subplt[plot_num].plot(pp[:,0],pp[:,1], label="")
	subplt[plot_num].set_xlabel("Temperature ($^\circ$C)", fontsize=9)
	subplt[plot_num].set_ylabel(label_text_perf, fontsize=9)
	
	subplt[plot_num].tick_params(axis='both', which='major', labelsize=10)
	subplt[plot_num].tick_params(axis='both', which='minor', labelsize=8)
	plot_num+=1


	figure_filename = log_file[:-4] + "_pareto_front_temp_perf" + ".pdf"
	fig.savefig(figure_filename,bbox_inches="tight")
	print("\tPlotted")

except NameError:
	print("\tEither no power or no performance monitor was found")

