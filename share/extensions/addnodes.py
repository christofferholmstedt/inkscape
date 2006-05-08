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
import inkex, cubicsuperpath, simplestyle, copy, math, re, bezmisc

def numsegs(csp):
    return sum([len(p)-1 for p in csp])
def tpoint((x1,y1), (x2,y2), t = 0.5):
    return [x1+t*(x2-x1),y1+t*(y2-y1)]
def cspbezsplit(sp1, sp2, t = 0.5):
    m1=tpoint(sp1[1],sp1[2],t)
    m2=tpoint(sp1[2],sp2[0],t)
    m3=tpoint(sp2[0],sp2[1],t)
    m4=tpoint(m1,m2,t)
    m5=tpoint(m2,m3,t)
    m=tpoint(m4,m5,t)
    return [[sp1[0][:],sp1[1][:],m1], [m4,m,m5], [m3,sp2[1][:],sp2[2][:]]]
def cspbezsplitatlength(sp1, sp2, l = 0.5, tolerance = 0.001):
    bez = (sp1[1][:],sp1[2][:],sp2[0][:],sp2[1][:])
    t = bezmisc.beziertatlength(bez, l, tolerance)
    return cspbezsplit(sp1, sp2, t)
def cspseglength(sp1,sp2, tolerance = 0.001):
    bez = (sp1[1][:],sp1[2][:],sp2[0][:],sp2[1][:])
    return bezmisc.bezierlength(bez, tolerance)    
def csplength(csp):
    total = 0
    lengths = []
    for sp in csp:
        lengths.append([])
        for i in xrange(1,len(sp)):
            l = cspseglength(sp[i-1],sp[i])
            lengths[-1].append(l)
            total += l            
    return lengths, total
def numlengths(csplen):
    retval = 0
    for sp in csplen:
        for l in sp:
            if l > 0:
                retval += 1
    return retval

class SplitIt(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("-m", "--max",
                        action="store", type="float", 
                        dest="max", default=0.0,
                        help="maximum segment length")
    def effect(self):
        for id, node in self.selected.iteritems():
            if node.tagName == 'path':
                d = node.attributes.getNamedItem('d')
                p = cubicsuperpath.parsePath(d.value)
                
                #lens, total = csplength(p)
                #avg = total/numlengths(lens)
                #inkex.debug("average segment length: %s" % avg)

                new = []
                for sub in p:
                    new.append([sub[0][:]])
                    i = 1
                    while i <= len(sub)-1:
                        length = cspseglength(new[-1][-1], sub[i])
                        if length > self.options.max:
                            splits = math.ceil(length/self.options.max)
                            for s in xrange(int(splits),1,-1):
                                new[-1][-1], next, sub[i] = cspbezsplitatlength(new[-1][-1], sub[i], 1.0/s)
                                new[-1].append(next[:])
                        new[-1].append(sub[i])
                        i+=1
                    
                d.value = cubicsuperpath.formatPath(new)

e = SplitIt()
e.affect()
