#!/usr/bin/env python
# -*- coding: cp1252 -*-
"""
eqtexsvg.py
functions for converting LaTeX equation string into SVG path
This extension need, to work properly:
    - a TeX/LaTeX distribution (MiKTeX ...)
    - pstoedit software: <http://www.pstoedit.net/pstoedit>

Copyright (C) 2006 Julien Vitard <julienvitard@gmail.com>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

"""

import inkex, os, tempfile, sys, xml.dom.minidom

def create_equation_tex(filename, equation):
    tex = open(filename, 'w')
    tex.write("""%% processed with eqtexsvg.py
\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amssymb}
\\usepackage{amsfonts}

\\thispagestyle{empty}
\\begin{document}
""")
    tex.write(equation)
    tex.write("\n\\end{document}\n")
    tex.close()

def svg_open(self,filename):
    doc_width = inkex.unittouu(self.document.documentElement.getAttribute('width'))
    doc_height = inkex.unittouu(self.document.documentElement.getAttribute('height'))
    doc_sizeH = min(doc_width,doc_height)
    doc_sizeW = max(doc_width,doc_height)

    def clone_and_rewrite(self, node_in):
        if node_in.localName != 'svg':
            node_out = self.document.createElement('svg:' + node_in.localName)
            for i in range(0, node_in.attributes.length):
                name = node_in.attributes.item(i).name
                value = node_in.attributes.item(i).value
                node_out.setAttribute(name, value)
        else:
            node_out = self.document.createElement('svg:g')
        for c in node_in.childNodes:
            if c.localName in ('g', 'path', 'polyline', 'polygon'):
                child = clone_and_rewrite(self, c)
                if c.localName == 'g':
                    child.setAttribute('transform','matrix('+str(doc_sizeH/700.)+',0,0,'+str(-doc_sizeH/700.)+','+str(-doc_sizeH*0.25)+','+str(doc_sizeW*0.75)+')')
                node_out.appendChild(child)

        return node_out

    doc = xml.dom.minidom.parse(filename)
    svg = doc.getElementsByTagName('svg')[0]
    group = clone_and_rewrite(self, svg)
    self.current_layer.appendChild(group)

class EQTEXSVG(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)
        self.OptionParser.add_option("-f", "--formule",
                        action="store", type="string",
                        dest="formula", default=10.0,
                        help="LaTeX formula")
    def effect(self):

        base_dir = tempfile.mkdtemp("", "inkscape-");
        latex_file = os.path.join(base_dir, "eq.tex")
        aux_file = os.path.join(base_dir, "eq.aux")
        log_file = os.path.join(base_dir, "eq.log")
        ps_file = os.path.join(base_dir, "eq.ps")
        dvi_file = os.path.join(base_dir, "eq.dvi")
        svg_file = os.path.join(base_dir, "eq.svg")
        out_file = os.path.join(base_dir, "eq.out")

        def clean():
            os.remove(latex_file)
            os.remove(aux_file)
            os.remove(log_file)
            os.remove(ps_file)
            os.remove(dvi_file)
            os.remove(svg_file)
            os.remove(out_file)
            os.rmdir(base_dir)

        create_equation_tex(latex_file, self.options.formula)
        #os.system('cd ' + base_dir)
        os.system('latex -output-directory=' + base_dir + ' -halt-on-error ' + latex_file + ' > ' + out_file)
	try:
	    os.stat(dvi_file)
	except OSError:
	    print >>sys.stderr, "invalid LaTeX input:"
            print >>sys.stderr, self.options.formula
            print >>sys.stderr, "temporary files were left in:", base_dir
            sys.exit(1)

        os.system('dvips -q -f -E -D 600 -y 5000 -o ' + ps_file + ' ' + dvi_file)
        #os.system('cd ' + base_dir)
        os.system('pstoedit -f plot-svg -dt -ssp ' + ps_file + ' ' + svg_file + '> ' + out_file)
        svg_open(self, svg_file)

        clean()

e = EQTEXSVG()
e.affect()
