import visa
import numpy as np
import time
from matplotlib import pyplot as plt


# Future ideas ---------------------

# List output for sin wave or similar
# inst.write(':SOUR:VOLT:MODE LIST')
# inst.write(':SOUR:LIST:VOLT 0,2,4,6,8,10,0')

# inst.write(':source:voltage:mode list')
# inst.write(':souce:list:voltage {}'.format(','.join(voltages)))

# np.fft.rfft(, norm='ortho')

# ----------------------------------


rm = visa.ResourceManager()
print(rm.list_resources())
inst  = rm.open_resource(rm.list_resources()[0], timeout=20000)

inst.write('*CLS') # Clear
inst.write('*RST') # Reset
inst.write(':system:lfrequency 60')
inst.write(':form real,32')
inst.write(':form:border swap')
inst.values_format.use_binary('d', False, np.array)

channels = [1,2]
Npoints = 10000
Vds = 0.01
Vgs = -15
voltageSetpoints = [None, Vds, Vgs]

for channel in channels:
	inst.write(':source{}:function:mode voltage'.format(channel))
	inst.write(':sense{}:function current'.format(channel))
	# inst.write(':sense{}:current:nplc 1'.format(channel))
	# inst.write(':sense{}:current:prot 10e-6'.format(channel))
	
	inst.write(':sense{}:current:range:auto on'.format(channel))
	inst.write(':sense{}:current:range 1E-6'.format(channel))
	# inst.write(':sense{}:current:range:auto:mode speed'.format(channel))
	# inst.write(':sense{}:current:range:auto:THR 80'.format(channel))
	inst.write(':sense{}:current:range:auto:LLIM 1E-8'.format(channel))
	
	inst.write(':source{}:voltage:mode sweep'.format(channel))

	inst.write(':source{}:voltage:start {}'.format(channel, voltageSetpoints[channel]))
	inst.write(':source{}:voltage:stop {}'.format(channel, voltageSetpoints[channel])) 
	inst.write(':source{}:voltage:points {}'.format(channel, Npoints))
	# inst.write(':source2:voltage:start {}'.format(Vgs))
	# inst.write(':source2:voltage:stop {}'.format(Vgs)) 
	# inst.write(':source2:voltage:points {}'.format(Npoints))
	
	inst.write(':output{} on'.format(channel))

inst.write("*WAI")
time.sleep(0.1)

for vds in np.linspace(0,Vds,30):
	inst.write(':source1:voltage {}'.format(vds))
	time.sleep(0.01)
	inst.query_binary_values(':measure? (@1:2)')

for vgs in np.linspace(0,Vgs,30):
	inst.write(':source2:voltage {}'.format(vgs))
	time.sleep(0.01)
	inst.query_binary_values(':measure? (@1:2)')

for channel in channels:
	sinVoltages = 0.1*np.cos(np.linspace(0,2*np.pi,2500))
	sinVoltages = voltageSetpoints[channel]*np.linspace(1,1,2500)
	
	inst.write(':source{}:voltage:mode list'.format(channel))
	inst.write(':source{}:list:voltage {}'.format(channel, ','.join(map(str, sinVoltages))))

for channel in channels:
	inst.write(':trig{}:source timer'.format(channel))
	inst.write(':trig{}:timer {}'.format(channel, 10e-6))
	
	inst.write(':trig{}:count {}'.format(channel, Npoints))
	# inst.write(':trig{}:ACQ:DEL 1E-4'.format(channel))
	# inst.write(':sense{}:current:APER 1E-4'.format(channel))

	# smu.write(':source{}:voltage:mode sweep'.format(channel))
	# smu.write(':source{}:voltage:start 0'.format(channel, src1start))
	# smu.write(':source{}:voltage:stop 0'.format(channel, src1stop)) 
	# smu.write(':source{}:voltage:points 100'.format(channel, points))
	
	# inst.write(':output{} on'.format(channel))



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


for vgs in np.linspace(Vgs,0,30):
	inst.write(':source2:voltage {}'.format(vgs))
	time.sleep(0.01)
	inst.query_binary_values(':measure? (@1:2)')

for vds in np.linspace(Vds,0,30):
	inst.write(':source1:voltage {}'.format(vds))
	time.sleep(0.01)
	inst.query_binary_values(':measure? (@1:2)')

totalTime = endTime - startTime
print('Total time: {} s'.format(totalTime))
print('Rate: {}'.format(Npoints/totalTime))


times = np.array(times)
totalTime = max(times) - min(times)
print('Total time: {} s'.format(totalTime))
print('Rate: {}'.format(len(times)/totalTime))
print('Rate: {:e}'.format(len(times)/totalTime))


print('Max is:')
print(np.max(currents))

plt.plot(times, currents)
plt.plot(times2, currents2)
plt.xlabel('Time [s]')
plt.ylabel('Current [A]')
plt.show()

exit()

plt.psd(currents, Fs=len(times)/totalTime, NFFT=2**16)
plt.psd(currents2, Fs=len(times)/totalTime, NFFT=2**16)
plt.show()

plt.magnitude_spectrum(currents, Fs=len(times)/totalTime)
plt.magnitude_spectrum(currents2, Fs=len(times)/totalTime)
plt.show()

plt.plot(np.linspace(0, len(times)/totalTime/2, len(times)//2 + 1), np.abs(np.fft.rfft(currents, norm='ortho')))
plt.plot(np.linspace(0, len(times)/totalTime/2, len(times)//2 + 1), np.abs(np.fft.rfft(currents2, norm='ortho')))
plt.show()

plt.semilogy(np.linspace(0, len(times)/totalTime/2, len(times)//2 + 1), np.abs(np.fft.rfft(currents, norm='ortho')))
plt.semilogy(np.linspace(0, len(times)/totalTime/2, len(times)//2 + 1), np.abs(np.fft.rfft(currents2, norm='ortho')))
plt.show()


