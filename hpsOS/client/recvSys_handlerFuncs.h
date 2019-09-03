


void rcvSetRecLen(RCVsys_t *RCV, uint32_t recLen){
	DREF32(RCV->recLen) = recLen;
    RCV->recLen_ref = recLen;
	usleep(5);
}

void rcvSetPioVarGain(RCVsys_t *RCV, uint32_t val){
	DREF32(RCV->pioVarGain) = val & 0x03;
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

char *convertTo16bit(RCVsys_t *RCV, int data_idx){
    uint64_t drecLen;
    uint32_t recLen = RCV->recLen_ref;
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
                b16[n+drecLen].u.ch5 = ( ( b1[n].u.ch5hi << 4 ) | b0[n].u.ch5lo );
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
                b16[n+drecLen].s.ch5 = ( ( b1[n].s.ch5hi << 4 ) | b0[n].s.ch5lo );
                b16[n+drecLen].s.ch6 = b1[n].s.ch6;
                b16[n+drecLen].s.ch7 = b1[n].s.ch7;
            }
        }
    }
    
    return( (char *)b16 );
}

int sendData(SOCK_t *enet, char *data, size_t dsize){

    struct timespec st1,et1,dt1;
    
    int nsent0=0;
    int toSend=0;
    uint32_t nrecvd=0;
    uint32_t recvcount=0;

    //setblocking(enet->fd);
    clock_gettime(CLOCK_MONOTONIC,&st1);
    while(nsent0<dsize){
        toSend = ( ( dsize-nsent0 ) > MAX_PACKETSIZE ) ? MAX_PACKETSIZE : ( dsize-nsent0 );
        nsent0 += send(enet->fd,&data[nsent0],toSend,MSG_CONFIRM);
        recv(enet->fd,&nrecvd,sizeof(uint32_t),MSG_WAITALL);
        recvcount+=nrecvd;
    }
    clock_gettime(CLOCK_MONOTONIC,&et1);
    dt1 = diff(st1,et1);
    printf("nsent: %d (%d) -- time: %ld us\n",nsent0,dsize,dt1.tv_nsec/1000);
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


void queryData(RCVsys_t *RCV, SOCK_t *enet){
	
    static int pulse_counter = 0;
    
    int dataStatus=0;
    uint32_t recLen = RCV->recLen_ref;
    uint32_t npulses = RCV->npulses;
    
    DREF32(RCV->stateReset)=1;
    usleep(5);
	
    if( RCV->queryMode.realTime ){
        if( RCV->queryMode.is16bit ){
            rcvmemcpy( &(RCV->data[0][0]), DREFPCHAR(RCV->ramBank0), 2*recLen*sizeof(uint32_t));
            rcvmemcpy( &(RCV->data[0][2*recLen*sizeof(uint32_t)]), DREFPCHAR(RCV->ramBank1), recLen*sizeof(uint32_t));
            char *b16c;
            b16c = convertTo16bit(RCV,0);
            dataStatus = sendData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
            if(dataStatus){
                free(b16c);
            }
        } else {
            dataStatus = sendData(enet,DREFPCHAR(RCV->ramBank0),2*recLen*sizeof(int32_t));
            dataStatus = sendData(enet,DREFPCHAR(RCV->ramBank1),recLen*sizeof(int32_t));
        }
    } else {
        
        rcvmemcpy( &(RCV->data[1][pulse_counter*3*recLen*sizeof(uint32_t)]), DREFPCHAR(RCV->ramBank0), 2*recLen*sizeof(uint32_t));
        rcvmemcpy( &(RCV->data[1][(pulse_counter*3*recLen+2*recLen)*sizeof(uint32_t)]), DREFPCHAR(RCV->ramBank1), recLen*sizeof(uint32_t));
        pulse_counter++;

        if( pulse_counter == npulses ){
            pulse_counter = 0;
            
            if( RCV->queryMode.is16bit ){
                char *b16c;
                b16c = convertTo16bit(RCV,1);
                
                if ( RCV->queryMode.transferData ){
                    dataStatus = sendData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
                }
                
                if ( RCV->queryMode.saveDataFile ){
                    dataStatus = saveData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
                }
                
                if(dataStatus){
                    free(b16c);
                }

            } else {
                if ( RCV->queryMode.transferData ){
                    dataStatus = sendData(enet,RCV->data[1],3*recLen*npulses*sizeof(uint32_t));
                } 
                
                if ( RCV->queryMode.saveDataFile ){
                    dataStatus = saveData(enet,RCV->data[1],3*recLen*npulses*sizeof(uint32_t));
                }
            }
    
        } else {
            DREF32(RCV->stateReset)=0;
            usleep(5);
        }
    }
}


void recvSysMsgHandler(POLLserver_t *PS, RCVsys_t *RCV, FMSG_t *msg, int *runner){

    switch(msg->u[0]){
        case(CASE_SET_RECLEN):{
            RCV->setRecLen(RCV,msg->u[1]);
            break;
        }
        
        case(CASE_SET_PIO_VAR_GAIN):{
            RCV->setPioVarGain(RCV,msg->u[1]);
            break;
        }
        
        case(CASE_SET_LEDS):{
            RCV->setLEDs(RCV,msg->u[1]);
            break;
        }
        
        case(CASE_QUERY_DATA):{
            DREF32(RCV->stateReset)=1;
            usleep(5);
            DREF32(RCV->stateReset)=0;
            usleep(5);
            break;
        }
        
        case(CASE_ADC_SET_GAIN):{
            RCV->ADC->setGain(RCV->ADC,msg->d[1]);
            break;
        }
        
        case(CASE_ADC_SET_UNSIGNED):{
            RCV->ADC->setUnsignedInt(RCV->ADC, msg->u[1]);
            break;
        }
        
        case(CASE_ADC_SET_LOW_NOISE_MODE):{
            RCV->ADC->setLowNoiseMode(RCV->ADC, msg->u[1]);
            break;
        }
        
        case(CASE_ADC_TOGGLE_CHANNEL_POWER):{
            RCV->ADC->toggleChannelPower(RCV->ADC,msg->u[1]);
            break;
        }
        
        case(CASE_ADC_SET_FILTER_BW):{
            RCV->ADC->setFilterBW(RCV->ADC,msg->u[1]);
            break;
        }
        
        case(CASE_ADC_SET_INTERNAL_AC_COUPLING):{
            RCV->ADC->setInternalAcCoupling(RCV->ADC,msg->u[1]);
            break;
        }
        
        case(CASE_ADC_ISSUE_DIRECT_CMD):{
            RCV->ADC->issueDirectCommand(RCV->ADC, msg);
            break;
        }
        
        case(CASE_CONNECT_INTERRUPT):{
            if(msg->u[1] && ( RCV->interrupt.ps == NULL ) ){
                connectPollInterrupter(PS,&(RCV->interrupt),"gpio@0x100000000");
            } else if ( !msg->u[1] && ( RCV->interrupt.ps != NULL ) ){
                disconnectPollSock(&(RCV->interrupt));
            }
            break;
        }
        
        case(CASE_SET_QUERY_MODE):{
            RCV->queryMode.all = 0;
            
            if(msg->u[1]){
                RCV->queryMode.realTime = 1;
            } else {
                RCV->queryMode.realTime = 0;
            }
            
            if(msg->u[2]){
                RCV->queryMode.transferData = 1;
            } else {
                RCV->queryMode.transferData = 0;
            }

            if(msg->u[3]){
                RCV->queryMode.saveDataFile = 1;
            } else {
                RCV->queryMode.saveDataFile = 0;
            }

            if(msg->u[4]){
                RCV->queryMode.is16bit = 1;
            } else {
                RCV->queryMode.is16bit = 0;
            }
            printf("queryMode: rt %d, td %d, sdf %d, -- 16bit: %d\n",RCV->queryMode.realTime,RCV->queryMode.transferData,RCV->queryMode.saveDataFile,RCV->queryMode.is16bit);
            break;
        }
        
        case(CASE_SETUP_LOCAL_STORAGE):{
            if( msg->u[1] ){
                RCV->npulses = msg->u[1];
                if( RCV->data[1] != NULL ){
                    free( RCV->data[1] );
                    RCV->data[1] = NULL;
                }
                RCV->data[1] = (char *)malloc(3*RCV->recLen_ref*msg->u[1]*sizeof(uint32_t));
            } else {
                RCV->npulses = 1;
                if ( RCV->data[1] != NULL ){
                    free(RCV->data[1]);
                    RCV->data[1] = NULL;
                }
                RCV->data[1] = (char *)malloc(3*MAX_RECLEN*sizeof(uint32_t));
            }
            break;
        }
        
        case(CASE_ADC_SET_DEFAULT_SETTINGS):{
            RCV->ADC->setDefaultSettings(RCV->ADC);
            break;
        }
        
        case(CASE_UPDATE_AUTO_SHUTDOWN_SETTING):{
            if(msg->u[1]){
                g_auto_shutdown_enabled = 1;
            } else {
                g_auto_shutdown_enabled = 0;
            }
            break;
        }
        
        case(CASE_EXIT_PROGRAM):{
            *runner=0;
            break;
        }
        
        default:{
            *runner=0;
            break;
        }
    }
}


void controlMsgHandler(POLLserver_t *PS, FMSG_t *msg, int *runner){

    switch(msg->u[0]){
        case(CASE_ADD_DATA_SOCKET):{
            break;
        }
        
        default:{
            break;
        }
    }
}
