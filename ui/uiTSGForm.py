#!/usr/bin/env python
# encoding: utf-8
############################################################################
"""
uiTSGForm.py

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

from uiPRIMEApp import PRIMEApp

valid_cmds = ["ADD_APP", "APP_START", "APP_STOP", "APP_REG", "APP_DEREG", "APP_MON_MIN", "APP_MON_MAX", "APP_MON_WEIGHT", \
			"USER_APP_WEIGHT", "RTM_POLICY_SWITCH", \
			"FILE_FILTER_MUX", "VISUAL_FILTER_MUX"]

rtm_address = "/tmp/rtm.ui.uds"
log_address = "/tmp/logger.uds"
dev_address = "/tmp/dev.ui.uds"

############################################################################
# Test Script Generator Form
############################################################################
class TestGenForm(npyscreen.ActionFormV2):
	def create(self):
		self.name = "PRiME Framework UI - Test Script Generator"
		self.info = self.add(npyscreen.TitleMultiLine, name="Info:", max_height=7, editable=False, \
								values=["Test Script Generator: Compile test scripts to run in auto mode.", \
								"Options:", \
								"- Add or detect Apps.", \
								"- Start/Stop Apps", \
								"- Register/Deregister Apps with RTM.", \
								"- Adjust App Monitor min/max/weight.", \
								"- Adjust App weight."])
		self.nextrely += 1
						
		#Data Objects
		self.apps = {}
		self.selectedApp = str()
		global fd
		
		self.tfc = self.add(npyscreen.TitleFilenameCombo, name="Filename:", value="[Enter] Select a file to write script to.", scroll_exit=True)
		self.fileOpts = self.add(npyscreen.TitleSelectOne, name="File Options", max_height=2, values=["Create/Overwrite File", "Append to File"], scroll_exit=True)
		self.openFileBtn = self.add(npyscreen.ButtonPress, name="Open File")
		self.openFileBtn.whenPressed = self.openFile
		self.closeFileBtn = self.add(npyscreen.ButtonPress, name="Close File", editable=False)
		self.closeFileBtn.whenPressed = self.closeFile
		self.nextrely += 1
		
		#Form Objects
		self.tfc = self.add(npyscreen.TitleFilenameCombo, name="Application Binary/Directory: ", value="[Enter] Select file/directory", scroll_exit=True)
		self.findAppBtn = self.add(npyscreen.ButtonPress, name="Add App/Find Apps")
		self.findAppBtn.whenPressed = self.findApps
		
		self.labelApps = self.add_widget(npyscreen.FixedText, value="Applications:", editable=False)
		self.appList = self.add(npyscreen.MultiSelect, values = self.apps, max_height=len(self.apps)+2, scroll_exit=True, editable=False)
		self.appList.add_handlers({curses.ascii.SP: self.appSelected})
		self.appList.add_handlers({curses.ascii.NL: self.appSelected})
		self.nextrely += 1
		
		self.labelOptions = self.add_widget(npyscreen.FixedText, value="Test Options:", editable=False)
		self.appRegBtn = self.add(npyscreen.ButtonPress, name="Register", editable=False)
		self.appRegBtn.whenPressed = self.appReg_json
		self.timestamp = self.add_widget(npyscreen.TitleText, name="Timestamp: (ms)", value="", editable=False)
		self.monRangeText = self.add(npyscreen.FixedText, name="Update Monitor Range:", editable=False)
		self.minSlider = self.add(npyscreen.TitleSlider, name="Min Value", editable=False)
		self.maxSlider = self.add(npyscreen.TitleSlider, name="Max Value", editable=False)
		self.appDeregBtn = self.add(npyscreen.ButtonPress, name="Deregister", editable=False)
		self.appDeregBtn.whenPressed = self.appDereg_json
		self.nextrely += 1
		
		self.labelPager = self.add_widget(npyscreen.FixedText, editable=False, value="Trace Log:")
		self.traceLog = self.add(npyscreen.BufferPager, autowrap=True, name = "Log", max_height=-1, editable=False, scroll_exit=True)
		self.how_exited_handers[npyscreen.wgwidget.EXITED_ESCAPE]  = self.exit_application    
		
		
	def exit_application(self, *args):
		self.parentApp.setNextForm("MAIN")
		self.editing = False
	
	def on_ok(self, *args):
		self.parentApp.setNextForm("MAIN")
		self.editing = False

	def on_cancel(self, *args):
		self.parentApp.setNextForm("MAIN")
		self.editing = False
	
	def log(self, message):
		if(type(message) == PRIMEApp):
			message = message.info
		elif(type(message) == dict):
			message = json.dumps(message)
		elif(type(message) != str):
			message = str(message)
		self.traceLog.values.append(message)
		self.display()
		
	def openFile(self, *args):
		if self.tfc.value == "[Enter] Select a file to write script to.":
			self.tfc.edit()
			return
		filename = self.tfc.value
		
		if not self.fileOpts.value:
			self.fileOpts.edit()
			return
		filemode = self.fileOpts.value
		
		if filemode == 0:
			self.fd = open(filename, 'w')
		else:
			self.fd = open(filename, 'a')

		self.tfc.editable = False
		self.fileOpts.editable = False
		self.openFileBtn.editable = False
		
		self.closeFileBtn.editable = True
		self.appList.editable = True
		
	def closeFile(self, *args):
		self.fd.close()
		self.closeFileBtn.editable = False
		self.appList.editable = False
		
		self.tfc.editable = True
		self.fileOpts.editable = True
		self.openFileBtn.editable = True
	
	def appReg_json(self, *args):
		ts_val = self.timestampText.value
		json_dict = {'ts':ts_val, 'type':'APP_REG'}
		self.fd.write(json.dumps(json_dict, sort_keys=True))
		self.log(json.dumps(json_dict, sort_keys=True))
	
	def appDereg_json(self, *args):
		ts_val = self.timestampText.value
		json_dict = {'ts':ts_val, 'type':'APP_DEREG'}
		self.fd.write(json.dumps(json_dict, sort_keys=True))
		self.log(json.dumps(json_dict, sort_keys=True))
		
				
	def findApps(self, *args):
		if self.tfc.value == "[Enter] Select file/directory":
			self.tfc.edit()
			return
		app_dir = self.tfc.value
		if os.path.isdir(app_dir):
			try:		
				for filename in os.listdir(app_dir):
					if os.path.isfile(app_dir + filename) and \
					os.access(app_dir + filename, os.X_OK) and \
					filename[:4] != "dev_" and \
					filename[:4] != "rtm_": 
						self.apps.append(filename)
						self.appList.values.append(filename)
			except OSError:
				if not os.path.exists(app_dir):
					self.log("Please enter a valid Application directory or filename.")
					self.tfc.edit()
					return
				else:
					raise
		elif os.path.isfile(app_dir):
			if os.access(app_dir, os.X_OK):
				self.apps.append(app_dir)
				self.appList.values.append(app_dir)
	
	def appSelected(self, *args):
		#copy code from demo but don't execute - write json to file
		self.appList.entry_widget.h_select('X')
		self.selectedApp = self.appList.get_selected_objects()[0]
		self.log("Selected " + str(self.selectedApp) + " Application")
		
		if self.selectedApp in self.apps:
			self.log("Known Application Selected")
			self.timestamp.editable = True
			self.minSlider.editable = True
			self.maxSlider.editable = True
			self.appDeregBtn.editable = True
		else:
			self.appRegBtn.editable = True


