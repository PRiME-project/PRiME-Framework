#!/usr/bin/env python
# encoding: utf-8
############################################################################
"""
logClasses.py

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

Created by Charles Leech on 2017-07-27.

"""
############################################################################

class PRIMEKnob:
	def __init__(self, id, type, min, max, init):
		self.id = id
		self.type = type
		self.min = min
		self.max = max
		self.init = init

class PRIMEMonitor:
	def __init__(self, id, type, min, max, weight):
		self.id = id
		self.type = type
		self.min = min
		self.max = max
		self.weight = weight

############################################################################
# Application Class Info Container
############################################################################
class PRIMEApp:
	def __init__(self, pid):
		self.pid = pid
		self.knobs = {}
		self.mons = {}

############################################################################
# Application Class Info Container
############################################################################
class PRIMEDev:
	def __init__(self):
		self.knobs = {}
		self.mons = {}
