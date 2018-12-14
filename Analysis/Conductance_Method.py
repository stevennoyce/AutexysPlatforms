import numpy as np
import math
import sys
from matplotlib import pyplot as plt

sys.path.insert(0, '../../AutexysHost/source/utilities')

import DataLoggerUtility as dlu


G_p_omega_fn = lambda G, C, omega, C_ox: (omega * G * (C_ox**2)) / (G**2 + (omega**2)*((C_ox - C)**2))

def G_p_omega(data_dict):
	freq_data = data_dict['Freq']
	angular_freq_data = (2 * math.pi) * np.array(freq_data)
	G_data = data_dict['G']
	C_data = data_dict['Cp']
	
	G_p = []
	for i in range(len(angular_freq_data)):
		G_p.append(G_p_omega_fn(G_data[i], C_data[i], angular_freq_data[i], np.mean(C_data[0:50]) ))
	
	# G_p = np.array(G_p) / max(G_p)
	
	return list(G_p)
	

def start():
	
	names = ['/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_1-2(4) ; 12_11_2018 1_33_56 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_3-4(5) ; 12_11_2018 1_39_48 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_3(6) ; 12_11_2018 1_41_07 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_4(1) ; 12_11_2018 1_42_57 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_5(2) ; 12_11_2018 1_44_55 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_6(3) ; 12_11_2018 1_46_09 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_7(4) ; 12_11_2018 1_47_49 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_9-10(1) ; 12_11_2018 1_27_12 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_9(2) ; 12_11_2018 1_30_04 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C127E_10(3) ; 12_11_2018 1_31_36 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [C144W_9-10(1) ; 12_11_2018 1_17_04 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(1) _Only back gate connected, no jumpers on s_d_; 12_11_2018 12_45_30 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(1) ; 12_11_2018 12_31_55 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(2) _Device Not connected, just BNCs and PCB_; 12_11_2018 12_34_17 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(3) ; 12_11_2018 12_36_07 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(8) ; 12_11_2018 1_00_43 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53(6) ; 12_11_2018 12_58_09 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_54(7) ; 12_11_2018 12_59_32 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_bg-lg-sg(4) ; 12_11_2018 12_53_40 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_bg-sg(2) ; 12_11_2018 12_47_13 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_bg-sg(5) ; 12_11_2018 12_55_40 PM].csv','/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_sg(3) ; 12_11_2018 12_50_22 PM].csv']
	
# 	names = [
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(1) ; 12_11_2018 12_31_55 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(3) ; 12_11_2018 12_36_07 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(8) ; 12_11_2018 1_00_43 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53(6) ; 12_11_2018 12_58_09 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_54(7) ; 12_11_2018 12_59_32 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_bg-lg-sg(4) ; 12_11_2018 12_53_40 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_bg-sg(2) ; 12_11_2018 12_47_13 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_bg-sg(5) ; 12_11_2018 12_55_40 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_sg(3) ; 12_11_2018 12_50_22 PM].csv'
# ]
	
# 	names = [
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(1) ; 12_11_2018 12_31_55 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(3) ; 12_11_2018 12_36_07 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(8) ; 12_11_2018 1_00_43 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53(6) ; 12_11_2018 12_58_09 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_54(7) ; 12_11_2018 12_59_32 PM].csv',
# ]
	
# 	names = [
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(1) ; 12_11_2018 12_31_55 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(3) ; 12_11_2018 12_36_07 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53-54(8) ; 12_11_2018 1_00_43 PM].csv',
# ]
	
# 	names = [
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_53(6) ; 12_11_2018 12_58_09 PM].csv',
# '/Volumes/KEYSIGHT/CVf2/Generic C-f [JM3B_54(7) ; 12_11_2018 12_59_32 PM].csv',
# ]
	
	fig, ax = plt.subplots(1,1, figsize=(4,3))
	
	Dits = []
	
	for name in names:
		# === IMPORT DATA ===
		data_dict = dlu.loadCSV('', name, dataNamesLabel='DataName', dataValuesLabel='DataValue')

		freq_data = data_dict['Freq']
		angular_freq_data = (2 * math.pi) * np.array(freq_data)
		G_data = data_dict['G']
		C_data = data_dict['Cp']
		
		# === Calculate ===
		G_p = G_p_omega(data_dict)
		
		ax.plot(freq_data, 10**12 * np.array(G_p)/max(G_p), marker='o', lw=0, ms=1.5)
		ax.set_xlabel('Frequency (Hz)')
		ax.set_ylabel('$ G_{{p}} / \\omega $ (pF)')
		
		ax.set_xscale('log')
		#ax.set_xlim(left=1e4)
		# ax.set_ylim(bottom=-2)
		
		Dit = (2.5/1.6e-19)*max(G_p)
		Dits.append(Dit)
		print('D_it = {:.5e} states/eV*cm^2'.format(Dit))
		
		#print('tau_it = {:.5e} sec'.format(2/angular_freq_data[G_p.index(max(G_p[250:]))]))
	
	fig.tight_layout()
	plt.show()
	
	plt.semilogy(Dits)
	plt.ylabel('$D_{it}$')
	plt.show()
	
	#print(data_dict)
	
	
	





if(__name__ == '__main__'):
	start()