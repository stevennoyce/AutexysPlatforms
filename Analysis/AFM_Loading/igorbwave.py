#!/usr/bin/env python
#
# Copyright (C) 2012 W. Trevor King <wking@tremily.us>
#
# This file is part of igor.
#
# igor is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# igor is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with igor.  If not, see <http://www.gnu.org/licenses/>.

"IBW -> ASCII conversion"

import pprint

import numpy

from igor.binarywave import load
from igor.script import Script


class WaveScript (Script):
    def _run(self, args):
        wave = load(args.infile)
        numpy.savetxt(
            args.outfile, wave['wave']['wData'], fmt='%g', delimiter='\t')
        self.plot_wave(args, wave)
        if args.verbose > 0:
            wave['wave'].pop('wData')
            pprint.pprint(wave)


infile = open("AFM_Test_Files/EFM_0000.ibw", "r")
s = WaveScript(description=__doc__)
s.run(infile)