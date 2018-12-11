import numpy as np
import math

from matplotlib import pyplot as plt

import DataLoggerUtility as dlu


G_p_omega_fn = lambda G, C, omega, C_ox: (omega * G * (C_ox**2)) / (G**2 + (omega**2)*((C_ox - C)**2))

def G_p_omega(data_dict):
	freq_data = data_dict['Freq']
	angular_freq_data = (2 * math.pi) * np.array(freq_data)
	G_data = data_dict['G']
	C_data = data_dict['Cp']
	
	G_p = []
	for i in range(len(angular_freq_data)):
		G_p.append(G_p_omega_fn(G_data[i], C_data[i], angular_freq_data[i], 1e-12))
	
	return G_p
	

def start():
	
	# === IMPORT DATA ===
	data_dict = dlu.loadCSV('', 'test.csv', dataNamesLabel='DataName', dataValuesLabel='DataValue')

	freq_data = data_dict['Freq']
	angular_freq_data = (2 * math.pi) * np.array(freq_data)
	G_data = data_dict['G']
	C_data = data_dict['Cp']
	
	# === Calculate ===
	G_p = G_p_omega(data_dict)
	
	fig, ax = plt.subplots(1,1, figsize=(4,3))
	ax.plot(freq_data, 10**12 * np.array(G_p), marker='o', lw=0, ms=1.5)
	ax.set_xlabel('Frequency (Hz)')
	ax.set_ylabel('$ G_{{p}} / \\omega $ (pF)')
	
	ax.set_xscale('log')
	#ax.set_xlim(left=1e4)
	ax.set_ylim(bottom=-2)
	
	Dit = (2.5/1.6e-19)*max(G_p[250:])
	print('D_it = {:.5e} states/eV*cm^2'.format(Dit))
	
	print('tau_it = {:.5e} sec'.format(2/angular_freq_data[G_p.index(max(G_p[250:]))]))
	
	fig.tight_layout()
	plt.show()
	
	#print(data_dict)
	
	
	





if(__name__ == '__main__'):
	start()