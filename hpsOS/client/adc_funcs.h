



void adcmemcpy( char *dest, char *src, size_t nbytes){
	for(int i=0;i<nbytes;i++){
		dest[i] = src[i];
	}
}


void setRecLen(ADCvars_t *ADC, uint32_t recLen){
	DREF32(ADC->recLen) = recLen;
    ADC->recLen_ref = recLen;
	usleep(5);
}


void setPioVarGain(ADCvars_t *ADC, uint32_t val){
	DREF32(ADC->pioVarGain) = val & 0x03;
    usleep(5);
}


void setLEDS(ADCvars_t *ADC, uint32_t val){
    LED_t led;
    led.vals = ( val & 0x1f );
    led.hiRest = ~led.hiRest;
    DREF32(ADC->leds) = led.vals;
    usleep(5);
}


void cycleLEDS(ADCvars_t *ADC, int ncycles){
	
	uint32_t tmp = 0;
	LED_t led;
	led.vals=0;
	
	while( tmp<ncycles ){
		led.vals = ( tmp & 0x1F );
		led.hiRest = ~led.hiRest;
		DREF32(ADC->leds) = led.vals;
		usleep(100000);
		tmp++;
	}
}


void adcSync(ADCvars_t *ADC){
	DREF32(ADC->controlComms) = ADC_SYNC_COMMAND;
	usleep(5);
}


char *convertTo16bit(ADCvars_t *ADC, int data_idx){
    uint64_t drecLen;
    uint32_t recLen = ADC->recLen_ref;
    uint32_t npulses;
    npulses = (data_idx) ? ADC->npulses : 1;

    RAMBANK16_t *b16;
    b16 = (RAMBANK16_t *)malloc(npulses*recLen*sizeof(RAMBANK16_t));
    
    RAMBANK0_t *b0;
    RAMBANK1_t *b1;
    if(ADC->gpreg4.DFS){
        for(int m=0;m<npulses;m++){
            b0 = (RAMBANK0_t *)(&(ADC->data[data_idx][m*3*recLen*sizeof(int32_t)]));        
            b1 = (RAMBANK1_t *)(&(ADC->data[data_idx][(m*3*recLen+2*recLen)*sizeof(uint32_t)]));
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
            b0 = (RAMBANK0_t *)(&(ADC->data[data_idx][m*3*recLen*sizeof(int32_t)]));        
            b1 = (RAMBANK1_t *)(&(ADC->data[data_idx][(m*3*recLen+2*recLen)*sizeof(uint32_t)]));
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


int saveData(ADCvars_t *ADC, SOCK_t *enet, char *data, size_t dsize){
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


void queryData(ADCvars_t *ADC, SOCK_t *enet){
	
    static int pulse_counter = 0;
    
    int dataStatus=0;
    uint32_t recLen = ADC->recLen_ref;
    uint32_t npulses = ADC->npulses;
    
    DREF32(ADC->stateReset)=1;
    usleep(5);
	
    if( ADC->queryMode.realTime ){
        if( ADC->queryMode.is16bit ){
            adcmemcpy( &(ADC->data[0][0]), DREFPCHAR(ADC->ramBank0), 2*recLen*sizeof(uint32_t));
            adcmemcpy( &(ADC->data[0][2*recLen*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank1), recLen*sizeof(uint32_t));
            char *b16c;
            b16c = convertTo16bit(ADC,0);
            dataStatus = sendData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
            if(dataStatus){
                free(b16c);
            }
        } else {
            dataStatus = sendData(enet,DREFPCHAR(ADC->ramBank0),2*recLen*sizeof(int32_t));
            dataStatus = sendData(enet,DREFPCHAR(ADC->ramBank1),recLen*sizeof(int32_t));
        }
    } else {
        
        adcmemcpy( &(ADC->data[1][pulse_counter*3*recLen*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank0), 2*recLen*sizeof(uint32_t));
        adcmemcpy( &(ADC->data[1][(pulse_counter*3*recLen+2*recLen)*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank1), recLen*sizeof(uint32_t));
        pulse_counter++;

        if( pulse_counter == npulses ){
            pulse_counter = 0;
            
            if( ADC->queryMode.is16bit ){
                char *b16c;
                b16c = convertTo16bit(ADC,1);
                
                if ( ADC->queryMode.transferData ){
                    dataStatus = sendData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
                }
                
                if ( ADC->queryMode.saveDataFile ){
                    dataStatus = saveData(ADC,enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
                }
                
                if(dataStatus){
                    free(b16c);
                }

            } else {
                if ( ADC->queryMode.transferData ){
                    dataStatus = sendData(enet,ADC->data[1],3*recLen*npulses*sizeof(uint32_t));
                } 
                
                if ( ADC->queryMode.saveDataFile ){
                    dataStatus = saveData(ADC,enet,ADC->data[1],3*recLen*npulses*sizeof(uint32_t));
                }
            }
    
        } else {
            DREF32(ADC->stateReset)=0;
            usleep(5);
        }
    }
}


void adcIssueSerialCmd(ADCvars_t *ADC, uint32_t cmd){
	DREF32(ADC->serialCommand) = cmd;
	usleep(5);
	DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(5);
	DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(5);
}

void adcSetDefaultSettings(ADCvars_t *ADC){
	
	DREF32(ADC->controlComms) = ADC_HARDWARE_RESET;
	usleep(100000);
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(10);
	
	ADC->gpreg0.SOFTWARE_RESET = 1;
	ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	usleep(100000);
	
	ADC->gpreg0.SOFTWARE_RESET = 0;
	ADC->gpreg0.TGC_REGISTER_WREN = 1;
	ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);

	// WITHOUT INTERP_ENABLE=1, THIS ONLY ALLOWS COARSE_GAIN TO BE SET
	ADC->tgcreg0x99.STATIC_PGA = 1;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x99.adccmd);
	
	// NOT IN MANUAL!!!, NEEDED FOR FINE GAIN CONTROL
	ADC->tgcreg0x97.INTERP_ENABLE = 1;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x97.adccmd);
    
    // GO TO GENERAL PURPOSE REGISTERS
	ADC->gpreg0.TGC_REGISTER_WREN = 0;
	ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);

    // SET TO DC COUPLING (VARIABE NAME IS MISLEADING/BACKWARDS)
    ADC->gpreg7.INTERNAL_AC_COUPLING = 1;
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);

    // SET DATA TYPE TO UNSIGNED
    ADC->gpreg4.DFS = 1;
    ADC->issueSerialCommand(ADC,ADC->gpreg4.adccmd);
    
    // DISABLE THE CLAMP
    ADC->gpreg70.CLAMP_DISABLE = 1;
    ADC->issueSerialCommand(ADC,ADC->gpreg70.adccmd);
}

void adcSetGain(ADCvars_t *ADC, double gainVal){
	
	uint32_t coarseGain, fineGain;
	
    gainVal += 5.0;
    coarseGain = (uint32_t )(gainVal);
    fineGain = (uint32_t )((gainVal-floor(gainVal))*8);

	// set TGC_REG_WREN = 1 to access gain controls
	if( !(ADC->gpreg0.TGC_REGISTER_WREN) ){
		ADC->gpreg0.TGC_REGISTER_WREN = 1;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
    ADC->tgcreg0x9A.COARSE_GAIN = coarseGain;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x9A.adccmd);
    printf("ADC->tgcreg0x9A.COARSE_GAIN:\n\t0x%02x (%u) ->  gain: %g [%g]\n\n",
        ADC->tgcreg0x9A.COARSE_GAIN, ADC->tgcreg0x9A.COARSE_GAIN, gainVal-5.0, gainVal);
	
    ADC->tgcreg0x99.FINE_GAIN = fineGain;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x99.adccmd);
    printf("ADC->tgcreg0x99.FINE_GAIN:\n\t0x%02x (%u) -> gain: %g [%g]\n\n",
        ADC->tgcreg0x99.FINE_GAIN,ADC->tgcreg0x99.FINE_GAIN, gainVal-5.0,gainVal);
	
	
}

void adcSetUnsignedInt(ADCvars_t *ADC, uint32_t val){
	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	// set dtype as signed (val=0), unsigned (val=1) int
	if(val){
		ADC->gpreg4.DFS = 1;
	} else {
		ADC->gpreg4.DFS = 0;
	}	
    printf("ADC->gpreg4.DFS: (SIGNED/UNSIGNED)\n\t0x%02x\n\n",ADC->gpreg4.DFS);
	ADC->issueSerialCommand(ADC,ADC->gpreg4.adccmd);
}

void adcSetLowNoiseMode(ADCvars_t *ADC, uint32_t val){
	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	if(val){
		ADC->gpreg7.VCA_LOW_NOISE_MODE = 1;
	} else {
		ADC->gpreg7.VCA_LOW_NOISE_MODE = 0;
	}
	
    printf("ADC->gpreg7.VCA_LOW_NOISE_MODE:\n\t0x%02x\n\n",ADC->gpreg7.VCA_LOW_NOISE_MODE);
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);
}

void adcToggleChannelPwr(ADCvars_t *ADC, uint32_t pwrOn){
	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg1.PDN_CHANNEL = pwrOn;	
    printf("ADC->gpreg1.PDN_CHANNEL:\n\t0x%02x\n",ADC->gpreg1.PDN_CHANNEL);
    printf(
        "\n\tPDN_CHANNEL<1,...,8> = <%u, %u, %u, %u, %u, %u, %u, %u>\n", 
        ADC->gpreg1.PDN_CHANNEL1, ADC->gpreg1.PDN_CHANNEL2, 
        ADC->gpreg1.PDN_CHANNEL3, ADC->gpreg1.PDN_CHANNEL4, 
        ADC->gpreg1.PDN_CHANNEL5, ADC->gpreg1.PDN_CHANNEL6, 
        ADC->gpreg1.PDN_CHANNEL7, ADC->gpreg1.PDN_CHANNEL8);
	ADC->issueSerialCommand(ADC,ADC->gpreg1.adccmd);
}

void adcSetFilterBW(ADCvars_t *ADC, uint32_t filter){	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.FILTER_BW = filter;
    printf("ADC->gpreg7.FILTER_BW:\n\t0x%02x\n\n",ADC->gpreg7.FILTER_BW);
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);
}

void adcSetInternalAcCoupling(ADCvars_t *ADC, uint32_t accoupling){	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.INTERNAL_AC_COUPLING = accoupling;
    printf("ADC->gpreg7.INTERNAL_AC_COUPLING:\n\t0x%02x\n\n",ADC->gpreg7.INTERNAL_AC_COUPLING);
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);
}

void adcIssueDirectCmd(ADCvars_t *ADC, FMSG_t *msg){
    uint32_t regaddr;
    if( msg->u[1] ){
        ADC->gpreg0.TGC_REGISTER_WREN = 1;
        ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);

        regaddr = ADC->reg_dict[1][msg->u[2]];
        ADC->reg[regaddr]->cmd = msg->u[3];
        ADC->issueSerialCommand(ADC,ADC->reg[regaddr]->adccmd);
    } else {
        ADC->gpreg0.TGC_REGISTER_WREN = 0;
        ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
    
        regaddr = ADC->reg_dict[0][msg->u[2]];
        ADC->reg[regaddr]->cmd = msg->u[3];
        ADC->issueSerialCommand(ADC,ADC->reg[regaddr]->adccmd);
    }
}


void recvSysMsgHandler(POLLserver_t *PS, ADCvars_t *ADC, FMSG_t *msg, int *runner){

    switch(msg->u[0]){
        case(CASE_SET_RECLEN):{
            setRecLen(ADC,msg->u[1]);
            break;
        }
        case(CASE_SET_PIO_VAR_GAIN):{
            setPioVarGain(ADC,msg->u[1]);
            break;
        }
        case(CASE_SET_LEDS):{
            setLEDS(ADC,msg->u[1]);
            break;
        }
        case(CASE_QUERY_DATA):{
            DREF32(ADC->stateReset)=1;
            usleep(5);
            DREF32(ADC->stateReset)=0;
            usleep(5);
            break;
        }
        case(CASE_ADC_SET_GAIN):{
            ADC->setGain(ADC,msg->d[1]);
            break;
        }
        case(CASE_ADC_SET_UNSIGNED):{
            ADC->setUnsignedInt(ADC, msg->u[1]);
            break;
        }
        case(CASE_ADC_SET_LOW_NOISE_MODE):{
            ADC->setLowNoiseMode(ADC, msg->u[1]);
            break;
        }
        case(CASE_ADC_TOGGLE_CHANNEL_POWER):{
            ADC->toggleChannelPower(ADC,msg->u[1]);
            break;
        }
        case(CASE_ADC_SET_FILTER_BW):{
            ADC->setFilterBW(ADC,msg->u[1]);
            break;
        }
        case(CASE_ADC_SET_INTERNAL_AC_COUPLING):{
            ADC->setInternalAcCoupling(ADC,msg->u[1]);
            break;
        }
        case(CASE_ADC_ISSUE_DIRECT_CMD):{
            ADC->issueDirectCommand(ADC, msg);
            break;
        }
        case(CASE_CONNECT_INTERRUPT):{
            if(msg->u[1] && ( ADC->interrupt.ps == NULL ) ){
                connectPollInterrupter(PS,ADC,"gpio@0x100000000");
            } else if ( !msg->u[1] && ( ADC->interrupt.ps != NULL ) ){
                disconnectPollSock(&(ADC->interrupt));
            }
            break;
        }
        case(CASE_SET_QUERY_MODE):{
            ADC->queryMode.all = 0;
            
            if(msg->u[1]){
                ADC->queryMode.realTime = 1;
            } else {
                ADC->queryMode.realTime = 0;
            }
            
            if(msg->u[2]){
                ADC->queryMode.transferData = 1;
            } else {
                ADC->queryMode.transferData = 0;
            }

            if(msg->u[3]){
                ADC->queryMode.saveDataFile = 1;
            } else {
                ADC->queryMode.saveDataFile = 0;
            }

            if(msg->u[4]){
                ADC->queryMode.is16bit = 1;
            } else {
                ADC->queryMode.is16bit = 0;
            }
            printf("queryMode: rt %d, td %d, sdf %d, -- 16bit: %d\n",ADC->queryMode.realTime,ADC->queryMode.transferData,ADC->queryMode.saveDataFile,ADC->queryMode.is16bit);
            break;
        }
        case(CASE_SETUP_LOCAL_STORAGE):{
            if( msg->u[1] ){
                ADC->npulses = msg->u[1];
                if( ADC->data[1] != NULL ){
                    free( ADC->data[1] );
                    ADC->data[1] = NULL;
                }
                ADC->data[1] = (char *)malloc(3*ADC->recLen_ref*msg->u[1]*sizeof(uint32_t));
            } else {
                ADC->npulses = 1;
                if ( ADC->data[1] != NULL ){
                    free(ADC->data[1]);
                    ADC->data[1] = NULL;
                }
                ADC->data[1] = (char *)malloc(3*MAX_RECLEN*sizeof(uint32_t));
            }
            break;
        }
        case(CASE_ADC_SET_DEFAULT_SETTINGS):{
            ADC->setDefaultSettings(ADC);
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

