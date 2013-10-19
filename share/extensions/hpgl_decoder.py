#!/usr/bin/env python 
# coding=utf-8
'''
Copyright (C) 2013 Sebastian Wüst, sebi@timewaster.de, http://www.timewasters-place.com/
This importer supports HP-GL commands only.

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

# standard library
from StringIO import StringIO
import math
# local library
import inkex


class hpglDecoder:
    def __init__(self, hpglString, options):
        ''' options:
                "resolutionX":float
                "resolutionY":float
                "showMovements":bool
        '''
        self.hpglString = hpglString
        self.options = options
        self.scaleX = options.resolutionX / 90.0 # dots/inch to dots/pixels
        self.scaleY = options.resolutionY / 90.0 # dots/inch to dots/pixels
        self.warnings = []

    def getSvg(self): # parse hpgl data
        # prepare document
        self.doc = inkex.etree.parse(StringIO('<svg xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" width="%s" height="%s"></svg>' % (self.options.docWidth, self.options.docHeight)))
        actualLayer = 0;
        self.layers = {}
        if self.options.showMovements:
        	self.layers[0] = inkex.etree.SubElement(self.doc.getroot(), 'g', {inkex.addNS('groupmode','inkscape'):'layer', inkex.addNS('label','inkscape'):'Movements'})
        # parse paths
        hpglData = self.hpglString.split(';')
        if len(hpglData) < 3:
            raise Exception('NO_HPGL_DATA')
        oldCoordinates = (0.0, self.options.docHeight) 
        path = ''
        for i, command in enumerate(hpglData):
            if command.strip() != '':
                if command[:2] == 'IN': # if Initialize command, ignore
                    pass
                elif command[:2] == 'SP': # if Select Pen command
                    actualLayer = command[2:]
                    self.createLayer(actualLayer)
                elif command[:2] == 'PU': # if Pen Up command
                    if ' L ' in path:
                        self.addPathToLayer(path, actualLayer)
                    if self.options.showMovements and i != len(hpglData) - 1:
                        path = 'M %f,%f' % oldCoordinates
                        path += ' L %f,%f' % self.getParameters(command[2:])
                        self.addPathToLayer(path, 0)
                    path = 'M %f,%f' % self.getParameters(command[2:])
                    oldCoordinates = self.getParameters(command[2:])
                elif command[:2] == 'PD': # if Pen Down command
                    parameterString = command[2:]
                    if parameterString.strip() != '':
                        parameterString = parameterString.replace(';', '').strip()
                        parameter = parameterString.split(',')
                        oldCoordinates = (float(parameter[-2]) / self.scaleX, self.options.docHeight - float(parameter[-1]) / self.scaleX)
                        for i, param in enumerate(parameter):
                            if i % 2 == 0:
                                parameter[i] = str(float(param) / self.scaleX)
                            else:
                                parameter[i] = str(self.options.docHeight - float(param) / self.scaleY)
                        parameterString = ','.join(parameter)
                        path += ' L %s' % parameterString
                else:
                    self.warnings.append('UNKNOWN_COMMANDS')
        if ' L ' in path:
            self.addPathToLayer(path, actualLayer)
        return (self.doc, self.warnings)

    def createLayer(self, layerNumber):
        self.layers[layerNumber] = inkex.etree.SubElement(self.doc.getroot(), 'g', {inkex.addNS('groupmode','inkscape'):'layer', inkex.addNS('label','inkscape'):'Drawing Pen ' + layerNumber})

    def addPathToLayer(self, path, layerNumber):
        if layerNumber == 0:
            lineColor = 'ff0000'
        else:
            lineColor = '000000'
        inkex.etree.SubElement(self.layers[layerNumber], 'path', {'d':path, 'style':'stroke:#' + lineColor + '; stroke-width:0.4; fill:none;'})

    def getParameters(self, parameterString): # process coordinates
        if parameterString.strip() == '':
            return []
        # remove command delimiter
        parameterString = parameterString.replace(';', '').strip()
        # split parameter
        parameter = parameterString.split(',')
        return (float(parameter[0]) / self.scaleX, self.options.docHeight - float(parameter[1]) / self.scaleY) # convert to svg coordinate system

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99