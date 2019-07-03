from dataServer import *

import numpy as np
import time
import subprocess
import os
import matplotlib.pyplot as plt


COMMS_ONLY_STATE = 0
ENTER_TX_STATE = np.uint32(1<<7)

ONCHIP_TO_DDR3 = np.uint32( (1<<6) | (1<<5) )
DDR3_TO_ONCHIP = np.uint32( (1<<6) | (1<<5) | (1<<4) )

ENABLE_OUTPUT = np.uint32(1<<1)
TRIGGER_ADC = np.uint32(1<<25)
END_TX_PROGRAM = np.uint32(1<<31)
RUN_TX_PROGRAM = ( ENTER_TX_STATE | ENABLE_OUTPUT )

# these correspond to the arduino header pins. will probably need to be changed after lvds pins re-configd
PIN0 = (1<<0)
PIN1 = (1<<1)
PIN2 = (1<<2)
PIN3 = (1<<3)
PIN4 = (1<<4)
PIN5 = (1<<5)
PIN6 = (1<<6)
PIN7 = (1<<7)
PIN8 = (1<<8)
PIN9 = (1<<9)
PIN10 = (1<<10)
PIN11 = (1<<11)
PIN12 = (1<<12)
PIN13 = (1<<13)

RCV_TRIG = PIN0
ADC_CH = [0]*8

ADC_CH[0] = PIN1
ADC_CH[1] = PIN2
ADC_CH[2] = PIN5
ADC_CH[3] = PIN6
ADC_CH[4] = PIN7
ADC_CH[5] = PIN8
ADC_CH[6] = PIN3
ADC_CH[7] = PIN4

ALL_TRIGS_OFF = 0

def getDataServer():

	#~ subprocess.call(['gnome-terminal', '-x', './a.out'])
	#~ subprocess.check_output("bash getIPs_sshIntoFPGAs.sh", shell=True)

	d = dataServer()
	d.connect()
	d.queryBoardInfo()
	print 'N boards = ', d.boardCount
	time.sleep(0.1)
	return d


def setupLoopers():
	
	d = getDataServer()
	d.zeroAllTxRegs()
	
	d.setTxChannelMask( 0xff )
	d.disableTransducerSafety( 0xff )	
	d.txControlComms( COMMS_ONLY_STATE )
	
	N1=21
	twidth = 0.05
	twidth2= 0.05
	
	d.setTxOutputControlReg(1,0)
	d.setTxTimingReg(1,0.1)
	d.setTxOutputControlReg(2,TRIGGER_ADC)
	d.setTxTimingReg(2,1)
	for n in range(3,N1):
		d.setTxOutputControlReg(n,0xffff*(n%2))
		if (n%2):
			print 0xffff*(n%2),twidth	
			d.setTxTimingReg(n,twidth)
		else:
			print 0xffff*(n%2),twidth2	
			d.setTxTimingReg(n,twidth2)
		time.sleep(1e-4)
			
	d.setTxTimingReg(N1,0.1)
	d.setTxLoopControl(0,3,N1,8,8)
	d.setTxOutputControlReg(N1+2,END_TX_PROGRAM)
	
	d.txControlComms( RUN_TX_PROGRAM )	
	time.sleep(9.3)		

			
	d.txControlComms( COMMS_ONLY_STATE )
	
	
def interrupter():
	d = getDataServer()
	d.setQueryDataTimeout(500000)
	d.setTrigDelay(100)
	d.setRecLen(4096,1024)
	for n in range(0,1000):
		d.interruptThyself()
		time.sleep(1e-3)
		d.uninterruptThyself()

#~ interrupter()
	
#~ setupLoopers()

def setupLoopersWRecv():
	TRIGDELAY, RECLEN, PACKETSIZE = 0, 1028*4, 1028*4
	NPULSES=1
	d = getDataServer()
	
	#
	d.setQueryDataTimeout(500000)
	d.setTrigDelay(TRIGDELAY)
	d.setRecLen(RECLEN,PACKETSIZE)
	d.setInterleaveDepthAndTimer(4,400)
	d.setDataArraySize(1,1,NPULSES)
	d.allocateDataArrayMemory()
	
	d.zeroAllTxRegs()
	sfreq = 25
	
	d.enableLVDS(0)
	time.sleep(0.1)
	d.enableLVDS(1)
	time.sleep(0.1)

	#~ d.lvdsDirectInstruction(0x04,0x0008)
	#~ time.sleep(0.1)
	
	d.initializeLVDS()
	time.sleep(0.1)
	
	d.setLvdsGain(20)   
	time.sleep(0.1)

	
	d.setTxChannelMask( 0xff )
	d.disableTransducerSafety( 0xff )	
	d.txControlComms( COMMS_ONLY_STATE )
	
	N1=21
	twidth = 0.1
	twidth2= 0.1
	
	d.setTxOutputControlReg(1,0)
	d.setTxTimingReg(1,0.1)
	d.setTxOutputControlReg(2,TRIGGER_ADC)
	d.setTxTimingReg(2,0.1)
	#~ for n in range(3,N1):
		#~ d.setTxOutputControlReg(n,0xfffe*(n%2))
		#~ if (n%2):
			#~ print 0xffff*(n%2),twidth	
			#~ d.setTxTimingReg(n,twidth)
		#~ else:
			#~ print 0xffff*(n%2),twidth2	
			#~ d.setTxTimingReg(n,twidth2)
		#~ time.sleep(1e-4)
			
	#~ d.setTxTimingReg(N1,5)
	#~ d.setTxLoopControl(0,3,N1,58,58)
	d.setTxOutputControlReg(4,END_TX_PROGRAM)
	
	d.toggleDataAcq(1)
	
	d.txControlComms( RUN_TX_PROGRAM )
	
	d.queryData(1)	
	time.sleep(1e-4)		
	d.ipcWait(2)
			
	d.txControlComms( COMMS_ONLY_STATE )
	d.saveDataLogically("dummy.csv")
	
	data = np.loadtxt("dummy.csv",delimiter=",").astype(np.int32)
	data2 = np.zeros(data.shape).astype(np.int32)

	for mm in range(1,len(data[:,0])):
		data2[mm,0] = ( data[mm,0]>>0) 

	fig=plt.figure(figsize=(20,12))
	ax = fig.add_subplot(211)
	ax2 = fig.add_subplot(212)
	for n in range(0,NPULSES):
		t = np.linspace(0,d.recLen/(1.0*sfreq),d.recLen)
		#~ for m in range(0,8):
		for m in range(0,1):
			#~ ax.plot(t,1.0*data[:,m]+0*m*2000)
			ax.plot(1.0*data2[:,m]+0*m*2000)
		#~ plt.plot(t,1.0*data[:,2]+2000)
		#~ plt.ylim([0,9])
		ax2.plot(t[0:-1],(data[1:,m]-data[0:-1,m]))
		plt.show()
	
	print(data.max())


	
	
setupLoopersWRecv()


def akshayDataProgram():
	d = getDataServer()
	
	TRIGDELAY, RECLEN, PACKETSIZE, NPULSES = 0, 8191, 4096, 1
	
	# setup the receive system
	d.setQueryDataTimeout(5000000)
	d.setTrigDelay(TRIGDELAY)
	d.setRecLen(RECLEN,PACKETSIZE)
	d.setInterleaveDepthAndTimer(4,400)
	d.setDataArraySize(1,1,NPULSES)
	d.allocateDataArrayMemory()
	
	# reset all the output control registers on the transmit system
	d.zeroAllTxRegs()
	
	# normally, output can only remain high for 5us to protect the transducer elements
	# by default the system shuts the output of the elements off once they've been on for 5us
	# this disables that 
	d.disableTransducerSafety( 0xff )
	
	# puts tx system into state where only communications are enabled	
	d.txControlComms( COMMS_ONLY_STATE )

	# the first value in each of these functions corresponds 
	# to the address in memory where the command is stored
	d.setTxOutputControlReg(1,TRIGGER_ADC) # upload command to trigger the ADC to record data
	d.setTxTimingReg(1,1) # set the time at which data acquisition begins after the trigger is received
	d.setTxOutputControlReg(2,END_TX_PROGRAM) # upload command to end tx program
	
	# turn on the receive system
	d.toggleDataAcq(1)
	
	# does what it says
	d.txControlComms( RUN_TX_PROGRAM )
	
	# put receive system in state to check for data
	d.queryData(1)			
	
	# wait for the receive system to send data to computer
	d.ipcWait(2)
	
	# put tx system back into comms state		
	d.txControlComms( COMMS_ONLY_STATE )
	
	# save the acquired data to a csv file
	d.saveDataLogically("dummy.csv")
	
	# load and plot the data
	data = np.loadtxt("dummy.csv",delimiter=",")
	for n in range(0,NPULSES):
		t = np.linspace(0,d.recLen/25.0,d.recLen)
		for m in range(0,8):
			plt.plot(t,1.0*data[:,m]+200.0*m)
		plt.show()
			
	










































