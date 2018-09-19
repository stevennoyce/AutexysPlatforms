import visa
import numpy as np
import time
from matplotlib import pyplot as plt

# [:SOURce]:DIGital:EXTernal:TOUTput[:EDGE]:POSi tion
# :TRIGger<:ACQuire|:TRANsient|[:ALL]>:SOURce[:SI GNal]

rm = visa.ResourceManager()
print(rm.list_resources())
inst  = rm.open_resource(rm.list_resources()[0], timeout=10000)

inst.write("*RST") # Reset
inst.write(':system:lfrequency 60')
inst.write(':form real,32')
inst.write(':form:bord swap')

inst.values_format.use_binary('d', False, np.array)

inst.write(":source:function:mode voltage")
inst.write(":sense:function curr")
# inst.write(":sense:curr:nplc 1")
inst.write(":sense:curr:prot 10e-6")

Npoints = 10000

inst.write(":SENS:CURR:RANG:AUTO ON")
inst.write(":SENS:CURR:RANG 1E-8")
inst.write(":SENS:CURR:RANG:AUTO:MODE SPE")
inst.write(":SENS:CURR:RANG:AUTO:THR 80")
inst.write(":SENS:CURR:RANG:AUTO:LLIM 1E-8")

inst.write(":TRIG:SOUR TIM")
inst.write(":TRIG:TIM {}".format(1e-5))
inst.write(":trig:count {}".format(Npoints))
# inst.write(":TRIG:ACQ:DEL 1E-4")
# inst.write(":SENS:CURR:APER 1E-4")

# smu.write(":source:voltage:mode sweep")
# smu.write(":source:voltage:start 0".format(src1start))
# smu.write(":source:voltage:stop 0".format(src1stop)) 
# smu.write(":source:voltage:points 100".format(points))

inst.write(":output on")
inst.write(":init (@1)")

startTime = time.time()

# Retrieve measurement result
currents = inst.query_binary_values(":fetch:array:curr? (@1)")
voltages = inst.query_binary_values(":fetch:array:voltage? (@1)")
times = inst.query_binary_values(":fetch:array:time?")

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
plt.xlabel('Time [s]')
plt.ylabel('Current [A]')
plt.show()

plt.psd(currents, NFFT=2**16, Fs=len(times)/totalTime)
plt.show()


