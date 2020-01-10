

void rcvSetStateReset(RCVsys_t *RCV, uint32_t val){
    DREF32(RCV->stateReset) = 1;
    usleep(5);
    DREF32(RCV->stateReset) = val;
    usleep(5);
}

void rcvSetRecLen(RCVsys_t *RCV, uint32_t recLen){
	if(recLen){
		DREF32(RCV->recLen) = recLen;
		RCV->refVals.recLen = recLen;
	} else {
		DREF32(RCV->recLen) = 1;
		RCV->refVals.recLen = 1;
	}
	usleep(5);
	if(RCV->refVals.pioSettings.compressor_mode && (RCV->refVals.recLen>1) ){
		DREF32(RCV->recLen)= (RCV->refVals.recLen*3)/4;
	} 
	usleep(5);
}

void rcvSetClkDivisor(RCVsys_t *RCV, uint32_t val){
	RCV->refVals.pioSettings.clkDiv = val;
	DREF32(RCV->pioSettings) = RCV->refVals.pioSettings.all;//0x03
    usleep(5);
}

void rcvSetSamplingMode(RCVsys_t *RCV, uint32_t val){
	RCV->refVals.pioSettings.sampling_mode = val;
	DREF32(RCV->pioSettings) = RCV->refVals.pioSettings.all;//0x03
    usleep(5);
}

void rcvSetCompressorMode(RCVsys_t *RCV, uint32_t val){
	RCV->refVals.pioSettings.compressor_mode = val;
	DREF32(RCV->pioSettings) = RCV->refVals.pioSettings.all;//0x03
	
	if( RCV->refVals.pioSettings.compressor_mode && (DREF32(RCV->recLen) == RCV->refVals.recLen ) && ( RCV->refVals.recLen>1 ) ){
		DREF32(RCV->recLen)= (RCV->refVals.recLen*3)/4;
	} else if ( (RCV->refVals.pioSettings.compressor_mode==0) && (DREF32(RCV->recLen) != RCV->refVals.recLen ) ){
		DREF32(RCV->recLen)= RCV->refVals.recLen;
	}
    usleep(5);
}

void rcvSetLEDs(RCVsys_t *RCV, uint32_t val){
    LED_t led;
    led.vals = ( val & 0x1f );
    led.hiRest = ~led.hiRest;
    DREF32(RCV->leds) = led.vals;
    usleep(5);
}

void rcvmemcpy( char *dest, char *src, size_t nbytes){
	for(int i=0;i<nbytes;i++){
		dest[i] = src[i];
	}
}

/*
char *convertTo16bit(RCVsys_t *RCV, int data_idx){
    uint64_t drecLen;
    uint32_t recLen = RCV->refVals.recLen;
    uint32_t npulses;
    npulses = (data_idx) ? RCV->npulses : 1;

    RAMBANK16_t *b16;
    b16 = (RAMBANK16_t *)malloc(npulses*recLen*sizeof(RAMBANK16_t));
    
    RAMBANK0_t *b0;
    RAMBANK1_t *b1;
    if(RCV->ADC->gpreg4.DFS){
        for(int m=0;m<npulses;m++){
            b0 = (RAMBANK0_t *)(&(RCV->data[data_idx][m*3*recLen*sizeof(int32_t)]));        
            b1 = (RAMBANK1_t *)(&(RCV->data[data_idx][(m*3*recLen+2*recLen)*sizeof(uint32_t)]));
            drecLen = m*recLen;
            for(int n=0;n<recLen;n++){
                b16[n+drecLen].u.ch0 = b0[n].u.ch0;
                b16[n+drecLen].u.ch1 = b0[n].u.ch1;
                b16[n+drecLen].u.ch2 = b0[n].u.ch2;
                b16[n+drecLen].u.ch3 = b0[n].u.ch3;
                b16[n+drecLen].u.ch4 = b0[n].u.ch4;
                //~ b16[n+drecLen].u.ch5 = (uint16_t)( ( (b1[n].dummy & 0xff) << 4 ) | ((uint32_t)( (b0[n].dummy & 0xf0000000 ) >> 60)) );
                b16[n+drecLen].u.ch5 = ( ( (b1[n].u.ch5hi & 0xff) << 4 ) | ( b0[n].u.ch5lo & 0xf ) ) ;
                //~ b16[n+drecLen].u.ch5 = ( ( b0[n].u.ch5lo ) ) ;
                b16[n+drecLen].u.ch6 = b1[n].u.ch6;
                b16[n+drecLen].u.ch7 = b1[n].u.ch7;
            }
        }
    } else {
        for(int m=0;m<npulses;m++){
            b0 = (RAMBANK0_t *)(&(RCV->data[data_idx][m*3*recLen*sizeof(int32_t)]));        
            b1 = (RAMBANK1_t *)(&(RCV->data[data_idx][(m*3*recLen+2*recLen)*sizeof(uint32_t)]));
            drecLen = m*recLen;
            for(int n=0;n<recLen;n++){
                b16[n+drecLen].s.ch0 = b0[n].s.ch0;
                b16[n+drecLen].s.ch1 = b0[n].s.ch1;
                b16[n+drecLen].s.ch2 = b0[n].s.ch2;
                b16[n+drecLen].s.ch3 = b0[n].s.ch3;
                b16[n+drecLen].s.ch4 = b0[n].s.ch4;
                b16[n+drecLen].s.ch5 = ( ( b1[n].s.ch5hi << 4 ) | ( b0[n].s.ch5lo ) );
                b16[n+drecLen].s.ch6 = b1[n].s.ch6;
                b16[n+drecLen].s.ch7 = b1[n].s.ch7;
            }
        }
    }
    
    return( (char *)b16 );
}
*/

int sendData(RCVsys_t *RCV, char *data, size_t dsize){
	SOCK_t *enet;
    enet = RCV->data_sock;
    
    //struct timespec st1,et1,dt1;
    
    int nsent0=0;
    int toSend=0;
    uint32_t nrecvd=0;
    uint32_t recvcount=0;
    
    //setblocking(enet->fd);
    //clock_gettime(CLOCK_MONOTONIC,&st1);
    while(nsent0<dsize){
        toSend = ( ( dsize-nsent0 ) > MAX_PACKETSIZE ) ? MAX_PACKETSIZE : ( dsize-nsent0 );
        nsent0 += send(enet->fd,&data[nsent0],toSend,MSG_CONFIRM);
        recv(enet->fd,&nrecvd,sizeof(uint32_t),MSG_WAITALL);
        recvcount+=nrecvd;
        
    //clock_gettime(CLOCK_MONOTONIC,&et1);
    //dt1 = diff(st1,et1);
    //printf("nsent: %d/%d (%d) -- time: %ld us\n",nsent0,dsize,toSend,dt1.tv_nsec/1000);
    }
    //clock_gettime(CLOCK_MONOTONIC,&et1);
    //dt1 = diff(st1,et1);
    //printf("nsent: %d (%d) -- time: %ld us\n",nsent0,dsize,dt1.tv_nsec/1000);
    //setnonblocking(enet->fd);
    return(1);
}

int saveData(SOCK_t *enet, char *data, size_t dsize){
    uint32_t dsz = dsize;
    FILE *datafile;
    remove("big_datafile");
    datafile = fopen("big_datafile","wb");
    fwrite(data,sizeof(char),dsize,datafile);
    fclose(datafile);
    if( send(enet->fd,&dsz,sizeof(int32_t),0) > 0 ){
        return(1);
    } else {
        return(0);
    }
}



void rcvQueryData(RCVsys_t *RCV){
	SOCK_t *enet;
    enet = RCV->data_sock;
    static int pulse_counter = 0;
    
    int dataStatus = 0;
    uint32_t recLen = RCV->refVals.recLen;
    uint32_t npulses = RCV->npulses;
    
    DREF32(RCV->stateReset)=1;
    usleep(1);
	
    if( RCV->queryMode.realTime ){

        dataStatus = sendData(RCV,DREFPCHAR(RCV->ramBank),8*recLen*sizeof(uint16_t));
    
    } else {
        
        rcvmemcpy( &(RCV->data[1][pulse_counter*8*recLen*sizeof(uint16_t)]), DREFPCHAR(RCV->ramBank), 8*recLen*sizeof(uint16_t));
        pulse_counter++;

        if( pulse_counter == npulses ){
            pulse_counter = 0;
            

			if ( RCV->queryMode.transferData ){
				if( !RCV->refVals.pioSettings.compressor_mode ){
					dataStatus = sendData(RCV,RCV->data[1],npulses*recLen*8*sizeof(uint16_t));
				} else {
					uint16_t *d16 = calloc(npulses*recLen*8,sizeof(uint16_t));
					
					RAMBANK12_t *d96;
					d96 = (RAMBANK12_t *) RCV->ramBank;
					for(int rl=0;rl<recLen;rl++){
						d16[rl*8+0] = d96[rl].c0;
						d16[rl*8+1] = d96[rl].c1;
						d16[rl*8+2] = d96[rl].c2l | ( d96[rl].c2u << 4 );
						d16[rl*8+3] = d96[rl].c3;
						d16[rl*8+4] = d96[rl].c4;
						d16[rl*8+5] = d96[rl].c5l | ( d96[rl].c5u << 8);
						d16[rl*8+6] = d96[rl].c6;
						d16[rl*8+7] = d96[rl].c7;
					}
					
					dataStatus = sendData(RCV,(char *)d16,npulses*recLen*8*sizeof(uint16_t));
				}
			}
			
			if ( RCV->queryMode.saveDataFile ){
				dataStatus = saveData(enet,RCV->data[1],npulses*recLen*8*sizeof(uint16_t));
			}
                
        } else {
            DREF32(RCV->stateReset)=0;
            usleep(1);
        }
    }
    DREF32(RCV->stateReset)=0;
    usleep(1);

    if ( RCV->printMsgs ){
        printf("dataStatus: %d\n",dataStatus);
    }
}



int setupInternalStorage(RCVsys_t *RCV){  
	//printf("internal storage1\n");
	//char *tmp;    
    unsigned long dsize;
    dsize = RCV->refVals.recLen*RCV->npulses*8;
	//tmp = (char *)malloc(RCV->refVals.recLen*RCV->npulses*8*sizeof(uint16_t));
	//printf("internal storage2\n");
    //printf("RCV->data[0,1] = %p,%p\n", RCV->data[0],RCV->data[1]);
	if( RCV->data[1] != NULL ){
		free( RCV->data[1] );
        usleep(1000);
        RCV->data[1] = NULL;
        usleep(1000);
	    //printf("internal storage3\n");
	} 
	
	//printf("internal storage4, sizeof(refVals) %d\n", sizeof(RCV->refVals)); 
	//RCV->data[1] = tmp;
	RCV->data[1] = (char *)malloc((dsize+1)*sizeof(uint16_t));
    //printf("RCV->data[0,1] = %p,%p\n", RCV->data[0],RCV->data[1]);
	//printf("ramBank-data[1] = %u\n",(uint32_t *)RCV->ramBank-(uint32_t *)RCV->data[1]);
    //printf("internal storage5\n"); 
    return(1);
}








