#!/usr/bin/python3
# encoding: utf-8
############################################################################
"""
ui.py

This file is part of the PRiME Framework UI.

The PRiME Framework UI is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The PRiME Framework UI is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the PRiME Framework UI.  If not, see <http://www.gnu.org/licenses/>.

Copyright 2017, 2018 University of Southampton

Created by Charles Leech on 2017-02-15.
Modified by Graeme Bragg on 2017-04-04: Added app argument functionality.

Sources:
http://npyscreen.readthedocs.io/form-objects.html#creating-a-form
https://github.com/docmeth02/re-wire/blob/master/includes
"""
############################################################################
import socket
import sys
import os
import signal
import subprocess
import json
import time
import threading

import argparse
import npyscreen
import curses

from uiPRIMEApp import PRIMEApp
from uiPRIMEApp import PRIMEMonitor
from uiMainForm import MainForm
from uiAutoForm import AutoForm
from uiDemoForm import DemoForm
from uiTSGForm  import TestGenForm

logger_address = "/tmp/logger.uds"
logger_port = 5000
logger_remote = False
logger_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

ui_address = "/tmp/ui.uds"
rtm_address = "/tmp/rtm.ui.uds"
dev_address = "/tmp/dev.ui.uds"

############################################################################
# UI Socket Class
############################################################################
class UISocket:
	def __init__(self):
	
		parser = argparse.ArgumentParser(description='PRiME Framework Logger.')
		parser.add_argument("-p", "--port", type=int, help="Set the port that the UI should listen on. If this is not set, the UI will bind to " + ui_address)
	
		parser.add_argument("-la", "--logger_addr", type=str, help="Set remote logger address.")
		parser.add_argument("-lp", "--logger_port", type=int, help="Set remote logger port.")
	
		args = parser.parse_args()
		
		if args.port is not None and args.port > 0 and args.port < 65536:
			#Remote UI
			sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			sock.bind(('0.0.0.0', args.port))
			remote = True
			print("Remote UI listening on port " + str(args.port))
		else:
			#Local UI
			try:
				os.unlink(ui_address)
			except OSError:
				if os.path.exists(ui_address):
					raise
		
			self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
			self.sock.bind(ui_address)
			self.lock = threading.Lock()
			remote = False
		
		global logger_remote
		if args.logger_addr:
			logger_remote = True
			global logger_address
			logger_address = args.logger_addr
			
		if args.logger_port:
			logger_remote = True
			global logger_port
			logger_port = args.logger_port
		
############################################################################
# UI App Function
############################################################################
class UIApp(npyscreen.NPSAppManaged):
	def onStart(self):
    		
		signal.signal(signal.SIGINT, self.signal_handler)
		self.signal_names = dict((k, v) for v, k in reversed(sorted(signal.__dict__.items())) \
			if v.startswith('SIG') and not v.startswith('SIG_'))
		
		self.dev_handle = None
		self.rtm_handle = None
		
		self.ui_sock = UISocket()
		self.log_lock = threading.Lock()

		npyscreen.setTheme(npyscreen.Themes.ElegantTheme)
		self.addForm("MAIN", MainForm, name="Main Form", minimum_lines=50)
		self.addForm("AUTO", AutoForm, name="Auto Mode", minimum_lines=50)
		self.addForm("DEMO", DemoForm, name="Demo Mode", minimum_lines=50)
		self.addForm("TSG", TestGenForm, name="Test Script Generator", minimum_lines=40)
		
		self.formChange = threading.Event()
		self.message_thread = threading.Thread(target=self.message_handler, args=(self.formChange,))
		self.message_thread.start()
		
	def onInMainLoop(self):
		if self.dev_handle is not None:
			if self.dev_handle.poll() == -signal.SIGABRT:
				target_form = self.getForm(self.ACTIVE_FORM_NAME)
				target_form.log("Device send SIGABRT signal")
				self.getForm("MAIN").devStopReturn()
		if self.rtm_handle is not None:
			if self.rtm_handle.poll() == -signal.SIGABRT:
				target_form = self.getForm(self.ACTIVE_FORM_NAME)
				target_form.log("RTM send SIGABRT signal")
				self.getForm("MAIN").rtmStopReturn()
    
	def clean_subprocesses(self):
		try:
			if self.dev_handle is not None:
				self.dev_handle.send_signal(signal.SIGTERM)
			if self.rtm_handle is not None:
				self.rtm_handle.send_signal(signal.SIGTERM)
		except:
			raise
			
		self.close_message_handler()
		unlink_socket(ui_address)
		
	def onCleanExit(self, *args):
		self.clean_subprocesses()
		npyscreen.notify_confirm("Terminating RTM and Device Processes.\nGoodbye!", editw=1)
		
	def signal_handler(self, signal_in, frame):
		self.clean_subprocesses()
		sys.exit(0)
	
	def send_message(self, message, address):
		if address not in ui_address:
			message["ts"] = int(time.time() * 1000)
		
		target_form = self.getForm(self.ACTIVE_FORM_NAME)
		if(type(message) != dict):
			try:
				message = json.loads(message)
			except ValueError:
				target_form.log("Error: Failed to send message to %s. Message must be in json dictionary format." %(str(address)))
				return -1
		try:
			if (address in "logger_address") and logger_remote:
				logger_sock.sendto(str.encode(json.dumps(message, sort_keys=True)), (logger_address, logger_port))
			else:
				self.ui_sock.sock.sendto(str.encode(json.dumps(message, sort_keys=True)), address)
		except ConnectionRefusedError:
			target_form.log("Error: ConnectionRefusedError (socket shutdown/closed by remote endpoint)")
			target_form.log("Error: Failed to send message to " + str(address))
			if address not in [ui_address, logger_address, dev_address, rtm_address]:
				for appName, app in target_form.apps.items():
					if app.address == address:
						try:
							proc_state = self.signal_names[-app.handle.poll()]
						except KeyError:
							proc_state = "Unknown"
						target_form.log("Error: Process: %s has crashed with signal: %s" %(app.name, proc_state))
						target_form.appStopReturn(app.handle.pid)
		except socket.error:
			target_form.log("Error: Failed to send message to " + str(address))
		if address not in [ui_address, logger_address]:
			try:
				# echo all messages to the logger
				if logger_remote:
					logger_sock.sendto(str.encode(json.dumps(message, sort_keys=True)), (logger_address, logger_port))
				else:
					self.ui_sock.sock.sendto(str.encode(json.dumps(message, sort_keys=True)), logger_address)
			except ConnectionRefusedError:
				target_form.log("Error: ConnectionRefusedError")
				target_form.log("Error: Failed to send message to " + str(address))
				target_form.log("Error: Logger crashed unexpectedly. Please restart it or debug.")
				target_form.log("Error: The source of the bug is most likely not in the logger.")
			except socket.error:
				target_form.log("Error: Failed to send message to " + str(logger_address))
				raise

	def message_handler(self, exit_flag):
		while not exit_flag.wait(0):
			self.ui_sock.lock.acquire()
			data, address = self.ui_sock.sock.recvfrom(4096*4)
			self.ui_sock.lock.release()
			#decode message
			data_string = data.decode("utf-8")
			try:
				json_msg = json.loads(data_string)
			except ValueError: #not json
				self.getForm(self.ACTIVE_FORM_NAME).log(json.loads(json.dumps(data_string)))
				continue
			try:
				msg_type = json_msg["type"]
			except KeyError:
				self.getForm(self.ACTIVE_FORM_NAME).log("UI socket received a message from %s but no type field was present.")
			else:
				target_form = self.getForm(self.ACTIVE_FORM_NAME)
				# UI - RTM Types:
				if msg_type == "PRIME_UI_RTM_RETURN_RTM_START":
					self.getForm("MAIN").rtmStartReturn()
					continue
				elif msg_type == "PRIME_UI_RTM_RETURN_RTM_STOP":
					self.getForm("MAIN").rtmStopReturn()
					continue
				elif msg_type == "PRIME_UI_RTM_ERROR":
					err_msg = json_msg["data"]["msg"]
					self.getForm("MAIN").rtmErrorHandler(err_msg)
					target_form.log("RTM ERROR! Message: " + err_msg)
					# target_form.log(err_msg)
					continue
					
				# UI - DEV Types:
				elif msg_type == "PRIME_UI_DEV_RETURN_DEV_START":
					self.getForm("MAIN").devStartReturn()
					continue
				elif msg_type == "PRIME_UI_DEV_RETURN_DEV_STOP":
					self.getForm("MAIN").devStopReturn()
					continue
				elif msg_type == "PRIME_UI_DEV_ERROR":
					err_msg = json_msg["data"]["msg"]
					self.getForm("MAIN").devErrorHandler(err_msg)
					target_form.log("DEV ERROR! Message:")
					target_form.log(err_msg)
					continue
				
				#Redirect APP related message to the last active app form rather than main.
				if self.ACTIVE_FORM_NAME == "MAIN":
					if self.LAST_ACTIVE_FORM_NAME != "MAIN":
						target_form = self.getForm(self.LAST_ACTIVE_FORM_NAME)
					else:
						target_form.log("APP message type: " + msg_type + " received from " + address)
						continue
				
				# UI - APP Types:
				if msg_type == "PRIME_UI_APP_RETURN_APP_START":
					target_form.appStartReturn(json_msg["data"]["proc_id"])
				elif msg_type == "PRIME_UI_APP_RETURN_APP_STOP":
					target_form.appStopReturn(json_msg["data"]["proc_id"])
				elif msg_type == "PRIME_UI_APP_ERROR":
					#enhancement: try to process some common errors (e.g. incorrect app arguments)
					try:
						target_form.log("Error received from application on socket: %s \n Message:'%s'" %(address, json_msg["data"]["msg"]))
					except KeyError:
						target_form.log("Error received from application on socket: %s \n No Message Attached" %(address))
				
				# UI - API_APP Types:
				elif msg_type in ["PRIME_API_APP_REG", "PRIME_API_APP_DEREG"]:
					continue
				elif msg_type == "PRIME_API_APP_RETURN_APP_REG":
					target_form.appRegReturn(json_msg["data"]["proc_id"])
				elif msg_type == "PRIME_API_APP_RETURN_APP_DEREG":
					target_form.appDeregReturn(json_msg["data"]["proc_id"])
					
				elif msg_type in ["PRIME_API_APP_MON_DISC_REG", "PRIME_API_APP_MON_CONT_REG"]:
					continue
				elif msg_type == "PRIME_API_APP_RETURN_MON_DISC_REG":
					msg_mon = json_msg["data"]["mon"]
					target_form.appMonReg(msg_mon["proc_id"], PRIMEMonitor(int(msg_mon["id"]), int(msg_mon["type"]), int(msg_mon["min"]), int(msg_mon["max"]), float(msg_mon["weight"])))
				elif msg_type == "PRIME_API_APP_RETURN_MON_CONT_REG":
					msg_mon = json_msg["data"]["mon"]
					target_form.appMonReg(msg_mon["proc_id"], PRIMEMonitor(int(msg_mon["id"]), int(msg_mon["type"]), float(msg_mon["min"]), float(msg_mon["max"]), float(msg_mon["weight"])))
				elif (msg_type == "PRIME_API_APP_MON_CONT_DEREG") or (msg_type == "PRIME_API_APP_MON_DISC_DEREG"):
					msg_mon = json_msg["data"]["mon"]
					target_form.appMonDereg(msg_mon["proc_id"], msg_mon["id"])
				elif (msg_type == "PRIME_API_APP_MON_CONT_MIN") or (msg_type == "PRIME_API_APP_MON_DISC_MIN"):
					msg_mon = json_msg["data"]["mon"]
					target_form.appMonMin(msg_mon["proc_id"], msg_mon["id"],  json_msg["data"]["min"])				
				elif (msg_type == "PRIME_API_APP_MON_CONT_MAX") or (msg_type == "PRIME_API_APP_MON_DISC_MAX"):
					msg_mon = json_msg["data"]["mon"]
					target_form.appMonMax(msg_mon["proc_id"], msg_mon["id"], json_msg["data"]["max"])					
				elif (msg_type == "PRIME_API_APP_MON_CONT_WEIGHT") or (msg_type == "PRIME_API_APP_MON_DISC_WEIGHT"):
					msg_mon = json_msg["data"]["mon"]
					target_form.appMonWeight(msg_mon["proc_id"], msg_mon["id"], json_msg["data"]["weight"])
				
				elif msg_type != "PRIME_UI_FORM_EXIT":
					#target_form.log("Unrecognised message type: " + msg_type + " received from " + address)
					continue
					
	def close_message_handler(self):
		self.formChange.set()
		self.send_message({"type": "PRIME_UI_FORM_EXIT"}, ui_address) #hack to make message_handler thread join
		self.message_thread.join()
	
	def read_subprocess_output(self, out):
		for line in iter(out.readline, b''):
			target_form = self.getForm(self.ACTIVE_FORM_NAME)
			target_form.log(str(line, "utf-8"))
		out.close()
		
############################################################################
# Global Utility Functions
############################################################################
def unlink_socket(address):
	try:
		os.unlink(address)
	except OSError:
		if os.path.exists(address):
			raise
		
############################################################################
# Main function
############################################################################
def main():
	if sys.version_info[0] < 3:
		print("ERROR: UI must be executed using Python 3")
		sys.exit(-1)
	
	UI = UIApp()
	UI.run()

if __name__ == '__main__':
    main()

