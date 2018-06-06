#!/usr/bin/python3
# encoding: utf-8
############################################################################
"""
uiMainForm.py

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

Created by Charles Leech on 2017-07-17.

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
# Main UI Control Form
############################################################################
class MainForm(npyscreen.ActionFormMinimal):
	def create(self):
		#Data Objects
		self.keypress_timeout = 1 #required to activate while_waiting function, number appears to be fairly arbitrary
		
		#Form Objects
		self.name = "Welcome to the PRiME Framework UI"
		self.info = self.add(npyscreen.TitleMultiLine, name="Info:", max_height=4, editable=False, \
								values=["The UI can be used in 2 modes:", \
								"Auto Mode: Run a generated test script to control the system.", \
								"Demo Mode: Manually change application and RTM settings.", \
								"Select a mode to be taken to the appropriate UI."])
		self.nextrely += 1
		
		self.devFileCombo = self.add(npyscreen.TitleFilenameCombo, name="Device File:", value="../build/dev", scroll_exit=True, must_exist=True)
		self.devArgsText = self.add(npyscreen.TitleText, name="Device Arguments:", value="", begin_entry_at=20)
		self.devModeBtn = self.add(npyscreen.ButtonPress, name="Start Device Process")
		self.devModeBtn.whenPressed = self.devModeButtonPress
		self.nextrely += 1
		
		self.rtmFileCombo = self.add(npyscreen.TitleFilenameCombo, name="RTM File:", value="../build/rtm_lrgd", scroll_exit=True, editable=False)
		self.rtmArgsText = self.add(npyscreen.TitleText, name="RTM Arguments:", value="", editable=False, begin_entry_at=20)
		self.rtmModeBtn = self.add(npyscreen.ButtonPress, name="Start RTM Process", editable=False)
		self.rtmModeBtn.whenPressed = self.rtmModeButtonPress
		self.nextrely += 1
		
		self.autoModeBtn = self.add(npyscreen.ButtonPress, name="Auto Mode", editable=False)
		self.autoModeBtn.whenPressed = self.change_to_auto_form
		self.demoModeBtn = self.add(npyscreen.ButtonPress, name="Demo Mode", editable=False)
		self.demoModeBtn.whenPressed = self.change_to_demo_form
		self.nextrely += 1
		
		self.writetestBtn = self.add(npyscreen.ButtonPress, name="Write Test Script (for Auto Mode)", editable=False, hidden=True)
		self.writetestBtn.whenPressed = self.change_to_TSG_form
		self.nextrely += 1
		
		self.labelPager = self.add_widget(npyscreen.FixedText, editable=False, value="Log:")
		self.traceLog = self.add(npyscreen.BufferPager, autowrap=True, name = "Log", max_height=-1, editable=False, scroll_exit=True)
		self.traceLog.values.append("First, select and start a device process.")
		self.traceLog.values.append("Second, select and start an RTM process.")

	def while_waiting(self, *args):
		self.display()
	
	def exit_application(self, *args):
		self.parentApp.setNextForm(None)
		self.editing = False
	
	def on_ok(self, *args):
		self.parentApp.setNextForm(None)
		self.editing = False
		
	def change_to_auto_form(self, *args):
		#if not os.path.exists(log_address):
		#	npyscreen.notify_confirm("Please start the logger before entering Auto mode.", editw=1)
		#	return
		self.parentApp.switchForm("AUTO")
		
	def change_to_demo_form(self, *args):
		#if not os.path.exists(log_address):
		#	npyscreen.notify_confirm("Please start the logger before entering Demo mode.", editw=1)
		#	return
		self.parentApp.switchForm("DEMO")
		
	def change_to_TSG_form(self, *args):
		self.parentApp.switchForm("TSG")
	
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
					
	def devModeButtonPress(self, *args):
		if self.devModeBtn.name == "Start Device Process":
			#Start the device process with the arguments specified.
			if os.path.isfile(self.devFileCombo.value):
				if os.access(self.devFileCombo.value, os.X_OK):
					try:
						#Start subprocess			
						self.parentApp.dev_handle = subprocess.Popen([self.devFileCombo.value] + self.devArgsText.value.split(), \
											stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
						#Start create thread to read stdout continuously 
						# inspiration form here: https://stackoverflow.com/questions/375427/non-blocking-read-on-a-subprocess-pipe-in-python
						dev_stdout_t = threading.Thread(target=self.parentApp.read_subprocess_output, args=(self.parentApp.dev_handle.stdout,))
						dev_stdout_t.daemon = True
						dev_stdout_t.start()
						
						#Disable device controls
						self.devFileCombo.editable = False
						self.devArgsText.editable = False
					except OSError:
						if os.path.exists(self.devFileCombo.value):
							raise
						self.log("Error Starting Device Process")
						#re-enable device controls
						self.devFileCombo.editable = True
						self.devArgsText.editable = True
				else:
					self.log("Error: File cannot be executed \"" + str(self.devFileCombo.value.split("/")[-1]) + "\"")
					self.devFileCombo.edit()
			else:
				self.log("Error: File does not exist \"" + str(self.devFileCombo.value) + "\"")
				self.devFileCombo.edit()
		else:
			#Stop the device process	
			msg = {"type": "PRIME_UI_DEV_STOP", "data":{}}
			self.parentApp.send_message(msg, dev_address)
	
	def devStartReturn(self):
		self.devModeBtn.name = "Stop Device Process"
		self.log("Device Process Started")
		#Enable RTM controls
		self.rtmFileCombo.editable = True
		self.rtmModeBtn.editable = True
		self.rtmArgsText.editable = True
		
	def devStopReturn(self):
		self.parentApp.dev_handle = None
		self.devModeBtn.name = "Start Device Process"
		self.log("Device Process Stopped")
		#Enable device controls
		self.devFileCombo.editable = True
		self.devArgsText.editable = True
		#Disable RTM controls
		self.rtmFileCombo.editable = False
		self.rtmModeBtn.editable = False
		self.rtmArgsText.editable = False
	
	def devErrorHandler(self, message):
		return
		
	def rtmModeButtonPress(self, *args):
		#Start the RTM process with the arguments specified.	
		if self.rtmModeBtn.name == "Start RTM Process":
			#Start the RTM process with the arguments specified.
			if os.path.isfile(self.rtmFileCombo.value):
				if os.access(self.rtmFileCombo.value, os.X_OK):
					try:					
						self.parentApp.rtm_handle = subprocess.Popen([self.rtmFileCombo.value] + self.rtmArgsText.value.split(), \
											stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
						#Start create queue and thread to read stdout continuously 
						rtm_stdout_t = threading.Thread(target=self.parentApp.read_subprocess_output, args=(self.parentApp.rtm_handle.stdout,))
						rtm_stdout_t.daemon = True
						rtm_stdout_t.start()
						
						#Disable RTM controls
						self.rtmFileCombo.editable = False
						self.rtmArgsText.editable = False	
						#Disable the device button
						self.devModeBtn.editable = False
					except OSError:
						if os.path.exists(self.rtmFileCombo.value):
							raise
						# Exit if no RTM file found
						self.log("Error Starting RTM Process")
						#re-enable the RTM controls
						self.rtmFileCombo.editable = True
						self.rtmArgsText.editable = True
						#re-enable the device button
						self.devModeBtn.editable = True
						
				else:
					self.log("Error: File cannot be executed \"" + str(self.rtmFileCombo.value.split("/")[-1]) + "\"")
					self.rtmFileCombo.edit()
			else:
				self.log("Error: File does not exist \"" + str(self.rtmFileCombo.value) + "\"")
				self.rtmFileCombo.edit()
		else:
			#Stop the RTM process	
			msg = {"type": "PRIME_UI_RTM_STOP", "data":{}}
			self.parentApp.send_message(msg, rtm_address)
		
	def rtmStartReturn(self):
		self.rtmModeBtn.name = "Stop RTM Process"
		self.log("RTM Process Started")
		#Enable Automode/Demomode buttons
		self.autoModeBtn.editable = True
		self.demoModeBtn.editable = True
			
	def rtmStopReturn(self):
		self.parentApp.rtm_handle = None
		self.rtmModeBtn.name = "Start RTM Process"
		self.log("RTM Process Stopped")
		#Enable the RTM controls
		self.rtmFileCombo.editable = True
		self.rtmArgsText.editable = True
		#Enable the device button
		self.devModeBtn.editable = True	
		#Disable Automode/Demomode buttons
		self.autoModeBtn.editable = False
		self.demoModeBtn.editable = False
	
	def rtmErrorHandler(self, message):
		return
		
