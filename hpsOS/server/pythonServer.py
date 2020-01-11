

import socket
import struct
import time
#~ import select
import numpy as np

import matplotlib.pyplot as plt
import subprocess
import os
import signal

class transmitter():
	
	def executeProgram(self):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_CONTROL_STATE,1,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		self.pd_sock.close()
		
	def terminateProgram(self):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_CONTROL_STATE,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setTrigRestLvls(self,rstLvls = 0):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_TRIG_REST_LVLS,rstLvls,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def setVarAttenRestLvl(self,rstLvl=0):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_VAR_ATTEN_REST_LVL,rstLvl,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
			
	def setActiveTransducers(self,activeTrans = 0):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_ACTIVE_TRANSDUCERS,activeTrans,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def beginSyncCmd(self):
		self.inSyncCmd = 1
		msg = struct.pack(self.u32cmsg,self.CASE_TX_MAKE_PIO_CMD,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def endSyncCmd(self):
		self.inSyncCmd = 0
		msg = struct.pack(self.u32cmsg,self.CASE_TX_END_PIO_CMD,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def startCounter(self,cntrId=0,startIdx=0,endIdx=1,stepSize=1):
		print 'start counter'
		tmp_cmsg = '2I4Q' ### [ u32 , u32 , u64 , u64 , u64 , u64 ]
		msg = struct.pack(tmp_cmsg,self.CASE_TX_MAKE_COUNTER_START,cntrId,startIdx,endIdx,stepSize,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def endCounter(self,cntrID=0):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_MAKE_COUNTER_END,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def startNamedLoop(self,loopId=0,startVal=0,endVal=1,stepSize=1,units=1e-3):
		if units < 0:
			units = 1e-3
		elif units == 'mm':
			units = 1e-3
		elif units == 'cm':
			units = 1e-2
		elif units == 'm':
			units = 1.0
		else:
			units = 1e-3
			
		tmp_cmsg = '2I4d' ### [ u32 , u32 , double , double , double , double ]
		msg = struct.pack(tmp_cmsg,self.CASE_TX_MAKE_LOOP_START,loopId,startVal,endVal,stepSize,units)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def endNamedLoop(self,loopID=0):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_MAKE_LOOP_END,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setTrig(self,trigN=0,duration_us=0):
		duration = np.uint32(duration_us*100)
		msg = struct.pack(self.u32cmsg,self.CASE_TX_BUFFER_TRIG_TIMINGS,trigN,duration,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def toggleVarAtten(self,duration_us=0):
		duration = np.uint32(duration_us*100)
		msg = struct.pack(self.u32cmsg,self.CASE_TX_BUFFER_VAR_ATTEN_TIMINGS,duration,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setChargeTime(self,chargeTime_us=0):
		chargeTime = np.uint32(chargeTime_us*100)
		msg = struct.pack(self.u32cmsg,self.CASE_TX_BUFFER_CHARGE_TIME,chargeTime,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setMask(self,mask_val=0):
		msg = struct.pack(self.u32cmsg,self.CASE_TX_BUFFER_TMP_MASK_CMD,np.uint32(mask_val),0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setPhaseDelays(self,pd = np.zeros(8)):
		tmp_cmsg = '1I8H1I2Q' ### [ u32 , u16 , u16 , u16 , u16 , u16 , u16 , u16 , u16 , u32 , u64 , u64 ]
		pd = (pd_us*100).astype(np.uint16)	
		msg = struct.pack(tmp_cmsg,self.CASE_TX_BUFFER_PHASE_DELAYS,pd[0],pd[1],pd[2],pd[3],pd[4],pd[5],pd[6],pd[7],0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def fire(self):		
		msg = struct.pack(self.u32cmsg,self.CASE_TX_BUFFER_FIRE_CMD,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def setPhaseAtLoc(self,xloc=0,yloc=0,zloc=0,units=1e-3):
		if units < 0:
			units = 1e-3
		elif units == 'mm':
			units = 1e-3
		elif units == 'cm':
			units = 1e-2
		elif units == 'm':
			units = 1.0
		else:
			units = 1e-3
			
		tmp_cmsg = '2I4d'	### [ u32 , u32 , double , double , double , double ]
		msg = struct.pack(tmp_cmsg,self.CASE_TX_CALC_AND_SET_PHASE_AT_SPECIFIED_COORD_VALS,0,xloc*units,yloc*units,zloc*units,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def fireAtLoc(self,xloc=0,yloc=0,zloc=0):
		self.setPhaseAtLoc(xloc,yloc,zloc)
		self.fire()
	
	def setPhaseFromCounterIdx(self,counterID=0):		
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_PHASE_FROM_LOOP_IDX_AS_MEM_IDX,counterID,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def fireAtCounterIdx(self,counterID=0):
		self.setPhaseFromCounterIdx(counterID)
		self.fire()
		
	def setPhaseFromNamedLoops(self,xID=0,yID=0,zID=0):		
		msg = struct.pack(self.u32cmsg,self.CASE_TX_CALC_AND_SET_PHASE_FROM_LOOP_IDXS_AS_COORD_VALS,xID,yID,zID,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
			
	def fireAtNamedLoopCoords(self,xID=0,yID=0,zID=0):		
		self.setPhaseFromNamedLoops(xID,yID,zID)
		self.fire()
			
	def rcvData(self):		
		msg = struct.pack(self.u32cmsg,self.CASE_TX_BUFFER_RECV_TRIG,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def async_wait_usec(self,waitVal_usec=0):
		if waitVal_usec < 0:
			waitVal_usec = 0
			print 'illegal value in async_wait_usec. must be >= 0. setting to 0'
			
		tmp_cmsg = '2I4Q'	### [ u32 , u32 , u64 , u64 , u64 , u64 ]
		wv = np.uint64(waitVal_usec*100)
		msg = struct.pack(tmp_cmsg,self.CASE_TX_BUFFER_ASYNC_WAIT,0,wv,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def async_wait_sec(self,waitVal_sec=0):
		if waitVal_sec < 0:
			waitVal_sec = 0
			print 'illegal value in async_wait_sec. must be >= 0. setting to 0'
			
		tmp_cmsg = '2I4Q'	### [ u32 , u32 , u64 , u64 , u64 , u64 ]
		wv = np.uint64(waitVal_sec*1e8)
		msg = struct.pack(tmp_cmsg,self.CASE_TX_BUFFER_ASYNC_WAIT,0,wv,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def wait_usec(self,waitVal_usec=0):
		if waitVal_usec < 0:
			waitVal_usec = 0
			print 'illegal value in wait_usec. must be >= 0. setting to 0'
			
		if self.inSyncCmd:
			wv = np.uint32(waitVal_usec*100)
			msg = struct.pack(self.u32cmsg,self.CASE_TX_WAIT_CMD,wv,0,0,0,0,0,0,0,0)
			self.sock.send(msg)	
			bb = self.sock.recv(4,socket.MSG_WAITALL)
		else:
			self.async_wait_usec(waitVal_usec)
			
	def wait_sec(self,waitVal_sec=0):
		if waitVal_sec < 0:
			waitVal_sec = 0
			print 'illegal value in wait_sec. must be >= 0. setting to 0'
			
		if self.inSyncCmd:
			wv = np.uint32(waitVal_sec*100000000)
			msg = struct.pack(self.u32cmsg,self.CASE_TX_WAIT_CMD,wv,0,0,0,0,0,0,0,0)
			self.sock.send(msg)	
			bb = self.sock.recv(4,socket.MSG_WAITALL)
		
		else:
			self.async_wait_sec(waitVal_sec)
		
	def at_usec(self,atusecVal=0):
		if atusecVal < 0:
			atusecVal = 0
			print 'illegal value in at_usec. must be >= 0. setting to 0'
			
		if self.inSyncCmd:
			usecval = np.uint32(atusecVal*100)
			msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_SYNC_CMD_TIME_VAL,usecval,0,0,0,0,0,0,0,0)
			self.sock.send(msg)	
			bb = self.sock.recv(4,socket.MSG_WAITALL)
		else:
			print "WARNING: outside of synchronous command blocks (i.e. between begin/endSyncCmd s) all commands have an associated execution time and can ONLY be executed sequentially. as a result, outside of a synchronous command block 'at_usec' cannot be used to set the time at which a command is executed to an earlier value. outside of a synchronous command block 'at_usec' is interpreted as a 'wait' statement and replaced by 'async_wait_usec'."
			self.async_wait_usec(atusecVal)
			
	def at_sec(self,atsecVal=0):
		if atsecVal < 0:
			atsecVal = 0
			print 'illegal value in at_sec. must be >= 0. setting to 0'
			
		if self.inSyncCmd:
			secval = np.uint32(atsecVal*100000000)
			msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_SYNC_CMD_TIME_VAL,secval,0,0,0,0,0,0,0,0)
			self.sock.send(msg)	
			bb = self.sock.recv(4,socket.MSG_WAITALL)
		else:
			print "WARNING: outside of synchronous command blocks (i.e. between begin/endSyncCmd s) all commands have an associated execution time and can ONLY be executed sequentially. as a result, outside of a synchronous command block 'at_sec' cannot be used to set the time at which a command is executed to an earlier value. outside of a synchronous command block 'at_sec' is interpreted as a 'wait' statement and replaced by 'async_wait_sec'."
			self.async_wait_sec(atsecVal)
		
	def setNumSteeringLocs(self,numLocs=1):
		print 'numlocs', numLocs
		msg = struct.pack(self.u32cmsg,self.CASE_TX_SET_NSTEERING_LOCS,numLocs,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		print 'numlocs len(bb)',len(bb)
	
	def setSoundSpeed(self,soundSpeed=1482.0):
		tmp_cmsg = '2I4d' ### [ u32 , u32 , double , double , double , double ]
		self.SOUND_SPEED = 1.0*soundSpeed
		msg = struct.pack(tmp_cmsg,self.CASE_TX_SET_SOUND_SPEED,0,self.SOUND_SPEED,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
			
	def uploadPhaseDelays_usec(self,pd=np.zeros((1,8))):
		self.pd_sock.connect(('192.168.1.101',3700))
		
		maxItems = self.MAX_16BIT_PHASE_DELAYS_PER_PACKET
		
		a = np.shape(pd)
		nlocs = a[0]
		nphases = a[0]*a[1]
		self.setNumSteeringLocs(nlocs)
		
		nloops = (nphases)/maxItems + 1	
		pd_array = np.zeros((1,maxItems*nloops)).astype(np.uint16)
		
		for nl in range(0,nlocs):
			pd_array[0,8*nl:(8*(nl+1))] = pd[nl,:]*100
		
		for nl in range(0,nloops):
			if ((nl+1)<nloops):
				msg = struct.pack(self.pd_msg, self.CASE_TX_UPLOAD_PHASE_DELAYS, maxItems, *pd_array[0,maxItems*nl:(maxItems*(nl+1))])
			else:
				msg = struct.pack(self.pd_msg, self.CASE_TX_UPLOAD_PHASE_DELAYS, ( nphases % maxItems ), *pd_array[0,maxItems*nl:(maxItems*(nl+1))])
			
			self.pd_sock.send(msg)	
			
		bb = self.pd_sock.recv(4,socket.MSG_WAITALL)
		
	def uploadSteeringLocs_mm(self,sl=np.zeros((1,3))):
		self.pd_sock.connect(('192.168.1.101',3700))
		maxItems = self.MAX_DOUBLE_PRECISION_STEERING_COORD_VALS_PER_PACKET
		maxLocs = self.MAX_STEERING_LOCS_PER_PACKET
		
		a = np.shape(sl)
		nlocs = a[0]
		ncoord_vals = a[0]*a[1]
		self.setNumSteeringLocs(nlocs)
		
		nloops = (ncoord_vals)/maxItems + 1	
		sl_array = np.zeros((1,maxItems*nloops)).astype(np.float64)
		
		for nl in range(0,nlocs):
			sl_array[0,3*nl:(3*(nl+1))] = sl[nl,:]*1e-3
		
		for nl in range(0,nloops):
			if ((nl+1)<nloops):
				msg = struct.pack(self.sl_msg, self.CASE_TX_UPLOAD_STEERING_LOCS, maxLocs, *sl_array[0,maxItems*nl:(maxItems*(nl+1))])
			else:
				msg = struct.pack(self.sl_msg, self.CASE_TX_UPLOAD_STEERING_LOCS, ( nlocs % maxLocs ), *sl_array[0,maxItems*nl:(maxItems*(nl+1))])
			
			self.pd_sock.send(msg)	
			
		print 'hello'
		bb = self.pd_sock.recv(4,socket.MSG_WAITALL)
		print 'goodbye'
			
	def connectToFpga(self):
		self.sock.connect(('192.168.1.101',3600))
		
	def __init__(self):
		# defined variables in the C code running on arm
		self.CASE_TX_SET_CONTROL_STATE = 0
		self.CASE_TX_SET_TRIG_REST_LVLS = 1
		self.CASE_TX_SET_ACTIVE_TRANSDUCERS = 2 
		self.CASE_TX_MAKE_PIO_CMD = 3
		self.CASE_TX_END_PIO_CMD = 4
		self.CASE_TX_MAKE_COUNTER_START = 5
		self.CASE_TX_MAKE_COUNTER_END = 6
		self.CASE_TX_MAKE_LOOP_START = 7
		self.CASE_TX_MAKE_LOOP_END = 8
		self.CASE_TX_BUFFER_TRIG_TIMINGS = 10
		self.CASE_TX_BUFFER_CHARGE_TIME = 11
		self.CASE_TX_BUFFER_PHASE_DELAYS = 12
		self.CASE_TX_BUFFER_FIRE_CMD = 13
		self.CASE_TX_BUFFER_RECV_TRIG = 14
		self.CASE_TX_BUFFER_ASYNC_WAIT= 15
		self.CASE_TX_WAIT_CMD = 16
		self.CASE_TX_SET_SYNC_CMD_TIME_VAL = 17
		self.CASE_TX_SET_NSTEERING_LOCS = 18
		self.CASE_TX_CONNECT_INTERRUPT = 19
		self.CASE_TX_SET_EXTERNAL_TRIGGER_MODE = 20
		self.CASE_TX_BUFFER_VAR_ATTEN_TIMINGS = 21
		self.CASE_TX_SET_VAR_ATTEN_REST_LVL = 22
		self.CASE_TX_BUFFER_TMP_MASK_CMD = 23
		self.CASE_TX_SET_SOUND_SPEED = 24
		
		self.CASE_TX_SET_PHASE_FROM_LOOP_IDX_AS_MEM_IDX = 25
		self.CASE_TX_CALC_AND_SET_PHASE_AT_SPECIFIED_COORD_VALS = 26
		self.CASE_TX_CALC_AND_SET_PHASE_FROM_LOOP_IDXS_AS_COORD_VALS = 27
		
		self.CASE_TX_UPLOAD_PHASE_DELAYS = 0
		self.CASE_TX_UPLOAD_STEERING_LOCS = 1

		self.CASE_EXIT_PROGRAM = 100
		
		self.PDMSG_SIZE = 2048
		self.FLOAT_SIZE = 4
		self.DOUBLE_SIZE = 8
		self.U64_SIZE = 8
		self.U32_SIZE = 4
		self.U16_SIZE = 2
		self.MAX_16BIT_PHASE_DELAYS_PER_PACKET = (self.PDMSG_SIZE-2*self.U32_SIZE)/(self.U16_SIZE)
		self.MAX_DOUBLE_PRECISION_STEERING_COORD_VALS_PER_PACKET = (self.PDMSG_SIZE-2*self.U32_SIZE)/(self.DOUBLE_SIZE)
		self.MAX_STEERING_LOCS_PER_PACKET = self.MAX_DOUBLE_PRECISION_STEERING_COORD_VALS_PER_PACKET/3
		

		self.SOUND_SPEED = 1482.0
		
		self.inSyncCmd = 0
		
		# ethernet sockets for transferring data to/from arm
		# 'sock' is used to send commands to arm
		# 'pd_sock' sends phase delays to arm
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self.pd_sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		
		# for converting enet msg's into c-readable form
		self.u32cmsg = '10I'
		self.pd_msg = '{}{}{}'.format('2I',self.MAX_16BIT_PHASE_DELAYS_PER_PACKET ,'H')
		self.sl_msg = '{}{}{}'.format('2I',self.MAX_DOUBLE_PRECISION_STEERING_COORD_VALS_PER_PACKET ,'d')
		
		''' character meanings in cmsg: 
		#	(https://docs.python.org/2/library/struct.html)
		#	'h','H' = signed,unsigned short (2 bytes)
		#	'i','I' = signed,unsigned int (4 bytes)
		#	'q','Q' = signed,unsigned long long (8bytes)
		#	'f' = float (4bytes)
		#	'd' = double (8bytes)
		'''

class receiver():
	
	def activateRecvr(self,plotLater=0):
		self.pid = os.fork()
		if (self.pid == 0):
			#~ self.pt = subprocess.Popen(['gnuplot','-e','-p'],shell=True,stdin=subprocess.PIPE,)
			#~ self.pt.stdin.write("set term x11 size 650,650 font 'Helvetica,16'\n")
			self.connectRcvDataSock()
			if plotLater:
				self.allData = np.zeros((self.nplotpulses,self.recLen,8))
			n=0
			while n<self.nplotpulses:
				if plotLater:
					self.queryData(nthpls=(n+1))
				else:
					self.queryData()
				n+=1
				
			self.disconnectDataSock()
			os._exit(0)
			
		else:
			self.stateReset(0)
					
	def stateReset(self,val=0):
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_STATE_RESET,val,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setRecLen(self,recLen,allocate=0):
		if recLen <= self.MAX_RECLEN:
			self.recLen = recLen
		else:
			self.recLen = 2048
			print 'max recLen = 16384'
			
		if allocate > 0:
			self.allocated = 0
		else:
			self.allocated = 1
			
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'I')
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_RECLEN,self.recLen,self.allocated,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setRecDuration(self,duration):
		recLen = int(duration*self.ADC_CLK/(self.clockDiv+1.0))
		if recLen <= self.MAX_RECLEN:
			self.recLen = recLen
		else:
			self.recLen = 2048
			print 'max duration = ', np.round(self.MAX_RECLEN/self.ADC_CLK,2)
		self.cmsg4 = '{}{}{}{}'.format(self.recLen,'q',self.recLen,'I')
		
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_RECLEN,self.recLen,0,0,0,0,0,0,0,0)
		self.sock.send(msg)	
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setPioVarAtten(self,varGain):
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_PIO_VAR_GAIN, ( varGain & 0x3 ) ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setClockDivisor(self,clockDiv):
		self.clockDiv = clockDiv-1
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_CLOCK_DIVISOR, ( np.uint32(clockDiv-1) & 0xf ) ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)

	def setSamplingMode(self,samplingMode):
		# 0 -> sample every Nth
		# 1 -> average of pulses [N:(N+clockDivisor-1)]
		self.samplingMode = samplingMode
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_SAMPLING_MODE, self.samplingMode ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setCompressorMode(self,compressorMode):
		# 0 -> no compression (16bit)
		# 1 -> compressed (12bit)
		self.compressorMode = compressorMode
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_COMPRESSOR_MODE, self.compressorMode ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)

	def setLeds(self,ledVal):
		msg = struct.pack(self.u32cmsg,self.CASE_SET_LEDS, ( ledVal & 0x1f ) ,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)

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
	
	def plotDatasRealTime(self,datas):
		
		print 'npulses',self.npulses
		
		if self.samplingMode:
			clockDiv = (self.clockDiv+1.0)
		else:
			clockDiv = 1
			
		t = np.linspace(0,self.recLen/self.ADC_CLK*clockDiv,self.recLen)
		
		dd = np.zeros((self.recLen,9))
		dd[:,0] = t
		for npls in range(0,self.npulses):
			dd[:,npls+1] = datas[npls*self.recLen:(npls+1)*self.recLen,0]/clockDiv
		
		np.savetxt("fdata.dat",dd,delimiter=' ')
		
		self.pt.stdin.write("plot 'fdata.dat' u 1:8 w lines noti\n")
				
	def plotDatas(self,datas):
		
		print 'npulses',self.npulses
		
		clockDiv = self.clockDiv + 1
			
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
						ax[nr,nc].plot(t+npls*t[-1],datas[npls*self.recLen:(npls+1)*self.recLen,n]/clockDiv,'-')
						pk2pk = (np.max(datas[npls*self.recLen:(npls+1)*self.recLen,n])-np.min(datas[npls*self.recLen:(npls+1)*self.recLen,n]))/clockDiv
						rmsval = (np.sqrt(np.mean((datas[npls*self.recLen:(npls+1)*self.recLen,n]-np.mean(datas[npls*self.recLen:(npls+1)*self.recLen,n]))**2)))/clockDiv

					ax[nr,nc].set_title('{}{}{}{}{}{}'.format('Ch[',n,'], pk-pk ',pk2pk,', RMS: ',np.round(rmsval,2) ))
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
	
	def plotDatasLater(self,datas):
		
		print 'npulses',self.npulses
		
		clockDiv = self.clockDiv + 1
			
		t = np.linspace(0,self.recLen/self.ADC_CLK*clockDiv,self.recLen)
		
		fig,ax = plt.subplots(self.nrows,self.ncols,sharey=True,sharex=True,figsize=(self.figwidth, self.figheight))
		
		if self.nrows==1 and self.ncols==1:
			ax.set_ylim((self.ylims[0],self.ylims[1]))
			for n in range(0,8):
				ax.plot(t+n*t[-1],datas[:,n])
			ax.grid(linestyle=':')
		else:
			n = 0
			
			datas2 = np.zeros((datas.shape[1]*datas.shape[0],datas.shape[2]))
			datas3 = np.mean(datas,axis=0)
			print 'data3.shape:', datas3.shape
			for m in range(0,self.nplotpulses):
				datas2[self.recLen*m:self.recLen*(m+1),:] = datas[m,:,:]
				
			rmsvals = np.zeros(8)
			pk2pks = np.zeros(8)
			for m in range(0,8):
				rmsvals[m] = (np.sqrt(np.mean((datas2[:,m]-np.mean(datas2[:,m]))**2)))/clockDiv
				pk2pks[m] = (np.max(datas2[:,m])-np.min(datas2[:,m]))/clockDiv

			
			
			
			for nc in range(0,self.ncols):
				for nr in range(0,self.nrows):	
					for npls in range(0,self.npulses):
						#~ ax[nr,nc].plot(t+npls*t[-1],datas[npls*self.recLen:(npls+1)*self.recLen,n]/clockDiv,'-')
						ax[nr,nc].plot(t+npls*t[-1],datas3[:,n]/clockDiv,'-')
						#~ pk2pk = (np.max(datas[npls*self.recLen:(npls+1)*self.recLen,n])-np.min(datas[npls*self.recLen:(npls+1)*self.recLen,n]))/clockDiv
						#~ rmsval = (np.sqrt(np.mean((datas[npls*self.recLen:(npls+1)*self.recLen,n]-np.mean(datas[npls*self.recLen:(npls+1)*self.recLen,n]))**2)))/clockDiv

					ax[nr,nc].set_title('{}{}{}{}{}{}'.format('Ch[',n,'], pk-pk ',pk2pks[n],', RMS: ',np.round(rmsvals[n],2) ))
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
	
		
	def queryDataLocal16(self,pltr=1,nthpls=0):
		#~ msg = struct.pack(self.u32cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		#~ self.sock.send(msg)
		#~ bb = self.sock.recv(4,socket.MSG_WAITALL)
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
		
		if self.isUnsigned:
			self.ylims = [-200,4300]
			#~ self.ylims = [1800,2300]
		else:
			num_bits = 12
			mask = 2**(num_bits - 1)
			self.ylims = [-2200,2200]
			
		if self.isUnsigned:
			cc0 = np.array(struct.unpack(self.cmsg5,bb)).astype(np.uint16)
			cc = (cc0 & 0x0fff)
		else:
			cc0 = np.array(struct.unpack(self.cmsg5,bb)).astype(np.int16)
			cc = -(cc0 & mask) + (cc0 & ~mask)
		
		if nthpls == 0:	
			dd = np.zeros((self.npulses*self.recLen,8))
			print 'dd:',dd.shape,'cc:',cc.shape
				
			for n in range(0,8):
				dd[:,n] = cc[n::8]

			if pltr:
				print "pltr", pltr	
				self.plotDatas(dd)
		else:
			for n in range(0,8):
				self.allData[nthpls-1,:,n] = cc[n::8]
			
			if pltr and (nthpls == self.nplotpulses):
				print "pltr", pltr
				
				self.plotDatasLater(self.allData)
		
	def queryDataLocal(self):
		msg = struct.pack(self.u32cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
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
			
	def queryData(self,pltr=1,nthpls=0):
		if self.allocated == 0:
			self.setupLocalStorage()
			
		if(self.realTime or self.transferData):
			
			if(self.is16bit):
				print 'helper is 16bit'
				self.queryDataLocal16(pltr,nthpls)
			else:
				#~ print 'helper'
				self.queryDataLocal()
		
		else:
			msg = struct.pack(self.u32cmsg,self.CASE_QUERY_DATA,0,0,0,0,0,0,0,0,0)
			self.sock.send(msg)
			#~ print 'issue query cmd'
			bb = self.sock.recv(4,socket.MSG_WAITALL)
					
	def setAdcGain(self,adcGain):
		msg = struct.pack(self.cmsg3,self.CASE_ADC_SET_GAIN,0,adcGain,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setAdcUnsigned(self,uns):
		if uns:
			self.isUnsigned = 1
		else:
			self.isUnsigned = 0
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_UNSIGNED,uns,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setAdcLowNoiseMode(self,lnm):
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_LOW_NOISE_MODE,lnm,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def powerDownAdcChannels(self,chpwr):
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_TOGGLE_CHANNEL_POWER,chpwr,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def setAdcFilterBw(self,filterbw):
		#0b00: 14MHz. 0b01: 10MHz. 0b10: 7.5MHz. 0b11: Not used.
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_FILTER_BW,filterbw,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def setAdcInternalAcCoupling(self,accoupling):
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_INTERNAL_AC_COUPLING,accoupling,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def issueDirectAdcCmd(self,gp_tgc,addr,cmd):
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_ISSUE_DIRECT_CMD,gp_tgc,addr,cmd,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def setRamp(self):
		self.issueDirectAdcCmd(0,2,(111<<13))
		
	def resetToDefaultAdcSettings(self):
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_DEFAULT_SETTINGS,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	
	def connectInterrupt(self):
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_CONNECT_INTERRUPT,1,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def disconnectInterrupt(self):
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_CONNECT_INTERRUPT,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def setupLocalStorage(self):
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SETUP_LOCAL_STORAGE,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
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
			
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_NPULSES,self.npulses,self.allocated,0,0,0,0,0,0,0)	
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def plotNPulses(self,npulses):
		self.nplotpulses = npulses
		

	def setQueryMode(self,realTime=1, transferData=1, saveData=0, is16bit=1, sendOnRequest=0):
		
		self.realTime = realTime
		self.transferData = transferData
		self.saveData = saveData
		self.is16bit = is16bit
		self.sendOnRequest = sendOnRequest
		
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_SET_QUERY_MODE,self.realTime,self.transferData,self.saveData,self.is16bit,self.sendOnRequest,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setFclkDelay(self,fclk):
		# fclk_delay -> 0-11
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_FCLOCK_DELAY,fclk,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
		
	def setAutoShutdown(self,asd=0):
		msg = struct.pack(self.u32cmsg,self.CASE_UPDATE_AUTO_SHUTDOWN_SETTING,asd,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def interruptSelf(self,val=0):
		msg = struct.pack(self.u32cmsg,self.CASE_RCV_INTERRUPT_THYSELF,val,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def powerDownAdc(self):
		power_down = 1
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_POWER_ON_OFF,power_down,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def powerUpAdc(self):
		power_up = 0
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_SET_POWER_ON_OFF,power_up,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def disableAdcClamp(self):
		self.clampVal = 1
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_DISABLE_CLAMP,self.clampVal,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
	def enableAdcClamp(self):
		self.clampVal = 0	
		msg = struct.pack(self.u32cmsg,self.CASE_ADC_DISABLE_CLAMP,self.clampVal,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		bb = self.sock.recv(4,socket.MSG_WAITALL)
	
						
	def connectToFpga(self):
		self.sock.connect(('192.168.1.101',3400))
		self.resetToDefaultAdcSettings()
		self.setAutoShutdown(0)

		# can't do 'moving sum' with compression. will corrupt data
		#~ self.setClockDivisor(1)
		#~ self.setFclkDelay(2) # accepts values 0-5
		
	def connectRcvDataSock(self):
		self.dsock.connect(('192.168.1.101',3500))
	
	def disconnectCommSock(self):
		self.sock.close()
		
	def disconnectDataSock(self):
		self.dsock.close()
		
	def closeProgram(self):
		msg = struct.pack(self.u32cmsg,self.CASE_EXIT_PROGRAM,0,0,0,0,0,0,0,0,0)
		self.sock.send(msg)
		self.sock.close()
			
	def __init__(self):
		# defined variables in the C code running on arm
		self.CASE_RCV_SET_RECLEN = 0
		self.CASE_RCV_SET_PIO_VAR_GAIN = 1
		self.CASE_SET_LEDS = 2
		self.CASE_RCV_STATE_RESET = 3
		self.CASE_ADC_SET_GAIN = 4
		self.CASE_ADC_SET_UNSIGNED = 5
		self.CASE_ADC_SET_LOW_NOISE_MODE = 6
		self.CASE_ADC_TOGGLE_CHANNEL_POWER = 7
		self.CASE_ADC_SET_FILTER_BW = 8
		self.CASE_ADC_SET_INTERNAL_AC_COUPLING = 9
		self.CASE_ADC_ISSUE_DIRECT_CMD = 10
		self.CASE_RCV_CONNECT_INTERRUPT = 11
		self.CASE_RCV_SETUP_LOCAL_STORAGE = 12
		self.CASE_ADC_SET_DEFAULT_SETTINGS = 13
		self.CASE_RCV_SET_QUERY_MODE = 14
		self.CASE_UPDATE_AUTO_SHUTDOWN_SETTING = 15
		self.CASE_RCV_SET_NPULSES = 16
		self.CASE_RCV_SET_CLOCK_DIVISOR	= 17
		self.CASE_RCV_SET_SAMPLING_MODE	= 18
		self.CASE_RCV_SET_COMPRESSOR_MODE = 19
		self.CASE_RCV_INTERRUPT_THYSELF = 20
		self.CASE_ADC_SET_POWER_ON_OFF = 21
		self.CASE_ADC_DISABLE_CLAMP = 22
		
		self.CASE_ADC_SET_FCLOCK_DELAY = 52
		self.CASE_EXIT_PROGRAM = 100
		
		# it is what it says
		self.ADC_CLK = 50.0
		
		self.MAX_RECLEN = 32768
		
		# ethernet sockets for transferring data to/from arm
		# 'sock' is used to send data to arm
		# 'dsock' receives data from arm
		self.sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self.dsock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		
		self.allocated = 0
		
		self.clampVal = 0
		
		# for allocating storage on arm/setting up enet transfer
		self.npulses = 1
		self.recLen = 2048
		
		# mode settings for receiving data
		self.realTime = 1
		self.transferData = 0
		self.saveData = 0
		self.is16bit = 1
		self.isUnsigned = 1
		self.sendOnRequest = 0
		self.clockDiv = 0
		self.samplingMode = 0
		self.compressorMode = 0
		
		# sampling mode names
		self.EVERY_NTH = 0
		self.MOVING_SUM = 1
		
		# compressor mode names
		self.RAW16 = 0
		self.RAW12 = 1
		self.DIFF_8BIT = 2

		self.nplotpulses = 1
		
		# variables to control size of plotted data
		self.ylims = [-200,4300]
		self.xlims = [-100,-1,0]
		self.figwidth = 30
		self.figheight = 18
		self.nrows = 4
		self.ncols = 2
		self.pt = 0

		
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
		
		self.pid = 1
		
		# for for converting enet msg's into c-readable form
		self.u32cmsg = '10I'
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
		
		
		

npulses = 1

r = receiver()
r.connectToFpga()
#~ r.resetToDefaultAdcSettings()
#~ r.setAutoShutdown(0)

r.setClockDivisor(1)
#~ r.setFclkDelay(2) # accepts values 0-5
#~ r.setSamplingMode(r.EVERY_NTH)
#~ r.setCompressorMode(r.RAW16)
#~ r.setRecLen(32768/2)
r.setRecDuration(250.0)

r.setAdcGain(31.875)

#~ r.setNPulses(1)
#~ r.setAdcInternalAcCoupling(0)
#~ r.setAdcLowNoiseMode(0)
#~ r.setAdcFilterBw(0)
#~ r.setAdcUnsigned(0)
#~ r.setRamp()

r.setQueryMode(realTime=1,transferData=0,saveData=0)
#~ r.plotterSetup(figheight = 15, figwidth = 10, nrows = 4, ncols = 2)
#~ r.plotterSetup(ylims = [-200,4300], xlims = [-100,2600], figheight = 10, figwidth = 30, nrows = 4, ncols = 2)

r.plotNPulses(4*npulses)
#~ r.powerDownAdc()

r.activateRecvr(plotLater=0)

#~ r.interruptSelf(1)
#~ time.sleep(0.1)
#~ r.interruptSelf(0)
if (r.pid):
	
	t = transmitter()
	t.connectToFpga()
	
	t.setTrigRestLvls(0x1f)
	t.setVarAttenRestLvl(0x0)
	t.setActiveTransducers(0b00110000)
	
	phaseDelays = np.zeros((100,8)).astype(np.uint16)
	steerLocs = np.zeros((100,3)).astype(np.float64)
	
	for mm in range(0,100):
		phaseDelays[mm,0] = 12
		phaseDelays[mm,1] = 14
		phaseDelays[mm,2] = 16
		phaseDelays[mm,3] = 18
		phaseDelays[mm,4] = 5
		phaseDelays[mm,5] = 0
		phaseDelays[mm,6] = 5
		phaseDelays[mm,7] = 10
		steerLocs[mm,0] = (mm-50)/10.0
		steerLocs[mm,1] = (mm-50)/10.0
		steerLocs[mm,2] = (mm-50)/10.0
		
		
	#~ t.uploadPhaseDelays_usec(phaseDelays)
	t.uploadSteeringLocs_mm(steerLocs)
	
	memLocCntrID = 1
	xID,yID,zID = 100,101,102
	
	t.startCounter( cntrId=memLocCntrID , startIdx=0 , endIdx=npulses , stepSize=1 )
	if 1:
		
		t.startNamedLoop( loopId=xID , startVal=-10.0 , endVal=1.0 , stepSize=20.0 , units='mm')
		t.startNamedLoop( loopId=yID , startVal=-10.0 , endVal=1.0 , stepSize=20.0 , units='mm')
		t.startNamedLoop( loopId=zID , startVal=-10.0 , endVal=1.0 , stepSize=20.0 , units='mm')
		if 1:
			
			t.beginSyncCmd()
			if 1:
				t.setChargeTime(5)
				t.at_usec(10)
				t.fireAtNamedLoopCoords(xID,yID,zID)
				t.setTrig(1,10)
				t.setTrig(2,5)
				t.rcvData()
			t.endSyncCmd()
			
			t.wait_sec(0.1)
			
			t.beginSyncCmd()
			if 1:
				t.at_usec(10)
				t.fireAtCounterIdx(memLocCntrID)
				t.setTrig(1,10)
				t.setTrig(2,5)
				t.rcvData()
			t.endSyncCmd()
			t.async_wait_sec(0.1)
			
			t.beginSyncCmd()
			if 1:
				t.at_usec(10)
				t.fireAtLoc(-10,10,0)
				t.setTrig(1,10)
				t.setTrig(2,5)
				t.rcvData()
			t.endSyncCmd()
			t.wait_sec(0.01)
			
			t.beginSyncCmd()
			if 1:
				t.setChargeTime(5)
				t.at_usec(10)
				t.fire()
				t.setTrig(1,10)
				t.setTrig(2,5)
				t.rcvData()
			t.endSyncCmd()
			t.wait_sec(0.1)
	
		t.endNamedLoop(xID)
		t.endNamedLoop(yID)
		t.endNamedLoop(zID)	
	
	t.endCounter(memLocCntrID )
	
	
	
	t.executeProgram()
	print 'hello'


	os.waitpid(r.pid,0)
	print "ohms"
	r.disconnectCommSock()
	#~ r.closeProgram()
	print "the child is dead"




		

	



