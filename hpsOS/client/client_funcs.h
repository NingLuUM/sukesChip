
int epfd;
struct epoll_event ev;
struct epoll_event events[MAX_SOCKETS];

struct ENETsock{
	int sockfd;
	int is_interrupt;
	int is_commsock;
	int portNum;
	char *dataAddr;
	int bytesInPacket;
    int is_active;
	struct ENETsock *next;
	struct ENETsock *prev;
    struct ENETsock *commsock;
};


struct FPGAvars{ // structure to hold variables that are mapped to the FPGA hardware registers
	void *virtual_base;
	void *axi_virtual_base;
	int fd_pio;
	int fd_ram;	
};


struct ADCvars{
	// memory mapped variables in FPGA
	uint32_t volatile *stateReset;
	uint32_t volatile *controlComms;
	uint32_t volatile *recLen;
	uint32_t volatile *trigDelay;
	uint32_t volatile *serialCommand;
	uint32_t volatile *interrupt0;
	
	int32_t volatile *ramBank0;
	int32_t volatile *ramBank1;
	int32_t volatile *ramBank2;
	
	// local variables on arm proc only
	uint32_t recLen_ref;
	uint32_t trigDelay_ref;
	char *data;
	
	// functions that act on ADC vars
	void (*resetADC_vars)(struct ADCvars *);
};


struct TXvars{
	uint32_t volatile *controlComms;
	uint32_t volatile *channelMask;
	uint32_t volatile *setInstructionReadAddr;
	uint32_t volatile *errorReply;
	uint32_t volatile *errorMsgFromInterrupt;
	uint32_t volatile *errorMsgFromTransducer;
	uint32_t volatile *setTrigRestLevel;
	uint32_t volatile *setTrig;
	uint32_t volatile *setLed;
	uint32_t volatile *interrupt0;
	uint32_t volatile *interrupt1;
	
	uint32_t volatile **fireCmdPhaseCharge;
	uint32_t volatile *fireCmdPhaseCharge0;
	uint32_t volatile *fireCmdPhaseCharge1;
	uint32_t volatile *fireCmdPhaseCharge2;
	uint32_t volatile *fireCmdPhaseCharge3;
	uint32_t volatile *fireCmdPhaseCharge4;
	uint32_t volatile *fireCmdPhaseCharge5;
	uint32_t volatile *fireCmdPhaseCharge6;
	uint32_t volatile *fireCmdPhaseCharge7;
	
	uint32_t volatile **fireAtCmdPhaseCharge;
	uint32_t volatile *fireAtCmdPhaseCharge0;
	uint32_t volatile *fireAtCmdPhaseCharge1;
	uint32_t volatile *fireAtCmdPhaseCharge2;
	uint32_t volatile *fireAtCmdPhaseCharge3;
	uint32_t volatile *fireAtCmdPhaseCharge4;
	uint32_t volatile *fireAtCmdPhaseCharge5;
	uint32_t volatile *fireAtCmdPhaseCharge6;
	uint32_t volatile *fireAtCmdPhaseCharge7;
	
	uint16_t volatile **instructionTypeReg;
	uint32_t volatile **instructionReg;
	uint32_t volatile **timingReg;
	uint32_t volatile **loopAddressReg;
	uint32_t volatile **loopCounterReg;
	
	uint16_t *instructionTypeReg_local;
	uint32_t *instructionReg_local;
	uint32_t *timingReg_local;
	uint32_t *loopAddressReg_local;
	uint32_t *loopCounterReg_local;
	
	void (*resetTX_vars)(struct TXvars *);
};


struct UserProgram{
	int instructionsCount;
	int fireCmdPhaseChargesCount;
	int fireAtCmdPhaseChargesCount;
	uint32_t *instructionList;
	uint16_t *instructionTypeList;
	uint32_t *instructionTiming;
	uint32_t *fireCmdPhaseCharge;
	uint32_t *fireAtCmdPhaseCharge;
};


void FPGAclose(struct FPGAvars *FPGA){ // closes the memory mapped file with the FPGA hardware registers
	
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


int FPGA_init(struct FPGAvars *FPGA){ // maps the FPGA hardware registers to the variables in the FPGAvars struct
	
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


int ADC_init(struct FPGAvars *FPGA, struct ADCvars *ADC){
	

	ADC->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->trigDelay = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_TRIG_DELAY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_INTERRUPT_GENERATOR_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

	ADC->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

	ADC->ramBank2 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK2_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	
	// ADC->data = (char *)malloc( ADC_RAMBANK0_SPAN + ADC_RAMBANK1_SPAN + ADC_RAMBANK2_SPAN );
	// memset(ADC->data, 0, ADC_RAMBANK0_SPAN + ADC_RAMBANK1_SPAN + ADC_RAMBANK2_SPAN );
	
	DREF32(ADC->trigDelay) = 0;
	DREF32(ADC->recLen) = 2048;
	
	ADC->trigDelay_ref = 0;
	ADC->recLen_ref = 2048;
	
	DREF32(ADC->stateReset)=1; 
	usleep(10);
	DREF32(ADC->stateReset)=0;
	usleep(10);
	
	// turn on ADC
	//~ DREF32(ADC->controlComms) = ADC_POWER_ON;
	//~ sleep(0.25);
	
	//~ // software reset, just to be safe
	//~ DREF32(ADC->serialCommand) = ( ( ADC_SOFTWARE_RESET_ADDR << 16 ) | ADC_SOFTWARE_RESET_CMD );
	//~ usleep(100);
	//~ DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	//~ usleep(500);
	//~ DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	//~ usleep(500);
	//~ DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	//~ sleep(0.2);
	
	//~ // set tgc, allows setting of gain
	//~ DREF32(ADC->serialCommand) = ( ( ADC_SET_TGC_ADDR << 16 ) | ADC_SET_TGC_CMD );
	//~ usleep(100);
	//~ DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	//~ usleep(500);
	//~ DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	//~ usleep(500);
	//~ DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	//~ sleep(0.2);

	//~ // declare the gain to be constant with time
	//~ DREF32(ADC->serialCommand) = ( ( ADC_SET_FIXED_GAIN_ADDR << 16 ) |  ADC_SET_FIXED_GAIN_CMD );
	//~ usleep(100);
	//~ DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	//~ usleep(500);
	//~ DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	//~ usleep(500);
	//~ DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	//~ sleep(0.2);
	
	return(1);
}


int TX_init(struct FPGAvars *FPGA, struct TXvars *TX){
	
	TX->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->interrupt1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	TX->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TXCONTROLCOMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	TX->channelMask = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_TXCHANNELMASK_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	TX->setLed = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->setTrig = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_VAL_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->setTrigRestLevel = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_REST_LEVELS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->fireCmdPhaseCharge = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge2 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH2_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge3 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH3_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge4 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH4_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge5 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH5_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge6 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH6_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge7 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH7_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->fireAtCmdPhaseCharge = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge2 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH2_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge3 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH3_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge4 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH4_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge5 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH5_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge6 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH6_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge7 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH7_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->errorMsgFromTransducer = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_OUTPUT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->errorMsgFromInterrupt = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_INTERRUPT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->errorReply = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_ERROR_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->setInstructionReadAddr = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_SET_INSTRUCTION_READ_ADDR_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->instructionTypeReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONTYPEREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->instructionReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->timingReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_TIMINGREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->loopAddressReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_LOOPADDRESSREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->loopCounterReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_LOOPCOUNTERREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	
	TX->instructionReg_local 		= (uint32_t *)malloc( TX_INSTRUCTIONREGISTER_SPAN );
	TX->instructionTypeReg_local 	= (uint16_t *)malloc( TX_INSTRUCTIONTYPEREGISTER_SPAN );
	TX->timingReg_local 			= (uint32_t *)malloc( TX_TIMINGREGISTER_SPAN );
	TX->loopAddressReg_local		= (uint32_t *)malloc( TX_LOOPADDRESSREGISTER_SPAN );
	TX->loopCounterReg_local		= (uint32_t *)malloc( TX_LOOPCOUNTERREGISTER_SPAN );
	
	memset(TX->instructionReg_local,0,TX_INSTRUCTIONREGISTER_SPAN);
	memset(TX->instructionTypeReg_local,0,TX_INSTRUCTIONTYPEREGISTER_SPAN);
	memset(TX->timingReg_local,0,TX_TIMINGREGISTER_SPAN);
	memset(TX->loopAddressReg_local,0,TX_LOOPADDRESSREGISTER_SPAN);
	memset(TX->loopCounterReg_local,0,TX_LOOPCOUNTERREGISTER_SPAN);
	
	DREF32(TX->channelMask) = 0xff;
	DREF32(TX->controlComms) = 0;
	return(1);
}


void resetGlobalVars(){
	g_recLen = 2048; g_packetsize = 2048;
    g_queryTimeout = 1000;
    g_moduloBoardNum = 1;
	g_moduloTimer = 0;
	g_packetWait = 0;
	g_numPorts = (g_recLen-1)/g_packetsize+1;
}


void getBoardData(){ // load the boards specific data from files stored on SoC		
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
}


void setnonblocking(int sock){
    int opts;
    if((opts=fcntl(sock,F_GETFL))<0) perror("GETFL nonblocking failed");
    
    opts = opts | O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0) perror("SETFL nonblocking failed");
}


void connectPollInterrupter(struct ENETsock **INTR, char *gpio_lab, int portnum){
	
	struct ENETsock* enet;	
	enet = (struct ENETsock *)malloc(sizeof(struct ENETsock));
    enet->next = *INTR;
	enet->prev = NULL;
    enet->sockfd = 0;
	enet->portNum = portnum;
	enet->is_interrupt = 1;
    enet->is_active = 1;
	enet->is_commsock = 0;
	
	if(*INTR != NULL){
		(*INTR)->prev = enet;
    }
    
	// CHANGE THIS TO DESIRED LABEL
	const char *gpio_label = gpio_lab;//"gpio@0x100000080";
	const char *edge_type = "rising";
	//~ int poll_interval = 1000;

	// Other vars
	DIR *gpio_dir;
	const char *gpio_dir_path = "/sys/class/gpio";
	const char *gpiochip_str = "gpiochip";
	size_t gpiochip_str_len = strlen(gpiochip_str);
	int result;
	struct dirent *dir_entry;
	char path[PATH_MAX];
	int path_length;
	int file_fd;
	char buffer[PATH_MAX + 1] = {0};
	char gpio_number_buffer[PATH_MAX + 1] = {0};
	char *str_result = NULL;
	char *newline_ptr;
	//~ struct pollfd pollfd_struct;

	// Open the sysfs gpio directory
	gpio_dir = opendir(gpio_dir_path);
	if (gpio_dir == NULL)
		error(1, errno, "could not open directory '%s'", gpio_dir_path);

	// Find the gpio controller for given gpio_label
	while (1)
	{
		// Read next dir
		errno = 0;
		dir_entry = readdir(gpio_dir);
		if (dir_entry == NULL)
		{
			if (errno != 0)
				error(1, errno, "reading directory '%s'",
					  gpio_dir_path);
			else
				break;
		}

		// check if this is a gpio controller entry
		result = strncmp(dir_entry->d_name, gpiochip_str,
						 gpiochip_str_len);
		if (result != 0)
			continue;

		// open the gpio controller label file and read label value
		path_length = snprintf(path, PATH_MAX, "%s/%s/label",
							   gpio_dir_path, dir_entry->d_name);
		if (path_length < 0)
			error(1, 0, "path output error");
		if (path_length >= PATH_MAX)
			error(1, 0, "path length overflow");

		file_fd = open(path, O_RDONLY | O_SYNC);
		if (file_fd < 0)
			error(1, errno, "could not open file '%s'", path);

		result = read(file_fd, buffer, PATH_MAX);
		if (result < 0)
			error(1, errno, "reading from '%s'", path);
		if (result == PATH_MAX)
			error(1, errno, "buffer overflow reading '%s'", path);

		result = close(file_fd);
		if (result < 0)
			error(1, errno, "could not close file '%s'", path);

		buffer[PATH_MAX] = 0;
		// test the gpio controller label value for our gpio controller
		str_result = strstr(buffer, gpio_label);
		if (str_result != NULL)
			break;
	}

	if (str_result == NULL)
		error(1, 0, "unable to locate gpio controller");

	// Create path to read base value
	path_length = snprintf(path, PATH_MAX, "%s/%s/base",
						   gpio_dir_path, dir_entry->d_name);
	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	// Open file and read base value
	file_fd = open(path, O_RDONLY | O_SYNC);
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	result = read(file_fd, gpio_number_buffer, PATH_MAX);
	if (result < 0)
		error(1, errno, "reading from '%s'", path);
	if (result == PATH_MAX)
		error(1, errno, "buffer overflow reading '%s'", path);

	result = close(file_fd);
	if (result < 0)
		error(1, errno, "could not close file '%s'", path);

	gpio_number_buffer[PATH_MAX] = 0;

	// Remove the newline at the end of the string
	newline_ptr = strchr(gpio_number_buffer, '\n');
	if (newline_ptr != NULL)
		memset(newline_ptr, '\0', 1);

	// Open the gpio export file and write our gpio number
	path_length = snprintf(path, PATH_MAX, "%s/export",
						   gpio_dir_path);
	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	file_fd = open(path, O_WRONLY | O_SYNC);
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	result = write(file_fd, gpio_number_buffer, strlen(gpio_number_buffer));
	// Don't check errors because device could already exist

	result = close(file_fd);
	if (result < 0)
		error(1, errno, "could not close file '%s'", path);

	// Open the gpio edge file and write edge type (falling, both, etc)
	path_length = snprintf(path, PATH_MAX, "%s/gpio%s/edge",
						   gpio_dir_path, gpio_number_buffer);

	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	file_fd = open(path, O_WRONLY | O_SYNC);
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	// Write edge type
	result = write(file_fd, edge_type, 7);
	if (result < 0)
		error(1, errno, "writing to '%s'", path);
	if (result != 7)
		error(1, errno, "buffer underflow writing '%s'", path);

	result = close(file_fd);
	if (result < 0)
		error(1, errno, "could not close file '%s'", path);

	// Open the gpio value file and poll the file
	path_length = snprintf(path, PATH_MAX, "%s/gpio%s/value",
						   gpio_dir_path, gpio_number_buffer);
	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	printf("path: ");
	printf("%s",path);
	printf("\n");

	// path = /sys/class/gpio/gpio331/edge

	file_fd = open(path, O_RDONLY | O_RSYNC | O_NONBLOCK ); // Set to nonblocking
	
	int flags = fcntl(file_fd, F_GETFL, 0);
	fcntl(file_fd, F_SETFL, flags | O_NONBLOCK);
		
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	// Need to read the file before we can poll it
	// poll will keep returning 1 otherwise
	result = read(file_fd, buffer, PATH_MAX);
	if (result < 0)
		error(1, errno, "reading from '%s'", path);
	if (result == PATH_MAX)
		error(1, errno, "buffer overflow reading '%s'", path);

	enet->sockfd = file_fd;

	*INTR = enet;
		
    ev.data.ptr = enet;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, enet->sockfd, &ev);
    
}


void disconnectPollInterrupter(struct ENETsock **INTR){
	
	struct ENETsock **enet, *prev, *next;
    enet = INTR;
    while((*enet) != NULL){
        next = (*enet)->next;
        prev = (*enet)->prev;
        epoll_ctl(epfd, EPOLL_CTL_DEL, (*enet)->sockfd, &ev);
        close((*enet)->sockfd);
        
        
		free(*enet);
		if ( prev == NULL && next == NULL ){
			break;
		} else if(next != NULL){
			next->prev = prev;
			(*enet)=next; 
		} else if (prev != NULL){
			prev->next = next;
			(*enet)=next; 
		} 
    }
}


void addPollSock(struct ENETsock **ENET, int portNum){
    struct ENETsock* enet;	
	enet = (struct ENETsock *)malloc(sizeof(struct ENETsock));
    enet->next = *ENET;
	enet->prev = NULL;
    enet->sockfd = 0;
	enet->portNum = portNum;
	enet->is_interrupt = 0;
    enet->is_active = 0;
	if(portNum == 0){
		enet->is_commsock = 1;
        enet->commsock = enet;
	} else {
		enet->is_commsock = 0;
	}
	if(*ENET != NULL){
		(*ENET)->prev = enet;
        enet->commsock = (*ENET)->commsock;
    }
	*ENET = enet;
}


void connectPollSock(struct ENETsock **ENET, int portNum){ /* function to accept incoming ethernet connections from the socs */
    struct sockaddr_in server_sockaddr;	
	struct timeval t0,t1;
	int diff;
	struct ENETsock *enet, *enet0;

	enet = (*ENET)->commsock;
    while(enet->portNum != portNum){
		enet = enet->prev;
	}		
    enet->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_sockaddr.sin_port = htons(enet->portNum+INIT_PORT);
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = inet_addr(g_serverIP);
    
    gettimeofday(&t0,NULL); 
    if(connect(enet->sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))  == -1){		
        while(connect(enet->sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))  == -1){
            gettimeofday(&t1,NULL);
            diff = (t1.tv_sec-t0.tv_sec);
            if(diff>(600)){
                printf("NO CONNECT!!!!\n");
                break;
            }	
        }
    }  
    setsockopt(enet->sockfd,IPPROTO_TCP,TCP_NODELAY,&ONE,sizeof(int));
    setsockopt(enet->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
	setnonblocking(enet->sockfd);

    enet->is_active = 1;	
    
    ev.data.ptr = enet;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, enet->sockfd, &ev);
    
	while(enet != NULL){
        if(enet->is_active){
            enet0 = enet;
        }
		enet = enet->prev;
	}
	*ENET = enet0;
	printf("board %d, port %d is active\n", (*ENET)->sockfd, (*ENET)->portNum);
	
    
}


void disconnectPollSock(struct ENETsock **ENET, int portNum){ 
    struct ENETsock *enet, *prev, *next;
    int sockfd = -1;
    enet = (*ENET);
    while(enet != NULL){
        next = enet->next;
        prev = enet->prev;
        if(enet->portNum == portNum){
			sockfd = enet->sockfd;
            if(next != NULL){
                next->prev = prev;
            }
            if(prev != NULL){
                prev->next = next;
            }
            if(prev == NULL){
                (*ENET) = next;
            }
            free(enet);
            break;
        }
        enet = enet->next;
    }
    epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &ev);
    if(sockfd>(-1))
		close(sockfd);
}



void setDataAddrPointers(struct ENETsock **ENET, char *adcData){
    struct ENETsock *enet;
    int bytesRemaining = g_recLen*ADC_BYTES_PER_TIMEPOINT;
    int requestedBytesPerPacket = g_packetsize*ADC_BYTES_PER_TIMEPOINT;
    int actualBytesInPacket;
    
    int sendbuff;
    enet = (*ENET)->commsock;

    /* check if sockets exist on the desired ports, if not add them */
    while( enet->portNum < g_numPorts ){
        if( enet->prev == NULL ){
            addPollSock(ENET,enet->portNum+1);
        }
        enet = enet->prev;
    }

    /* if more sockets exist than the number of active ports, disable them */
    while( enet != NULL ){
        if( enet->portNum > g_numPorts ){
            disconnectPollSock(ENET,enet->portNum);
        }
        enet = enet->prev;
    }
    
    enet = (*ENET)->commsock->prev;
    /*  go through the list of active sockets, connect them if they aren't already.
        set pointers to appropriate addresses in data array where each socket should sent data from.
        set the size of the data the socket is responsible for sending to cServer. */
    while( bytesRemaining > 0 ){
        if( !(enet->is_active) )
            connectPollSock(ENET,enet->portNum);
            
        actualBytesInPacket = ( requestedBytesPerPacket < bytesRemaining ) ? requestedBytesPerPacket : bytesRemaining;
        
        enet->dataAddr = adcData + (enet->portNum-1)*requestedBytesPerPacket;
        enet->bytesInPacket = actualBytesInPacket;
        sendbuff = enet->bytesInPacket;
		setsockopt(enet->sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff)); 
        bytesRemaining -= requestedBytesPerPacket;
        enet = enet->prev;
    }
    
    while(enet != NULL){
		disconnectPollSock(ENET,enet->portNum);
        enet = enet->prev;
	}
}


void adcmemcpy(char *dest, int32_t volatile *sourceData, size_t nbytes){
	char *src = (char *)sourceData;
	for(int i=0;i<nbytes;i++)
		dest[i] = src[i];
}


void sendAcqdData(struct ENETsock **ENET, struct ADCvars *ADC, char *adcData, int portNum){
	struct ENETsock *enet0, *commsock;
	commsock = (*ENET)->commsock;
	static int tmp = 0;
    int nsent;
    printf("sendAcqdData, tmp=%d\n",tmp);
    if ( DREF32(ADC->interrupt0) || tmp>0 ){
		printf("sendAcqdData, tmp=%d\n",tmp);
		if( g_queryMode == 0 || tmp == 0 ){
			adcmemcpy(&adcData[0],DREFP32S(ADC->ramBank0),g_recLen*sizeof(int32_t));
			adcmemcpy(&adcData[g_recLen*sizeof(int32_t)],DREFP32S(ADC->ramBank1),g_recLen*sizeof(int32_t));
			adcmemcpy(&adcData[2*g_recLen*sizeof(int32_t)],DREFP32S(ADC->ramBank2),g_recLen*sizeof(int32_t));
		}
		printf("sendAcqdData, tmp=%d\n",tmp);
		printf("g_queryMode=%d\n",g_queryMode);
		if(g_queryMode == 0){
			enet0 = commsock->prev;
			while( enet0 != NULL && enet0->is_active ){
				setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ONE,sizeof(int));
				nsent = send(enet0->sockfd, enet0->dataAddr, enet0->bytesInPacket,0);	
				setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ZERO,sizeof(int));
				if( nsent < 0 )
					perror("error sending data:");
				//~ printf("oh my, data sent. enetmsg1 = 0\n");
				setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
				enet0 = enet0->prev;
				usleep(g_packetWait);
			}
		} else {
			tmp += 1;
			if(portNum==0){
				portNum=commsock->prev->portNum;
			}
			if(portNum <= g_numPorts){
				enet0 = commsock->prev;
				while( enet0->portNum != portNum )
					enet0 = enet0->prev;
				
				printf("%d\n",enet0->dataAddr);
				setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ONE,sizeof(int));
				nsent = send(enet0->sockfd, enet0->dataAddr, enet0->bytesInPacket,0);	
				setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ZERO,sizeof(int));
				if( nsent < 0 )
					perror("error sending data:");
				//~ printf("oh my, data sent. enetmsg1 != 0\n");
				setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
			}
			printf("tmp %d, portNum %d, g_numPorts %d\n",tmp,portNum, g_numPorts);
			if(portNum == g_numPorts){
				tmp = 0;
				usleep(10);
			}
		}
	}
}


void ADC_Controller(struct FPGAvars *FPGA, struct ADCvars *ADC, struct TXvars *TX, struct ENETsock **ENET, char *adcData,  uint32_t *phaseDelays, uint32_t *chargeTimes){ // process that talks to the FPGA and transmits data to the SoCs	,fd_set *masterfds,
	struct ENETsock *enet0, *commsock;
	commsock = (*ENET)->commsock;
	
	
	switch(enetmsg[0]){
		
		case(CASE_ADC_RECORD_LENGTH):{ // change record length
            DREF32(ADC->transReady) = 0;
			if( enetmsg[1]>=MIN_PACKETSIZE && enetmsg[1]<=MAX_RECLEN ){
				DREF32(ADC->recLen) = enetmsg[1];
				g_recLen = enetmsg[1];
				if( enetmsg[2]>=MIN_PACKETSIZE && enetmsg[2]<=enetmsg[1] ){
					g_packetsize = enetmsg[2];
				} else {
					g_packetsize = g_recLen;
				}
				printf("[ recLen, packetsize ] set to [ %zu, %zu ]\n",DREF32(ADC->recLen), g_packetsize);
			} else {
				DREF32(ADC->recLen) = 2048;
				g_recLen = DREF32(ADC->recLen);
                g_packetsize = 2048;
				printf("invalid recLen, defaulting to 2048, packetsize to 512\n");
			}
            g_numPorts = (g_recLen-1)/g_packetsize + 1;
            setDataAddrPointers(ENET, adcData);
			break;
		}
		
		case(CASE_ADC_TRIGGER_DELAY):{ // change adc trig delay
			DREF32(ADC->trigDelay) = ADC_CLK*enetmsg[1];
			printf("ADC: Trig Delay -- %.2f\n",(float)DREF32(ADC->trigDelay)/((float)ADC_CLK));
			break;
		}

		case(CASE_ADC_TOGGLE_DATA_ACQ):{ // if dataGo is zero it won't transmit data to server, basically a wait state
            g_dataAcqGo = ( enetmsg[1] == 1 || enetmsg[1] == 0 ) ? enetmsg[1] : 0;
            if( g_dataAcqGo )
                setDataAddrPointers(ENET, adcData);
			DREF32(ADC->stateReset) = 1; 
			DREF32(ADC->stateReset) = 0;
			break;
		}

		case(CASE_QUERY_ADC_FOR_DATA):{
			//~ printf("query (%0.2f ms)\n",(0.001)*g_queryTimeout);
			if( enetmsg[1] == 0 || enetmsg[2] == 0 ){
				usleep((g_boardNum%g_moduloBoardNum)*g_moduloTimer);
				tmp = 0;
			}
			while( !DREF32(ADC->transReady) && (++tmp)<g_queryTimeout ){
				usleep(10);	
			}
			if( DREF32(ADC->transReady) ){
				//~ printf("tmp = %d\n",tmp);
				//~ printf("transReady\n");
				adcmemcpy(&adcData[0],DREFP32S(ADC->ramBank0),g_recLen*sizeof(int32_t));
				adcmemcpy(&adcData[g_recLen*sizeof(int32_t)],DREFP32S(ADC->ramBank1),g_recLen*sizeof(int32_t));
				adcmemcpy(&adcData[2*g_recLen*sizeof(int32_t)],DREFP32S(ADC->ramBank2),g_recLen*sizeof(int32_t));
				//~ printf("memcpy'd\n");
				//~ for(int j=0;j<100;j++){
					//~ printf("adcData[%d]=%lu\n",j*67,(long unsigned int)((DREFP32S(ADC->ramBank0))[j*67])&0xfff);
				//~ }
				
				if(enetmsg[1] == 0){
					enet0 = commsock->prev;
					while( enet0 != NULL && enet0->is_active ){
						setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ONE,sizeof(int));
						nsent = send(enet0->sockfd, enet0->dataAddr, enet0->bytesInPacket,0);	
						setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ZERO,sizeof(int));
						if( nsent < 0 )
							perror("error sending data:");
						//~ printf("oh my, data sent. enetmsg1 = 0\n");
						setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
						enet0 = enet0->prev;
						usleep(g_packetWait);
					}
					portNum = 0;
				} else {
					portNum = enetmsg[2]+1;
					if(portNum <= g_numPorts){
						enet0 = commsock->prev;
						while( enet0->portNum != portNum )
							enet0 = enet0->prev;
						
						setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ONE,sizeof(int));
						nsent = send(enet0->sockfd, enet0->dataAddr, enet0->bytesInPacket,0);	
						setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ZERO,sizeof(int));
						if( nsent < 0 )
							perror("error sending data:");
						//~ printf("oh my, data sent. enetmsg1 != 0\n");
						setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
					}
					if(portNum == g_numPorts){
						portNum = 0;
						usleep(10);	
						//~ printf("portNum %d\n",g_numPorts);
					}
				}
				
			} else {
                emsg[0] = CASE_QUERY_ADC_FOR_DATA; emsg[1] = g_boardNum; 
                //send(commsock->sockfd,emsg,enetMsgSize,0);
                printf("query data timed out (10ms)\n");
            }
            //~ printf("trans ready = %lu\n",(unsigned long) DREF32(ADC->transReady));
			if(portNum == 0){
				DREF32(ADC->stateReset) = 1; 
				usleep(10);
				DREF32(ADC->stateReset) = 0;
				tmp = 0;
			}
			//~ printf("trans ready = %lu\n",(unsigned long) DREF32(ADC->transReady));
			break;	
		}
		
		case(CASE_SET_QUERY_DATA_TIMEOUT):{
			if(enetmsg[1] > 999){
				g_queryTimeout = enetmsg[1]/10;
			} else {
				g_queryTimeout = 10000/10;
			}
			break;
		}
		
		case(CASE_QUERY_BOARD_INFO):{// loads board-specific data from onboard file		
			send(commsock->sockfd,g_boardData,enetMsgSize,0);
            setsockopt(commsock->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
            break;
		}
		
		
        case(CASE_ENET_INTERLEAVE_DEPTH_AND_TIMER):{
			if( enetmsg[1] > 0 ){
                g_moduloBoardNum = enetmsg[1];
				if( enetmsg[2] < 5000 ){
					g_moduloTimer = enetmsg[2];
				} else {
					g_moduloTimer = 0;
				}
				if( enetmsg[3] < 5000 ){
					g_packetWait = enetmsg[3];
				} else {
					g_packetWait = 0;
				}
			} else {
                g_moduloBoardNum = 1;
                g_moduloTimer = 0;
                g_packetWait = 0;
            }
            break;
        }
        
        case(CASE_ENET_SET_PACKETSIZE):{
			DREF32(ADC->transReady) = 0;
			if( enetmsg[1]>=MIN_PACKETSIZE && enetmsg[1]<=MAX_RECLEN ){
				DREF32(ADC->recLen) = enetmsg[1];
				g_recLen = enetmsg[1];
				if( enetmsg[2]>=MIN_PACKETSIZE && enetmsg[2]<=enetmsg[1] ){
					g_packetsize = enetmsg[2];
				} else {
					g_packetsize = g_recLen;
				}
				//~ printf("[ recLen, packetsize ] set to [ %zu, %zu ]\n",DREF32(ADC->recLen), g_packetsize);
			} else {
				DREF32(ADC->recLen) = 2048;
				g_recLen = DREF32(ADC->recLen);
                g_packetsize = 2048;
				printf("invalid recLen, defaulting to 2048, packetsize to 512\n");
			}
            g_numPorts = (g_recLen-1)/g_packetsize + 1;
            setDataAddrPointers(ENET, adcData);
			break;
        }
		
		
		case(CASE_TX_RECV_OUTPUT_CONTROL_REG_SINGLE):{// loads board-specific data from onboard file
			DREFP32(TX->instructionReg)[enetmsg[1]] = enetmsg[2];
			//~ printf("trigvals[%d], %lu, %lu\n0b",enetmsg[1],enetmsg[2],DREFP32(TX->instructionReg)[enetmsg[1]]);
			//~ for (int j = 31; j>0; j--){
				//~ if((DREFP32(TX->instructionReg)[enetmsg[1]] & (1<<j)))
				//~ printf("1");
				//~ else
				//~ printf("0");
			//~ }
			//~ printf("\n");
			break;
		}
		
		case(CASE_TX_RECV_TIMING_REG_SINGLE):{// loads board-specific data from onboard file
			DREFP32(TX->timingReg)[enetmsg[1]] = (enetmsg[2] > 0) ? ( enetmsg[2]-1 ) : 0;
			//~ printf("trigwaits[%d], %d, %lu\n",enetmsg[1],enetmsg[2],DREFP32(TX->timingReg)[enetmsg[1]]);
			break;
		}
		
		case(CASE_TX_RECV_LOOP_CONTROL_REG_SINGLE):{// loads board-specific data from onboard file
			DREFP32(TX->loopAddressReg)[enetmsg[1]] = ( ( enetmsg[2] & 0xffff ) | ( ( enetmsg[3] << 16 ) & 0xffff0000 ) );
			DREFP32(TX->loopCounterReg)[enetmsg[1]] = ( ( enetmsg[4] & 0xffff ) | ( ( enetmsg[5] << 16 ) & 0xffff0000 ) );
			DREFP32(TX->instructionReg)[enetmsg[3]] |= TX_IS_LOOP_ENDPOINT_BIT ;
			//~ printf("loopControl[%d], %lu\n",enetmsg[1],(long unsigned int)enetmsg[2]);
			//~ printf("loopControl[%d], %lu\n",enetmsg[1],(long unsigned int)(DREFP32(TX->loopControlReg)[enetmsg[1]]));
			break;
		}
		
		
		case(CASE_RESET_TX_REGISTERS_ALL):{
			//~ printf("reseting regs\n");
			memset(DREFP32(TX->instructionReg),0,TX_OUTPUTCONTROLREGISTER_SPAN);
			memset(DREFP32(TX->timingReg),0,TX_TIMINGREGISTER_SPAN);
			memset(DREFP32(TX->loopAddressReg),0,TX_LOOPADDRESSREGISTER_SPAN);
			memset(DREFP32(TX->loopCounterReg),0,TX_LOOPCOUNTERREGISTER_SPAN);
			memset(DREFP32(TX->dmaOutputControlReg),0,TX_DMAOUTPUTCONTROLREGISTER_SPAN);
			memset(DREFP32(TX->dmaTimingReg),0,TX_DMATIMINGREGISTER_SPAN);
			//~ printf("regs reseted\n");
			break;
		}
		
		case(CASE_ADC_POWER):{
			DREF32(ADC->controlComms) = (enetmsg[1]<<7);
			printf("lvds power = %lu\n",enetmsg[1]);	
			sleep(0.2);
			break;
		}
		
		case(CASE_ADC_SYNC):{
			sleep(0.25);
			break;
		}
		
		case(CASE_ADC_INITIALIZE):{
			
			DREF32(ADC->serialCommandAddr) = ADC_SOFTWARE_RESET_ADDR;
			usleep(50);
			DREF32(ADC->serialCommand) = ADC_SOFTWARE_RESET_CMD;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
				
			DREF32(ADC->serialCommandAddr) = ADC_SET_TGC_ADDR;
			usleep(50);
			DREF32(ADC->serialCommand) = ADC_SET_TGC_CMD;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);

			DREF32(ADC->serialCommandAddr) = ADC_SET_FIXED_GAIN_ADDR;
			usleep(50);
			DREF32(ADC->serialCommand) = ADC_SET_FIXED_GAIN_CMD;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
			
			break;
		}
		
		case(CASE_ADC_SAMPLING_FREQ):{
			
			sleep(0.25);
			break;
		}
		
		case(CASE_ADC_GAIN):{
			DREF32(ADC->serialCommandAddr) = ADC_SET_COARSE_GAIN_ADDR;
			usleep(50);
			DREF32(ADC->serialCommand) = ADC_SET_COARSE_GAIN_CMD( enetmsg[1] );
			usleep(50);
			DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
			
			
			DREF32(ADC->serialCommandAddr) = ADC_SET_FINE_GAIN_ADDR;
			usleep(50);
			DREF32(ADC->serialCommand) = ADC_SET_FINE_GAIN_CMD( enetmsg[2] );
			usleep(50);
			DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
			
			printf("gain -- enetmsg[1]: %d, enetmsg[2]: %d\n",enetmsg[1], enetmsg[2]);
			
			break;
		}
			
		case(CASE_ADC_DIRECT_INSTRUCTION):{
			DREF32(ADC->serialCommandAddr) = (enetmsg[1] & 0xff);
			printf("%d,%d\n", enetmsg[1], enetmsg[2]);
			usleep(50);
			DREF32(ADC->serialCommand) = (enetmsg[2] & 0xffff);
			usleep(50);
			DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
			break;
		}
		
		case(CASE_INTERRUPT_THYSELF):{	
			DREF32(ADC->controlComms) = (1<<7) | (1<<3);
			clock_gettime(CLOCK_MONOTONIC, &gstart);
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
			break;
		}
		
		
		case(CASE_UNINTERRUPT_THYSELF):{	
			DREF32(ADC->controlComms) = (1<<7) | (1<<4);
			usleep(50);
			DREF32(ADC->controlComms) = ADC_IDLE_STATE;
			sleep(0.1);
			break;
		}
		
		case(CASE_ADC_CONTROL_COMMS):{
			DREF32(ADC->serialCommandAddr) = enetmsg[1];
			printf("adccomms[%d], %lu\n",enetmsg[1],(long unsigned int)enetmsg[2]);			
			break;
		}					
		
		case(CASE_TX_CONTROL_COMMS):{// loads board-specific data from onboard file
			DREF32(TX->controlComms) = enetmsg[1];
			printf("txcomms[%d], %lu\n",enetmsg[1],(long unsigned int)enetmsg[2]);
			break;
		}
		
		case(CASE_TX_RECV_PHASE_DELAYS_SINGLE):{
			memcpy(&phaseDelays[enetmsg[1]*TX_NCHAN],&enetmsg[2],TX_NCHAN*sizeof(uint32_t));
			printf("phaseDelays[%lu] set\n",enetmsg[1]);
			break;
		}
		
		case(CASE_TX_RECV_CHARGE_TIMES_SINGLE):{
			memcpy(&chargeTimes[enetmsg[1]*TX_NCHAN],&enetmsg[2],TX_NCHAN*sizeof(uint32_t));
			printf("chargeTimes[%lu] set\n",enetmsg[1]);
			break;		
		}
		
		case(CASE_TX_USER_MASK):{
			DREF32(TX->channelMask) = enetmsg[1];
			printf("channelMask [%lu] set\n",enetmsg[1]);
			break;		
		}
		
		case(CASE_TX_DISABLE_TRANSDUCER_SAFETY_TIMEOUT):{
			DREF32(TX->disableTransducerSafety) = enetmsg[1];
			printf("safetyDisabled set to [%lu] \n",enetmsg[1]);
			break;		
		}
		
		default:{
			printf("default case, doing nothing\n");
            break;
		}
		
	}
}














