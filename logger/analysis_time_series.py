#!/usr/bin/python3
############################################################################
"""
analysis_time_series.py

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

Created by Charles Leech on 2017-07-12.

"""
############################################################################
import numpy as np
import matplotlib
matplotlib.use("pdf")
import matplotlib.pyplot as plt
import time
import sys
from csv import reader

style_name = "seaborn-paper"
if style_name in plt.style.available:
	plt.style.use(style_name)
	print("Using %s style" % style_name)
	
############################################################################
#### Defined Constants
############################################################################
MILLION = 1000000

############################################################################
#### Defined Functions
############################################################################
def new_subplot(figure):
	n = len(fig.axes)
	for i in range(n):
	    fig.axes[i].change_geometry(n+1, 1, i+1)
	return fig.add_subplot(n+1, 1, n+1)

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
	source = msg.pop(0)
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
#### Start Plotting figures
############################################################################
#NUM_SUBPLOTS = 6
#fig, subplt = plt.subplots(3, sharex=True, figsize=(10, 12))
#fig, subplt = plt.subplots(NUM_SUBPLOTS, figsize=(10, NUM_SUBPLOTS*2.5))
plot_num = 0
fig = plt.figure(figsize=(6, 12))
ax = fig.add_subplot(111)

############################################################################
#### Plot Device Temperature monitors
############################################################################
try:
	dev_mon_cont_updates = table_api_types["PRIME_API_DEV_RETURN_MON_CONT_GET"]
except:
	print("Warning: No device monitor values read by RTM (PRIME_API_DEV_RETURN_MON_CONT_GET). Some device graphs may not be plotted.")
else:
	dev_mon_temp_updates = dev_mon_cont_updates[dev_mon_cont_updates["type"] == "PRIME_TEMP"]
	if(len(dev_mon_temp_updates)):
		dev_mon_temp = dict()
		if plot_num:
			ax = new_subplot(fig)
		for idx in np.unique(dev_mon_temp_updates["id"]):
			dev_mon_temp[idx] = dev_mon_temp_updates[dev_mon_temp_updates["id"] == idx]
	
			#remove outliers - replace with previous value
			if(len(dev_mon_temp[idx][dev_mon_temp[idx]['val'] < 20])):
				print("Removed Temp Outlier(s)")
				np.copyto(dev_mon_temp[idx], np.roll(dev_mon_temp[idx], -1), where=dev_mon_temp[idx]['val'] < 20)

			ax.plot(dev_mon_temp[idx]["ts"], dev_mon_temp[idx]["val"], label=idx)
			ax.set_xlim(0, duration)

		ax.legend(bbox_to_anchor=(1.02, 1), loc=2, borderaxespad=0.,title="Monitor IDs")
		ax.set_ylabel("Temperature ($^\circ$C)")
		print("Plotted temperature monitors")
		plot_num+=1
		
	dev_mon_pow_updates = dev_mon_cont_updates[dev_mon_cont_updates["type"] == "PRIME_POW"]
	if(len(dev_mon_pow_updates)):
		dev_mon_pow = dict()
		if plot_num:
			ax = new_subplot(fig)
		for idx in np.unique(dev_mon_pow_updates["id"]):
			dev_mon_pow[idx] = dev_mon_pow_updates[dev_mon_pow_updates["id"] == idx]
	
			#remove outliers - replace with previous value
			if(len(dev_mon_pow[idx][dev_mon_pow[idx]['val'] > 20])):
				print("Removed Power Outlier(s)")
				np.copyto(dev_mon_pow[idx], np.roll(dev_mon_pow[idx], -1), where=dev_mon_pow[idx]['val'] < 20)

			ax.plot(dev_mon_pow[idx]["ts"], dev_mon_pow[idx]["val"], label=idx)
			ax.set_xlim(0, duration)

		ax.legend(bbox_to_anchor=(1.02, 1), loc=2, borderaxespad=0.,title="Monitor IDs")
		ax.set_ylabel("Power (W)")
		print("Plotted power monitors")
		plot_num+=1

	############################################################################
	#### Plot Device Power monitors
	############################################################################
	else:
		try:
			dev_log_mon_cont = table_api_types["PRIME_LOG_DEV_MON_CONT"]
		except:
			print("No device monitor values logged directly from the device (PRIME_LOG_DEV_MON_CONT).")
		else:
			dev_log_mon_pow = dev_log_mon_cont[dev_log_mon_cont["type"] == "PRIME_POW"]
			dev_mon_pow = dict()
			
			if plot_num:
				ax = new_subplot(fig)
			for idx in np.unique(dev_log_mon_pow["id"]):
				dev_mon_pow[idx] = dev_log_mon_pow[dev_log_mon_pow["id"] == idx]
	
				#remove outliers - replace with previous value
				if(len(dev_mon_pow[idx][dev_mon_pow[idx]['val'] > 20])):
					print("Removed Power Outlier(s)")
					np.copyto(dev_mon_pow[idx], np.roll(dev_mon_pow[idx], -1), where=dev_mon_pow[idx]['val'] > 20)
	
				ax.plot(dev_mon_pow[idx]["ts"], dev_mon_pow[idx]["val"], label=idx)
				ax.set_xlim(0, duration)

			ax.legend(bbox_to_anchor=(1.02, 1), loc=2, borderaxespad=0.,title="Monitor IDs")
			ax.set_ylabel("Power (W)")
			print("Plotted power monitors")
			plot_num+=1

############################################################################
#### Plot Device Frequency Knobs
############################################################################
try:
	dev_knob_disc_updates = table_api_types["PRIME_API_DEV_KNOB_DISC_SET"]
except KeyError:
	print("No Device Knobs Set")
else:	
	try:
		dev_knob_freq_updates = dev_knob_disc_updates[dev_knob_disc_updates["type"] == "PRIME_FREQ"]
		dev_knob_freq = dict()

		if plot_num:
			ax = new_subplot(fig)
		for idx in np.unique(dev_knob_freq_updates["id"]):
			dev_knob_freq[idx] = dev_knob_freq_updates[dev_knob_freq_updates["id"] == idx]
	
			ax.step(dev_knob_freq[idx]["ts"], (dev_knob_freq[idx]["val"]+2)*100, where="post", label=idx)
			ax.set_xlim(0, duration)

		ax.legend(bbox_to_anchor=(1.02, 1), loc=2, borderaxespad=0.,title="Knob IDs")
		ax.set_ylabel("Frequency (MHz)")
		print("Plotted frequency knobs")
		plot_num+=1
	except KeyError:
		print("No Frequency Knobs Set")

############################################################################
#### Plot App Monitors
############################################################################
app_mon_cont_updates = table_api_types["PRIME_API_APP_MON_CONT_SET"]

app_mon_cont = dict()
for idx in np.unique(app_mon_cont_updates["id"]):
	app_mon_cont[idx] = app_mon_cont_updates[app_mon_cont_updates["id"] == idx]
	
	if plot_num:
		ax = new_subplot(fig)
	ax.plot(app_mon_cont[idx]["ts"], app_mon_cont[idx]["val"])
	ax.set_xlim(0, duration)
	if app_mon_cont[idx][0]["type"] == "PRIME_ERR":
		ax.set_yscale('log')
		ax.set_ylabel("Error Monitor " + str(idx))
		for label in ax.yaxis.get_ticklabels()[::2]:
			label.set_visible(False)
		app_mon_err = app_mon_cont[idx]
	elif app_mon_cont[idx][0]["type"] == "PRIME_PERF":
		ax.set_ylabel("Performance Monitor " + str(idx))
		app_mon_perf = app_mon_cont[idx]
print("Plotted app monitors")

############################################################################
#### Plot App Knobs
############################################################################
try:
	#val is contained in return message from KNOB_DISC_GET
	app_knob_disc_updates = table_api_types["PRIME_API_APP_RETURN_KNOB_DISC_GET"]
	#app_knob_disc_updates = app_knob_disc_updates[app_knob_disc_updates["type"] != "PRIME_DEV_SEL"] 
	
	app_knob_disc = dict()
	for idx in np.unique(app_knob_disc_updates["id"]):
		app_knob_disc[idx] = app_knob_disc_updates[app_knob_disc_updates["id"] == idx]
		
		ax = new_subplot(fig)
		ax.plot(app_knob_disc[idx]["ts"], app_knob_disc[idx]["val"])
		ax.set_xlim(0, duration)
		ax.set_ylabel("App Knob " + str(idx))
		ax.margins(0, 0.02)
	print("Plotted app knobs")
except KeyError:
	pass

ax.set_xlabel("Time (s)")

figure_filename = log_file[:-4] + "_time_series" + ".pdf"
fig.savefig(figure_filename,bbox_inches="tight")

