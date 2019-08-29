



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


char *convertTo16bit(ADCvars_t *ADC){
    uint64_t drecLen;
    uint32_t recLen = ADC->recLen_ref;
    uint32_t npulses = ADC->npulses;
    
    RAMBANK16_t *b16;
    b16 = (RAMBANK16_t *)malloc(npulses*recLen*sizeof(RAMBANK16_t));
    
    RAMBANK0_t *b0;
    RAMBANK1_t *b1;
    if(ADC->gpreg4.DFS){
        for(int m=0;m<npulses;m++){
            b0 = (RAMBANK0_t *)(&(ADC->data[0][m*3*recLen*sizeof(int32_t)]));        
            b1 = (RAMBANK1_t *)(&(ADC->data[0][(m*3*recLen+2*recLen)*sizeof(uint32_t)]));
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
            b0 = (RAMBANK0_t *)(&(ADC->data[0][m*3*recLen*sizeof(int32_t)]));        
            b1 = (RAMBANK1_t *)(&(ADC->data[0][(m*3*recLen+2*recLen)*sizeof(uint32_t)]));
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
        //usleep(1);
        recv(enet->fd,&nrecvd,sizeof(uint32_t),MSG_WAITALL);
        recvcount+=nrecvd;
        //printf("nrecvd: %u\n",recvcount);
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
    fwrite(ADC->data[0],sizeof(char),dsize,datafile);
    fclose(datafile);
    send(enet->fd,&dsz,sizeof(int32_t),0);
    return(1);
}


void queryData(ADCvars_t *ADC, SOCK_t *enet){
	
    static int pulsen = 0;
    
    DREF32(ADC->stateReset)=1;
    usleep(5);
	
    int dataStatus=0;
    uint32_t recLen = ADC->recLen_ref;
    uint32_t npulses = ADC->npulses;

    printf("reclen %u\n",recLen);

    if( !(ADC->queryMode.realTime) || ADC->queryMode.is16bit ){
        adcmemcpy( &(ADC->data[0][pulsen*3*recLen*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank0), 2*recLen*sizeof(uint32_t));
        adcmemcpy( &(ADC->data[0][(pulsen*3*recLen+2*recLen)*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank1), recLen*sizeof(uint32_t));
    }
    
    pulsen++;

    if( pulsen == npulses ){
        pulsen = 0;
        
        if( ADC->queryMode.is16bit ){
            char *b16c;
            b16c = convertTo16bit(ADC);
            
            if ( ADC->queryMode.transferData ){
                dataStatus = sendData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
            } else if ( ADC->queryMode.saveDataFile ){
                dataStatus = saveData(ADC,enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
            }
            
            if(dataStatus){
                free(b16c);
            }

        } else {
            if ( ADC->queryMode.transferData ){
                if( ADC->queryMode.realTime ){
                    sendData(enet,DREFPCHAR(ADC->ramBank0),2*recLen*sizeof(int32_t));
                    sendData(enet,DREFPCHAR(ADC->ramBank1),recLen*sizeof(int32_t));
                } else {
                    sendData(enet,*(ADC->data),3*recLen*pulsen*sizeof(uint32_t));
                }
            
            } else if ( ADC->queryMode.saveDataFile ){
                dataStatus = saveData(ADC,enet,*(ADC->data),3*recLen*pulsen*sizeof(uint32_t));
            }
        }
    
    } else {
        DREF32(ADC->stateReset)=0;
        usleep(5);
    }
}

/*
void queryDataSaveFile(ADCvars_t *ADC, SOCK_t *enet){
    
    DREF32(ADC->stateReset)=1;
    usleep(5);
	
	remove("data_file.dat");
    
	uint32_t recLen;
	
	recLen = ADC->recLen_ref;
	printf("reclen %u\n",recLen);
	
	RAMBANK0_t *b0 = (RAMBANK0_t *)DREFPCHAR(ADC->ramBank0);//adcData0;
	RAMBANK1_t *b1 = (RAMBANK1_t *)DREFPCHAR(ADC->ramBank1);//adcData1;
		
	FILE *datafile;
	datafile = fopen("data_file.dat","w");
	
	if(ADC->gpreg4.DFS){	
		uint16_t ch5;
		for(int j=0;j<recLen;j++){
			ch5 = ( ( b1[j].u.ch5hi << 8 ) | b0[j].u.ch5lo );
			fprintf(datafile,"%u,%u,%u,%u,%u,%u,%u,%u\n",b0[j].u.ch0,b0[j].u.ch1,b0[j].u.ch2,b0[j].u.ch3,b0[j].u.ch4,ch5,b1[j].u.ch6,b1[j].u.ch7);
		}
	} else {
		int16_t ch5;
		for(int j=0;j<recLen;j++){
			ch5 = ( ( b1[j].s.ch5hi << 8 ) | b0[j].s.ch5lo );
			fprintf(datafile,"%d,%d,%d,%d,%d,%d,%d,%d\n",b0[j].s.ch0,b0[j].s.ch1,b0[j].s.ch2,b0[j].s.ch3,b0[j].s.ch4,ch5,b1[j].s.ch6,b1[j].s.ch7);
		}
	}
	
	fclose(datafile);
    send(enet->fd,&recLen,sizeof(int32_t),0);
}


void queryDataRealTime_Send(ADCvars_t *ADC, SOCK_t *enet){
	
    DREF32(ADC->stateReset)=1;
    usleep(5);
	
    uint32_t recLen;
	recLen = ADC->recLen_ref;
	printf("reclen %u\n",recLen);

    if(ADC->queryMode.is16bit){
        ADC->npulses = 1;
        if( *(ADC->data) != NULL ){
            free(*(ADC->data));
            *(ADC->data) = NULL;
        }
        *(ADC->data) = (char *)malloc(3*recLen*sizeof(uint32_t));
        char *b16c;
        b16c = convertTo16bit(ADC);
        if(sendData(enet,b16c,recLen*sizeof(RAMBANK16_t))){
            free(b16c);
        }
    } else {
        sendData(enet,DREFPCHAR(ADC->ramBank0),2*recLen*sizeof(int32_t));
        sendData(enet,DREFPCHAR(ADC->ramBank1),recLen*sizeof(int32_t));
    }
}


void queryDataStoreLocal_Send(ADCvars_t *ADC, SOCK_t *enet){
    static int pulsen = 0;
    
    DREF32(ADC->stateReset)=1;
    usleep(5);

    int sendStatus=0;
    uint32_t recLen;
    uint32_t npulses;

	recLen = ADC->recLen_ref;
    npulses = ADC->npulses;

    adcmemcpy( &(ADC->data[0][pulsen*3*recLen*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank0), 2*recLen*sizeof(uint32_t));
    adcmemcpy( &(ADC->data[0][(pulsen*3*recLen+2*recLen)*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank1), recLen*sizeof(uint32_t));
    
    pulsen++;
    if(pulsen==npulses){
    
        if(ADC->queryMode.is16bit){
            char *b16c;
            b16c = convertTo16bit(ADC);
            printf("%p\n",b16c);
            sendStatus = sendData(enet,b16c,npulses*recLen*sizeof(RAMBANK16_t));
            printf("sendStatus: %d\n",sendStatus);
            if(sendStatus){
                free(b16c);
            }
        } else {
            sendData(enet,*(ADC->data),3*recLen*pulsen*sizeof(uint32_t));
        }

        pulsen = 0;
    } else {
        DREF32(ADC->stateReset)=0;
        usleep(5);
    }
}


void queryDataStoreLocal_Save(ADCvars_t *ADC, SOCK_t *enet){
    static int pulsen = 0;
    
    DREF32(ADC->stateReset)=1;
    usleep(5);
	
    uint32_t recLen;
	recLen = ADC->recLen_ref;
    
    adcmemcpy( &(ADC->data[0][pulsen*3*recLen*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank0), 2*recLen*sizeof(uint32_t));
    adcmemcpy( &(ADC->data[0][(pulsen*3*recLen+2*recLen)*sizeof(uint32_t)]), DREFPCHAR(ADC->ramBank1), recLen*sizeof(uint32_t));
    
    pulsen++;

    if(pulsen==ADC->npulses){
        FILE *datafile;
        if(ADC->gpreg4.DFS){	
            remove("big_datafile_u");
            datafile = fopen("big_datafile_u","wb");
        } else {
            remove("big_datafile_s");
            datafile = fopen("big_datafile_s","wb");
        }
        fwrite(ADC->data[0],sizeof(char),ADC->npulses*recLen*12,datafile);
        fclose(datafile);
        send(enet->fd,&recLen,sizeof(int32_t),0);
        
        pulsen = 0;
    } else {
        DREF32(ADC->stateReset)=0;
        usleep(5);
    }
}
*/


void adcSetGain(ADCvars_t *ADC, double gainVal){
	
	uint32_t coarseGain, fineGain;
	
    gainVal += 5.0;
    coarseGain = (uint32_t )(gainVal);
    fineGain = (uint32_t )((gainVal-floor(gainVal))*8);

	// set TGC_REG_WREN = 1 to access gain controls
	if( !(ADC->gpreg0.TGC_REGISTER_WREN) ){
		ADC->gpreg0.TGC_REGISTER_WREN = 1;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
    ADC->tgcreg0x9A.COARSE_GAIN = coarseGain;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x9A.adccmd);
    printf("ADC->tgcreg0x9A.COARSE_GAIN:\n\t0x%02x (%u) ->  gain: %g [%g]\n\n",
        ADC->tgcreg0x9A.COARSE_GAIN, ADC->tgcreg0x9A.COARSE_GAIN, gainVal-5.0, gainVal);
	
    ADC->tgcreg0x99.FINE_GAIN = fineGain;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x99.adccmd);
    printf("ADC->tgcreg0x99.FINE_GAIN:\n\t0x%02x (%u) -> gain: %g [%g]\n\n",
        ADC->tgcreg0x99.FINE_GAIN,ADC->tgcreg0x99.FINE_GAIN, gainVal-5.0,gainVal);
	
	
}

void adcSetDTypeUnsignedInt(ADCvars_t *ADC, uint32_t val){
	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	// set dtype as signed (val=0), unsigned (val=1) int
	if(val){
		ADC->gpreg4.DFS = 1;
	} else {
		ADC->gpreg4.DFS = 0;
	}	
    printf("ADC->gpreg4.DFS: (SIGNED/UNSIGNED)\n\t0x%02x\n\n",ADC->gpreg4.DFS);
	adcIssueSerialCmd(ADC,ADC->gpreg4.adccmd);
}

void adcSetLowNoiseMode(ADCvars_t *ADC, uint32_t val){
	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	if(val){
		ADC->gpreg7.VCA_LOW_NOISE_MODE = 1;
	} else {
		ADC->gpreg7.VCA_LOW_NOISE_MODE = 0;
	}
	
    printf("ADC->gpreg7.VCA_LOW_NOISE_MODE:\n\t0x%02x\n\n",ADC->gpreg7.VCA_LOW_NOISE_MODE);
	adcIssueSerialCmd(ADC,ADC->gpreg7.adccmd);
}

void adcToggleChannelPwr(ADCvars_t *ADC, uint8_t pwrdOn){
	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg1.PDN_CHANNEL = pwrdOn;	
    printf("ADC->gpreg1.PDN_CHANNEL:\n\t0x%02x\n",ADC->gpreg1.PDN_CHANNEL);
    printf(
        "\n\tPDN_CHANNEL<1,...,8> = <%u, %u, %u, %u, %u, %u, %u, %u>\n", 
        ADC->gpreg1.PDN_CHANNEL1, ADC->gpreg1.PDN_CHANNEL2, 
        ADC->gpreg1.PDN_CHANNEL3, ADC->gpreg1.PDN_CHANNEL4, 
        ADC->gpreg1.PDN_CHANNEL5, ADC->gpreg1.PDN_CHANNEL6, 
        ADC->gpreg1.PDN_CHANNEL7, ADC->gpreg1.PDN_CHANNEL8);
	adcIssueSerialCmd(ADC,ADC->gpreg1.adccmd);
}

void adcSetFilterBW(ADCvars_t *ADC, uint8_t filter){	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.FILTER_BW = filter;
    printf("ADC->gpreg7.FILTER_BW:\n\t0x%02x\n\n",ADC->gpreg7.FILTER_BW);
	adcIssueSerialCmd(ADC,ADC->gpreg7.adccmd);
}

void adcSetInternalAcCoupling(ADCvars_t *ADC, uint8_t accoupling){	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.INTERNAL_AC_COUPLING = accoupling;
    printf("ADC->gpreg7.INTERNAL_AC_COUPLING:\n\t0x%02x\n\n",ADC->gpreg7.INTERNAL_AC_COUPLING);
	adcIssueSerialCmd(ADC,ADC->gpreg7.adccmd);
}

void adcSetReg1(ADCvars_t *ADC){
	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg1.GLOBAL_PDN = 0;
	ADC->gpreg1.OUTPUT_DISABLE = 0;
	ADC->gpreg1.PDN_CHANNEL = 0x00;
	ADC->gpreg1.STDBY = 0;
	ADC->gpreg1.LOW_FREQUENCY_NOISE_SUPRESSION = 0;
	ADC->gpreg1.EXTERNAL_REFERENCE = 0;
	ADC->gpreg1.OUTPUT_RATE_2X = 0;

    printf("ADC->gpreg1.adccmd:\n\t0x%02x_%02x_%04x (BLNK_ADDR_CMD)\n\n",
        ADC->gpreg1.blnk,ADC->gpreg1.addr,ADC->gpreg1.cmd);
	adcIssueSerialCmd(ADC,ADC->gpreg1.adccmd);
}

void adcSetReg7(ADCvars_t *ADC){

	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.VCA_LOW_NOISE_MODE = 0;
	ADC->gpreg7.FILTER_BW = 0x00;
	ADC->gpreg7.INTERNAL_AC_COUPLING = 1;
	
    printf("ADC->gpreg7.adccmd:\n\t0x%02x_%02x_%04x (BLNK_ADDR_CMD)\n\n"
        ,ADC->gpreg7.blnk,ADC->gpreg7.addr,ADC->gpreg7.cmd);
	adcIssueSerialCmd(ADC,ADC->gpreg7.adccmd);
}

void adcIssueDirectCmd(ADCvars_t *ADC, FMSG_t *msg){
    uint32_t regaddr;
    if( msg->u[1] ){
        ADC->gpreg0.TGC_REGISTER_WREN = 1;
        adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);

        regaddr = ADC->reg_dict[1][msg->u[2]];
        ADC->reg[regaddr]->cmd = msg->u[3];
        adcIssueSerialCmd(ADC,ADC->reg[regaddr]->adccmd);
    } else {
        ADC->gpreg0.TGC_REGISTER_WREN = 0;
        adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
    
        regaddr = ADC->reg_dict[0][msg->u[2]];
        ADC->reg[regaddr]->cmd = msg->u[3];
        adcIssueSerialCmd(ADC,ADC->reg[regaddr]->adccmd);
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
            adcSetGain(ADC,msg->d[1]);
            break;
        }
        case(CASE_ADC_SET_UNSIGNED):{
            adcSetDTypeUnsignedInt(ADC, msg->u[1]);
            break;
        }
        case(CASE_ADC_SET_LOW_NOISE_MODE):{
            adcSetLowNoiseMode(ADC, msg->u[1]);
            break;
        }
        case(CASE_ADC_TOGGLE_CHANNEL_POWER):{
            adcToggleChannelPwr(ADC,msg->u[1]);
            break;
        }
        case(CASE_ADC_SET_FILTER_BW):{
            adcSetFilterBW(ADC,msg->u[1]);
            break;
        }
        case(CASE_ADC_SET_INTERNAL_AC_COUPLING):{
            adcSetInternalAcCoupling(ADC,msg->u[1]);
            break;
        }
        case(CASE_ADC_ISSUE_DIRECT_CMD):{
            adcIssueDirectCmd(ADC, msg);
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
                if( *(ADC->data) != NULL ){
                    free(*(ADC->data));
                    *(ADC->data) = NULL;
                }
                *(ADC->data) = (char *)malloc(3*ADC->recLen_ref*msg->u[1]*sizeof(uint32_t));
            } else {
                ADC->npulses = 1;
                if ( *(ADC->data) != NULL ){
                    free(*(ADC->data));
                    *(ADC->data) = NULL;
                }
                *(ADC->data) = (char *)malloc(3*MAX_RECLEN*sizeof(uint32_t));
            }
            break;
        }
        case(CASE_ADC_SET_DEFAULT_SETTINGS):{
            adcSetDefaultSettings(ADC);
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

