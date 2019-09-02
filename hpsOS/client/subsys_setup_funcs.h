


struct timespec diff(struct timespec start, struct timespec end){
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
    
    /* timespec usage example
        clock_gettime(CLOCK_MONOTONIC, &gstart);
        something();
        clock_gettime(CLOCK_MONOTONIC, &gend);
        difftime = diff(gstart,gend);
        printf("adcmemcpy: %ld us\n",difftime.tv_nsec/1000);
    */
};

void FPGAclose(FPGAvars_t *FPGA){ // closes the memory mapped file with the FPGA hardware registers
	
	if( munmap( FPGA->virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( FPGA->fd_pio );
	}
	if( munmap( FPGA->axi_virtual_base, HW_FPGA_AXI_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( FPGA->fd_ram );
	}

	close( FPGA->fd_pio );
	close( FPGA->fd_ram );
}

int FPGA_init(FPGAvars_t *FPGA){ // maps the FPGA hardware registers to the variables in the FPGAvars struct
	
	if( ( FPGA->fd_pio = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 0 );
	}
	
	FPGA->virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, FPGA->fd_pio, HW_REGS_BASE );

	
	if( FPGA->virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( FPGA->fd_pio );
		return( 0 );
	}
	
	if( ( FPGA->fd_ram = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 0 );
	}
	
	FPGA->axi_virtual_base = mmap( NULL, HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, FPGA->fd_ram,ALT_AXI_FPGASLVS_OFST );

	if( FPGA->axi_virtual_base == MAP_FAILED ) {
		printf( "ERROR: axi mmap() failed...\n" );
		close( FPGA->fd_ram );
		return( 0 );
	}

	
	return(1);
}

int ADC_init(FPGAvars_t *FPGA, ADCvars_t *ADC){
	
	ADC->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->gpreg0.adccmd=0;       ADC->gpreg0.addr=0;	
	ADC->gpreg1.adccmd=0;       ADC->gpreg1.addr=1;
	ADC->gpreg2.adccmd=0;       ADC->gpreg2.addr=2;
	ADC->gpreg3.adccmd=0;       ADC->gpreg3.addr=3;
	ADC->gpreg4.adccmd=0;       ADC->gpreg4.addr=4;
	ADC->gpreg5.adccmd=0;       ADC->gpreg5.addr=5;
	ADC->gpreg7.adccmd=0;       ADC->gpreg7.addr=7;
	ADC->gpreg13.adccmd=0;		ADC->gpreg13.addr=13;
	ADC->gpreg15.adccmd=0;		ADC->gpreg15.addr=15;
	ADC->gpreg17.adccmd=0;		ADC->gpreg17.addr=17;
	ADC->gpreg19.adccmd=0;		ADC->gpreg19.addr=19;
	ADC->gpreg21.adccmd=0;		ADC->gpreg21.addr=21;
	ADC->gpreg25.adccmd=0;		ADC->gpreg25.addr=25;
	ADC->gpreg27.adccmd=0;		ADC->gpreg27.addr=27;
	ADC->gpreg29.adccmd=0;		ADC->gpreg29.addr=29;
	ADC->gpreg31.adccmd=0;		ADC->gpreg31.addr=31;
	ADC->gpreg33.adccmd=0;		ADC->gpreg33.addr=33;
	ADC->gpreg70.adccmd=0;		ADC->gpreg70.addr=70;

    ADC->reg = (ADCREG_t **)calloc(173,sizeof(ADCREG_t *));
	ADC->reg_dict = (uint32_t **)calloc(2,sizeof(uint32_t *));
    ADC->reg_dict[0] = (uint32_t *)calloc(173,sizeof(uint32_t));
    ADC->reg_dict[1] = (uint32_t *)calloc(173,sizeof(uint32_t));

    for(uint32_t n=0;n<173;n++){
        ADC->reg_dict[0][n] = 0xff;
        ADC->reg_dict[1][n] = 0xff;
    }
    
    ADC->reg[0] = (ADCREG_t *)(&(ADC->gpreg0));
	ADC->reg[1] = (ADCREG_t *)(&(ADC->gpreg1));
	ADC->reg[2] = (ADCREG_t *)(&(ADC->gpreg2));
	ADC->reg[3] = (ADCREG_t *)(&(ADC->gpreg3));
	ADC->reg[4] = (ADCREG_t *)(&(ADC->gpreg4));
	ADC->reg[5] = (ADCREG_t *)(&(ADC->gpreg5));
	ADC->reg[6] = (ADCREG_t *)(&(ADC->gpreg7));
	ADC->reg[7] = (ADCREG_t *)(&(ADC->gpreg13));
	ADC->reg[8] = (ADCREG_t *)(&(ADC->gpreg15));
	ADC->reg[9] = (ADCREG_t *)(&(ADC->gpreg17));
	ADC->reg[10] = (ADCREG_t *)(&(ADC->gpreg19));
	ADC->reg[11] = (ADCREG_t *)(&(ADC->gpreg31));
	ADC->reg[12] = (ADCREG_t *)(&(ADC->gpreg29));
	ADC->reg[13] = (ADCREG_t *)(&(ADC->gpreg27));
	ADC->reg[14] = (ADCREG_t *)(&(ADC->gpreg25));
	ADC->reg[15] = (ADCREG_t *)(&(ADC->gpreg21));
	ADC->reg[16] = (ADCREG_t *)(&(ADC->gpreg33));
	ADC->reg[17] = (ADCREG_t *)(&(ADC->gpreg70));
	
	for(uint32_t i=0; i<148; i++){
		ADC->tgcreg0x01[i].adccmd = 0;
		ADC->tgcreg0x01[i].addr = i+1;
        ADC->reg[i+18] = (ADCREG_t *)(&(ADC->tgcreg0x01[i]));
	}
	
	ADC->tgcreg0x95.adccmd=0; ADC->tgcreg0x95.addr=0x95;
	ADC->tgcreg0x96.adccmd=0; ADC->tgcreg0x96.addr=0x96;
	ADC->tgcreg0x97.adccmd=0; ADC->tgcreg0x97.addr=0x97;
	ADC->tgcreg0x98.adccmd=0; ADC->tgcreg0x98.addr=0x98;
	ADC->tgcreg0x99.adccmd=0; ADC->tgcreg0x99.addr=0x99;
	ADC->tgcreg0x9A.adccmd=0; ADC->tgcreg0x9A.addr=0x9A;
	ADC->tgcreg0x9B.adccmd=0; ADC->tgcreg0x9B.addr=0x9B;

	ADC->reg[166] = (ADCREG_t *)(&(ADC->tgcreg0x95));
	ADC->reg[167] = (ADCREG_t *)(&(ADC->tgcreg0x96));
	ADC->reg[168] = (ADCREG_t *)(&(ADC->tgcreg0x97));
	ADC->reg[169] = (ADCREG_t *)(&(ADC->tgcreg0x98));
	ADC->reg[170] = (ADCREG_t *)(&(ADC->tgcreg0x99));
	ADC->reg[171] = (ADCREG_t *)(&(ADC->tgcreg0x9A));
	ADC->reg[172] = (ADCREG_t *)(&(ADC->tgcreg0x9B));
	
    ADC->reg_dict[0][0] = 0;    ADC->reg_dict[0][1] = 1;
    ADC->reg_dict[0][2] = 2;    ADC->reg_dict[0][3] = 3;
    ADC->reg_dict[0][4] = 4;    ADC->reg_dict[0][5] = 5;
    ADC->reg_dict[0][7] = 6;    ADC->reg_dict[0][13] = 7;
    ADC->reg_dict[0][15] = 8;   ADC->reg_dict[0][17] = 9;
    ADC->reg_dict[0][19] = 10;  ADC->reg_dict[0][31] = 11;
    ADC->reg_dict[0][29] = 12;  ADC->reg_dict[0][27] = 13;
    ADC->reg_dict[0][25] = 14;  ADC->reg_dict[0][21] = 15;
    ADC->reg_dict[0][33] = 16;  ADC->reg_dict[0][70] = 17;
    
	for(uint32_t i=1; i<0x95; i++){ ADC->reg_dict[1][i] = i+17; }

    ADC->reg_dict[1][0x95] = 0x95+17;
    ADC->reg_dict[1][0x96] = 0x96+17;
    ADC->reg_dict[1][0x97] = 0x97+17;
    ADC->reg_dict[1][0x98] = 0x98+17;
    ADC->reg_dict[1][0x99] = 0x99+17;
    ADC->reg_dict[1][0x9A] = 0x9A+17;
    ADC->reg_dict[1][0x9B] = 0x9B+17;
    
    // setup function pointers
    ADC->issueSerialCommand = &adcIssueSerialCmd;
    ADC->setDefaultSettings = &adcSetDefaultSettings;
    ADC->setGain = &adcSetGain;
    ADC->setUnsignedInt = &adcSetUnsignedInt;
    ADC->setLowNoiseMode = &adcSetLowNoiseMode;
    ADC->toggleChannelPower = &adcToggleChannelPwr;
    ADC->setFilterBW = &adcSetFilterBW;
    ADC->setInternalAcCoupling = &adcSetInternalAcCoupling;
    ADC->issueDirectCommand = &adcIssueDirectCmd;
    ADC->sync = &adcSync;
	
    ADC->setDefaultSettings(ADC);
   
    return(1);
}

int RCV_init(FPGAvars_t *FPGA, ADCvars_t *ADC, RCVsys_t *RCV){

	RCV->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->pioVarGain = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_VAR_GAIN_SETTING_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->dataReadyFlag = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + DATA_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->leds = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	RCV->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
    
    // setup function pointers
    RCV->setRecLen = &rcvSetRecLen;   
    RCV->setPioVarGain = &rcvSetPioVarGain; 
    RCV->setLEDs = &rcvSetLEDs; 
 
    DREF32(RCV->pioVarGain) = 0;
	DREF32(RCV->recLen) = 2048;
	DREF32(RCV->stateReset)=1; 
    
    RCV->ADC = ADC;

    RCV->interrupt.ps = NULL;
    
    RCV->recLen_ref = 2048;
    RCV->npulses = 1;
    RCV->queryMode.all = 0;
    RCV->queryMode.realTime = 1;
    RCV->data = (char **)calloc(2,sizeof(char *));
    RCV->data[0] = (char *)calloc(3*MAX_RECLEN,sizeof(uint32_t));
    RCV->data[1] = (char *)calloc(3*MAX_RECLEN,sizeof(uint32_t));	
    
    RCV->setLEDs(RCV,0x1f);
   
    return(1);
}
