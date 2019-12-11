


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
	RCV->interrupt_reg = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + RCV_INTERRUPT_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	RCV->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->pioSettings = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SETTINGS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->dataReadyFlag = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + RCV_INTERRUPT_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->leds = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->ramBank = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
    
    // setup function pointers
    RCV->setRecLen = &rcvSetRecLen;   
    RCV->setPioVarGain = &rcvSetPioVarGain; 
    RCV->setClkDivisor = &rcvSetClkDivisor;
    RCV->setSamplingMode = &rcvSetSamplingMode;
    RCV->setCompressorMode = &rcvSetCompressorMode;
    RCV->setLEDs = &rcvSetLEDs; 
 
	RCV->pioSettings_ref.all = 0;
	RCV->pioSettings_ref.fclk_delay = 1;
	RCV->pioSettings_ref.sampling_mode = 1;
	
    DREF32(RCV->pioSettings) = RCV->pioSettings_ref.all;
	DREF32(RCV->recLen) = 2048;
	DREF32(RCV->stateReset)=1; 
    
    RCV->ADC = ADC;

    RCV->comm_sock = NULL;
    RCV->data_sock = NULL;
    RCV->interrupt.ps = NULL;
    
    RCV->recLen_ref = 2048;
    RCV->npulses = 1;
    RCV->queryMode.all = 0;
    RCV->queryMode.realTime = 1;
    RCV->data = (char **)calloc(2,sizeof(char *));
    RCV->data[0] = (char *)calloc(8*MAX_RECLEN,sizeof(uint16_t));
    RCV->data[1] = (char *)calloc(8*MAX_RECLEN,sizeof(uint16_t));	
    
    //~ RCV->setLEDs(RCV,0x1f);
   
    return(1);
}

int TX_init(FPGAvars_t *FPGA, TXsys_t *TX){
    TX->interrupt_reg = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_INTERRUPT_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg = (uint32_t **)malloc(27*sizeof(uint32_t *));
    TX->pio_reg[0] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[1] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[2] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG2_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[3] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG3_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[4] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG4_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[5] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG5_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[6] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG6_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[7] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG7_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[8] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG8_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[9] = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG9_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[10] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG10_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[11] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG11_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[12] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG12_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[13] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG13_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[14] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG14_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[15] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG15_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[16] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG16_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[17] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG17_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[18] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG18_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[19] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG19_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[20] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG20_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[21] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG21_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[22] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG22_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[23] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG23_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[24] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG24_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[25] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG25_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    TX->pio_reg[26] = FPGA->virtual_base+( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_PIO_REG26_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
    //TX->instructions = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONMEM_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//TX->phaseDelays = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_PHASEDELAYMEM_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	
    TX->reg9_24 = (TXtrigtimings_t *)malloc(16*sizeof(TXtrigtimings_t));

    TX->pio_cmd_list = (TXpiocmd_t **)malloc(sizeof(TXpiocmd_t *));
    *(TX->pio_cmd_list) = (TXpiocmd_t *)malloc(sizeof(TXpiocmd_t));

    TXpiocmd_t *pio_cmd;
    pio_cmd = *(TX->pio_cmd_list);

    pio_cmd->cmdNumber = 0;
    pio_cmd->reg2.all = 0;
    pio_cmd->reg2.set_async_wait = 1;
    pio_cmd->reg3.all = 0;
    pio_cmd->reg4.all = 0;
    pio_cmd->reg5.all = 0;
    pio_cmd->reg6.all = 0;
    pio_cmd->reg7.all = 0;
    pio_cmd->reg8.all = 0;
    pio_cmd->flags.all = 0;
    pio_cmd->flags.isCmd0 = 1;
    
    pio_cmd->reg25_26.all = 100000; // wait 1ms from when program begins before issuing commands
    
    pio_cmd->reg9_24 = NULL;
    pio_cmd->top = *(TX->pio_cmd_list);
    pio_cmd->prev = NULL;
    pio_cmd->next = NULL;

    TX->nSteeringLocs = 1;
    TX->nPhaseDelaysWritten = 0;
    TX->phaseDelays = (uint16_t **)malloc(sizeof(uint16_t *));
    *(TX->phaseDelays) = (uint16_t *)calloc(8,sizeof(uint16_t));

    TX->comm_sock = NULL;
    TX->pd_data_sock = NULL;
    TX->interrupt.ps = NULL;
    
    // control comms
    TX->reg0.all = 0;
    DREF32(TX->pio_reg[0]) = TX->reg0.all;

    // board identifiers, active transducers, trig/led rest lvls
    TX->reg1.all = 0;
    TX->reg1.isSolo = 1;
    TX->reg1.isMaster = 1;
    TX->reg1.activeTransducers = 0xff;
    TX->reg1.trigRestLvls = 0;
    DREF32(TX->pio_reg[1]) = TX->reg1.all;
    
    // pio commands
    TX->reg2.all = 0;
    DREF32(TX->pio_reg[2]) = 0;

    // phase delays ch0 & ch1
    TX->reg3.all = 0;
    DREF32(TX->pio_reg[3]) = TX->reg3.all;
    
    // phase delays ch2 & ch3
    TX->reg4.all = 0;
    DREF32(TX->pio_reg[4]) = TX->reg4.all;

    // phase delays ch4 & ch5
    TX->reg5.all = 0;
    DREF32(TX->pio_reg[5]) = TX->reg5.all;

    // phase delays ch6 & ch7
    TX->reg6.all = 0;
    DREF32(TX->pio_reg[6]) = TX->reg6.all;

    // transducer chargetime & fire cmd delay
    TX->reg7.all = 0;
    DREF32(TX->pio_reg[7]) = TX->reg7.all;

    // recv trig delay
    TX->reg8.all = 0;
    DREF32(TX->pio_reg[8]) = TX->reg8.all;
    
    // trig/led delays and durations
    for(int i=0;i<16;i++){
        TX->reg9_24[i].all = 0;
        DREF32(TX->pio_reg[9+i]) = TX->reg9_24[i].all;
    }

    // instruction request timer
    TX->reg25_26.all = 0;
    DREF32(TX->pio_reg[25]) = TX->reg25_26.reg25;
    DREF32(TX->pio_reg[26]) = TX->reg25_26.reg26;

    // setup function pointers
    TX->setControlState = &txSetControlState;
    TX->setTrigRestLvls = &txSetTrigRestLvls;
    TX->setActiveTransducers = &txSetActiveTransducers;
    
    TX->setTrigs = &txSetTrigs;
    TX->setChargeTime = &txSetChargeTime;
    TX->setFireCmdDelay = &txSetFireCmdDelay;
    TX->setPhaseDelay = &txSetPhaseDelay;
    TX->setRecvTrigDelay = &txSetRecvTrigDelay;
    TX->setAsyncWait = &txSetAsyncWait;
    TX->issuePioCommand = &txIssuePioCommand;

    TX->addCmd = &txAddPioCmd_f;
    TX->delCmd = &txDelPioCmd_f;

    TX->makeLoopStart = &txMakeLoopStart;
    TX->makeLoopEnd = &txMakeLoopEnd;
    TX->makeSteeringLoopStart = &txMakeSteeringLoopStart;
    TX->makeSteeringLoopEnd = &txMakeSteeringLoopEnd;
    TX->makePioCmd = &txMakePioCmd;

    TX->bufferTrigTimings = &txBufferTrigTimingCmd;
    TX->bufferChargeTime = &txBufferChargeTimeCmd;
    TX->bufferFireCmd = &txBufferFireDelayCmd;
    TX->bufferPhaseDelays = &txBufferPhaseDelayCmd;
    TX->bufferRecvTrig = &txBufferRecvTrigDelayCmd;
    TX->bufferAsyncWait = &txBufferAsyncWaitCmd;
    TX->resetTxInterrupt = &txResetTxInterrupt;
    TX->resetRcvTrig = &txResetRcvTrig;

    TX->setNumSteeringLocs = &txSetNumSteeringLocs;
    TX->storePhaseDelays = &txStorePhaseDelays;

    TX->setControlState(TX,0);
    TX->resetTxInterrupt(TX);
    TX->resetRcvTrig(TX);
    return(1);
}
