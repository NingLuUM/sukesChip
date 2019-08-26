

#include "adc_register_defs.h"
#include "structure_defs.h"

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

void setnonblocking(int sockfd){
    int opts;
    if((opts=fcntl(sockfd,F_GETFL))<0) perror("GETFL nonblocking failed");

    opts = opts | O_NONBLOCK;
    if(fcntl(sockfd,F_SETFL,opts)<0) perror("SETFL nonblocking failed");
}

void addEnetServerSock(POLLserver_t *ps, SOCK_t *sock, int portNum){
    
    struct sockaddr_in server;

    sock->is.flags = 0;
    sock->is.listener = 1;
    sock->portNum = portNum;
    sock->ps = ps;

    sock->fd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(sock->fd);
    ps->ev.data.ptr = sock;
    ps->ev.events = EPOLLIN;
    epoll_ctl(ps->epfd,EPOLL_CTL_ADD,sock->fd,&(ps->ev));

    memset(&server,0,sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(portNum);

    setsockopt(sock->fd,SOL_SOCKET,SO_REUSEADDR, &ONE, sizeof(int));

    if( bind(sock->fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in))<0){
        perror("ERROR binding socket");
        exit(1);
    }
    if( listen(sock->fd, MAX_SERVER_QUEUE_LENGTH) ){
        perror("ERROR listening sock->fd");
        exit(1);
    }

}

void acceptEnetClientSock(SOCK_t *servsock){
    struct sockaddr_in client, peername;
    socklen_t clilen, peerlen;
    
    POLLserver_t *ps;
    ps = servsock->ps;
    
    SOCK_t *clisock;
    clisock = servsock->partner;

    memset(&client,0,sizeof(struct sockaddr_in));
    clilen = sizeof(client);
    memset(&peername,0,sizeof(struct sockaddr_in));
    peerlen = sizeof(peername);

    clisock->fd = accept(servsock->fd, (struct sockaddr *)&client, &clilen);

    if( getpeername(clisock->fd, (struct sockaddr *)&peername, &peerlen) ){
        perror("ERROR getting peername");
    }

    clisock->is.flags = 0;
    clisock->portNum = servsock->portNum;
    if(clisock->portNum == INIT_PORT){
        clisock->is.commsock = 1;
    } else if (clisock->portNum == ADC_CONTROL_PORT){
        clisock->is.adc_control = 1;
    }

    setnonblocking(clisock->fd);
    setsockopt(clisock->fd,IPPROTO_TCP,TCP_NODELAY,&ONE,sizeof(int));

    clisock->ps = ps;
    ps->ev.data.ptr = clisock;
    ps->ev.events = EPOLLIN;

    epoll_ctl(ps->epfd,EPOLL_CTL_ADD,clisock->fd,&(ps->ev));
}

void disconnectPollSock(SOCK_t *tmp){

	epoll_ctl(tmp->ps->epfd, EPOLL_CTL_DEL, tmp->fd, &(tmp->ps->ev));
	close(tmp->fd);
    tmp->is.flags = 0;
    tmp->portNum = 0;
    tmp->ps = NULL;
    tmp->fd = 0;
}

void connectPollInterrupter(POLLserver_t *ps, ADCvars_t *ADC, char *gpio_lab){
	 
	// CHANGE THIS TO DESIRED LABEL
	const char *gpio_label = gpio_lab;
	const char *edge_type = "rising";

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

	//~ printf("path: ");
	//~ printf("%s",path);
	//~ printf("\n");

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

	ADC->interrupt.fd = file_fd;
    ADC->interrupt.ps = ps;
    ADC->interrupt.is.flags = 0;
    ADC->interrupt.is.rcv_interrupt = 1;
    ADC->interrupt.portNum = 0;
	
	result = read(ADC->interrupt.fd, buffer, PATH_MAX);
	
    ps->ev.data.ptr = &(ADC->interrupt);
    ps->ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(ps->epfd, EPOLL_CTL_ADD, ADC->interrupt.fd, &(ps->ev));
    
    // need to have epoll_wait once before interrupt can really be used
    epoll_wait(ps->epfd, ps->events, MAX_POLL_EVENTS, 10); // 10 ms
}

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

int ADC_init(FPGAvars_t *FPGA, ADCvars_t *ADC){
	
	ADC->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->pioVarGain = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_VAR_GAIN_SETTING_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->dataReadyFlag = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + DATA_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->leds = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	ADC->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

	ADC->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

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
	
    ADC->reg_dict[0][0] = 0;
    ADC->reg_dict[0][1] = 1;
    ADC->reg_dict[0][2] = 2;
    ADC->reg_dict[0][3] = 3;
    ADC->reg_dict[0][4] = 4;
    ADC->reg_dict[0][5] = 5;
    ADC->reg_dict[0][7] = 6;
    ADC->reg_dict[0][13] = 7;
    ADC->reg_dict[0][15] = 8;
    ADC->reg_dict[0][17] = 9;
    ADC->reg_dict[0][19] = 10;
    ADC->reg_dict[0][31] = 11;
    ADC->reg_dict[0][29] = 12;
    ADC->reg_dict[0][27] = 13;
    ADC->reg_dict[0][25] = 14;
    ADC->reg_dict[0][21] = 15;
    ADC->reg_dict[0][33] = 16;
    ADC->reg_dict[0][70] = 17;
    
	for(uint32_t i=1; i<0x95; i++){
        ADC->reg_dict[1][i] = i+17;
	}
    ADC->reg_dict[1][0x95] = 0x95+17;
    ADC->reg_dict[1][0x96] = 0x96+17;
    ADC->reg_dict[1][0x97] = 0x97+17;
    ADC->reg_dict[1][0x98] = 0x98+17;
    ADC->reg_dict[1][0x99] = 0x99+17;
    ADC->reg_dict[1][0x9A] = 0x9A+17;
    ADC->reg_dict[1][0x9B] = 0x9B+17;
    
    DREF32(ADC->pioVarGain) = 0;
	DREF32(ADC->recLen) = 2048;
	
	DREF32(ADC->stateReset)=1; 
	usleep(10);
	
	DREF32(ADC->controlComms) = ADC_HARDWARE_RESET;
	usleep(100000);
	
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(10);
	
	ADC->gpreg0.SOFTWARE_RESET = 1;
	adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);
	
	ADC->gpreg0.SOFTWARE_RESET = 0;
	ADC->gpreg0.TGC_REGISTER_WREN = 1;
	adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);

	// WITHOUT INTERP_ENABLE=1, THIS ONLY ALLOWS COARSE_GAIN TO BE SET
	ADC->tgcreg0x99.STATIC_PGA = 1;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x99.adccmd);
	
	// NOT IN MANUAL!!!, NEEDED FOR FINE GAIN CONTROL
	ADC->tgcreg0x97.INTERP_ENABLE = 1;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x97.adccmd);
  
    // GO TO GENERAL PURPOSE REGISTERS
	ADC->gpreg0.TGC_REGISTER_WREN = 0;
	adcIssueSerialCmd(ADC,ADC->gpreg0.adccmd);

    // SET TO DC COUPLING (VARIABE NAME IS MISLEADING/BACKWARDS)
    ADC->gpreg7.INTERNAL_AC_COUPLING = 1;
	adcIssueSerialCmd(ADC,ADC->gpreg7.adccmd);

    // SET DATA TYPE TO UNSIGNED
    ADC->gpreg4.DFS = 1;
    adcIssueSerialCmd(ADC,ADC->gpreg4.adccmd);
    
    ADC->interrupt.ps = NULL;
    
    ADC->recLen_ref = 2048;
    ADC->npulses = 0;
    ADC->queryMode.all = 0;
    ADC->queryMode.sendRealTime = 1;
    ADC->data = (char **)calloc(1,sizeof(char *));
    *(ADC->data) = NULL;
	return(1);
}


