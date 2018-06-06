#!/usr/bin/python3
############################################################################
"""
monitor_OPs.py

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

Created by Charles Leech on 2017-09-02.

"""
############################################################################
import numpy as np

class mon_ops:
	
	def __init__(self, mon, knobs):

		self.mon = mon
		#self.knobs = knobs
		self.op_knobs = dict()
		
		#Init arrays using a line from each of the input knob arrays
		for idx in knobs:
			self.op_knobs[idx] = knobs[idx][0]
			self.op_knobs[idx] = np.delete(self.op_knobs[idx], 0)
	
		for entry in mon:
			for idx in knobs:
				try:
					self.op_knobs[idx] = np.append(self.op_knobs[idx], knobs[idx][knobs[idx]["ts"] < entry["ts"]][-1])
				except IndexError:
					self.op_knobs[idx] = np.append(self.op_knobs[idx], knobs[idx][-1])

				
		#Remove the first lines (added to create arrays at the start)
		for idx in knobs:
			self.op_knobs[idx] = np.delete(self.op_knobs[idx], 0)
			
	def avg_repeats(self):
		#Average repeat entries for the monitor
		knobs_same = True
		iter_idx = 0
		while(iter_idx < len(self.mon)):
			curr_sum = 1
			while(knobs_same & (iter_idx + 1 < len(self.mon))):
				#Check all knobs are the same
				for k_idx in self.op_knobs:
					#print(self.op_knobs[k_idx][iter_idx]["val"])
					#print(self.op_knobs[k_idx][iter_idx+1]["val"])
					if(self.op_knobs[k_idx][iter_idx]["val"] != self.op_knobs[k_idx][iter_idx+1]["val"]):
						knobs_same = False

				if(knobs_same):
					#Only get here if knobs are all the same. Therefore can sum 2 mon entries (average at end)
					curr_sum += 1
					self.mon[iter_idx]["val"] += self.mon[iter_idx+1]["val"]
					#print("sum: " + str(self.mon[iter_idx]["val"]))
				
					#Remove repeat entry and corresponding knob entries
					self.mon = np.delete(self.mon, iter_idx+1)
					for k_idx in self.op_knobs:
						self.op_knobs[k_idx] = np.delete(self.op_knobs[k_idx], iter_idx+1)
					
			#Get here when while loop breaks
			#First do average of mon entry
			self.mon[iter_idx]["val"] = self.mon[iter_idx]["val"]/curr_sum
			#print("avg: " + str(self.mon[iter_idx]["val"]))
			#move to next mon op entry
			iter_idx += 1
			knobs_same = True
			
	def pick_last(self):
		#For temperature - remove repeat entries until temp has stabilised
		knobs_same = True
		iter_idx = 0
		while(iter_idx < len(self.mon)):
			while(knobs_same & (iter_idx + 1 < len(self.mon))):
				#Check all knobs are the same
				for k_idx in self.op_knobs:
					#print(self.op_knobs[k_idx][iter_idx]["val"])
					#print(self.op_knobs[k_idx][iter_idx+1]["val"])
					if(self.op_knobs[k_idx][iter_idx]["val"] != self.op_knobs[k_idx][iter_idx+1]["val"]):
						knobs_same = False

				if(knobs_same):
					#Only get here if knobs are all the same. 
					#Remove repeat entry and corresponding knob entries
					self.mon = np.delete(self.mon, iter_idx)
					for k_idx in self.op_knobs:
						self.op_knobs[k_idx] = np.delete(self.op_knobs[k_idx], iter_idx+1)
					
			#Get here when while loop breaks
			#move to next mon op entry
			iter_idx += 1
			knobs_same = True
			
			
			
			
			
			
			
			
	def multi_array_init(self, app_mon_cont, app_knob_disc, dev_knob_freq):

		self.op_app_knob_disc = dict()
		self.op_dev_knob_freq = dict()
		
		#Init arrays using a line from each of the input knob arrays
		for adk_idx in app_knob_disc:
			self.op_app_knob_disc[adk_idx] = app_knob_disc[adk_idx][0]
			self.op_app_knob_disc[adk_idx] = np.delete(self.op_app_knob_disc[adk_idx], 0)
	
		for dfk_idx in dev_knob_freq:
			self.op_dev_knob_freq[dfk_idx] = dev_knob_freq[dfk_idx][0]
			self.op_dev_knob_freq[dfk_idx] = np.delete(self.op_dev_knob_freq[dfk_idx], 0)

		for entry in app_mon_cont:
			#print(entry)
	
			#App Knobs
			for adk_idx in app_knob_disc:
				#print(app_knob_disc[adk_idx][app_knob_disc[adk_idx]["ts"] < entry["ts"]][-1])
				self.op_app_knob_disc[adk_idx] = np.append(self.op_app_knob_disc[adk_idx], app_knob_disc[adk_idx][app_knob_disc[adk_idx]["ts"] < entry["ts"]][-1])
		
			#Dev Knobs
			for dfk_idx in dev_knob_freq:
				#print(dev_knob_freq[dfk_idx][dev_knob_freq[dfk_idx]["ts"] < entry["ts"]][-1])
				self.op_dev_knob_freq[dfk_idx] = np.append(self.op_dev_knob_freq[dfk_idx], dev_knob_freq[dfk_idx][dev_knob_freq[dfk_idx]["ts"] < entry["ts"]][-1])
		
		#Remove the first lines (added to create arrays at the start)
		for adk_idx in app_knob_disc:
			self.op_app_knob_disc[adk_idx] = np.delete(self.op_app_knob_disc[adk_idx], 0)
	
		for dfk_idx in dev_knob_freq:
			self.op_dev_knob_freq[dfk_idx] = np.delete(self.op_dev_knob_freq[dfk_idx], 0)
