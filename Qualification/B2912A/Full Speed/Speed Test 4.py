import visa
import numpy as np
import time
from matplotlib import pyplot as plt

# [:SOURce]:DIGital:EXTernal:TOUTput[:EDGE]:POSi tion
# :TRIGger<:ACQuire|:TRANsient|[:ALL]>:SOURce[:SI GNal]

rm = visa.ResourceManager()
print(rm.list_resources())
inst  = rm.open_resource(rm.list_resources()[0], timeout=10000)

inst.write("*rst") # Reset
inst.write(':system:lfrequency 60')
inst.write(':form real,32')
inst.write(':form:border swap')

inst.values_format.use_binary('d', False, np.array)

inst.write(":source:function:mode voltage")
inst.write(":sense:function current")
# inst.write(":sense:current:nplc 1")
inst.write(":sense:current:prot 10e-6")

Npoints = 10000

inst.write(":sense:current:range:auto on")
inst.write(":sense:current:range 1E-8")
inst.write(":sense:current:range:auto:mode speed")
inst.write(":sense:current:range:auto:THR 80")
inst.write(":sense:current:range:auto:LLIM 1E-8")

inst.write(":trig:source tim")
inst.write(":trig:tim {}".format(1e-5))
inst.write(":trig:count {}".format(Npoints))
# inst.write(":trig:ACQ:DEL 1E-4")
# inst.write(":sense:current:APER 1E-4")

# smu.write(":source:voltage:mode sweep")
# smu.write(":source:voltage:start 0".format(src1start))
# smu.write(":source:voltage:stop 0".format(src1stop)) 
# smu.write(":source:voltage:points 100".format(points))

inst.write(":output on")
inst.write(":init (@1)")

startTime = time.time()

# Retrieve measurement result
currents = inst.query_binary_values(":fetch:array:current? (@1)")
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


