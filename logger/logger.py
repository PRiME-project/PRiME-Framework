#!/usr/bin/python3
# coding=utf-8
############################################################################
"""
logger.py

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

Created by Charles Leech on 2017-05-09.
Modified to include threading and remote capability by Graeme Bragg on 2017-12-29

"""
############################################################################
import socket
import sys
import os
import json
import datetime
import time
import signal
import threading
import argparse
import numpy

try:
    import queue
except ImportError:
    #Python 2.7
	import Queue as queue

from loggerClasses import PRIMEKnob
from loggerClasses import PRIMEMonitor
from loggerClasses import PRIMEApp
from loggerClasses import PRIMEDev

############################################################################
# Global Definitions
############################################################################
#Remote endpoint addresses of the logger
logger_address = '/tmp/logger.uds'
#Local endpoints addresses of sockets to the logger (fixed addresses only)
ui_log_address = '/tmp/ui.logger.uds'
rtm_log_address = '/tmp/rtm.logger.uds'
dev_log_address = '/tmp/dev.logger.uds'

# Available types of application-level knob
app_knob_types = ["PRIME_PAR", "PRIME_PREC", "PRIME_DEV_SEL", "PRIME_ITR", "PRIME_GEN"]
#Available types of application-level monitor
app_mon_types = ["PRIME_PERF", "PRIME_ACC", "PRIME_ERR", "PRIME_POW"]

# Available types of device-level knob
dev_knob_types = ["PRIME_VOLT", "PRIME_FREQ", "PRIME_EN", "PRIME_PMC_CNT", "PRIME_GOVERNOR", "PRIME_FREQ_EN"]
#Available types of device-level monitor
dev_mon_types = ["PRIME_POW", "PRIME_TEMP", "PRIME_CYCLES", "PRIME_PMC"]

apps = {}
devs = {}
dev = PRIMEDev()

#Fast lane decoding
API_DELIMINATOR = "¿"

fast_msg_dict= {
				"0": "PRIME_API_APP_RETURN_KNOB_DISC_GET",
				"1": "PRIME_API_APP_RETURN_KNOB_CONT_GET",
				"2": "PRIME_API_APP_KNOB_DISC_MIN",
				"3": "PRIME_API_APP_KNOB_DISC_MAX",
				"4": "PRIME_API_APP_KNOB_CONT_MIN",
				"5": "PRIME_API_APP_KNOB_CONT_MAX",
				"6": "PRIME_API_APP_KNOB_DISC_GET",
				"7": "PRIME_API_APP_KNOB_CONT_GET",
				"8": "PRIME_API_APP_MON_DISC_MIN",
				"9": "PRIME_API_APP_MON_DISC_MAX",
				"a": "PRIME_API_APP_MON_DISC_WEIGHT",
				"b": "PRIME_API_APP_MON_CONT_MIN",
				"c": "PRIME_API_APP_MON_CONT_MAX",
				"d": "PRIME_API_APP_MON_CONT_WEIGHT",
				"e": "PRIME_API_APP_MON_DISC_SET",
				"f": "PRIME_API_APP_MON_CONT_SET",
				"g": "PRIME_API_DEV_KNOB_DISC_SET",
				"h": "PRIME_API_DEV_KNOB_CONT_SET",
				"i": "PRIME_API_DEV_MON_DISC_GET",
				"j": "PRIME_API_DEV_MON_CONT_GET",
				"k": "PRIME_API_DEV_RETURN_MON_DISC_GET",
				"l": "PRIME_API_DEV_RETURN_MON_CONT_GET"
				}

visualiser_en = False
visualiser_addr = "127.0.0.1"
visualiser_port = 9000
vis_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
board_addrs = ["0.0.0.0", "0.0.0.0", "0.0.0.0"]
app_urid = 0

############################################################################
# Graphing Foo
############################################################################


############################################################################
# Utility Functions
############################################################################
def log_file(filename, print_str):
	#enhancement: log to filename the string in print_str
	pass

############################################################################
# Parse JSON Messages
############################################################################
def parse_message(msg, msg_src):
	msg_type = msg["type"]
	print_str = str()
	
	# old def parse_message_dev_rtm(msg):
	if msg_type in ["PRIME_API_DEV_RETURN_KNOB_DISC_SIZE", \
						"PRIME_API_DEV_RETURN_KNOB_CONT_SIZE", \
						"PRIME_API_DEV_RETURN_MON_DISC_SIZE", \
						"PRIME_API_DEV_RETURN_MON_CONT_SIZE"]:
		try:
			size = int(msg["data"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("size:" + str(size) + ",")
			
	elif msg_type == "PRIME_API_DEV_RETURN_KNOB_DISC_REG":
		for knob in msg["data"]:
			try:
				knob_id = int(knob["id"])
				knob_type = dev_knob_types[int(knob["type"])]
				knob_min = int(knob["min"])
				knob_max = int(knob["max"])
				#knob_val = int(knob["val"])
				knob_init = int(knob["init"])
			except KeyError:
				print(msg_type + ": Required Field Not Present")
				continue
			else:
				try:
					devs[msg_src].knobs[knob_id] = PRIMEKnob(knob_id, knob_type, knob_min, knob_max, knob_init)
				except KeyError:
					devs[msg_src] = PRIMEDev()
					devs[msg_src].knobs[knob_id] = PRIMEKnob(knob_id, knob_type, knob_min, knob_max, knob_init)
					
				print_str += str("id:" + str(knob_id) + ",")
				print_str += str("type:" + str(knob_type) + ",")
				print_str += str("min:" + str(knob_min) + ",")
				print_str += str("max:" + str(knob_max) + ",")
				#print_str += str("val:" + str(knob_val) + ",")
				print_str += str("init:" + str(knob_init) + ",")
				
	elif msg_type == "PRIME_API_DEV_RETURN_KNOB_CONT_REG":
		for knob in msg["data"]:
			try:
				knob_id = int(knob["id"])
				knob_type = dev_knob_types[int(knob["type"])]
				knob_min = float(knob["min"])
				knob_max = float(knob["max"])
				#knob_val = float(knob["val"])
				knob_init = float(knob["init"])
			except KeyError:
				print(msg_type + ": Required Field Not Present")
				continue
			else:
				try:
					devs[msg_src].knobs[knob_id] = PRIMEKnob(knob_id, knob_type, knob_min, knob_max, knob_init)
				except KeyError:
					devs[msg_src] = PRIMEDev()
					devs[msg_src].knobs[knob_id] = PRIMEKnob(knob_id, knob_type, knob_min, knob_max, knob_init)
				print_str += str("id:" + str(knob_id) + ",")
				print_str += str("type:" + str(knob_type) + ",")
				print_str += str("min:" + str(knob_min) + ",")
				print_str += str("max:" + str(knob_max) + ",")
				#print_str += str("val:" + str(knob_val) + ",")
				print_str += str("init:" + str(knob_init) + ",")
					
	elif msg_type == "PRIME_API_DEV_RETURN_MON_DISC_REG":
		for mon in msg["data"]:
			try:
				mon_id = int(mon["id"])
				mon_type = dev_mon_types[int(mon["type"])]
				#mon_val = int(mon["val"])
				#mon_min = int(mon["min"])
				#mon_max = int(mon["max"])
			except KeyError:
				print(msg_type + ": Required Field Not Present")
				continue
			else:
				try:
					devs[msg_src].mons[mon_id] = PRIMEMonitor(mon_id, mon_type, "", "", "")
				except KeyError:
					devs[msg_src] = PRIMEDev()
					devs[msg_src].mons[mon_id] = PRIMEMonitor(mon_id, mon_type, "", "", "")
				print_str += str("id:" + str(mon_id) + ",")
				print_str += str("type:" + str(mon_type) + ",")
				#print_str += str("val:" + str(mon_val) + ",")
				#print_str += str("min:" + str(mon_min) + ",")
				#print_str += str("max:" + str(mon_max) + ",")
	
	elif msg_type == "PRIME_API_DEV_RETURN_MON_CONT_REG":
		for mon in msg["data"]:
			try:
				mon_id = int(mon["id"])
				mon_type = dev_mon_types[int(mon["type"])]
				#mon_val = float(mon["val"])
				#mon_min = int(mon["min"])
				#mon_max = int(mon["max"])
				try:
					devs[msg_src].mons[mon_id] = PRIMEMonitor(mon_id, mon_type, "", "", "")
				except KeyError:
					devs[msg_src] = PRIMEDev()	
					devs[msg_src].mons[mon_id] = PRIMEMonitor(mon_id, mon_type, "", "", "")
			except KeyError:
				print(msg_type + ": Required Field Not Present")
				continue
			else:
				print_str += str("id:" + str(mon_id) + ",")
				print_str += str("type:" + str(mon_type) + ",")
				#print_str += str("val:" + str(mon_val) + ",")
				#print_str += str("min:" + str(mon_min) + ",")
				#print_str += str("max:" + str(mon_max) + ",")
	
	elif msg_type == "PRIME_API_DEV_RETURN_MON_DISC_GET":
		try:
			mon_val = int(msg["data"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("val:" + str(mon_val) + ",")
			
	elif msg_type == "PRIME_API_DEV_RETURN_MON_CONT_GET":
		try:
			mon_val = float(msg["data"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("val:" + str(mon_val) + ",")
			
	elif msg_type == "PRIME_API_DEV_RETURN_ARCH_GET":
		try:
			arch_file = msg["data"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("file:" + str(arch_file) + ",")
	
	
	# old def parse_message_dev(msg):
	elif msg_type == "PRIME_LOG_DEV_MON_DISC":
		try:
			msg_data = msg["data"]
			mon_id = int(msg_data["id"])
			mon_type = dev_mon_types[int(msg_data["type"])]
			mon_val = int(msg_data["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("val:" + str(mon_val) + ",")

	elif msg_type == "PRIME_LOG_DEV_MON_CONT":
		try:
			msg_data = msg["data"]
			mon_id = int(msg_data["id"])
			mon_type = dev_mon_types[int(msg_data["type"])]
			mon_val = float(msg_data["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("val:" + str(mon_val) + ",")

	elif msg_type == "PRIME_LOG_DEV_KNOB_DISC":
		try:
			msg_data = msg["data"]
			knob_id = int(msg_data["id"])
			knob_type = dev_knob_types[int(msg_data["type"])]
			knob_min = int(msg_data["min"])
			knob_max = int(msg_data["max"])
			knob_val = int(msg_data["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("max: " + str(knob_max) + ",")
			print_str += str("val:" + str(knob_val) + ",")
			
	elif msg_type == "PRIME_LOG_DEV_KNOB_CONT":
		try:
			msg_data = msg["data"]
			knob_id = int(msg_data["id"])
			knob_type = dev_knob_types[int(msg_data["type"])]
			knob_min = float(msg_data["min"])
			knob_max = float(msg_data["max"])
			knob_val = float(msg_data["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("max: " + str(knob_max) + ",")
			print_str += str("val:" + str(knob_val) + ",")
	
	
	# old def parse_message_dev_ui(msg):
	elif msg_type == "PRIME_UI_DEV_STOP":
		#no fields sent
		pass	
	elif msg_type == "PRIME_UI_DEV_RETURN_DEV_START":
		devs[msg_src] = PRIMEDev()
		#no fields sent
		pass		
	elif msg_type == "PRIME_UI_DEV_RETURN_DEV_STOP":
		del devs[msg_src]
		#no fields sent
		pass	
	elif msg_type == "PRIME_UI_DEV_ERROR":
		try:
			err_msg = msg["data"]["msg"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("msg:" + err_msg + ",")
	
	
	# old def parse_message_rtm_dev(msg):
	elif msg_type == "PRIME_API_DEV_KNOB_DISC_SIZE":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_KNOB_CONT_SIZE":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_KNOB_DISC_REG":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_KNOB_CONT_REG":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_KNOB_DISC_SET":
		try:
			knob = msg["data"]["knob"]
			knob_id = int(knob["id"])
			knob_type = dev_knob_types[int(knob["type"])]
			knob_min = int(knob["min"])
			knob_max = int(knob["max"])
			knob_val = int(knob["val"])
			knob_init = int(knob["init"])
			knob_val_set = int(msg["data"]["val"])
		except KeyError:
			return "\"Required Field Not Present\"," 
		else:
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("max:" + str(knob_max) + ",")
			print_str += str("val:" + str(knob_val) + ",")
			print_str += str("init:" + str(knob_init) + ",")
			print_str += str("val_set:" + str(knob_val_set) + ",")
			
	elif msg_type == "PRIME_API_DEV_KNOB_CONT_SET": 
		try:
			knob = msg["data"]["knob"]
			knob_id = int(knob["id"])
			knob_type = dev_knob_types[int(knob["type"])]
			knob_min = float(knob["min"])
			knob_max = float(knob["max"])
			knob_val = float(knob["val"])
			knob_init = float(knob["init"])
			knob_val_set = float(msg["data"]["val"])
		except KeyError:
			return "\"Required Field Not Present\"," 
		else:
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("max:" + str(knob_max) + ",")
			print_str += str("val:" + str(knob_val) + ",")
			print_str += str("init:" + str(knob_init) + ",")
			print_str += str("val_set:" + str(knob_val_set) + ",")
			
	elif msg_type in ["PRIME_API_DEV_KNOB_DISC_DEREG", "PRIME_API_DEV_KNOB_CONT_DEREG"]:
		for knob in msg["data"]["knobs"]:
			try:
				knob_id = int(knob["id"])
				knob_type = dev_knob_types[int(knob["type"])]
			except KeyError:
				print(msg_type + ": Required Field Not Present")
				continue
			else:
				del devs[msg_src].knobs[knob_id]
				print_str += str("id:" + str(knob_id) + ",")
				print_str += str("type:" + str(knob_type) + ",")
				
	elif msg_type == "PRIME_API_DEV_MON_DISC_SIZE":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_MON_CONT_SIZE":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_MON_DISC_REG":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_MON_CONT_REG":
		#no fields sent
		pass
	elif msg_type == "PRIME_API_DEV_MON_DISC_GET":
		try:
			mon = msg["data"]["mon"]
			mon_id = int(mon["id"])
			mon_type = dev_mon_types[int(mon["type"])]
			mon_val = int(mon["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("val:" + str(mon_val) + ",")
			
	elif msg_type == "PRIME_API_DEV_MON_CONT_GET":
		try:
			mon = msg["data"]["mon"]
			mon_id = int(mon["id"])
			mon_type = dev_mon_types[int(mon["type"])]
			mon_val = float(mon["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("val:" + str(mon_val) + ",")
			
	elif msg_type in ["PRIME_API_DEV_MON_DISC_DEREG", "PRIME_API_DEV_MON_CONT_DEREG"]:
		for mon in msg["data"]["mons"]:
			try:
				mon_id = int(mon["id"])
				mon_type = dev_mon_types[int(mon["type"])]
			except KeyError:
				print(msg_type + ": Required Field Not Present")
				continue
			else:
				del devs[msg_src].mons[mon_id]
				print_str += str("id:" + str(mon_id) + ",")
				print_str += str("type:" + str(mon_type) + ",")
				
	elif msg_type == "PRIME_API_DEV_ARCH_GET":
		#no fields sent
		pass
	
	
	# old def parse_message_rtm_app(msg):
	elif msg_type == "PRIME_API_APP_RETURN_APP_REG":
		try:
			app_pid = int(msg["data"]["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			
	elif msg_type == "PRIME_API_APP_RETURN_APP_DEREG":
		try:
			app_pid = int(msg["data"]["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			
	elif msg_type == "PRIME_API_APP_RETURN_KNOB_DISC_GET":
		try:
			knob_val = int(msg["data"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("val:" + str(knob_val) + ",")
			
	elif msg_type == "PRIME_API_APP_RETURN_KNOB_CONT_GET":
		try:
			knob_val = float(msg["data"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("val:" + str(knob_val) + ",")
	
	elif msg_type == "PRIME_API_APP_RETURN_KNOB_DISC_REG":
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = int(knob["min"])
			knob_max = int(knob["max"])
			knob_val = int(knob["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			try:
				apps[app_pid].knobs[knob_id] = PRIMEKnob(knob_id, knob_type, knob_min, knob_max, "")
			except KeyError:
				print_str += str("\tERROR: unregistered app with PID " + str(app_pid))
			else:
				print_str += str("proc_id:" + str(app_pid) + ",")
				print_str += str("id:" + str(knob_id) + ",")
				print_str += str("type:" + str(knob_type) + ",")
				print_str += str("min:" + str(knob_min) + ",")
				print_str += str("max:" + str(knob_max) + ",")
				print_str += str("val:" + str(knob_val) + ",")
			
	elif msg_type == "PRIME_API_APP_RETURN_KNOB_CONT_REG":
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = float(knob["min"])
			knob_max = float(knob["max"])
			knob_val = float(knob["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			try:
				apps[app_pid].knobs[knob_id] = PRIMEKnob(knob_id, knob_type, knob_min, knob_max, "")
			except KeyError:
				print_str += str("\tERROR: unregistered app with PID " + str(app_pid))
			else:
				print_str += str("proc_id:" + str(app_pid) + ",")
				print_str += str("id:" + str(knob_id) + ",")
				print_str += str("type:" + str(knob_type) + ",")
				print_str += str("min:" + str(knob_min) + ",")
				print_str += str("max:" + str(knob_max) + ",")
				print_str += str("val:" + str(knob_val) + ",")
			
	elif msg_type == "PRIME_API_APP_RETURN_MON_DISC_REG":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = int(mon["min"])
			mon_max = int(mon["max"])
			mon_val = int(mon["val"])
			mon_weight = float(mon["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			try:
				apps[app_pid].mons[mon_id] = PRIMEMonitor(mon_id, mon_type, mon_min, mon_max, mon_weight)
			except KeyError:
				print_str += str("\tERROR: unregistered app with PID " + str(app_pid))
			else:
				print_str += str("proc_id:" + str(app_pid) + ",")
				print_str += str("id:" + str(mon_id) + ",")
				print_str += str("type:" + str(mon_type) + ",")
				print_str += str("min:" + str(mon_min) + ",")
				print_str += str("max:" + str(mon_max) + ",")
				print_str += str("val:" + str(mon_val) + ",")
				print_str += str("weight:" + str(mon_weight) + ",")
			
	elif msg_type == "PRIME_API_APP_RETURN_MON_CONT_REG":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = float(mon["min"])
			mon_max = float(mon["max"])
			mon_val = float(mon["val"])
			mon_weight = float(mon["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			try:
				apps[app_pid].mons[mon_id] = PRIMEMonitor(mon_id, mon_type, mon_min, mon_max, mon_weight)
			except KeyError:
				print_str += str("\tERROR: unregistered app with PID " + str(app_pid))
			else:
				print_str += str("proc_id:" + str(app_pid) + ",")
				print_str += str("id:" + str(mon_id) + ",")
				print_str += str("type:" + str(mon_type) + ",")
				print_str += str("min:" + str(mon_min) + ",")
				print_str += str("max:" + str(mon_max) + ",")
				print_str += str("val:" + str(mon_val) + ",")
				print_str += str("weight:" + str(mon_weight) + ",")
	
	
	# old def parse_message_rtm_ui(msg):
	elif msg_type == "PRIME_UI_RTM_DEV_DEREG":
		#no fields sent
		pass
	elif msg_type == "PRIME_UI_RTM_STOP":
		#no fields sent
		pass	
	elif msg_type == "PRIME_UI_RTM_RETURN_RTM_STOP":
		#no fields sent
		pass	
	elif msg_type == "PRIME_UI_RTM_ERROR":
		try:
			err_msg = msg["data"]["msg"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("msg:" + err_msg + ",")
	
	
	# old def parse_message_app_rtm(msg):
	elif msg_type == "PRIME_API_APP_REG":
		try:
			app_pid = int(msg["data"]["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			apps[app_pid] = PRIMEApp(app_pid)
			print_str += str("proc_id:" + str(app_pid) + ",")
# Get the App URID			
			if msg_src == board_addrs[2]:
				global app_urid
				app_urid = int(msg["data"]["ur_id"])
			
	elif msg_type == "PRIME_API_APP_DEREG":
		try:
			app_pid = int(msg["data"]["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			del apps[app_pid]
			print_str += str("proc_id:" + str(app_pid) + ",")
			
	elif msg_type == "PRIME_API_APP_KNOB_DISC_REG":
		try:
			knob = msg["data"]
			app_pid = int(knob["proc_id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = int(knob["min"])
			knob_max = int(knob["max"])
			knob_val = int(knob["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("max:" + str(knob_max) + ",")
			print_str += str("val:" + str(knob_val) + ",")
			
	elif msg_type == "PRIME_API_APP_KNOB_CONT_REG":
		try:
			knob = msg["data"]
			app_pid = int(knob["proc_id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = float(knob["min"])
			knob_max = float(knob["max"])
			knob_val = float(knob["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("max:" + str(knob_max) + ",")
			print_str += str("val:" + str(knob_val) + ",")
	
	elif msg_type == "PRIME_API_APP_KNOB_DISC_MIN":
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = int(knob["min"])
			knob_max = int(knob["max"])
			knob_val = int(knob["val"])
			knob_min_set = int(msg["data"]["min"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("min_set:" + str(knob_min_set) + ",")
			
	elif msg_type == "PRIME_API_APP_KNOB_DISC_MAX":
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = int(knob["min"])
			knob_max = int(knob["max"])
			knob_val = int(knob["val"])
			knob_max_set = int(msg["data"]["max"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("max:" + str(knob_max) + ",")
			print_str += str("max_set:" + str(knob_max_set) + ",")
			
	elif msg_type == "PRIME_API_APP_KNOB_CONT_MIN":
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = float(knob["min"])
			knob_max = float(knob["max"])
			knob_val = float(knob["val"])
			knob_min_set = float(msg["data"]["min"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("min:" + str(knob_min) + ",")
			print_str += str("min_set:" + str(knob_min_set) + ",")
			
	elif msg_type == "PRIME_API_APP_KNOB_CONT_MAX":
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			knob_min = float(knob["min"])
			knob_max = float(knob["max"])
			knob_val = float(knob["val"])
			knob_max_set = float(msg["data"]["max"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("max:" + str(knob_min) + ",")
			print_str += str("max_set:" + str(knob_max_set) + ",")
			
	elif msg_type in ["PRIME_API_APP_KNOB_DISC_GET", \
						"PRIME_API_APP_KNOB_CONT_GET"]:
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			if msg_type == "PRIME_API_APP_KNOB_DISC_GET":
				knob_min = int(knob["min"])
				knob_max = int(knob["max"])
				knob_val = int(knob["val"])
			if msg_type == "PRIME_API_APP_KNOB_CONT_GET":
				knob_min = float(knob["min"])
				knob_max = float(knob["max"])
				knob_val = float(knob["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
			print_str += str("val:" + str(knob_val) + ",")
	
	elif msg_type in ["PRIME_API_APP_KNOB_DISC_DEREG", \
						"PRIME_API_APP_KNOB_CONT_DEREG"]:
		try:
			knob = msg["data"]["knob"]
			app_pid = int(knob["proc_id"])
			knob_id = int(knob["id"])
			knob_type = app_knob_types[int(knob["type"])]
			if msg_type == "PRIME_API_APP_KNOB_DISC_DEREG":
				knob_min = int(knob["min"])
				knob_max = int(knob["max"])
				knob_val = int(knob["val"])
			if msg_type == "PRIME_API_APP_KNOB_CONT_DEREG":
				knob_min = float(knob["min"])
				knob_max = float(knob["max"])
				knob_val = float(knob["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(knob_id) + ",")
			print_str += str("type:" + str(knob_type) + ",")
	
	elif msg_type == "PRIME_API_APP_MON_DISC_REG":
		try:
			mon = msg["data"]
			app_pid = int(mon["proc_id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = int(mon["min"])
			mon_max = int(mon["max"])
			mon_weight = float(mon["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("min:" + str(mon_min) + ",")
			print_str += str("max:" + str(mon_max) + ",")
			print_str += str("weight:" + str(mon_weight) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_CONT_REG":
		try:
			mon = msg["data"]
			app_pid = int(mon["proc_id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = float(mon["min"])
			mon_max = float(mon["max"])
			mon_weight = float(mon["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("min:" + str(mon_min) + ",")
			print_str += str("max:" + str(mon_max) + ",")
			print_str += str("weight:" + str(mon_weight) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_DISC_MIN":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = int(mon["min"])
			mon_max = int(mon["max"])
			mon_val = int(mon["val"])
			mon_weight = float(mon["weight"])
			mon_min_set = int(msg["data"]["min"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("min:" + str(mon_min) + ",")
			print_str += str("min_set:" + str(mon_min_set) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_DISC_MAX":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = int(mon["min"])
			mon_max = int(mon["max"])
			mon_val = int(mon["val"])
			mon_weight = float(mon["weight"])
			mon_max_set = int(msg["data"]["max"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("max:" + str(mon_max) + ",")
			print_str += str("max_set:" + str(mon_max_set) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_DISC_WEIGHT":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = int(mon["min"])
			mon_max = int(mon["max"])
			mon_val = int(mon["val"])
			mon_weight = float(mon["weight"])
			mon_weight_set = float(msg["data"]["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("weight:" + str(mon_weight) + ",")
			print_str += str("weight_set:" + str(mon_weight_set) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_CONT_MIN":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = float(mon["min"])
			mon_max = float(mon["max"])
			mon_val = float(mon["val"])
			mon_weight = float(mon["weight"])
			mon_min_set = float(msg["data"]["min"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("min:" + str(mon_min) + ",")
			print_str += str("min_set:" + str(mon_min_set) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_CONT_MAX":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = float(mon["min"])
			mon_max = float(mon["max"])
			mon_val = float(mon["val"])
			mon_weight = float(mon["weight"])
			mon_max_set = float(msg["data"]["max"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("max:" + str(mon_max) + ",")
			print_str += str("max_set:" + str(mon_max_set) + ",")
			
	elif msg_type == "PRIME_API_APP_MON_CONT_WEIGHT":
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_min = float(mon["min"])
			mon_max = float(mon["max"])
			mon_val = float(mon["val"])
			mon_weight = float(mon["weight"])
			mon_weight_set = float(msg["data"]["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("weight:" + str(mon_weight) + ",")
			print_str += str("weight_set:" + str(mon_weight_set) + ",")
			
	elif msg_type in ["PRIME_API_APP_MON_DISC_SET", \
						"PRIME_API_APP_MON_CONT_SET"]:
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_weight = float(mon["weight"])
			if msg_type == "PRIME_API_APP_MON_DISC_SET":
				mon_min = int(mon["min"])
				mon_max = int(mon["max"])
				mon_val = int(mon["val"])
				mon_val_set = int(msg["data"]["val"])
			if msg_type == "PRIME_API_APP_MON_CONT_SET":
				mon_min = float(mon["min"])
				mon_max = float(mon["max"])
				mon_val = float(mon["val"])
				mon_val_set = float(msg["data"]["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
			print_str += str("val:" + str(mon_val) + ",")
			print_str += str("val_set:" + str(mon_val_set) + ",")
		
	elif msg_type in ["PRIME_API_APP_MON_DISC_DEREG", \
						"PRIME_API_APP_MON_CONT_DEREG"]:
		try:
			mon = msg["data"]["mon"]
			app_pid = int(mon["proc_id"])
			mon_id = int(mon["id"])
			mon_type = app_mon_types[int(mon["type"])]
			mon_weight = float(mon["weight"])
			if msg_type == "PRIME_API_APP_MON_DISC_DEREG":
				mon_min = int(mon["min"])
				mon_max = int(mon["max"])
				mon_val = int(mon["val"])
			if msg_type == "PRIME_API_APP_MON_CONT_DEREG":
				mon_min = float(mon["min"])
				mon_max = float(mon["max"])
				mon_val = float(mon["val"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("id:" + str(mon_id) + ",")
			print_str += str("type:" + str(mon_type) + ",")
	
	
	# old def parse_message_app_ui(msg):
	elif msg_type == "PRIME_UI_APP_RETURN_APP_START":
		try:
			app_pid = int(msg["data"]["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			
	elif msg_type == "PRIME_UI_APP_RETURN_APP_STOP":
		try:
			app_pid = int(msg["data"]["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
		
	elif msg_type == "PRIME_UI_APP_ERROR":
		try:
			app_pid = int(msg["data"]["proc_id"])
			err_msg = msg["data"]["msg"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			print_str += str("msg:" + err_msg + ",")
	
	
	# old def parse_message_ui(msg):
	elif msg_type == "PRIME_UI_APP_START":
		try:
			app_name = msg["name"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("name:" + app_name + ",")
			
	elif msg_type == "PRIME_UI_APP_STOP":
		try:
			data = msg["data"]
			app_pid = int(data["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("proc_id:" + str(app_pid) + ",")
			
	elif msg_type in ["PRIME_UI_APP_REG", \
						"PRIME_UI_APP_DEREG"]:
		try:
			data = msg["data"]
			app_pid = int(data["proc_id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			
			print_str += str("proc_id:" + str(app_pid) + ",")
		
	elif msg_type in ['PRIME_UI_APP_MON_DISC_MIN', 'PRIME_UI_APP_MON_DISC_MAX', 'PRIME_UI_APP_MON_DISC_WEIGHT', \
						'PRIME_UI_APP_MON_CONT_MIN', 'PRIME_UI_APP_MON_CONT_MAX', 'PRIME_UI_APP_MON_CONT_WEIGHT']:
		try:
			data = msg["data"]
			mon_id = int(data["id"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("mon_id:" + str(mon_id) + ",")
		try:
			if msg_type == "PRIME_UI_APP_MON_DISC_MIN":
				mon_min = int(data["min"])
				print_str += str("min:" + str(mon_min) + ",")
			elif msg_type == "PRIME_UI_APP_MON_DISC_MAX":
				mon_max = int(data["max"])
				print_str += str("max:" + str(mon_max) + ",")
			elif msg_type == "PRIME_UI_APP_MON_DISC_WEIGHT":
				mon_weight = float(data["weight"])
				print_str += str("weight:" + str(mon_weight) + ",")
			if msg_type == "PRIME_UI_APP_MON_CONT_MIN":
				mon_min = float(data["min"])
				print_str += str("min:" + str(mon_min) + ",")
			elif msg_type == "PRIME_UI_APP_MON_CONT_MAX":
				mon_max = float(data["max"])
				print_str += str("max:" + str(mon_max) + ",")
			elif msg_type == "PRIME_UI_APP_MON_CONT_WEIGHT":
				mon_weight = float(data["weight"])
				print_str += str("weight:" + str(mon_weight) + ",")
		except KeyError:
			return "\"Required Field Not Present\","
	
	elif msg_type == "PRIME_UI_APP_WEIGHT":
		try:
			data = msg["data"]
			app_pid = int(data["proc_id"])
			weight = float(data["weight"])
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("name:" + str(app_pid) + ",")
			print_str += str("weight:" + str(weight) + ",")
			
	elif msg_type == "PRIME_UI_RTM_POLICY_SWITCH":
		try:
			data = msg["data"]
			policy = data["policy"]
			params = data["params"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("policy:" + policy + ",")
			print_str += str("params:" + str(params) + ",")		
		
	elif msg_type in ["PRIME_UI_LOG_FILE_FILTER_MUX", \
						"PRIME_UI_LOG_VISUAL_FILTER_MUX"]:
		try:
			data = msg["data"]
			message_list = data["messages"]
		except KeyError:
			return "\"Required Field Not Present\","
		else:
			print_str += str("messages:" + str(message_list) + ",")
	
	return print_str
	
############################################################################
# Parse "Fast" Messages
############################################################################
def parse_message_fast(msg, msg_src):
	print_str = str()
	#decode the message
	split_msg = msg.split(API_DELIMINATOR)
	km_type = str()
	
	try:
		# Assume that we have a 6-field message
		msg_ts = split_msg[5]
		msg_type = fast_msg_dict[split_msg[0]]
		msg_id = int(split_msg[1])
		msg_val = split_msg[2]
		msg_min = split_msg[3]
		msg_max = split_msg[4]
		#msg_pid = int(split_msg[5])
		
		print_str = str("api:" + msg_type + ",")
		print_str += str("ts:" + msg_ts + ",")
		print_str += str("id:" + str(msg_id) + ",")
		print_str += str("val:" + msg_val + ",")
		print_str += str("min:" + msg_min + ",")
		print_str += str("max:" + msg_max + ",")
		#print_str += str("proc_id:" + str(msg_pid) + ",")
		
		#Append knob/monitor type field - infer from msg_id (and proc_id for apps)
		try:
			if msg_type[:13] == "PRIME_API_DEV":
				if msg_type[14:24] == "RETURN_MON":
					km_type = devs[msg_src].mons[msg_id].type				
		except KeyError:
				km_type = "ERROR: Unknown Type"
						
		print_str += str("type:" + km_type + ",")
		
		return print_str
	except IndexError:
		try:
			# Assume that we have a 5-field message
			msg_ts = split_msg[4]
			msg_type = fast_msg_dict[split_msg[0]]
			msg_id = int(split_msg[1])
			msg_val = split_msg[2]
			msg_pid = int(split_msg[3])
			
			print_str = str("api:" + msg_type + ",")
			print_str += str("ts:" + msg_ts + ",")
			print_str += str("id:" + str(msg_id) + ",")
			print_str += str("val:" + msg_val + ",")
			print_str += str("proc_id:" + str(msg_pid) + ",")
			
			#Append knob/monitor type field - infer from msg_id (and proc_id for apps)
			try:
				if msg_type[:13] == "PRIME_API_DEV":
					if msg_type[14:18] == "KNOB":
						km_type = devs[msg_src].knobs[msg_id].type
					elif msg_type[14:17] == "MON":
						km_type = devs[msg_src].mons[msg_id].type
				elif msg_type[:13] == "PRIME_API_APP":
					if msg_type[14:18] == "KNOB" or msg_type[14:25] == "RETURN_KNOB":
						km_type = apps[msg_pid].knobs[msg_id].type
					elif msg_type[14:17] == "MON" or msg_type[14:24] == "RETURN_MON":
						km_type = apps[msg_pid].mons[msg_id].type				
			except KeyError:
					km_type = "ERROR: Unknown Type"
							
			print_str += str("type:" + km_type + ",")
			
			return print_str
		except IndexError:
			try:
				# Assume that we have a 4-field message, either Type, ID, Val, TS or Type, ID, PID, TS
				msg_ts = split_msg[3]
				msg_type = fast_msg_dict[split_msg[0]]
				msg_id = int(split_msg[1])
				msg_val = split_msg[2]
					
				print_str = str("api:" + msg_type + ",")
				print_str += str("ts:" + msg_ts + ",")
				print_str += str("id:" + str(msg_id) + ",")
				
				if split_msg[0] < 'g':
					# It's and App type so field 4 is PID
					print_str += str("proc_id:" + msg_val + ",")
				else:
					# Its a different type
					print_str += str("val:" + msg_val + ",")
				
				#Append knob/monitor type field - infer from msg_id (and proc_id for apps)
				try:
					if msg_type[:13] == "PRIME_API_DEV":
						if msg_type[14:18] == "KNOB" or msg_type[14:25] == "RETURN_KNOB":
							km_type = devs[msg_src].knobs[msg_id].type
														
						elif msg_type[14:17] == "MON" or msg_type[14:24] == "RETURN_MON":
							km_type = devs[msg_src].mons[msg_id].type
					elif msg_type[:13] == "PRIME_API_APP":
						if msg_type[14:18] == "KNOB" or msg_type[14:25] == "RETURN_KNOB":
							km_type = apps[int(msg_val)].knobs[msg_id].type
						elif msg_type[14:17] == "MON" or msg_type[14:24] == "RETURN_MON":
							km_type = apps[int(msg_val)].mons[msg_id].type
				except KeyError:
					km_type = "ERROR: Unknown Type"
					
				print_str += str("type:" + km_type + ",")
					
				return print_str
					
			except IndexError:
				try:
					# Assume that we have a 3-field message
					msg_ts = split_msg[2]
					msg_type = fast_msg_dict[split_msg[0]]
					msg_id = int(split_msg[1])
					
					print_str += str("api:" + msg_type + ",")
					print_str += str("ts:" + msg_ts + ",")
					print_str += str("id:" + str(msg_id) + ",")
					return print_str

				except IndexError:
					raise ValueError('Message not in json or fast format')
				

############################################################################
# Main Utility functions
############################################################################
def signal_handler(signal_in, frame):
	#print("\nTerminating Logger...")
	try:
		#print("Deleting socket...")
		os.unlink(logger_address)
	except OSError:
		if os.path.exists(logger_address):
			raise
	#print("All Done!")
	sys.exit(0)

############################################################################
# Message Processor
############################################################################
def message_process(rec_q):
	while True:
		data, msg_src, remote = rec_q.get()
		
		data_string = data.decode("utf-8")
		print_str = str()
		
		if remote:
			print_str += str("source:" + msg_src + ",")
		else:
			print_str += str("source:UDS,")
		
		try:
			json_msg = json.loads(data_string)
		except ValueError: #not json
			try:
				print_str += parse_message_fast(data_string, msg_src)
			except ValueError: #not JSON or Fast, invalid...
				print("Error: message not in json or fast format")
				data_string.encode('ascii', 'replace')
				print(data_string)
				continue
		else:
			try:
				#Msg must have type
				msg_type = json_msg["type"]
				print_str += str("api:" + msg_type + ",")
			except KeyError:
				print("Type Not Present")
				#enhancement: dump msg + error to error file
				continue
			try:
				#Msg must have timestamp
				print_str += str("ts:" + str(json_msg["ts"]) + ",")
			except KeyError:
				print(msg_type + ": Timestamp Not Present")
				#enhancement: dump msg + error to error file
				continue
			
			print_str += parse_message(json_msg, msg_src)

		print(print_str)

		
def socket_process(rec_q, sock, remote):
	while True:
		data, address = sock.recvfrom(65535)
		rec_q.put((data, str(address[0]), remote))

		
############################################################################
# Main function
############################################################################
def main():
	if sys.version_info[0] < 3:
		print("ERROR: Logger must be executed using Python 3")
		sys.exit(-1)
	
	signal.signal(signal.SIGINT, signal_handler)
	
	parser = argparse.ArgumentParser(description='PRiME Framework Logger.')
	parser.add_argument("-p", "--port", type=int, help="Set the port that the logger should listen on. If this is not set, the logger with bind to " + logger_address)
	
	parser.add_argument("-v", "--visualiser", action='store_true', help="Enable sending to the visualiser")
	
	parser.add_argument("-va", "--visualiser_addr", type=str, help="Set Visualiser address. If not specified, 127.0.0.1 will be used.")
	parser.add_argument("-vp", "--visualiser_port", type=int, help="Set Visualiser port. If not specified, 9000 will be used.")
	
	# Addresses for parsing log
	parser.add_argument("-a", "--boarda", type=str, help="Address of the first board in the demonstrator")
	parser.add_argument("-b", "--boardb", type=str, help="Address of the second board in the demonstrator")
	parser.add_argument("-c", "--boardc", type=str, help="Address of the third board in the demonstrator")
	
	args = parser.parse_args()
	
	if args.port is not None and args.port > 0 and args.port < 65536:
		# Logger port specified, remote logger.
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		sock.bind(('0.0.0.0', args.port))
		remote = True
		print("Remote logger listening on port " + str(args.port))
	else:
		# Local Logger
		try:
			os.unlink(logger_address)
		except OSError:
			if os.path.exists(logger_address):
				raise
		sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
		sock.bind(logger_address)
		remote = False
	
	global visualiser_en
	if args.visualiser:
		# Visualiser enabled
		visualiser_en = True
	
	if args.visualiser_addr:
		visualiser_en = True
		global visualiser_addr
		visualiser_addr = args.visualiser_addr
	
	if args.visualiser_port is not None and args.visualiser_port > 0 and args.visualiser_port < 65536:
		visualiser_en = True
		global visualiser_port
		visualiser_port = args.visualiser_port
	
	global board_addrs
	if args.boarda:
		board_addrs[0] = args.boarda
	
	if args.boardb:
		board_addrs[1] = args.boardb
	
	if args.boardc:
		board_addrs[2] = args.boardc
	

	start_time = time.time()
		
	rec_q = queue.Queue(maxsize=0)	# Thread Queues
	
	process_thread = threading.Thread(name='process',target=message_process,args=(rec_q,))
	process_thread.daemon = True
	process_thread.start()
	
	socket_thread = threading.Thread(name='socket',target=socket_process,args=(rec_q, sock, remote))
	socket_thread.daemon = True
	socket_thread.start()
	
	while(1):
		time.sleep(1)
	
		
############################################################################
# Script Entry Point
############################################################################
if __name__ == '__main__':
    main()
    
