
void resetVars_rcv(RCVsys_t *RCV){
	DREF32(RCV->recLen) = 2048; 
	DREF32(RCV->trigDelay) = 0;
	DREF32(RCV->controlComms) = 0;
	RCV->sizeof_bytesPerTimePoint = ADC_BYTES_PER_TIMEPOINT;
	RCV->recLen_ref = 2048;
	RCV->trigDelay_ref = 0;
    RCV->isLocal = 0;
    RCV->nPulses = 1;
    RCV->currentPulse = 0;
	RCV->allocateLocalStorage(RCV);
}
	
int setRecLen_rcv(RCVsys_t *RCV, uint32_t recLen){
    if(recLen<=MAX_RECLEN){
        DREF32(RCV->recLen) = recLen;
        RCV->recLen_ref = recLen;
        return(0);
    } else {
        DREF32(RCV->recLen) = 2048;
        RCV->recLen_ref = 2048;
        return(1);
    }
}

int setTrigDelay_rcv(RCVsys_t *RCV, uint32_t trigDelay){
	DREF32(RCV->trigDelay) = trigDelay;
	RCV->trigDelay_ref = trigDelay;
}

void stateResetFPGA_rcv(RCVsys_t *RCV){
	DREF32(RCV->stateReset)=1; 
	usleep(5);
	DREF32(RCV->stateReset)=0;
	usleep(5);
}

void adcmemcpy(char *dest, int32_t volatile *sourceData, size_t nbytes){
	char *src = (char *)sourceData;
	for(int i=0;i<nbytes;i++)
		dest[i] = src[i];
}

void copyDataToMem_rcv(RCVsys_t *RCV){
	size_t nbytes = (RCV->recLen_ref)*sizeof(int32_t);
    int curPos = 3*(RCV->currentPulse)*(RCV->recLen_ref)*sizeof(int32_t);	
    adcmemcpy(&(*(RCV->data))[ curPos ],DREFP32S(RCV->ramBank0),nbytes);
	adcmemcpy(&(*(RCV->data))[ curPos + (RCV->recLen_ref)*sizeof(int32_t) ],DREFP32S(RCV->ramBank1),nbytes);
	adcmemcpy(&(*(RCV->data))[ curPos + 2*(RCV->recLen_ref)*sizeof(int32_t) ],DREFP32S(RCV->ramBank2),nbytes);
}

void setDataAddrPointers_rcv(RCVsys_t *RCV, ENETsock_t **ENET){
    ENETsock_t *enet;
    int sendbuff;
    enet = (*ENET)->commsock;
    
    int bytesRemaining = (RCV->recLen_ref)*ADC_BYTES_PER_TIMEPOINT;
    int requestedBytesPerPacket = (enet->settings->packetsize)*ADC_BYTES_PER_TIMEPOINT;
    int actualBytesInPacket;
    
    

    /* check if sockets exist on the desired ports, if not add them */
    while( enet->portNum < (enet->settings->numPorts) ){
        if( enet->prev == NULL ){
            addEnetSock_enet(ENET,enet->portNum+1,0);
        }
        enet = enet->prev;
    }

    /* if more sockets exist than the number of active ports, disable them */
    while( enet != NULL ){
        if( enet->portNum > (enet->settings->numPorts) ){
            disconnectSock_enet(ENET,enet->portNum);
        }
        enet = enet->prev;
    }
    
    enet = (*ENET)->commsock->prev;
    /*  go through the list of active sockets, connect them if they aren't already.
        set pointers to appropriate addresses in data array where each socket should sent data from.
        set the size of the data the socket is responsible for sending to cServer. */
    while( bytesRemaining > 0 ){
        if( !(enet->sock.is.active) )
            connectEnetSock_enet(ENET,enet->portNum);
            
        actualBytesInPacket = ( requestedBytesPerPacket < bytesRemaining ) ? requestedBytesPerPacket : bytesRemaining;
        
        enet->dataAddr = *(RCV->data) + (enet->portNum-1)*requestedBytesPerPacket;
        enet->bytesInPacket = actualBytesInPacket;
        sendbuff = enet->bytesInPacket;
		setsockopt(enet->sock.fd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)); 
        bytesRemaining -= requestedBytesPerPacket;
        enet = enet->prev;
    }
    
    while(enet != NULL){
		disconnectSock_enet(ENET,enet->portNum);
        enet = enet->prev;
	}
}

void setLocalStorage_rcv(RCVsys_t *RCV, uint32_t isLocal, uint32_t nPulses){
	RCV->isLocal = isLocal;
	if( isLocal ){
		RCV->nPulses = nPulses;
		RCV->currentPulse = 0;
	} else {
		RCV->nPulses = 1;
		RCV->currentPulse = 0;
	}
}

void allocateLocalStorage_rcv(RCVsys_t *RCV){
	free(*(RCV->data));
	*(RCV->data) = (char *)malloc((RCV->nPulses)*(RCV->recLen_ref)*(RCV->sizeof_bytesPerTimePoint));
}


uint32_t getInterruptMsg_rcv(RCVsys_t *RCV){
	return (DREF32(RCV->interrupt0));
}


void setDataTransferPacketSize_rcv(RCVsys_t *RCV, uint32_t packetsize){

}


void spawnDataTransferSocks_rcv(RCVsys_t *RCV){

}

