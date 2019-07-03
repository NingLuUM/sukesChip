
void resetVars_rcv(RCVvars *RCV){
	DREF32(RCV->recLen) = 2048; 
	DREF32(RCV->trigDelay) = 0;
	DREF32(RCV->controlComms) = 0;
	RCV->sizeof_bytesPerTimePoint = RCV_BYTES_PER_TIMEPOINT;
	RCV->recLen_ref = 2048;
	RCV->trigDelay_ref = 0;
    RCV->isLocal = 0;
    RCV->nPulses = 1;
    RCV->currentPulse = 0;
	RCV->board->packetsize = MAX_ENET_TRANSMIT_SIZE;
    RCV->board->queryTimeout = 1000;
    RCV->board->moduloBoardNum = 1;
	RCV->board->moduloTimer = 0;
	RCV->board->packetWait = 0;
	RCV->board->numPorts = (RCV->recLen_ref-1)/(RCV->board->packetsize)+1;
	RCV->board->queryMode = 0;
	RCV->allocateLocalStorage(RCV);
}
	
void setRecLen_rcv(RCVvars *RCV, uint32_t recLen){
	DREF32(RCV->recLen) = recLen;
	RCV->recLen_ref = recLen;
}

void setTrigDelay_rcv(RCVvars *RCV, uint32_t trigDelay){
	DREF32(RCV->trigDelay) = trigDelay;
	RCV->trigDelay_ref = trigDelay;
}

void stateResetFPGA_rcv(RCVvars *RCV){
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

void copyDataToMem_rcv(RCVvars *RCV){
	size_t nbytes = (RCV->recLen_ref)*sizeof(int32_t);
    int curPos = 3*(RCV->currentPulse)*(RCV->recLen_ref)*sizeof(int32_t);	
    adcmemcpy(&(*(RCV->data))[ curPos ],DREFP32S(RCV->ramBank0),nbytes);
	adcmemcpy(&(*(RCV->data))[ curPos + (RCV->recLen_ref)*sizeof(int32_t) ],DREFP32S(RCV->ramBank1),nbytes);
	adcmemcpy(&(*(RCV->data))[ curPos + 2*(RCV->recLen_ref)*sizeof(int32_t) ],DREFP32S(RCV->ramBank2),nbytes);
}

void setDataAddrPointers_rcv(RCVvars *RCV, ENETsock **ENET){
    ENETsock *enet;
    int bytesRemaining = (RCV->recLen_ref)*RCV_BYTES_PER_TIMEPOINT;
    int requestedBytesPerPacket = (RCV->board->packetsize)*RCV_BYTES_PER_TIMEPOINT;
    int actualBytesInPacket;
    
    int sendbuff;
    enet = (*ENET)->commsock;

    /* check if sockets exist on the desired ports, if not add them */
    while( enet->portNum < (RCV->board->numPorts) ){
        if( enet->prev == NULL ){
            addPollSock_enet(ENET,enet->portNum+1);
        }
        enet = enet->prev;
    }

    /* if more sockets exist than the number of active ports, disable them */
    while( enet != NULL ){
        if( enet->portNum > (RCV->board->numPorts) ){
            disconnectSock_enet(ENET,enet->portNum);
        }
        enet = enet->prev;
    }
    
    enet = (*ENET)->commsock->prev;
    /*  go through the list of active sockets, connect them if they aren't already.
        set pointers to appropriate addresses in data array where each socket should sent data from.
        set the size of the data the socket is responsible for sending to cServer. */
    while( bytesRemaining > 0 ){
        if( !(enet->is_active) )
            connectPollSock_enet(ENET,enet->portNum);
            
        actualBytesInPacket = ( requestedBytesPerPacket < bytesRemaining ) ? requestedBytesPerPacket : bytesRemaining;
        
        enet->dataAddr = *(RCV->data) + (enet->portNum-1)*requestedBytesPerPacket;
        enet->bytesInPacket = actualBytesInPacket;
        sendbuff = enet->bytesInPacket;
		setsockopt(enet->sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)); 
        bytesRemaining -= requestedBytesPerPacket;
        enet = enet->prev;
    }
    
    while(enet != NULL){
		disconnectSock_enet(ENET,enet->portNum);
        enet = enet->prev;
	}
}

void setLocalStorage_rcv(RCVvars *RCV, uint32_t isLocal, uint32_t nPulses){
	RCV->isLocal = isLocal;
	if( isLocal ){
		RCV->nPulses = nPulses;
		RCV->currentPulse = 0;
	} else {
		RCV->nPulses = 1;
		RCV->currentPulse = 0;
	}
}

void allocateLocalStorage_rcv(RCVvars *RCV){
	free(*(RCV->data));
	*(RCV->data) = (char *)malloc((RCV->nPulses)*(RCV->recLen_ref)*(RCV->sizeof_bytesPerTimePoint));
}

uint32_t getInterruptMsg_rcv(RCVvars *RCV){
	return (DREF32(RCV->interrupt0));
}

void getBoardData(RCVvars *RCV){ // load the boards specific data from files stored on SoC		
	char const* const fileName = "boardData";
    FILE* file = fopen(fileName, "r");
    char line[256];
	int n=0;
	
    while( fgets(line, sizeof(line), file) && n<4 ){
        g_boardData[n] = atoi(line);
        n++;    
    }  
    fclose(file);
    g_boardNum = g_boardData[0];
    RCV->board->boardNum = g_boardData[0];
}


void RCV_Settings(RCVvars *RCV, ENETsock **ENET, ENETsock *INTR, uint32_t *msg){
	
	switch(msg[0]){
		
		case(CASE_RCV_RECORD_LENGTH):{ // change record length
			if( msg[1]<=MAX_RECLEN ){
				RCV->setRecLen(msg[1]);
			} else {
				RCV->setRecLen(RCV,2048);
				printf("invalid recLen, defaulting to 2048, packetsize to 512\n");
			}
			break;
		}
		
		case(CASE_RCV_TRIGGER_DELAY):{ // change adc trig delay
			RCV->setTrigDelay(RCV, ADC_CLK*msg[1]);
			printf("RCV: Trig Delay -- %.2f\n",((float) RCV->trigDelay_ref)/((float)ADC_CLK));
			break;
		}

		case(CASE_RCV_SET_LOCAL_STORAGE):{
            // msg[1] = isLocal ( 0 = False , 1 = True )
            // msg[2] = nPulses 
			RCV->setLocalStorage(RCV,msg[1],msg[2]);
			break;
		}
		
		case(CASE_RCV_TOGGLE_DATA_ACQ):{
            g_dataAcqGo = ( msg[1] == 1 || msg[1] == 0 ) ? msg[1] : 0;
			if( g_dataAcqGo ){
                RCV->allocateLocalStorage(RCV);
                RCV->board->numPorts = (RCV->recLen_ref-1)/(RCV->board->packetsize) + 1;
                if(RCV->isLocal==0){
                    RCV->setDataAddrPointers(RCV, ENET);
                }
                RCV->setDataAddrPointers(RCV, ENET);
            }
			break;
		}
		
		case(CASE_RCV_DIRECT_CONTROL_COMMS):{
			DREF32(RCV->controlComms) = msg[1];
			printf("adccomms[%d], %lu\n",msg[1],(long unsigned int)msg[2]);			
			break;
		}					
	
        case(CASE_RESET_RCV_SYSTEM):{
            RCV->resetVars(RCV);
            RCV->setDataAddrPointers(RCV,ENET);
            break;
        }

		default:{
			printf("default case ADC_settings, doing nothing\n");
            break;
		}
		
	}
}


void BOARD_settings(RCVsys *RCV, ENETsock **ENET, uint32_t *msg){
	ENETsock  *commsock;
	commsock = (*ENET)->commsock;
	
	switch(msg[0]){

		case(CASE_QUERY_BOARD_INFO):{// loads board-specific data from onboard file		
			send(commsock->sockfd,g_boardData,enetMsgSize,0);
            setsockopt(commsock->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
            break;
		}
		
        default:{
			printf("default case BOARD_settings, doing nothing\n");
		}
	}
}











