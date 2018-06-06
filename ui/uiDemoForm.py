#!/usr/bin/python3
# encoding: utf-8
############################################################################
"""
uiDemoForm.py

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
import npyscreen
import curses
import threading

from uiPRIMEApp import PRIMEApp
from uiPRIMEApp import PRIMEMonitor

ui_address = "/tmp/ui.uds"
rtm_address = "/tmp/rtm.ui.uds"
dev_address = "/tmp/dev.ui.uds"

############################################################################
# Demo Mode Form
############################################################################
class DemoForm(npyscreen.ActionFormMinimal):
	def create(self):
		#Data Objects
		self.apps = {}
		self.currApp = str()
		self.currMonID = str()
		self.app_stdout_t = {}
		self.keypress_timeout = 1 #required to activate while_waiting function, number appears to be fairly arbitrary
		
		#Form Objects
		self.name = "PRiME Framework UI - Demo Mode"
		self.info = self.add(npyscreen.TitleMultiLine, name="Info:", max_height=4, editable=False, \
								values=["Demo Mode: Manually change application and RTM settings.", \
								"Options: Add or detect Apps, Start/Stop Apps, Register/Deregister Apps with RTM.", \
								"- Adjust App Monitor min/max/weight, Adjust App weight."])
		self.nextrely += 1
		
		self.tfc = self.add(npyscreen.TitleFilenameCombo, name="Application Search: ", value="../build", scroll_exit=True)
		self.findAppBtn = self.add(npyscreen.ButtonPress, name="Add App/Find Apps")
		self.findAppBtn.whenPressed = self.findApps
		self.nextrely += 1
		
		self.appList = self.add(npyscreen.TitleSelectOne, name="Applications:", values = self.apps, max_height=5, scroll_exit=True, editable=False, select_exit=True)
		self.appList.add_handlers({curses.ascii.SP: self.appSelected})
		self.appList.add_handlers({curses.ascii.NL: self.appSelected})
		
		#appLibTreeData = npyscreen.TreeData(content='Root', selectable=False, ignore_root=True)
		#self.appLibTree = self.add(npyscreen.MLTreeMultiSelect, values = appLibTreeData, max_height=3, scroll_exit=True, editable=False, select_exit=True)
		
		self.appArgsText = self.add(npyscreen.TitleText, name="Arguments:", value="", hidden=True)
		self.nextrely += 1
		
		self.appStartStopBtn = self.add(npyscreen.ButtonPress, name="Start", hidden=True)
		self.appStartStopBtn.whenPressed = self.appStartStop
		self.appRegBtn = self.add(npyscreen.ButtonPress, name="Register", hidden=True)
		self.appRegBtn.whenPressed = self.appReg
		self.userWeightSlider = self.add(npyscreen.TitleSliderPercent, name="App Weighting:", accuracy=1, editable=False, hidden=True)
		self.userWeightSlider.add_handlers({curses.ascii.NL: self.appWeightUpdate})
		
		self.appMonList = self.add(npyscreen.TitleSelectOne, name="Application Monitors:", max_height=5, scroll_exit=True, select_exit=True, editable=False, hidden=True)
		self.appMonList.add_handlers({curses.ascii.SP: self.monSelected})
		self.appMonList.add_handlers({curses.ascii.NL: self.monSelected})
		self.monText = self.add(npyscreen.FixedText, name="Update Monitor Settings:", editable=False, hidden=True)
		self.monMinText = self.add(npyscreen.TitleText, name="Minimum:", editable=False, hidden=True)
		self.monMinText.add_handlers({curses.ascii.NL: self.monMinUpdate})
		self.monMaxText = self.add(npyscreen.TitleText, name="Maximum:", editable=False, hidden=True)
		self.monMaxText.add_handlers({curses.ascii.NL: self.monMaxUpdate})
		self.monWeightText = self.add(npyscreen.TitleText, name="Weighting:", editable=False, hidden=True)
		self.monWeightText.add_handlers({curses.ascii.NL: self.monWeightUpdate})
		self.nextrely += 1
		
		self.labelPager = self.add_widget(npyscreen.FixedText, editable=False, value="Log:", colour="green")
		self.traceLog = self.add(npyscreen.BufferPager, autowrap=True, name="Log", max_height=-1, editable=False, scroll_exit=True)
		self.how_exited_handers[npyscreen.wgwidget.EXITED_ESCAPE]  = self.exit_application
		
	def while_waiting(self, *args):
		self.display()
		
	def exit_application(self, *args):
		self.parentApp.setNextForm("MAIN")
		self.editing = False
	
	def on_ok(self, *args):
		for app in self.apps:
			if self.apps[app].started:				
				npyscreen.notify_confirm("Please stop all applications before leaving Demo mode.", editw=1)
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
		self.traceLog.values.append(message)
		if len(self.traceLog.values) > 40:
			self.traceLog.values.popleft()
		self.parentApp.log_lock.release()

	def findApps(self, *args):
		if self.tfc.value == "[Enter] Select file/directory":
			self.tfc.edit()
			return
		app_dir = self.tfc.value
		if os.path.isdir(app_dir):
			try:		
				for filename in os.listdir(app_dir):
					if os.path.isfile(app_dir + "/" + filename) and \
					os.access(app_dir + "/" + filename, os.X_OK) and \
					filename[:3] in "app_" and \
					filename not in self.apps: 
						self.apps[filename] = PRIMEApp(filename, "", app_dir + "/" + filename, "", "", "", False, False)
						self.appList.values.append(filename)
						#self.appLibTreeData.newchild(content=filename, selectable=True)
						self.appList.editable=True
						self.log("Found Application: " + str(filename))
						self.log("Application Path: " + str(app_dir))
			except OSError:
				if not os.path.exists(app_dir):
					self.log("Please enter a valid Application directory or filename.")
					self.tfc.edit()
					return
				else:
					raise
		elif os.path.isfile(app_dir):
			if os.access(app_dir, os.X_OK):
				filename = os.path.basename(app_dir)
				if filename not in self.apps:
					self.apps[filename] = PRIMEApp(filename, "", app_dir, "", "", "", False, False)
					self.appList.values.append(filename)
					#self.appLibTreeData.newchild(content=filename, selectable=True)
					self.appList.editable=True
					self.log("Found Application: " + str(filename))
					self.log("Application Path: " + str(app_dir))
				else:
					self.log("Application already found: " + str(filename))
	
	def appSelected(self, *args):
		try: 
			#save current arguments
			self.apps[self.currApp].arguments = self.appArgsText.value
		except AttributeError:
			#no current app selected, field should not be editable.
			self.appArgsText.hidden = True
		except KeyError:
			#currApp empty if first app to be selected
			self.appArgsText.hidden = True
			
		#reset all buttons
		self.appStartStopBtn.editable = False
		self.appRegBtn.editable = False
		self.appMonList.editable = False
		self.monMinText.editable = False
		self.monMaxText.editable = False
		self.monWeightText.editable = False
		self.userWeightSlider.editable = False
		
		self.appStartStopBtn.hidden = True
		self.appRegBtn.hidden = True
		self.appMonList.hidden = True
		self.monMinText.hidden = True
		self.monMaxText.hidden = True
		self.monWeightText.hidden = True
		self.userWeightSlider.hidden = True
		
		#update current application to match selection
		self.appList.entry_widget.h_select('X')
		self.currApp = self.appList.get_selected_objects()[0]
		self.appArgsText.value = self.apps[self.currApp].arguments
		self.appArgsText.editable = True
		self.appArgsText.hidden = False
		self.log("Selected " + self.currApp + " Application")
		self.log("Choose your next action")
		
		if self.apps[self.currApp].registered:
			self.appStartStopBtn.hidden = False
			self.appRegBtn.name = "Deregister"
			self.appRegBtn.editable = True
			self.appRegBtn.hidden = False
			self.userWeightSlider.editable = True
			self.userWeightSlider.hidden = False
			
			#show available monitors if app registered
			self.appMonList.editable = True
			self.appMonList.hidden = False
			self.appMonList.values.clear()
			for mon_id in self.apps[self.currApp].monitors:
				self.appMonList.values.append(mon_id)
		else:
			self.appStartStopBtn.editable = True
			self.appStartStopBtn.hidden = False
			self.appRegBtn.name = "Register"
			if self.apps[self.currApp].started:
				self.appStartStopBtn.name = "Stop"
				self.appRegBtn.editable = True
				self.appRegBtn.hidden = False
			else:
				self.appStartStopBtn.name = "Start"
				
	
	def appStartStop(self):
		if self.appStartStopBtn.name == "Start":
			self.log("Starting Application: " + self.currApp)
			try:
				#save the specified arguments to the App's entry and then try to execute the app with the specified arguments
				self.apps[self.currApp].arguments = self.appArgsText.value
				self.apps[self.currApp].handle = subprocess.Popen([self.apps[self.currApp].abspath] + self.apps[self.currApp].arguments.replace('"','').split(), \
					stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
				self.apps[self.currApp].address = '/tmp/app.ui.' + str(self.apps[self.currApp].handle.pid) + '.uds'
			except OSError:
				self.log("Application Start Failed for: " + self.currApp)
				raise
			else:
				msg = {"name": self.currApp, "type": "PRIME_UI_APP_START"}
				self.parentApp.send_message(msg, "logger_address")
				#enhancement: start output capture thread
				self.app_stdout_t[self.currApp] = threading.Thread(target=self.parentApp.read_subprocess_output, args=(self.apps[self.currApp].handle.stdout,))
				self.app_stdout_t[self.currApp].daemon = True
				self.app_stdout_t[self.currApp].start()
		
		elif self.appStartStopBtn.name == "Stop":
			self.log("Stopping Application: " + self.currApp)
			msg = {"data": {"proc_id": self.apps[self.currApp].handle.pid}, "type": "PRIME_UI_APP_STOP"}
			self.parentApp.send_message(msg, self.apps[self.currApp].address)
	
	def appStartReturn(self, pid):
		for appName in self.apps:
			if self.apps[appName].handle:
				if self.apps[appName].handle.pid == int(pid):
					self.log("Application Started: " + self.apps[appName].name + " " + self.apps[appName].arguments)
					self.log("UDS Address: " + self.apps[appName].address)
					self.apps[appName].started = 1
					break
		self.appArgsText.editable = False
		self.appStartStopBtn.name = "Stop"
		self.appRegBtn.editable = True
		self.appRegBtn.hidden = False
			
	def appStopReturn(self, pid):
		for appName in self.apps:
			if self.apps[appName].handle:
				if self.apps[appName].handle.pid == int(pid):
					self.log("Application Stopped: " + self.apps[appName].name)
					self.apps[appName].started = 0
					break
		self.appArgsText.editable = True
		self.appStartStopBtn.name = "Start"
		self.appRegBtn.editable = False
		self.appRegBtn.hidden = True
		
	def appReg(self, *args):
		if self.appRegBtn.name == "Register":
			self.log("Registering Application: " + self.currApp)
			msg = {"data": {"proc_id": self.apps[self.currApp].handle.pid}, "type": "PRIME_UI_APP_REG"}
			self.parentApp.send_message(msg, self.apps[self.currApp].address)
		elif self.appRegBtn.name == "Deregister":
			self.log("Deregistering Application: " + self.currApp)
			msg = {"data": {"proc_id": self.apps[self.currApp].handle.pid}, "type": "PRIME_UI_APP_DEREG"}
			self.parentApp.send_message(msg, self.apps[self.currApp].address)
			self.appMonList.editable = False
			self.monMinText.editable = False
			self.monMaxText.editable = False
			self.monWeightText.editable = False
			self.userWeightSlider.editable = False
		self.appRegBtn.editable = False
			
	def appRegReturn(self, pid):
		self.log("Application Registered: " + self.currApp)
		self.log("Choose your next action")
		self.apps[self.currApp].registered = 1
		self.appStartStopBtn.editable = False
		self.appRegBtn.name = "Deregister"
		self.appRegBtn.editable = True
		self.userWeightSlider.editable = True
		self.userWeightSlider.hidden = False
		
		if len(self.apps[self.currApp].monitors):
			self.appMonList.hidden = False
			self.appMonList.values.clear()
			for mon_id in self.apps[self.currApp].monitors:
				self.appMonList.values += str(mon_id)
	
	def appDeregReturn(self, pid):
		self.log("Application Deregistered: " + self.currApp)
		self.log("Choose your next action")
		self.apps[self.currApp].registered = 0
		self.appStartStopBtn.editable = True
		self.appRegBtn.name = "Register"
		
		self.appMonList.hidden = True
		self.monMinText.hidden = True
		self.monMaxText.hidden = True
		self.monWeightText.hidden = True
		self.userWeightSlider.hidden = True
	
	def appMonReg(self, pid, mon):
		for appName in self.apps:
			if self.apps[appName].handle:
				if self.apps[appName].handle.pid == int(pid): #must find app as may not be currApp
					self.apps[appName].monitors[str(mon.id)] = mon
					self.log("Monitor registered: Application PID = %s, Monitor ID = %s (%s < val < %s) weight = %s." %(pid, mon.id, mon.min, mon.max, mon.weight))
					break
		
		#show monitor controls if the registering app was the current app
		if self.apps[self.currApp].handle.pid == int(pid):
			self.appMonList.values.append(str(mon.id))
			self.appMonList.editable = True
			self.appMonList.hidden = False
	
	def appMonDereg(self, pid, mon_id):
		for appName in self.apps:
			if self.apps[appName].handle:
				if self.apps[appName].handle.pid == int(pid): #must find app as may not be currApp
					del self.apps[appName].monitors[mon_id]
					self.log("Monitor deregistered: Application PID = %s, Monitor ID = %s." %(pid, mon_id))
					break
		
		#hide monitor controls if the deregistering app was the current app
		if self.apps[self.currApp].handle.pid == int(pid):
			try:
				self.appMonList.values.remove(mon_id)
			except ValueError:
				self.log("Error: Monitor with ID %s not found for Application PID = %s" %(mon_id, pid))
			if not len(self.apps[self.currApp].monitors):
				self.appMonList.editable = False
				self.monMinText.editable = False
				self.monMaxText.editable = False
				self.monWeightText.editable = False
				self.appMonList.hidden = True
				self.monMinText.hidden = True
				self.monMaxText.hidden = True
				self.monWeightText.hidden = True
		
	def monSelected(self, *args):
		self.appMonList.entry_widget.h_select('X')
		self.currMonID = self.appMonList.get_selected_objects()[0]
		self.monMinText.value = str(self.apps[self.currApp].monitors[self.currMonID].min)			
		self.monMaxText.value = str(self.apps[self.currApp].monitors[self.currMonID].max)
		self.monWeightText.value = str(self.apps[self.currApp].monitors[self.currMonID].weight)
		
		self.monMinText.editable = True
		self.monMaxText.editable = True
		self.monWeightText.editable = True
		self.monMinText.hidden = False
		self.monMaxText.hidden = False
		self.monWeightText.hidden = False
		
	def appMonMin(self, pid, mon_id, mon_min):
		self.monMinText.value = mon_min
		self.apps[self.currApp].monitors[mon_id].min = mon_min

	def appMonMax(self, pid, mon_id, mon_max):
		self.monMaxText.value = mon_max
		self.apps[self.currApp].monitors[mon_id].max = mon_max
	
	def appMonWeight(self, pid, mon_id, mon_weight):
		self.monWeightText.value = mon_weight
		self.apps[self.currApp].monitors[mon_id].weight = mon_weight
	
	def	monMinUpdate(self, *args):
		try:
		   min_float = float(self.monMinText.value)
		except ValueError:
			self.log("Minimum bound must be an number")
			return
		else:
			if float(self.monMinText.value) > float(self.apps[self.currApp].monitors[self.currMonID].max):
				self.log("Minimum bound must be less than or equal to maximum bound")
				return
			msg = {"data": {
					"id": self.apps[self.currApp].monitors[self.currMonID].id, \
					"min": self.monMinText.value}}
			if type(self.apps[self.currApp].monitors[self.currMonID].min) is int:
				msg["type"] = "PRIME_UI_APP_MON_DISC_MIN"
			else:
				msg["type"] = "PRIME_UI_APP_MON_CONT_MIN"
			self.parentApp.send_message(msg, self.apps[self.currApp].address)
			try:
				self.apps[self.currApp].monitors[self.currMonID].min = self.monMinText.value
				self.log("Updating Monitor Minimum Bound to " + self.monMinText.value)
			except KeyError:
					self.log("That monitor was just deregistered: ID = " + self.currMonID)
		
	def	monMaxUpdate(self, *args):
		try:
			max_float = float(self.monMaxText.value)
		except ValueError:
			self.log("Maximum bound must be an number")
			return
		else:
			if float(self.monMaxText.value) < float(self.apps[self.currApp].monitors[self.currMonID].min):
				self.log("Maximum bound must be greater than or equal to minimum bound")
				return
			msg = {"data": {
				"id": self.apps[self.currApp].monitors[self.currMonID].id, \
				"max": self.monMaxText.value}}
			if type(self.apps[self.currApp].monitors[self.currMonID].max) is int:
				msg["type"] = "PRIME_UI_APP_MON_DISC_MAX"
			else:
				msg["type"] = "PRIME_UI_APP_MON_CONT_MAX"
			self.parentApp.send_message(msg, self.apps[self.currApp].address)
			try:
				self.apps[self.currApp].monitors[self.currMonID].max = self.monMaxText.value
				self.log("Updating Monitor Maximum Bound to " + self.monMaxText.value)
			except KeyError:
					self.log("That monitor was just deregistered: ID = " + self.currMonID)
			
	def	monWeightUpdate(self, *args):
		try:
			weight_float = float(self.monWeightText.value)
		except ValueError:
			self.log("Maximum bound must be an number")
			return
		else:
			msg = {"data": {
				"id": self.apps[self.currApp].monitors[self.currMonID].id, \
				"weight": self.monWeightText.value}}
			if type(self.apps[self.currApp].monitors[self.currMonID].min) is int: #weight always float - must use min to check type of mon
				msg["type"] = "PRIME_UI_APP_MON_DISC_WEIGHT"
			else:
				msg["type"] = "PRIME_UI_APP_MON_CONT_WEIGHT"
			self.parentApp.send_message(msg, self.apps[self.currApp].address)
			try:
				self.apps[self.currApp].monitors[self.currMonID].weight = self.monWeightText.value
				self.log("Updating Monitor Weighting to " + self.monWeightText.value)
			except KeyError:
					self.log("That monitor was just deregistered: ID = " + self.currMonID)
			
	def	appWeightUpdate(self, *args):
		msg = {"data": {
			"proc_id": self.apps[self.currApp].handle.pid, \
			"weight": self.userWeightSlider.value}, \
			"type":"PRIME_UI_APP_WEIGHT"}
		self.parentApp.send_message(msg, rtm_address)
		self.apps[self.currApp].weight = self.userWeightSlider.value
		self.log("Updating Application " + str(self.apps[self.currApp].handle.pid) + " Weighting to " + str(self.userWeightSlider.value))

