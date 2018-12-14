from matplotlib import pyplot as plt
import numpy as np
from neo import io
from neo.core import AnalogSignal
import quantities as pq
import igor.binarywave as igorbinary
# neo depends on igor
# pip install igor
# pip install neo

# Gwyddion can be used to view .ibw AFM files
# We want to be able to plot similar images
# We also want to get any available data/experiment parameters
#	- Tip Voltage
#	- Scan size (X and Y dimensions in microns)
#	- Nap Height

# The instrument that generates these files is an Asylum MFP-3D AFM
# There could be some documentation on the file type

# The software that generates the files is based on IGOR, a plotting software

def main_method(fn):
	# file = io.IgorIO(filename=fn)
	# seg1 = file.read_segment()

	# # https://media.readthedocs.org/pdf/neo/latest/neo.pdf  : Page 10
	# # above shows all components of segment
	# print("analogsignals: " , seg1.analogsignals)
	# print("epochs: ",seg1.epochs)
	# print("events: " , seg1.events)
	# print("irregularlysampledsignals: ", seg1.irregularlysampledsignals)
	# print("spiketrains: ", seg1.spiketrains)

	data = igorbinary.load(fn)
	print(data)

	#obtain_analogsignal(seg1)


'''
	def: 		obtain_analogsignal
	param: 		Segment seg
	method: 	obtains analogsignal data from Segment object, plots based on channels
	notes:		if subplots are to be used, fullscreening the resulting image works best
'''
def obtain_analogsignal(seg):

	# obtain analogsignal array
	analogsignal = seg.analogsignals[0]

	# constants provided by neo.core.AnalogSignal
	# https://media.readthedocs.org/pdf/neo/latest/neo.pdf  : Page 44-45
	units = analogsignal.units
	sampling_rate = analogsignal.sampling_rate
	t_start = analogsignal.t_start
	t_stop = analogsignal.t_stop 
	name = analogsignal.name
	description = analogsignal.description
	times = analogsignal.times

	#print(units, sampling_rate, t_start, t_stop, name, description)
	channel_count = len(analogsignal[0][0])

	# I figured it would help to have all the plots in one figure, but 
	# if that's not the case, individual plots can be created (nc)
	for channel in range(channel_count):
		# space out subplots to make them more readable
		subplot_channel = channel + int(channel / 2)*2
		plt.subplot(channel_count, 2, subplot_channel+1)
		plt.ticklabel_format(style='sci', axis='x', scilimits=(0,0))
		plt.ticklabel_format(style='sci', axis='y', scilimits=(0,0))
		plt.plot(np.array(times), np.array(analogsignal)[..., channel])
		plt.xlabel("time (s)")
		plt.ylabel("signal (" + str(units) + ")")
		title_str = "Channel #" + str(channel+1);
		plt.title(title_str)

	sup_title_str = "AnalogSignals Plot for " + name + " Dataset, Channel Count: "+ str(channel_count);
	plt.suptitle(sup_title_str)
	plt.show()


main_method('AFM_Test_Files/SGM0000.ibw')


# I got this simple demo working with the neo package
# The neo package uses/requires the igor package
# It would be preferable to use the igor package directly, but we can stick with neo if necessary


# Here are a few links I have found that are somewhat helpful
#	- https://media.readthedocs.org/pdf/neo/latest/neo.pdf
#	- https://github.com/wking/igor/tree/master/igor
#	- https://github.com/reflectometry/igor.py/blob/master/igor.py

# Note that the igor.py package is different from the igor package
# igor is based on igor.py, but has a different interface


