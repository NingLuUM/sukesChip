

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


int FPGA_init(FPGAvars_t *FPGA){ // maps the FPGA hardware registers to the variables in the FPGAvars_t struct
	
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


void ADC_setup(FPGAvars_t *FPGA, ADCchip_t *ADC){

	ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	ADC->powerOn = &powerOn_adc;
	ADC->powerOff = &powerOff_adc;
	ADC->sync = &sync_adc;
	ADC->initializeSettings = &initializeSettings_adc;
	ADC->setGain = &setGain_adc;
	ADC->issueDirectSerialCommand = &issueDirectSerialCommand_adc;
}


void RCV_init(FPGAvars_t *FPGA, RCVsys_t *RCV, ADCchip_t *ADC){
    
    ADC = (ADCchip_t *)calloc(1,sizeof(ADCchip_t));
    ADC_setup(FPGA,ADC);
    
    RCV->ADC = ADC;

    RCV->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    RCV->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    RCV->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    RCV->trigDelay = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_TRIG_DELAY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    RCV->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_INTERRUPT_GENERATOR_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    RCV->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
    RCV->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
    RCV->ramBank2 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK2_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

    RCV->data = (char **)malloc(sizeof(char *));
    *(RCV->data)=NULL;
    RCV->resetVars = &resetVars_rcv;
    RCV->stateResetFPGA = &stateResetFPGA_rcv;
    RCV->setRecLen = &setRecLen_rcv;
    RCV->setTrigDelay = &setTrigDelay_rcv;
    RCV->copyDataToMem = &copyDataToMem_rcv;
    RCV->setDataAddrPointers = &setDataAddrPointers_rcv;
    RCV->setLocalStorage = &setLocalStorage_rcv;
    RCV->allocateLocalStorage = &allocateLocalStorage_rcv;
    RCV->getInterruptMsg = &getInterruptMsg_rcv;
    RCV->setDataTransferPacketSize = &setDataTransferPacketSize_rcv;
    RCV->spawnDataTransferSocks = &spawnDataTransferSocks_rcv;
	
}

/*
//~ int TX_init(FPGAvars_t *FPGA, TXsys *TX){
	
	//~ TX->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->interrupt1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	//~ TX->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TXCONTROLCOMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );	
	//~ TX->channelMask = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_TXCHANNELMASK_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	//~ TX->setLed = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->setTrig = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_VAL_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->setTrigRestLevel = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_REST_LEVELS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );


	//~ TX->errorMsgFromTransducer = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_OUTPUT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->errorMsgFromInterrupt = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_INTERRUPT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->errorReply = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_ERROR_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	//~ TX->instructionReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//~ TX->phaseDelayReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_PHASEDELAYREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//~ TX->fireAt_phaseDelayReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_FIREAT_PHASEDELAYREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	

    //~ TX->instructionBuff = (char **)malloc(sizeof(char *));
    //~ TX->phaseDelayBuff = (char **)malloc(sizeof(char *));
    //~ TX->instructions = (uint32_t **)malloc(sizeof(uint32_t *));
    //~ TX->fireAtList = (uint32_t ***)malloc(sizeof(uint32_t **));
    //~ TX->steeringLoopsDef = (TX_SteeringLoopDefs_t *)calloc(MAX_LOOPS,sizeof(TX_SteeringLoopDefs_t));
	
	//~ DREF32(TX->channelMask) = 0xff;
	//~ DREF32(TX->controlComms) = 0;
	//~ return(1);
//~ } */


BOARDdata_t *BOARD_init(int gettingKey){ // load the boards specific data from files stored on SoC	
	static BOARDdata_t *BOARD = NULL;
    static int shmid;
    
    if(gettingKey){
		printf("getting shared Board Data\n");
		
        // Create a shared memory segment of size: data_size and obtain its shared memory id
        if((shmid = shmget(g_shmkey_board, sizeof(BOARDdata_t), IPC_CREAT | 0660)) < 0) {
            printf("Error getting shared memory id\n");
        }

        // Make shared_memory point to the newly created shared memory segment
        if((BOARD = shmat(shmid, NULL, 0)) == (BOARDdata_t *) -1) {
            printf("Error attaching shared memory\n");
        }

        char const* const fileName = "boardData";
		FILE* file = fopen(fileName, "r");
		char line[256];
		int n=0;
		uint32_t boardData[10];

		while( fgets(line, sizeof(line), file) && n<4 ){
			boardData[n] = atoi(line);
			n++;    
		}  
		fclose(file);
		BOARD->boardNum = boardData[0]; 
        
    } else if (!gettingKey && ( BOARD != NULL ) ){
        printf("releasing the shared memory\n");
        shmdt(BOARD);
        shmctl(shmid, IPC_RMID, NULL);
    } else {
        printf("bad shared memory something or other\n");
    }
    return(BOARD);
    	
}


