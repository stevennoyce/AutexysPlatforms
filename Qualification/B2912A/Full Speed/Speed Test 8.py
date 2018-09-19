import visa
import numpy as np
import time
from matplotlib import pyplot as plt

rm = visa.ResourceManager()
print(rm.list_resources())
inst  = rm.open_resource(rm.list_resources()[0], timeout=20000)

inst.write("*RST") # Reset
inst.write(':system:lfrequency 60')

inst.write(':sense1:curr:range:auto ON')
inst.write(':sense1:curr:range:auto:llim 1e-8')

inst.write(':sense2:curr:range:auto ON')
inst.write(':sense2:curr:range:auto:llim 1e-8')

inst.write(":source1:function:mode voltage")
inst.write(":source2:function:mode voltage")

inst.write(":source1:voltage 1")
inst.write(":source2:voltage 1")

inst.write(":sense1:curr:nplc {}".format(1))
inst.write(":sense2:curr:nplc {}".format(1))

inst.write(":outp1 ON")
inst.write(":outp2 ON")

inst.write("*WAI") # Explicitly wait for all of these commands to finish before handling new commands


inst.write(":sense1:curr:prot {}".format(10e-6))
inst.write(":sense2:curr:prot {}".format(10e-6))


inst.write(":source1:voltage:mode sweep")
inst.write(":source2:voltage:mode sweep")

inst.write(":source1:voltage:start {}".format(1))
inst.write(":source1:voltage:stop {}".format(1)) 
inst.write(":source1:voltage:points {}".format(100))
inst.write(":source2:voltage:start {}".format(1))
inst.write(":source2:voltage:stop {}".format(1)) 
inst.write(":source2:voltage:points {}".format(100))

inst.write(":trig1:source tim")
inst.write(':trig1:tim {}'.format(10e-6))
inst.write(":trig1:count {}".format(100))
inst.write(":trig2:source tim")
inst.write(':trig2:tim {}'.format(10e-6))
inst.write(":trig2:count {}".format(100))
inst.write(":init (@1:2)")

current1s = inst.query_ascii_values(":fetch:arr:curr? (@1)")
voltage1s = inst.query_ascii_values(":fetch:arr:voltage? (@1)")
current2s = inst.query_ascii_values(":fetch:arr:curr? (@2)")
voltage2s = inst.query_ascii_values(":fetch:arr:voltage? (@2)")

print(np.mean(current1s))
print(np.mean(current2s))


