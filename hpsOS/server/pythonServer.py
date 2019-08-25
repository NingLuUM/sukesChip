

import socket
import struct
import time
import select
import numpy as np
import matplotlib.pyplot as plt
#~ from dataServerParamConfig import *
#~ import sysv_ipc
import os


class receiver():
	
	def setRecLen(self,recLen):
		self.recLen = recLen
		self.cmsg4 = '{}{}{}{}'.format(recLen,'q',recLen,'I')
		msg = struct.pack(self.cmsg,self.CASE_SET_RECLEN,recLen,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setPioVarGain(self,varGain):
		msg = struct.pack(self.cmsg,self.CASE_SET_PIO_VAR_GAIN,varGain,0,0,0,0,0,0,0,0)
		self.sock.send(msg)

	def setLeds(self,ledVal):
		msg = struct.pack(self.cmsg,self.CASE_SET_LEDS,ledVal,0,0,0,0,0,0,0,0)
		self.sock.send(msg)

	def queryData(self):
		msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(self.recLen*12,socket.MSG_WAITALL)
		print len(bb)
		c1mask = 0xfff
		cc = np.array(struct.unpack(self.cmsg4,bb)).astype(np.uint64)
		cc &= c1mask
		
		plt.plot(cc[0:(self.recLen-1)].astype(np.int64)-2048)
		plt.show()
	
	def queryDataLocal(self):
		msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(self.npulses*self.recLen*12,socket.MSG_WAITALL)
		print len(bb)
		c1mask = 0xfff
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'I')
		cmsg5 = ''
		for n in range(0,self.npulses):
			cmsg5 = '{}{}'.format(cmsg5,self.cmsg4)
		
		cc = np.array(struct.unpack(cmsg5,bb)).astype(np.uint64)
		cc &= c1mask
		
		plt.plot(cc.astype(np.int64)-2048)
		plt.show()
		
	def setAdcGain(self,adcGain):
		msg = struct.pack(self.cmsg3,self.CASE_ADC_SET_GAIN,0,adcGain,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setAdcUnsigned(self,uns):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_UNSIGNED,uns,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setAdcLowNoiseMode(self,lnm):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_LOW_NOISE_MODE,lnm,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def toggleAdcChannelPower(self,chpwr):
		msg = struct.pack(self.cmsg,self.CASE_ADC_TOGGLE_CHANNEL_POWER,chpwr,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def setAdcFilterBw(self,filterbw):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_FILTER_BW,filterbw,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def setAdcInternalAcCoupling(self,accoupling):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_INTERNAL_AC_COUPLING,accoupling,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def issueDirectAdcCmd(self,gp_tgc,addr,cmd):
		msg = struct.pack(self.cmsg,self.CASE_ADC_ISSUE_DIRECT_CMD,gp_tgc,addr,cmd,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def connectInterrupt(self):
		msg = struct.pack(self.cmsg,self.CASE_CONNECT_INTERRUPT,1,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def disconnectInterrupt(self):
		msg = struct.pack(self.cmsg,self.CASE_CONNECT_INTERRUPT,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def setupLocalStorage(self,npulses=0):
		if npulses == 0:
			self.npulses = 1
			msg = struct.pack(self.cmsg,self.CASE_SETUP_LOCAL_STORAGE,1,0,0,0,0,0,0,0,0)
		else:
			self.npulses = npulses
			msg = struct.pack(self.cmsg,self.CASE_SETUP_LOCAL_STORAGE,npulses,0,0,0,0,0,0,0,0)
		
		self.sock.send(msg)
			
	def connectEnetFpga(self):
		self.sock.connect(('192.168.1.101',3400))
	
	def disconnectEnetFpga(self):
		self.sock.close()
		
	def closeProgram(self):
		msg = struct.pack(self.cmsg,self.CASE_EXIT_PROGRAM,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		self.sock.close()
			
	def __init__(self):
		self.CASE_SET_RECLEN = 0
		self.CASE_SET_PIO_VAR_GAIN = 1
		self.CASE_SET_LEDS = 2
		self.CASE_QUERY_DATA = 3
		self.CASE_ADC_SET_GAIN = 4
		self.CASE_ADC_SET_UNSIGNED = 5
		self.CASE_ADC_SET_LOW_NOISE_MODE = 6
		self.CASE_ADC_TOGGLE_CHANNEL_POWER = 7
		self.CASE_ADC_SET_FILTER_BW = 8
		self.CASE_ADC_SET_INTERNAL_AC_COUPLING = 9
		self.CASE_ADC_ISSUE_DIRECT_CMD = 10
		self.CASE_CONNECT_INTERRUPT = 11
		self.CASE_SETUP_LOCAL_STORAGE = 12
		self.CASE_EXIT_PROGRAM = 100
		self.npulses = 1
		self.recLen = 2048
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self.cmsg = '10I'
		self.cmsg2 = '1I1f8I'
		self.cmsg3 = '2I1d6I'
		self.cmsg4 = '2048q2048I'
		

r = receiver()

r.connectEnetFpga()
r.setRecLen(500)
r.setAdcInternalAcCoupling(1)
r.connectInterrupt()
r.setAdcGain(1.375)
r.setAdcUnsigned(1)
r.setupLocalStorage(10)
time.sleep(0.5)
#~ r.queryData()
r.queryDataLocal()
#~ r.disconnectInterrupt()
time.sleep(0.5)
r.closeProgram()
#~ r.disconnectEnetFpga()
#~ s.connect(('192.168.1.101',3400))

#~ msg = struct.pack(cmsg3,4,0,12.765,1,2,3,4,5,6)
#~ s.send(msg)

#~ s.close()

		

	



