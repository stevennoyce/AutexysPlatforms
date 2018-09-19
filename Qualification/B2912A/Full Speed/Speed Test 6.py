import visa
import numpy as np
import time
from matplotlib import pyplot as plt

rm = visa.ResourceManager()
print(rm.list_resources())
inst  = rm.open_resource(rm.list_resources()[0], timeout=20000)

inst.write('*CLS') # Clear
inst.write('*RST') # Reset
inst.write(':system:lfrequency 60')
inst.write(':form real,32')
inst.write(':form:border swap')
inst.values_format.use_binary('d', False, np.array)

for channel in [1,2]:
	inst.write(':source{}:function:mode voltage'.format(channel))
	inst.write(':sense{}:function current'.format(channel))
	# inst.write(':sense{}:current:nplc 1'.format(channel))
	inst.write(':sense{}:current:prot 10e-6'.format(channel))

	inst.write(':sense{}:current:range:auto on'.format(channel))
	inst.write(':sense{}:current:range 1E-8'.format(channel))
	inst.write(':sense{}:current:range:auto:mode speed'.format(channel))
	inst.write(':sense{}:current:range:auto:THR 80'.format(channel))
	inst.write(':sense{}:current:range:auto:LLIM 1E-8'.format(channel))

	Npoints = 10000
	
	inst.write(':trig{}:source tim'.format(channel))
	inst.write(':trig{}:tim {}'.format(channel, 10e-6))
	inst.write(':trig{}:count {}'.format(channel, Npoints))
	# inst.write(':trig{}:ACQ:DEL 1E-4'.format(channel))
	# inst.write(':sense{}:current:APER 1E-4'.format(channel))

	# smu.write(':source{}:voltage:mode sweep'.format(channel))
	# smu.write(':source{}:voltage:start 0'.format(channel, src1start))
	# smu.write(':source{}:voltage:stop 0'.format(channel, src1stop)) 
	# smu.write(':source{}:voltage:points 100'.format(channel, points))
	
	inst.write(':output{} on'.format(channel))


inst.write(':init (@1,2)')

startTime = time.time()

# Retrieve measurement result
currents = inst.query_binary_values(':fetch:array:current? (@1)')
voltages = inst.query_binary_values(':fetch:array:voltage? (@1)')
times = inst.query_binary_values(':fetch:array:time?')

currents2 = inst.query_binary_values(':fetch:array:current? (@2)')
voltages2 = inst.query_binary_values(':fetch:array:voltage? (@2)')
times2 = inst.query_binary_values(':fetch:array:time? (@2)')

endTime = time.time()


totalTime = endTime - startTime
print('Total time: {} s'.format(totalTime))
print('Rate: {}'.format(Npoints/totalTime))


times = np.array(times)
totalTime = max(times) - min(times)
print('Total time: {} s'.format(totalTime))
print('Rate: {}'.format(len(times)/totalTime))
print('Rate: {:e}'.format(len(times)/totalTime))


plt.plot(times, currents)
plt.plot(times2, currents2)
plt.xlabel('Time [s]')
plt.ylabel('Current [A]')
plt.show()

plt.psd(currents, NFFT=2**16, Fs=len(times)/totalTime)
plt.psd(currents2, NFFT=2**16, Fs=len(times)/totalTime)
plt.show()

