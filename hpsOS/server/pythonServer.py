

import socket
import struct
import time
#~ import select
import numpy as np
import matplotlib.pyplot as plt

import os


class receiver():
	
	def setRecLen(self,recLen,allocate=0):
		if recLen < self.MAX_RECLEN:
			self.recLen = recLen
		else:
			self.recLen = 2048
			print 'max recLen = 16384'
			
		if allocate > 0:
			self.allocated = 0
		else:
			self.allocated = 1
			
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'I')
		msg = struct.pack(self.cmsg,self.CASE_SET_RECLEN,self.recLen,self.allocated,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setRecDuration(self,duration):
		recLen = int(duration*self.ADC_CLK/(self.clockDiv+1.0))
		if recLen < self.MAX_RECLEN:
			self.recLen = recLen
		else:
			self.recLen = 2048
			print 'max duration = ', np.round(self.MAX_RECLEN/self.ADC_CLK,2)
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'I')
		msg = struct.pack(self.cmsg,self.CASE_SET_RECLEN,self.recLen,0,0,0,0,0,0,0,0)
		self.sock.send(msg)	
		
	def setPioVarAtten(self,varGain):
		msg = struct.pack(self.cmsg,self.CASE_SET_PIO_VAR_GAIN, ( varGain & 0x3 ) ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setClockDivisor(self,clockDiv):
		self.clockDiv = clockDiv-1
		msg = struct.pack(self.cmsg,self.CASE_SET_CLOCK_DIVISOR, ( np.uint32(clockDiv-1) & 0xf ) ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)

	def setSamplingMode(self,samplingMode):
		# 0 -> sample every Nth
		# 1 -> average of pulses [N:(N+clockDivisor-1)]
		self.samplingMode = samplingMode
		msg = struct.pack(self.cmsg,self.CASE_SET_RCV_SAMPLING_MODE, self.samplingMode ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setCompressorMode(self,compressorMode):
		# 0 -> no compression (16bit)
		# 1 -> compressed (12bit)
		self.compressorMode = compressorMode
		msg = struct.pack(self.cmsg,self.CASE_SET_RCV_COMPRESSOR_MODE, self.compressorMode ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)

	def setLeds(self,ledVal):
		msg = struct.pack(self.cmsg,self.CASE_SET_LEDS, ( ledVal & 0x1f ) ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)


	def plotterSetup(self,ylims = [0,0], xlims = [0,0], figheight = 0, figwidth = 0, nrows = 0, ncols = 0):
		if ylims[0] != ylims[1]:
			self.ylims = ylims
		if xlims[0] != xlims[1]:
			self.xlims[0:2] = xlims
			self.xlims[2] = 1
		
		if figheight > 0:
			self.figheight = figheight
		if figwidth > 0:
			self.figwidth = figwidth

		if nrows > 0:
			self.nrows = nrows
		if ncols > 0:
			self.ncols = ncols
			
	def plotDatas(self,datas):
		
		print 'npulses',self.npulses
		
		if self.samplingMode:
			clockDiv = (self.clockDiv+1.0)
		else:
			clockDiv = 1
			
		t = np.linspace(0,self.recLen/self.ADC_CLK*clockDiv,self.recLen)
		
			
		fig,ax = plt.subplots(self.nrows,self.ncols,sharey=True,sharex=True,figsize=(self.figwidth, self.figheight))
		
		if self.nrows==1 and self.ncols==1:
			ax.set_ylim((self.ylims[0],self.ylims[1]))
			for n in range(0,8):
				ax.plot(t+n*t[-1],datas[:,n])
			ax.grid(linestyle=':')
		else:
			n = 0
			
			for nc in range(0,self.ncols):
				for nr in range(0,self.nrows):	
					for npls in range(0,self.npulses):
						ax[nr,nc].plot(t+npls*t[-1],datas[npls*self.recLen:(npls+1)*self.recLen,n]/clockDiv,'-o')
						pk2pk = (np.max(datas[npls*self.recLen:(npls+1)*self.recLen,n])-np.min(datas[npls*self.recLen:(npls+1)*self.recLen,n]))/clockDiv
					ax[nr,nc].set_title('{}{}{}{}'.format('Ch[',n,'], pk-pk ',pk2pk))
					ax[nr,nc].set_ylim((self.ylims[0],self.ylims[1]))
					if self.xlims[2] > 0:
						ax[nr,nc].set_xlim((self.xlims[0],self.xlims[1]))
					else:
						ax[nr,nc].set_xlim((-(t[-1]*self.npulses)*.02,1.02*(t[-1]*self.npulses)))
					ax[nr,nc].grid(linestyle=':')
					ax[nr,nc].tick_params(axis='both', which='major', labelsize=15)
					
					if nr == (self.nrows-1):
						ax[nr,nc].set_xlabel('Time (us)',fontsize=15)
					n+=1
					if n>7:
						break
				if n>7:
					break
				
		plt.tight_layout()
		plt.show()
		
	def queryDataLocal16(self,pltr):
		msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		aa = 0
		bb=''
		while len(bb)<(self.npulses*self.recLen*16):
			bb += self.dsock.recv(self.npulses*self.recLen*16,0)
			if len(bb)-aa:
				self.dsock.send(struct.pack('I',len(bb)-aa))
			aa = len(bb)
		print 'len(bb):',len(bb)
		
		self.cmsg5 = '{}{}'.format(8*self.recLen*self.npulses,'H')
		self.cmsg6 = '{}{}'.format(4*self.recLen*self.npulses,'I')
		
		if self.compressorMode == 0:
			cc = np.array(struct.unpack(self.cmsg5,bb)).astype(np.uint16)
			dd = np.zeros((self.npulses*self.recLen,8))
			print 'dd:',dd.shape,'cc:',cc.shape
			for n in range(0,8):
				dd[:,n] = cc[n::8]&0xffff
		else:
			cc = np.array(struct.unpack(self.cmsg6,bb)).astype(np.uint32)
			dd = np.zeros((self.npulses*self.recLen*4,8)).astype(np.uint16)
			ch = 0
			print 'dd:',dd.shape,'cc:',cc.shape
			r = 0
			n=0

			for ccn in cc:
				if (n%3) == 0:	
					dd[r,7] = (( ( ccn & 0xfff00000 ) >> 20 ))
					dd[r,6] = (( ( ccn & 0x000fff00 ) >> 8 ))
					dd[r,5] = (( ( ccn & 0x000000ff ) << 4 ))
				elif (n%3) == 1:
					dd[r,5] |= (( ( ccn & 0xf0000000 ) >> 28 ) )
					dd[r,4] = (( ( ccn & 0x0fff0000 ) >> 16 ))
					dd[r,3] = (( ( ccn & 0x0000fff0 ) >> 4 ))
					dd[r,2] = (( ( ccn & 0x0000000f ) << 8 ))
				else:
					dd[r,2] |= (( ( ccn & 0xff000000 ) >> 24 ))
					dd[r,1] = (( ( ccn & 0x00fff000 ) >> 12))
					dd[r,0] = (0xfff & ccn)
					r+=1
					
				
				n+=1	
				#~ if r==dd.shape[0]:
					#~ print 'breaker'
					#~ break
			print 'cntr:', r	
		
		
		if pltr:	
			self.plotDatas(dd)
		
	def queryDataLocal(self):
		msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		aa = 0
		bb=''
		while len(bb)<(self.npulses*self.recLen*12):
			bb += self.dsock.recv(self.npulses*self.recLen*12,0)
			if len(bb)-aa:
				self.dsock.send(struct.pack('I',len(bb)-aa))
			aa = len(bb)
		
		
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'i')
		cmsg5 = ''
		for n in range(0,self.npulses):
			cmsg5 = '{}{}'.format(cmsg5,self.cmsg4)
		
		cc = np.array(struct.unpack(cmsg5,bb)).astype(np.uint64)
		c0 = cc & self.c0mask
		c1 = (cc & self.c1mask)>>12
		c2 = (cc & self.c2mask)>>24
		c3 = (cc & self.c3mask)>>36
		c4 = (cc & self.c4mask)>>48
		c5 = ((cc[0:self.recLen] & self.c5mask1)>>60) | ((cc[self.recLen:2*self.recLen]&self.c5mask2)<<4)
		c6 = (cc & self.c6mask)>>8
		c7 = (cc & self.c7mask)>>20
		
		dd = np.zeros((self.npulses*self.recLen,8))
		for n in range(0,self.npulses):
			dd[n*self.recLen:(n+1)*self.recLen,0] = c0[n*2*self.recLen:(n*2*self.recLen+self.recLen)]
			dd[n*self.recLen:(n+1)*self.recLen,1] = c1[n*2*self.recLen:(n*2*self.recLen+self.recLen)]
			dd[n*self.recLen:(n+1)*self.recLen,2] = c2[n*2*self.recLen:(n*2*self.recLen+self.recLen)]
			dd[n*self.recLen:(n+1)*self.recLen,3] = c3[n*2*self.recLen:(n*2*self.recLen+self.recLen)]
			dd[n*self.recLen:(n+1)*self.recLen,4] = c4[n*2*self.recLen:(n*2*self.recLen+self.recLen)]
			dd[n*self.recLen:(n+1)*self.recLen,5] = c5
			dd[n*self.recLen:(n+1)*self.recLen,6] = c6[self.recLen:2*self.recLen]
			dd[n*self.recLen:(n+1)*self.recLen,7] = c7[self.recLen:2*self.recLen]
		
		self.plotDatas(dd)
			
	def queryData(self,pltr=1):
		if self.allocated == 0:
			self.setupLocalStorage()
			
		if(self.realTime or self.transferData):
			
			if(self.is16bit):
				print 'helper'
				self.queryDataLocal16(pltr)
			else:
				#~ print 'helper'
				self.queryDataLocal()
		
		else:
			msg = struct.pack(self.cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
			self.sock.send(msg)
			#~ print 'issue query cmd'
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
		#0b00: 14MHz. 0b01: 10MHz. 0b10: 7.5MHz. 0b11: Not used.
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_FILTER_BW,filterbw,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def setAdcInternalAcCoupling(self,accoupling):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_INTERNAL_AC_COUPLING,accoupling,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def issueDirectAdcCmd(self,gp_tgc,addr,cmd):
		msg = struct.pack(self.cmsg,self.CASE_ADC_ISSUE_DIRECT_CMD,gp_tgc,addr,cmd,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def setRamp(self):
		self.issueDirectAdcCmd(0,2,(111<<13))
		
	def resetToDefaultAdcSettings(self):
		msg = struct.pack(self.cmsg,self.CASE_ADC_SET_DEFAULT_SETTINGS,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
	
	
	def connectInterrupt(self):
		msg = struct.pack(self.cmsg,self.CASE_CONNECT_INTERRUPT,1,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def disconnectInterrupt(self):
		msg = struct.pack(self.cmsg,self.CASE_CONNECT_INTERRUPT,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
	
	def setupLocalStorage(self):
		msg = struct.pack(self.cmsg,self.CASE_SETUP_LOCAL_STORAGE,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		self.allocated = 1
	
	def setNPulses(self,npulses,allocate=0):
		if npulses == 0:
			self.npulses = 1
		else:
			self.npulses = npulses
		
		if allocate > 0:
			self.allocated = 0
		else:
			self.allocated = 1
			
		msg = struct.pack(self.cmsg,self.CASE_SET_NPULSES,self.npulses,self.allocated,0,0,0,0,0,0,0)	
		self.sock.send(msg)
				
	def setQueryMode(self,realTime=1, transferData=1, saveData=0, is16bit=1):
		
		self.realTime = realTime
		self.transferData = transferData
		self.saveData = saveData
		self.is16bit = is16bit
		
		msg = struct.pack(self.cmsg,self.CASE_SET_QUERY_MODE,self.realTime,self.transferData,self.saveData,self.is16bit,0,0,0,0,0)
		self.sock.send(msg)
		
	def setChargeTime_us(self,ct1,ct2,fireDelay=0):
		
		msg = struct.pack(self.cmsg,self.CASE_TX_SET_CHARGETIME,np.uint32(ct1*100),np.uint32(ct2*100),(np.uint32(fireDelay*100)>>1),0,0,0,0,0,0)
		self.sock.send(msg)
		
	def fireTx(self):
		msg = struct.pack(self.cmsg,self.CASE_TX_FIRE,1,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setFclkDelay(self,fclk):
		# fclk_delay -> 0-5
		# bit_lag -> 0-1
		#~ fclk = fclk_delay/2
		bclk = 0#fclk_delay%2
		msg = struct.pack(self.cmsg,self.CASE_TX_SET_FCLOCK_DELAY,fclk,bclk,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def setAutoShutdown(self,asd=0):
		msg = struct.pack(self.cmsg,self.CASE_UPDATE_AUTO_SHUTDOWN_SETTING,asd,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		
	def connectToFpga(self):
		self.sock.connect(('192.168.1.101',3400))
		self.dsock.connect(('192.168.1.101',3500))
	
	def disconnectFromFpga(self):
		self.sock.close()
		
	def closeProgram(self):
		msg = struct.pack(self.cmsg,self.CASE_EXIT_PROGRAM,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		self.sock.close()
			
	def __init__(self):
		# defined variables in the C code running on arm
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
		self.CASE_SET_NPULSES = 16
		self.CASE_SET_CLOCK_DIVISOR = 17
		self.CASE_SET_RCV_SAMPLING_MODE = 18
		self.CASE_SET_RCV_COMPRESSOR_MODE = 19
		self.CASE_TX_SET_CHARGETIME = 50
		self.CASE_TX_FIRE = 51
		self.CASE_TX_SET_FCLOCK_DELAY = 52
		self.CASE_EXIT_PROGRAM = 100
		
		# it is what it says
		self.ADC_CLK = 50.0
		
		self.MAX_RECLEN = 16384
		
		# ethernet sockets for transferring data to/from arm
		# 'sock' is used to send data to arm
		# 'dsock' receives data from arm
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self.dsock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		
		self.allocated = 0
		
		# for allocating storage on arm/setting up enet transfer
		self.npulses = 1
		self.recLen = 2048
		
		# mode settings for receiving data
		self.realTime = 1
		self.transferData = 0
		self.saveData = 0
		self.is16bit = 1
		self.clockDiv = 0
		self.samplingMode = 1
		self.compressorMode = 0

		# variables to control size of plotted data
		self.ylims = [-200,4300]
		self.xlims = [-100,-1,0]
		self.figwidth = 30
		self.figheight = 18
		self.nrows = 4
		self.ncols = 2
		
		# bitwise masks to convert recv'd data if 'is16bit'==0
		self.c0mask = 0xfff
		self.c1mask = 0xfff000
		self.c2mask = 0xfff000000
		self.c3mask = 0xfff000000000
		self.c4mask = 0xfff000000000000
		self.c5mask1 = 0xf000000000000000
		self.c5mask2 = 0xff
		self.c6mask = 0xfff00
		self.c7mask = 0xfff00000
		
		# for for converting enet msg's into c-readable form
		self.cmsg = '10I'
		self.cmsg2 = '1I1f8I'
		self.cmsg3 = '2I1d6I'
		self.cmsg4 = '2048Q2048I'
		self.cmsg5 = '{}{}'.format(8*self.recLen*self.npulses,'H')
		self.cmsg6 = '{}{}'.format(4*self.recLen*self.npulses,'I')
		''' character meanings in cmsg: 
		#	(https://docs.python.org/2/library/struct.html)
		#	'i','I' = signed,unsigned int (4 bytes)
		#	'q','Q' = signed,unsigned long long (8bytes)
		#	'f' = float (4bytes)
		#	'd' = double (8bytes)
		'''
		

r = receiver()
r.connectToFpga()
r.resetToDefaultAdcSettings()
r.setAutoShutdown(0)

r.toggleAdcChannelPower(0b10011111)

r.setClockDivisor(1)
r.setSamplingMode(1)
r.setCompressorMode(1)

r.setRecLen(1000)
#~ r.setRecDuration(20.00) # us (max = 327.68)

r.setAdcGain(0)
r.setPioVarAtten(0)

r.setChargeTime_us(0.10,0.10,000)

#~ r.setNPulses(1)
#~ r.setFclkDelay(1) # accepts values 0-5
#~ r.setRecLen(17000) # npoints (max = 16384)
#~ r.setAdcLowNoiseMode(0)
#~ r.setAdcFilterBw(0)
#~ r.setAdcInternalAcCoupling(1)
#~ r.setAdcUnsigned(0)
#~ r.setRamp()

r.setQueryMode(realTime=0,transferData=1,saveData=0)
#~ r.plotterSetup(figheight = 15, figwidth = 10, nrows = 4, ncols = 2)
#~ r.plotterSetup(ylims = [-200,4300], xlims = [-100,2600], figheight = 10, figwidth = 30, nrows = 4, ncols = 2)


for n in range(0,1):
	r.fireTx()
	r.queryData(pltr=0)
	time.sleep(.15)
	
r.fireTx()
r.queryData(pltr=1)
#~ r.disconnectFromFpga()


#~ r.closeProgram()





		

	



