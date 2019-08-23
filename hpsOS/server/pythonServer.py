

import socket
import struct
import time
import select
import numpy as np
from dataServerParamConfig import *
import sysv_ipc
import os


class dataServer():
	
	
	def makeServer(self):
		for n in range(0,2):
			self.server.append(socket.socket(socket.AF_INET, socket.SOCK_STREAM))
			self.server[n].setblocking(0)
			self.server[n].setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
			self.server[n].bind(('192.168.1.100',3400+n))
			self.server[n].listen(1)
							
	def __init__(self):	
		self.server = []
		self.connections = []
		self.addresses = []
		self.CASE_ADC_RECORD_LENGTH 		= 1		
		self.CASE_ADC_TRIGGER_DELAY			= 2
		self.CASE_QUERY_ADC_FOR_DATA		= 3
		self.CASE_ADC_POWER					= 4
		self.CASE_ADC_SYNC					= 5
		self.CASE_ADC_INITIALIZE			= 6
		self.CASE_ADC_GAIN					= 7
		self.CASE_ADC_DIRECT_INSTRUCTION	= 8
		self.CASE_ADC_CONTROL_COMMS			= 9	
		self.CASE_KILLPROGRAM				= 100		

		
		outputs = []
		while True:
			print 'into server'
			readable, writable, exceptional = select.select(self.server, outputs, outputs)
			for sock in readable:
				if sock in self.server:
					connection, client_addresssock = sock.accept()
					self.connections.append((connection, client_addresssock))
				elif sock is self.lserver:
					print 'umpires'
					connection, client_addresssock = sock.accept()
					self.connections.append((connection, client_addresssock))
			

		
d = dataServer()	

		#~ # Bind the socket to the port
		#~ server_address = ('localhost', 10000)
		#~ server.bind(server_address)

		#~ # Listen for incoming connections
		#~ server.listen(5)
		

	



