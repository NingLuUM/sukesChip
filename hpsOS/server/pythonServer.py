

import socket
import struct
import time
import select
import numpy as np
import matplotlib.pyplot as plt
#~ from dataServerParamConfig import *
#~ import sysv_ipc
import os

SAVE_SINGLE = 0
REAL_TIME = 1
STORE_ON_ARM_TRANSFER_TO_ME = 2
STORE_ON_ARM_SAVE_ON_ARM = 3

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

	def queryDataLocal16(self):
		msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(self.npulses*self.recLen*16,socket.MSG_WAITALL)
		print len(bb)
		
		cmsg5 = '{}{}'.format(8*self.recLen,'H')
		cc = np.array(struct.unpack(cmsg5,bb)).astype(np.uint16)
		dd = cc[1::8].astype(np.int16)
		print 'min:',np.min(dd),'\nmax:',np.max(dd),'\nmean:',np.mean(dd),'\np-to-p:',np.max(dd)-np.min(dd)
		plt.plot(dd)
		plt.show()
		
	def queryDataLocal(self):
		msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(self.npulses*self.recLen*12,socket.MSG_WAITALL)
		
		#~ self.disconnectFromFpga()
		print len(bb)
		c1mask = 0xfff
		c2mask = 0xfff000
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'I')
		cmsg5 = ''
		for n in range(0,self.npulses):
			cmsg5 = '{}{}'.format(cmsg5,self.cmsg4)
		
		cc = np.array(struct.unpack(cmsg5,bb)).astype(np.uint64)
		#~ cc &= c1mask
		cc = (cc & c2mask)>>12

		dd = np.zeros(self.npulses*self.recLen)
		for n in range(0,self.npulses):
			dd[n*self.recLen:(n+1)*self.recLen] = cc[n*2*self.recLen:(n*2*self.recLen+self.recLen)]
		print 'min:',np.min(dd),'\nmax:',np.max(dd),'\nmean:',np.mean(dd),'\np-to-p:',np.max(dd)-np.min(dd)
		plt.plot(dd.astype(np.int64)-2048)
		plt.show()
		
	def queryData(self):
		if(self.queryMode == REAL_TIME or self.queryMode == STORE_ON_ARM_TRANSFER_TO_ME):
			if(self.is16bit):
				self.queryDataLocal16()
			else:
				self.queryDataLocal()
		
		else:
			msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
			self.sock.send(msg)
			bb = self.sock.recv(4,socket.MSG_WAITALL)
			
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
	
	def resetToDefaultAdcSettings(self):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_DEFAULT_SETTINGS,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setQueryMode(self,queryMode=REAL_TIME,npulses=0,is16bit=0):#_(self):
		
		self.queryMode = queryMode
		if(is16bit and (queryMode == REAL_TIME)):
			self.is16bit = 1
		else:
			self.is16bit = 0
			
		if queryMode == SAVE_SINGLE:
			msg = struct.pack(self.cmsg,self.CASE_SET_QUERY_MODE,SAVE_SINGLE,self.is16bit,0,0,0,0,0,0,0)
			self.sock.send(msg)
		elif queryMode == REAL_TIME:
			msg = struct.pack(self.cmsg,self.CASE_SET_QUERY_MODE,REAL_TIME,self.is16bit,0,0,0,0,0,0,0)
			self.sock.send(msg)
		elif queryMode == STORE_ON_ARM_TRANSFER_TO_ME:
			msg = struct.pack(self.cmsg,self.CASE_SET_QUERY_MODE,STORE_ON_ARM_TRANSFER_TO_ME,self.is16bit,0,0,0,0,0,0,0)
			self.sock.send(msg)
			self.setupLocalStorage(npulses)
		elif queryMode == STORE_ON_ARM_SAVE_ON_ARM:
			msg = struct.pack(self.cmsg,self.CASE_SET_QUERY_MODE,STORE_ON_ARM_SAVE_ON_ARM,self.is16bit,0,0,0,0,0,0,0)
			self.sock.send(msg)
			self.setupLocalStorage(npulses)
		else:
			self.queryMode = REAL_TIME
			msg = struct.pack(self.cmsg,self.CASE_SET_QUERY_MODE,REAL_TIME,self.is16bit,0,0,0,0,0,0,0)
			self.sock.send(msg)
	
	def setAutoShutdown(self,asd=0):
		msg = struct.pack(self.cmsg,self.CASE_UPDATE_AUTO_SHUTDOWN_SETTING,asd,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def connectToFpga(self):
		self.sock.connect(('192.168.1.101',3400))
	
	def disconnectFromFpga(self):
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
		self.CASE_ADC_SET_DEFAULT_SETTINGS = 13
		self.CASE_SET_QUERY_MODE = 14
		self.CASE_UPDATE_AUTO_SHUTDOWN_SETTING = 15
		self.CASE_EXIT_PROGRAM = 100
		self.npulses = 1
		self.recLen = 2048
		self.queryMode = REAL_TIME
		self.is16bit = 0
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		
		self.cmsg = '10I'
		self.cmsg2 = '1I1f8I'
		self.cmsg3 = '2I1d6I'
		self.cmsg4 = '2048Q2048I'
		
		''' character meanings in cmsg: 
		#	(https://docs.python.org/2/library/struct.html)
		#	'i','I' = signed,unsigned int (4 bytes)
		#	'q','Q' = signed,unsigned long long (8bytes)
		#	'f' = float (4bytes)
		#	'd' = double (8bytes)
		'''
		

r = receiver()
r.connectToFpga()
r.setAutoShutdown(0)
r.setPioVarGain(0)
r.setRecLen(500)
r.setAdcGain(0)
r.setAdcInternalAcCoupling(1)
r.setAdcLowNoiseMode(1)
r.setQueryMode(REAL_TIME,0,1)
#~ r.setQueryMode(STORE_ON_ARM_TRANSFER_TO_ME,npulses=10)
#~ r.setQueryMode(STORE_ON_ARM_SAVE_ON_ARM,npulses=10)

r.queryData()
#~ r.disconnectFromFpga()


#~ r.closeProgram()






		

	



