#!/usr/bin/env python
# encoding: utf-8
############################################################################
"""
uiPRIMEApp.py

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

class PRIMEMonitor:
	def __init__(self, id, type, min, max, weight):
		self.id = id
		self.type = type
		self.min = min
		self.max = max
#		self.val = val
		self.weight = weight

############################################################################
# Application Class Info Container
############################################################################
class PRIMEApp:
	def __init__(self, name, UIID, abspath, arguments, address, handle, started, registered):
		self.name = name
		self.UIID = UIID
		self.abspath = abspath
		self.arguments = "" + str(arguments)
		self.address = address
		self.handle = handle
		self.started = started
		self.registered = registered
		self.monitors = {}
		self.weight = 0.0
	
	def info(self):
		return "name: " + self.name + "UIID: " + self.UIID + ", abspath: \"" + self.abspath + \
					"\", arguments: \"" + self.arguments + "\", address: \"" + self.address + \
					"\", handle: \"" + self.handle + "\", registered: " + str(self.registered) + \
					", weight: " + str(self.weight)

