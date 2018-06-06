#!/usr/bin/python3
# encoding: utf-8
############################################################################
"""
uiAutoForm.py

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

Created by Charles Leech on 2017-04-07.

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

import npyscreen
import curses

from uiPRIMEApp import PRIMEApp
from uiPRIMEApp import PRIMEMonitor

valid_cmds = ["PRIME_UI_ADD_APP", "PRIME_UI_APP_START", "PRIME_UI_APP_STOP", \
				"PRIME_UI_APP_REG", "PRIME_UI_APP_DEREG", \
				"PRIME_UI_APP_MON_DISC_MIN", "PRIME_UI_APP_MON_DISC_MAX", "PRIME_UI_APP_MON_DISC_WEIGHT", \
				"PRIME_UI_APP_MON_CONT_MIN", "PRIME_UI_APP_MON_CONT_MAX", "PRIME_UI_APP_MON_CONT_WEIGHT", \
				"PRIME_UI_APP_WEIGHT", "PRIME_UI_RTM_POLICY_SWITCH", \
				"PRIME_UI_LOG_FILE_FILTER_MUX", "PRIME_UI_VISUAL_FILTER_MUX"]

ui_address = "/tmp/ui.uds"
rtm_address = "/tmp/rtm.ui.uds"
dev_address = "/tmp/dev.ui.uds"

############################################################################
# Auto Mode Form
############################################################################	
class AutoForm(npyscreen.ActionFormMinimal):	
	def create(self):
		#Data Objects
		self.appLib = {} #Library if applications that have been added to the UI
		self.apps = {} #Dictionary of active applications started by the UI
		self.timestamps = [] #List of message timestamps
		self.messages = [] #List of messages to be send to sockets
		self.app_stdout_t = {}
		self.keypress_timeout = 1 #required to activate while_waiting function, number appears to be fairly arbitrary
		
		#Form Objects
		self.name = "PRiME Framework UI - Auto Mode"
		self.info = self.add(npyscreen.TitleMultiLine, name="Info:", max_height=3, editable=False, \
								values=["Auto Mode: Run a generated test script to control the system.", \
								"Type the filename of the test script or browse to its location.", \
								"Select \"Run Test\" to load and start the test script."])
		self.nextrely += 1
		
		self.tfc = self.add(npyscreen.TitleFilenameCombo, name="Filename:", value="[Enter] Select a test file", scroll_exit=True)
		self.timeModeSelect = self.add(npyscreen.TitleSelectOne, name="Time Mode:", max_height=2, values=["Cumulative", "Additive"], scroll_exit=True)
		
		self.parseTestBtn = self.add(npyscreen.ButtonPress, name="Parse Test Script")
		self.parseTestBtn.whenPressed = self.parseTest
		self.runTestBtn = self.add(npyscreen.ButtonPress, name="Run Test Script", editable=False)
		self.runTestBtn.whenPressed = self.runTest
		self.nextrely += 1
		self.labelPager = self.add_widget(npyscreen.FixedText, editable=False, value="Trace Log:")
		self.traceLog = self.add(npyscreen.BufferPager, autowrap=True, name = "Log", max_height=-1, editable=False, scroll_exit=True)
		
		self.how_exited_handers[npyscreen.wgwidget.EXITED_ESCAPE]  = self.exit_application
	
	def while_waiting(self, *args):
		self.display()

	def exit_application(self, *args):
		self.parentApp.setNextForm("MAIN")
		self.editing = False
	
	def on_ok(self, *args):
		if self.apps:
			npyscreen.notify_confirm("Please wait for all applications to stop before leaving Demo mode.", editw=1)
			return
		self.parentApp.setNextForm("MAIN")
		self.editing = False
	
	def log(self, message):
		if(type(message) == PRIMEApp):
			message = message.info
		elif(type(message) == dict):
			message = json.dumps(message)
		elif(type(message) != str):
			message = str(message)
		self.parentApp.log_lock.acquire()
		self.traceLog.values.append(time.strftime('%H:%M:%S', time.gmtime()) + " :: " + message)
		if len(self.traceLog.values) > 40:
			self.traceLog.values.popleft()
		self.parentApp.log_lock.release()

	# parse entire test script line by line
	# check for correctness here, not during execution
	def parseTest(self, *args):
		#pre-parsing checks: valid file selected
		try:
			testfilename = self.tfc.value
			if not testfilename.endswith('.json'):
				self.log("Invalid Extension, must be .json file")
				self.tfc.edit()
				return
			testseq = open(testfilename, 'r')
		except OSError:
			self.log("Execption: Cannot open " + testfilename)
			self.tfc.edit()
			return
		#pre-parsing checks: time mode selected.
		if not self.timeModeSelect.value:
			self.timeModeSelect.edit()
			return
		timeMode = self.timeModeSelect.value[0]
		if timeMode:
			self.log("Time Mode is Additive.")
		else:
			self.log("Time Mode is Cumulative.")
			
		self.log("Parsing Test Script...")
		self.parseTestBtn.editable = False
		npyscreen.notify("Parsing Test Script.\nPlease wait...")
		
		curr_time = 0
		self.appLib = {}
		self.timestamps = []
		self.messages = []
		
		
		for testline in testseq:
			if testline == "\n":
				continue
			testline = json.loads(testline)				
			#self.log("Test Line: " + json.dumps(testline))
			
			try: # check command type is valid
				if testline['type'] not in valid_cmds:
					self.log("Invalid test command: " + json.dumps(testline['type']) +  " Ignoring command: " + json.dumps(testline))
					continue
			except KeyError:
				self.log("Type field not found. Ignoring command: " + json.dumps(testline))
				continue
			else:
				self.log(testline['type'])
				
			if testline['type'] == 'PRIME_UI_ADD_APP':
				self.addApp(testline)
				continue
				
			#process timestamp first
			if timeMode:
				self.timestamps.append(testline['exec_time'])
				curr_time += testline.pop('exec_time')
			else:
				self.timestamps.append(testline['exec_time'] - curr_time)
				curr_time = testline.pop('exec_time')
				
			
			try:
				ui_id = testline['UIID']
			except KeyError:
				# PRIME_UI_RTM_POLICY_SWITCH, PRIME_UI_FILE_FILTER_MUX and PRIME_UI_VISUAL_FILTER_MUX dont have (app) name field
				if testline['type'] not in ['PRIME_UI_RTM_POLICY_SWITCH', 'PRIME_UI_FILE_FILTER_MUX', 'PRIME_UI_VISUAL_FILTER_MUX']:
					self.log("UIID field not found in command that requires it. Ignoring command: " + json.dumps(testline))
					continue
					
				#no fields to check so can automatically approve message
				self.messages.append(testline)
				continue
				
			if testline['type'] == 'PRIME_UI_APP_START':
				try:
					appName = testline['name']
				except KeyError:
					self.log("Invalid test command: " + json.dumps(testline['type']) + " requires \"name\" field. Ignoring command: " + json.dumps(testline))
				else:
					if appName not in self.appLib:
						self.log("Application \"" + appName + "\" does not exist. Ignoring command: " + json.dumps(testline))
						del self.timestamps[-1] #remove timestamp for invalid command
					else:
						self.appStart_parse(testline)
				continue
				
				
			# check that app exists and is started
			if ui_id not in self.apps:
				self.log("Application UIID" + str(ui_id) + " (" + self.apps[ui_id].name + ") not started. Ignoring command: " + json.dumps(testline))
				del self.timestamps[-1] #remove timestamp for invalid command
				continue
				
			if testline['type'] == 'PRIME_UI_APP_REG':
				self.appReg_parse(testline)
				continue
				
			if testline['type'] == 'PRIME_UI_APP_STOP':
				self.appStop_parse(testline)
				continue
				
			# check that app exists, is started and already registered
			if not self.apps[ui_id].registered:
				self.log("Application UIID" + str(ui_id) + " (" + self.apps[ui_id].name + ") not registered. Ignoring command: " + json.dumps(testline))
				del self.timestamps[-1] #remove timestamp for invalid command
				continue
		
			if testline['type'] == 'PRIME_UI_APP_DEREG':
				self.appDereg_parse(testline)
				continue
				
			if testline['type'] in ['PRIME_UI_APP_MON_DISC_MIN', 'PRIME_UI_APP_MON_DISC_MAX', 'PRIME_UI_APP_MON_DISC_WEIGHT', \
									'PRIME_UI_APP_MON_CONT_MIN', 'PRIME_UI_APP_MON_CONT_MAX', 'PRIME_UI_APP_MON_CONT_WEIGHT']:
				self.appMon_parse(testline)
				continue
			
			if testline['type'] == 'PRIME_UI_APP_WEIGHT':
				self.appWeight_parse(testline)
				continue
			
		testseq.close()
		
		for app in self.appLib:
			self.log(" ")
			self.log("Applications Added:")
			self.log("Name: " + self.appLib[app].name)
			self.log("Path: " + self.appLib[app].abspath)
			self.log("Arguments: " + self.appLib[app].arguments)
			
		#reset app states before runTest
		self.apps.clear()
		
		self.parseTestBtn.editable = True
		self.runTestBtn.editable = True
		
	def addApp(self, testline):
		try:
			appName = testline['name']
			appAbspath = testline['abspath']
			appArguments = testline['arguments']
		except KeyError:
			self.log("Required data fields not found: " + json.dumps(testline))
			return;
		else:
			if appName in self.appLib:
				self.log("Application \"" + appName + "\" already added. Ignoring command: " + json.dumps(testline))
				return;
			self.log("Application " + appName + " Added to UI.")
			self.appLib[appName] = PRIMEApp(appName, "", appAbspath, appArguments, "", "", False, False)
		
	
	# add app must be specified in test script before app start
	# check that app not already started
	def appStart_parse(self, testline):
		appName = testline['name']
		ui_id = testline['UIID']
		if ui_id in self.apps:
			self.log("Application UIID " + str(ui_id) + " (" + self.apps[ui_id].name + ") already started. Ignoring command: " + json.dumps(testline))
			del self.timestamps[-1] #remove timestamp for invalid command
		else:
			self.apps[ui_id] = PRIMEApp(appName, ui_id, self.appLib[appName].abspath, self.appLib[appName].arguments, "", "", False, False)
			self.messages.append(testline)
	
	# check that app exists, is started and not registered
	def appReg_parse(self, testline):
		ui_id = testline['UIID']
		if self.apps[ui_id].registered:
			self.log("Application UIID " + str(ui_id) + " (" + self.apps[ui_id].name + ") already registered. Ignoring command: " + json.dumps(testline))
			del self.timestamps[-1] #remove timestamp for invalid command
		else:
			self.messages.append(testline)
			self.apps[ui_id].registered = True

	# check that app started and not registered
	def appStop_parse(self, testline):
		ui_id = testline['UIID']
		if self.apps[ui_id].registered:
			self.log("Application UIID " + str(ui_id) + " (" + self.apps[ui_id].name + ") registered, must deregister before stopping. Ignoring command: " + json.dumps(testline))
			del self.timestamps[-1] #remove timestamp for invalid command
		else:
			self.messages.append(testline)
			self.apps[ui_id].started = False

	# check that app exists, is started and registered
	def appDereg_parse(self, testline):
		self.messages.append(testline)
		self.apps[testline['UIID']].registered = False
	
	# check that app exists, is started and registered
	def appMon_parse(self, testline):
		try:
			msg_data = testline['data']
		except KeyError:
			self.log("Data field not found. Ignoring command: " + json.dumps(testline))
			return
		else:
			try:
				if not self.apps[testline['UIID']].registered: #check that app is registered
					self.log("Application UIID " + str(ui_id) + " (" + self.apps[ui_id].name + ") not registered. Ignoring command: " + json.dumps(testline))
					return
			except KeyError: #check if app exists
				self.log("Application UIID " + str(ui_id) + " not found. Ignoring command: " + json.dumps(testline))
				return
				
			if testline['type'] in ['PRIME_UI_APP_MON_DISC_MIN', 'PRIME_UI_APP_MON_CONT_MIN']:
				try:
					mon_min = msg_data['min']
					self.log("min = " + str(mon_min))
				except KeyError:
					self.log("min field not found. Ignoring command: " + json.dumps(testline))
					return
				
			elif testline['type'] in ['PRIME_UI_APP_MON_DISC_MAX', 'PRIME_UI_APP_MON_CONT_MAX']:
				try:
					mon_max = msg_data['max']
					self.log("max = " + str(mon_max))
				except KeyError:
					self.log("max field not found. Ignoring command: " + json.dumps(testline))
					return
					
			elif testline['type'] in ['PRIME_UI_APP_MON_DISC_WEIGHT', 'PRIME_UI_APP_MON_CONT_WEIGHT']:
				try:
					weight = msg_data['weight']
					self.log("weight = " + str(weight))
				except KeyError:
					self.log("weight field not found. Ignoring command: " + json.dumps(testline))
					return
			self.messages.append(testline)
	
	def appWeight_parse(self, testline):
		try:
			weight = testline['data']['weight']
			self.log("weight = " + str(weight))
		except KeyError:
			self.log("weight field not found. Ignoring command: " + json.dumps(testline))
			return
		else:
			self.messages.append(testline)


	# Run test - send messages and wait delays
	def runTest(self, *args):	
		self.runTestBtn.editable = False
		self._added_buttons['ok_button'].editable = False
		
		self.log(" ")
		self.log("Running Test Script...")
		#npyscreen.notify("Running Test Script.\nPlease wait...")
		
		for delay, msg in zip(self.timestamps, self.messages):
			self.traceLog.display()
			time.sleep(delay)
			self.log(msg['type'])
			
			try: #check app name exists
				ui_id = msg['UIID']
			except KeyError:
				# PRIME_UI_RTM_POLICY_SWITCH, PRIME_UI_FILE_FILTER_MUX and PRIME_UI_VISUAL_FILTER_MUX dont have (app) name field
				if msg['type'] not in ['PRIME_UI_RTM_POLICY_SWITCH', 'PRIME_UI_FILE_FILTER_MUX', 'PRIME_UI_VISUAL_FILTER_MUX']:
					self.log("UIID not found in command that requires it. Invalid command: " + json.dumps(msg))
					
				#rtm messages
				elif msg['type'] in ['PRIME_UI_RTM_POLICY_SWITCH']:
					self.parentApp.send_message(msg, rtm_address)
			
				#logger messages
				elif msg['type'] in ['PRIME_UI_FILE_FILTER_MUX', 'VISUAL_FILTER_MUX']:
					self.parentApp.send_message(msg, "logger_address")
				continue
			
			if msg['type'] == 'PRIME_UI_APP_START':
				try:
					appName = msg['name']
				except KeyError:
					self.log("Application name not specified. Ignoring command: " + json.dumps(msg))
				else:
					self.apps[ui_id] = PRIMEApp(appName, ui_id, self.appLib[appName].abspath, self.appLib[appName].arguments, "", "", False, False)
					self.appStart_run(appName, ui_id)
			
			# check that app exists and is started
			elif not self.apps[ui_id].started:
				self.log("Application " + ui_id + " not started. Ignoring command: " + json.dumps(msg))
			
			#app messages
			elif msg['type'] =='PRIME_UI_APP_STOP':
				address = self.apps[ui_id].address
				msg["data"] = {"proc_id": self.apps[ui_id].handle.pid}
				self.parentApp.send_message(msg, address)
				
			elif msg['type'] == 'PRIME_UI_APP_REG':
				address = self.apps[ui_id].address
				msg["data"] = {"proc_id": self.apps[ui_id].handle.pid}
				self.parentApp.send_message(msg, address)
				
			# check that app exists, is started and registered
			elif not self.apps[ui_id].registered:
				self.log("Application " + ui_id + " not registered. Ignoring command: " + json.dumps(msg))
			
			elif msg['type'] == 'PRIME_UI_APP_DEREG':
				address = self.apps[ui_id].address
				msg["data"] = {"proc_id": self.apps[ui_id].handle.pid}
				self.parentApp.send_message(msg, address)
				
			elif msg['type'] in ['PRIME_UI_APP_MON_DISC_MIN', 'PRIME_UI_APP_MON_DISC_MAX', 'PRIME_UI_APP_MON_DISC_WEIGHT', \
									'PRIME_UI_APP_MON_CONT_MIN', 'PRIME_UI_APP_MON_CONT_MAX', 'PRIME_UI_APP_MON_CONT_WEIGHT']:
				address = self.apps[ui_id].address
				mon_id = str(msg["data"]["id"])
				if mon_id not in self.apps[ui_id].monitors:
					self.log("Monitor not registered: Application UIID = %s, PID = %s, Monitor ID = %s" %(ui_id, self.apps[ui_id].handle.pid, mon_id))
				else:
					self.parentApp.send_message(msg, address)
			
			#rtm messages
			elif msg['type'] in ['PRIME_UI_APP_WEIGHT']:
				address = rtm_address
				msg["proc_id"] = self.apps[ui_id].handle.pid
				self.parentApp.send_message(msg, address)
		
		self.log("Test Script Complete!")
		self.apps.clear()
		self.runTestBtn.editable = True
		self._added_buttons['ok_button'].editable = True
		
	
	#app start has its own function as more complex and does not involve just passing on the message
	def appStart_run(self, appName, ui_id):
		self.log("Starting Application " + appName + " under UIID " + ui_id)
		try:
			self.apps[ui_id].handle = subprocess.Popen([self.appLib[appName].abspath] + self.appLib[appName].arguments.replace('"','').split(), \
				stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
			self.apps[ui_id].address = '/tmp/app.ui.' + str(self.apps[ui_id].handle.pid) + '.uds'
		except OSError:
			self.log("Test Script Halted! Error: Application Start Failed for: " + str(appName))
			raise

	def appStartReturn(self, pid):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].started = True
				self.log("Application Started: " + self.apps[ui_id].name + " " + self.apps[ui_id].arguments)
				self.log("UDS Address: " + self.apps[ui_id].address)
				msg = {"type": "PRIME_UI_APP_START", "name": self.apps[ui_id].name, "UIID": ui_id}
				self.parentApp.send_message(msg, "logger_address")
				
				self.app_stdout_t[ui_id] = threading.Thread(target=self.parentApp.read_subprocess_output, args=(self.apps[ui_id].handle.stdout,))
				self.app_stdout_t[ui_id].daemon = True
				self.app_stdout_t[ui_id].start()
				
				return
			
	def appStopReturn(self, pid):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].started = False
				self.log("Application Stopped %s (%s)" %(ui_id, self.apps[ui_id].name))
				return
		
	def appRegReturn(self, pid):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].registered = True
				self.log("Application Registered %s (%s)" %(ui_id, self.apps[ui_id].name))
				return
	
	def appDeregReturn(self, pid):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].registered = False
				self.log("Application Deregistered %s (%s)" %(ui_id, self.apps[ui_id].name))
				return

	def appMonReg(self, pid, mon):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].monitors[str(mon.id)] = mon
				self.log("Monitor registered: Application UIID = %s, PID = %s, Monitor ID = %s (%s < val < %s) weight = %s." %(ui_id, pid, mon.id, mon.min, mon.max, mon.weight))
				return

	def appMonDereg(self, pid, mon_id):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				del self.apps[ui_id].monitors[mon_id]
				self.log("Monitor deregistered: Application UIID = %s, PID = %s, Monitor ID = %s." %(ui_id, pid, mon_id))
				return
	
	def appMonMin(self, pid, mon_id, mon_min):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].monitors[mon_id].min = mon_min
				self.log("Monitor Min Updated: Application UIID = %s, PID = %s, Monitor ID = %s, min = %s." %(ui_id, pid, mon_id, mon_min))
				return

	def appMonMax(self, pid, mon_id, mon_max):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].monitors[mon_id].max = mon_max
				self.log("Monitor Max Updated: Application UIID = %s, PID = %s, Monitor ID = %s, max = %s." %(ui_id, pid, mon_id, mon_max))
				break
	
	def appMonWeight(self, pid, mon_id, mon_weight):
		for ui_id in self.apps:
			if int(self.apps[ui_id].handle.pid) == int(pid):
				self.apps[ui_id].monitors[mon_id].weight = mon_weight
				self.log("Monitor Weight Updated: Application UIID = %s, PID = %s, Monitor ID = %s, weight = %s." %(ui_id, pid, mon_id, mon_weight))
				return
				
