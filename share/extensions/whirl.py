#!/usr/bin/env python 
'''
Copyright (C) 2005 Aaron Spike, aaron@ekips.org

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
'''
import math, inkex, cubicsuperpath

class Whirl(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("-x", "--centerx",
                        action="store", type="float", 
                        dest="centerx", default=10.0,
                        help="")
        self.OptionParser.add_option("-y", "--centery",
                        action="store", type="float", 
                        dest="centery", default=0.0,
                        help="")
        self.OptionParser.add_option("-t", "--whirl",
                        action="store", type="float", 
                        dest="whirl", default=1.0,
                        help="amount of whirl")
        self.OptionParser.add_option("-r", "--rotation",
                        action="store", type="inkbool", 
                        dest="rotation", default=True,
                        help="direction of rotation")
    def effect(self):
        for id, node in self.selected.iteritems():
            rotation = -1
            if self.options.rotation == True:
                rotation = 1
            whirl = self.options.whirl / 1000
            if node.tagName == 'path':
                d = node.attributes.getNamedItem('d')
                p = cubicsuperpath.parsePath(d.value)
                for sub in p:
                    for csp in sub:
                        for point in csp:
                            point[0] -= self.options.centerx
                            point[1] -= self.options.centery
                            dist = math.sqrt((point[0] ** 2) + (point[1] ** 2))
                            if dist != 0:
                                a = rotation * dist * whirl
                                theta = math.atan2(point[1], point[0]) + a
                                point[0] = (dist * math.cos(theta))
                                point[1] = (dist * math.sin(theta))
                            point[0] += self.options.centerx
                            point[1] += self.options.centery
                d.value = cubicsuperpath.formatPath(p)

e = Whirl()
e.affect()
