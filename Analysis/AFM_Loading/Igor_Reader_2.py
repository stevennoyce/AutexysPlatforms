import numpy as np
import igor.binarywave as igorbinary

# pip install igor

# Gwyddion can be used to view .ibw AFM files
# We want to be able to plot similar images
# We also want to get any available data/experiment parameters
#	- Tip Voltage
#	- Scan size (X and Y dimensions in microns)
#	- Nap Height

# The instrument that generates these files is an Asylum MFP-3D AFM
# There could be some documentation on the file type

# The software that generates the files is based on IGOR, a plotting software


def loadAFM(path):
	data = igorbinary.load(path)
	note = data['wave']['note']
	
	noteData = {}
	
	for line in note.splitlines():
		line = line.replace(b' \xb0C', b'')
		line = line.replace(b'\xb0', b'')
		line = str(line, encoding='ascii')
		
		key, colon, value = line.partition(':')
		
		value = value.strip()
		
		try:
			value = float(value)
		except ValueError:
			if ',' in value:
				array = []
				for number in value.split(','):
					try:
						array.append(float(number))
					except ValueError:
						pass
				value = array
		
		noteData[key] = value
		
	data['wave']['note'] = noteData
	
	return data


def getAFMTimestamp(path):
	importantNoteNames = ['Date','Time','Seconds','ImageFrameTime','NapHeight','labels','NapTipVoltage','NapSurfaceVoltage','ScanSize','FastScanSize','SlowScanSize','ScanRate','ScanPoints','ScanLines','ScanAngle','NapMode','Channel1DataType','Channel2DataType','Channel3DataType','Channel4DataType','Channel5DataType','Channel6DataType','Channel7DataType','Channel8DataType','Chip','ScanSpeed','IntegralGain','ProportionalGain','AmplitudeSetpointVolts','AmplitudeSetpointMeters','DriveAmplitude','DriveFrequency','SlowScanEnabled','ScanDown','StartHeadTemp','StartScannerTemp','EndHeadTemp','EndScannerTemp','TipVoltage','SurfaceVoltage','User0Voltage','User1Voltage','FreeAirAmplitude','FreeAirPhase','NapIntegralGain','NapProportionalGain','NapAmplitudeSetpointVolts','NapDriveAmplitude','NapDriveFrequency','NapStartHeight','NapTime']
	
	afm = loadAFM(path)
	
	importantNotes = {name: afm['wave']['note'][name] for name in importantNoteNames}
	print(importantNotes)
	
# 'Date': '2018-09-26'
# 'Time': '12:05:27 PM'
# 'Seconds': 3620808327.836
# 'ImageFrameTime': 64.0
# 'NapHeight': 1e-08
# 'labels': [[], [], [b'', b'HeightRetrace', b'AmplitudeRetrace', b'NapAmplitudeRetrace', b'PhaseRetracee', b'NapPhaseRetrace', b'ZSensorRetrace'], []]
# 'NapTipVoltage': 0.0
# 'NapSurfaceVoltage': 0.0
# 'ScanSize': 1e-05
# 'FastScanSize': 1e-05
# 'SlowScanSize': 1e-05
# 'ScanRate': 0.5
# 'ScanPoints': 32.0
# 'ScanLines': 32.0
# 'ScanAngle': 90.0
# 'NapMode': 1.0
# 'Channel1DataType': 'Height'
# 'Channel2DataType': 'Amplitude'
# 'Channel3DataType': 'Phase'
# 'Channel4DataType': 'ZSensor'
# 'Channel5DataType': 'None'
# 'Channel6DataType': 'None'
# 'Channel7DataType': 'None'
# 'Channel8DataType': 'None'
# 'Chip': 'OutC'
# 'ScanSpeed': 1.25e-05
# 'IntegralGain': 8.0
# 'ProportionalGain': 0.0
# 'AmplitudeSetpointVolts': 0.3
# 'AmplitudeSetpointMeters': 1e-08
# 'DriveAmplitude': 0.41308
# 'DriveFrequency': 60218.0
# 'SlowScanEnabled': 1.0
# 'ScanDown': 0.0
# 'StartHeadTemp': 33.312
# 'StartScannerTemp': 27.438
# 'EndHeadTemp': 33.3125
# 'EndScannerTemp': 27.4375
# 'TipVoltage': 0.0
# 'SurfaceVoltage': 0.0
# 'User0Voltage': 0.0
# 'User1Voltage': 0.0
# 'FreeAirAmplitude': 0.96426
# 'FreeAirPhase': 68.901
# 'NapIntegralGain': 10.0
# 'NapProportionalGain': 0.0
# 'NapAmplitudeSetpointVolts': 0.8
# 'NapDriveAmplitude': 0.01
# 'NapDriveFrequency': 60309.0
# 'NapStartHeight': 0.0
# 'NapTime': 0.10016



if __name__ == '__main__':
	print(loadAFM('AFM_Test_Files/SGM0000.ibw'))
	# print(getAFMTimestamp('AFM_Test_Files/SGM0000.ibw'))


