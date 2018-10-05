from matplotlib import pyplot as plt
from neo import io

# Gwyddion can be used to view .ibw AFM files
# We want to be able to plot similar images
# We also want to get any available data/experiment parameters
#	- Tip Voltage
#	- Scan size (X and Y dimensions in microns)
#	- Nap Height

# The instrument that generates these files is an Asylum MFP-3D AFM
# There could be some documentation on the file type

# The software that generates the files is based on IGOR, a plotting software

file = io.IgorIO(filename='AFM_Test_Files/EFM_0002.ibw')
seg1 = file.read_segment()

plt.plot(np.array(seg1.analogsignals[0])[...,0]); plt.show()

# I got this simple demo working with the neo package
# The neo package uses/requires the igor package
# It would be preferable to use the igor package directly, but we can stick with neo if necessary


