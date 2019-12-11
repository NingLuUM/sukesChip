


void rcvSetRecLen(RCVsys_t *RCV, uint32_t recLen){
	if(recLen){
		DREF32(RCV->recLen) = recLen;
		RCV->recLen_ref = recLen;
	} else {
		DREF32(RCV->recLen) = 1;
		RCV->recLen_ref = 1;
	}
	usleep(5);
	if(RCV->pioSettings_ref.compressor_mode && (RCV->recLen_ref>1) ){
		DREF32(RCV->recLen)= (RCV->recLen_ref*3)/4;
	} 
	usleep(5);
}

void rcvSetPioVarGain(RCVsys_t *RCV, uint32_t val){
	RCV->pioSettings_ref.varGain = val;
	DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;//0x03
    usleep(5);
}

void rcvSetClkDivisor(RCVsys_t *RCV, uint32_t val){
	RCV->pioSettings_ref.clkDiv = val;
	DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;//0x03
    usleep(5);
}

void rcvSetSamplingMode(RCVsys_t *RCV, uint32_t val){
	RCV->pioSettings_ref.sampling_mode = val;
	DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;//0x03
    usleep(5);
}

void rcvSetCompressorMode(RCVsys_t *RCV, uint32_t val){
	RCV->pioSettings_ref.compressor_mode = val;
	DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;//0x03
	
	if( RCV->pioSettings_ref.compressor_mode && (DREF32(RCV->recLen) == RCV->recLen_ref ) && ( RCV->recLen_ref>1 ) ){
		DREF32(RCV->recLen)= (RCV->recLen_ref*3)/4;
	} else if ( (RCV->pioSettings_ref.compressor_mode==0) && (DREF32(RCV->recLen) != RCV->recLen_ref ) ){
		DREF32(RCV->recLen)= RCV->recLen_ref;
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
        dataStatus = sendData(enet,DREFPCHAR(RCV->ramBank),8*recLen*sizeof(uint16_t));
            
    } else {
        
        rcvmemcpy( &(RCV->data[1][pulse_counter*8*recLen*sizeof(uint16_t)]), DREFPCHAR(RCV->ramBank), 8*recLen*sizeof(uint16_t));
        pulse_counter++;

        if( pulse_counter == npulses ){
            pulse_counter = 0;
            

			if ( RCV->queryMode.transferData ){
				if( !RCV->pioSettings_ref.compressor_mode ){
					dataStatus = sendData(enet,RCV->data[1],npulses*recLen*8*sizeof(uint16_t));
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
					
					dataStatus = sendData(enet,(char *)d16,npulses*recLen*8*sizeof(uint16_t));
				}
			}
			
			if ( RCV->queryMode.saveDataFile ){
				dataStatus = saveData(enet,RCV->data[1],npulses*recLen*8*sizeof(uint16_t));
			}
                
        } else {
            DREF32(RCV->stateReset)=0;
            usleep(5);
        }
    }
    DREF32(RCV->stateReset)=0;
    usleep(5);
}



void setupInternalStorage(RCVsys_t *RCV){  
	printf("internal storage1\n");
	char *tmp;    
	
	tmp = (char *)malloc(RCV->recLen_ref*RCV->npulses*8*sizeof(uint16_t));
	printf("internal storage2\n");
	if( RCV->data[1] != NULL ){
		free( RCV->data[1] );
		printf("internal storage3\n");
	} 
	
	printf("internal storage4\n"); 
	RCV->data[1] = tmp;
}


void recvSysMsgHandler(POLLserver_t *PS, RCVsys_t *RCV, FMSG_t *msg, int *runner){

    switch(msg->u[0]){
        case(CASE_RCV_SET_RECLEN):{
            RCV->setRecLen(RCV,msg->u[1]);
            if( msg->u[2] ){
				setupInternalStorage(RCV);
			}
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("reclen set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_PIO_VAR_GAIN):{
            RCV->setPioVarGain(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("pio gain set successfully\n");
            }
            break;
        }
        
        case(CASE_SET_LEDS):{
            RCV->setLEDs(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("comm leds set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_STATE_RESET):{
            DREF32(RCV->stateReset) = 1;
            usleep(5);
            DREF32(RCV->stateReset) = msg->u[1];
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("state reset set successfully\n");
            }
            break;
        }

        case(CASE_ADC_SET_GAIN):{
            RCV->ADC->setGain(RCV->ADC,msg->d[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc gain set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_UNSIGNED):{
            RCV->ADC->setUnsignedInt(RCV->ADC, msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc unsigned mode set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_LOW_NOISE_MODE):{
            RCV->ADC->setLowNoiseMode(RCV->ADC, msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc low noise mode cmd set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_TOGGLE_CHANNEL_POWER):{
            RCV->ADC->toggleChannelPower(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc channel power set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_FILTER_BW):{
            RCV->ADC->setFilterBW(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc filter bandwidth set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_INTERNAL_AC_COUPLING):{
            RCV->ADC->setInternalAcCoupling(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc internal ac coupling cmd set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_ISSUE_DIRECT_CMD):{
            RCV->ADC->issueDirectCommand(RCV->ADC, msg);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc issue direct command set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_CONNECT_INTERRUPT):{
            if(msg->u[1] && ( RCV->interrupt.ps == NULL ) ){
                connectPollInterrupter(PS,&(RCV->interrupt),"gpio@0x100000000",RCV_INTERRUPT_ID);
            } else if ( !msg->u[1] && ( RCV->interrupt.ps != NULL ) ){
                disconnectPollSock(&(RCV->interrupt));
            }
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv interrupt connected successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_QUERY_MODE):{
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
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv query mode set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_NPULSES):{
            if( msg->u[1] ){
                RCV->npulses = msg->u[1];
            } else {
                RCV->npulses = 1;
            }
            if( msg->u[2] ){
				setupInternalStorage(RCV);
			}
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv npulses set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SETUP_LOCAL_STORAGE):{
            setupInternalStorage(RCV);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv internal storage set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_DEFAULT_SETTINGS):{
            RCV->ADC->setDefaultSettings(RCV->ADC);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc defaults set successfully\n");
            }
            break;
        }
        
        case(CASE_UPDATE_AUTO_SHUTDOWN_SETTING):{
            if(msg->u[1]){
                g_auto_shutdown_enabled = 1;
            } else {
                g_auto_shutdown_enabled = 0;
            }
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("system auto shutdown settings updated successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_CLOCK_DIVISOR):{
            RCV->setClkDivisor(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv sys clock divisor set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_SAMPLING_MODE):{
            RCV->setSamplingMode(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv sys sampling mode set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_COMPRESSOR_MODE):{
            RCV->setCompressorMode(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv sys compressor mode set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_INTERRUPT_THYSELF):{
            RCV->pioSettings_ref.interrupt_thyself = msg->u[1];
            DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("self interrupt set successfully\n");
            }
            break;
        }
        
        case(CASE_EXIT_PROGRAM):{
            *runner=0;
            break;
        }
       
		case(CASE_ADC_SET_FCLOCK_DELAY):{
			RCV->pioSettings_ref.fclk_delay = msg->u[1];
			DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("adc fclock delay set successfully\n");
            }
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

