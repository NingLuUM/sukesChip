

struct ENETsock_;
struct FPGAvars_;
struct ADCvars_;
struct SOCKgeneric_;

typedef struct ENETsock_ ENETsock_t;
typedef struct FPGAvars_ FPGAvars_t;
typedef struct ADCvars_ ADCvars_t;
typedef struct SOCKgeneric_ SOCKgeneric_t;

#include "adc_register_defs.h"

typedef struct ENETsock_{
	int sockfd;

	int is_commsock;
	int portNum;

} ENETsock_t;


typedef struct FPGAvars_{ // structure to hold variables that are mapped to the FPGA hardware registers
	void *virtual_base;
	void *axi_virtual_base;
	int fd_pio;
	int fd_ram;	
	
} FPGAvars_t;


typedef struct ADCvars_{
	// memory mapped variables in FPGA
	uint32_t volatile *stateReset;
	uint32_t volatile *controlComms;
	uint32_t volatile *recLen;
	uint32_t volatile *varGain;
	uint32_t volatile *serialCommand;
	uint32_t volatile *dataReadyFlag;
	uint32_t volatile *leds;
	
	REG0_t reg0;
	REG1_t reg1;
	REG2_t reg2;
	REG3_t reg3;
	REG4_t reg4;
	REG5_t reg5;
	REG7_t reg7;
	REG13_t reg13;
	REG15_t reg15;
	REG17_t reg17;
	REG19_t reg19;
	REG21_t reg21;
	REG25_t reg25;
	REG27_t reg27;
	REG29_t reg29;
	REG31_t reg31;
	REG33_t reg33;
	REG70_t reg70;
	
	TGCREG_0x01_0x94_t tgcreg0x01[148];
	
	TGCREG_0x95_t tgcreg0x95;
	TGCREG_0x96_t tgcreg0x96;
	TGCREG_0x97_t tgcreg0x97;
	TGCREG_0x98_t tgcreg0x98;
	TGCREG_0x99_t tgcreg0x99;
	TGCREG_0x9A_t tgcreg0x9A;
	TGCREG_0x9B_t tgcreg0x9B;
	
	ADCREG_t *adcreg[18];
	
	uint32_t volatile *ramBank0;
	uint32_t volatile *ramBank1;
	
	int interrupt_fd;
	int epfd;
	struct epoll_event ev;
	struct epoll_event events[5];
	
	
} ADCvars_t;


typedef union LED_{
	struct{
		uint32_t hiRest : 3;
		uint32_t loRest : 2;
		uint32_t zero : 27;
	};
	uint32_t vals;
} LED_t;

typedef union BANK0_{
	struct {
		uint64_t ch0 : 12;
		uint64_t ch1 : 12;
		uint64_t ch2 : 12;
		uint64_t ch3 : 12;
		uint64_t ch4 : 12;
		uint64_t ch5lo : 4;
	}u;
	struct{
		int64_t ch0 : 12;
		int64_t ch1 : 12;
		int64_t ch2 : 12;
		int64_t ch3 : 12;
		int64_t ch4 : 12;
		int64_t ch5lo : 4;
	}s;
} BANK0_t;

typedef union BANK1_{
	struct{
		uint32_t ch5hi : 8;
		uint32_t ch6 : 12;
		uint32_t ch7 : 12;
	}u;
	struct{
		int32_t ch5hi : 8;
		int32_t ch6 : 12;
		int32_t ch7 : 12;
	}s;
} BANK1_t;


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


void connectPollInterrupter(ADCvars_t *ADC, char *gpio_lab){
	 
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

	ADC->interrupt_fd = file_fd;
	
	result = read(ADC->interrupt_fd, buffer, PATH_MAX);
	
    ADC->ev.data.fd = ADC->interrupt_fd;
    ADC->ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(ADC->epfd, EPOLL_CTL_ADD, ADC->interrupt_fd, &(ADC->ev));
    
    // need to have epoll_wait once before interrupt can really be used
    epoll_wait(ADC->epfd, ADC->events, 5, 10); // 10 ms
}


void disconnectPollInterrupter(ADCvars_t *ADC){
	epoll_ctl(ADC->epfd, EPOLL_CTL_DEL, ADC->interrupt_fd, &(ADC->ev));
	close(ADC->interrupt_fd);
}



int ADC_init(FPGAvars_t *FPGA, ADCvars_t *ADC){
	
	ADC->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->varGain = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_VAR_GAIN_SETTING_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->dataReadyFlag = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + DATA_READY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	ADC->leds = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	ADC->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

	ADC->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

	ADC->reg0.adccmd=0; 		ADC->reg0.addr=0;	
	ADC->reg1.adccmd=0;			ADC->reg1.addr=1;
	ADC->reg2.adccmd=0;			ADC->reg2.addr=2;
	ADC->reg3.adccmd=0;			ADC->reg3.addr=3;
	ADC->reg4.adccmd=0;			ADC->reg4.addr=4;
	ADC->reg5.adccmd=0;			ADC->reg5.addr=5;
	ADC->reg7.adccmd=0;			ADC->reg7.addr=7;
	ADC->reg13.adccmd=0;		ADC->reg13.addr=13;
	ADC->reg15.adccmd=0;		ADC->reg15.addr=15;
	ADC->reg17.adccmd=0;		ADC->reg17.addr=17;
	ADC->reg19.adccmd=0;		ADC->reg19.addr=19;
	ADC->reg21.adccmd=0;		ADC->reg21.addr=21;
	ADC->reg25.adccmd=0;		ADC->reg25.addr=25;
	ADC->reg27.adccmd=0;		ADC->reg27.addr=27;
	ADC->reg29.adccmd=0;		ADC->reg29.addr=29;
	ADC->reg31.adccmd=0;		ADC->reg31.addr=31;
	ADC->reg33.adccmd=0;		ADC->reg33.addr=33;
	ADC->reg70.adccmd=0;		ADC->reg70.addr=70;
	
	ADC->adcreg[0] = (ADCREG_t *)(&(ADC->reg0));
	ADC->adcreg[1] = (ADCREG_t *)(&(ADC->reg1));
	ADC->adcreg[2] = (ADCREG_t *)(&(ADC->reg2));
	ADC->adcreg[3] = (ADCREG_t *)(&(ADC->reg3));
	ADC->adcreg[4] = (ADCREG_t *)(&(ADC->reg4));
	ADC->adcreg[5] = (ADCREG_t *)(&(ADC->reg5));
	ADC->adcreg[6] = (ADCREG_t *)(&(ADC->reg7));
	ADC->adcreg[7] = (ADCREG_t *)(&(ADC->reg13));
	ADC->adcreg[8] = (ADCREG_t *)(&(ADC->reg15));
	ADC->adcreg[9] = (ADCREG_t *)(&(ADC->reg17));
	ADC->adcreg[10] = (ADCREG_t *)(&(ADC->reg19));
	ADC->adcreg[11] = (ADCREG_t *)(&(ADC->reg31));
	ADC->adcreg[12] = (ADCREG_t *)(&(ADC->reg29));
	ADC->adcreg[13] = (ADCREG_t *)(&(ADC->reg27));
	ADC->adcreg[14] = (ADCREG_t *)(&(ADC->reg25));
	ADC->adcreg[15] = (ADCREG_t *)(&(ADC->reg21));
	ADC->adcreg[16] = (ADCREG_t *)(&(ADC->reg33));
	ADC->adcreg[17] = (ADCREG_t *)(&(ADC->reg70));
	
	for(uint32_t i=0; i<148; i++){
		ADC->tgcreg0x01[i].adccmd = 0;
		ADC->tgcreg0x01[i].addr = i+1;
	}
	
	ADC->tgcreg0x95.adccmd=0; ADC->tgcreg0x95.addr=0x95;
	ADC->tgcreg0x96.adccmd=0; ADC->tgcreg0x96.addr=0x96;
	ADC->tgcreg0x97.adccmd=0; ADC->tgcreg0x97.addr=0x97;
	ADC->tgcreg0x98.adccmd=0; ADC->tgcreg0x98.addr=0x98;
	ADC->tgcreg0x99.adccmd=0; ADC->tgcreg0x99.addr=0x99;
	ADC->tgcreg0x9A.adccmd=0; ADC->tgcreg0x9A.addr=0x9A;
	ADC->tgcreg0x9B.adccmd=0; ADC->tgcreg0x9B.addr=0x9B;
	
	DREF32(ADC->varGain) = 0;
	DREF32(ADC->recLen) = 2048;
	
	DREF32(ADC->stateReset)=1; 
	usleep(10);
	
	DREF32(ADC->controlComms) = ADC_HARDWARE_RESET;
	usleep(100000);
	
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(10);
	
	ADC->epfd = epoll_create(5);
	
	
	return(1);
}


void adcmemcpy( char *dest, char *src, size_t nbytes){

	for(int i=0;i<nbytes;i++){
		dest[i] = src[i];
	}
}


void setRecLen(ADCvars_t *ADC, uint32_t recLen){
	DREF32(ADC->recLen) = recLen-1;
	usleep(5);
}


void adcSync(ADCvars_t *ADC){
	DREF32(ADC->controlComms) = ADC_SYNC_COMMAND;
	usleep(5);
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


void setLEDS(ADCvars_t *ADC, uint32_t val){
	
	DREF32(ADC->leds) = val & 0x1F;
		
}


void setVarGain(ADCvars_t *ADC, uint32_t val){
	
	DREF32(ADC->varGain) = val & 0x03;
		
}


void queryData(ADCvars_t *ADC){
	
	static int nqueries = 0;
	
	// doesn't need to be connected unless trying to collect data. 
	connectPollInterrupter(ADC,"gpio@0x100000000");
	
	clock_t start_timer,end_timer;
	
	int nfds,timeout_ms;
	uint32_t tmp, recLen;
	
	recLen = DREF32(ADC->recLen)+1;
	printf("reclen %u\n",recLen);
	
	char adcData0[MAX_RECLEN*sizeof(int64_t)];
	char adcData1[MAX_RECLEN*sizeof(int32_t)];
	
	BANK0_t *b0 = (BANK0_t *)adcData0;
	BANK1_t *b1 = (BANK1_t *)adcData1;
	
	DREF32(ADC->stateReset)=1;
	usleep(5);
	DREF32(ADC->stateReset)=0;
	usleep(5);
	
	tmp = 0;
	nfds = 0;
	timeout_ms = 10;
	while( !nfds ){
		tmp++;
		if(!(tmp%100)){
			printf("query data waiting... (%d s)\n",tmp/100);
			DREF32(ADC->leds) = ( ( tmp/100 ) & 0x1F );
		}
		nfds = epoll_wait(ADC->epfd, ADC->events, 5, timeout_ms);
		if( nfds < 0 ){
			perror("error sending data:");
		}
		if(DREF32(ADC->dataReadyFlag)){
			printf("nfds %d, dataReadyFlag %u\n",nfds,DREF32(ADC->dataReadyFlag));
			break;
		}
	}
	disconnectPollInterrupter(ADC);
	
	start_timer = clock();
	adcmemcpy(adcData0,DREFPCHAR(ADC->ramBank0),recLen*sizeof(int64_t));
	adcmemcpy(adcData1,DREFPCHAR(ADC->ramBank1),recLen*sizeof(int32_t));
	end_timer = clock();

	printf("adcmemcpy clock: %g us, %ld clks_per_sec\n",((double)(end_timer-start_timer))/CLOCKS_PER_SEC*1e6,CLOCKS_PER_SEC);
		
	FILE *datafile;
	datafile = fopen("data_file.dat","w");
	
	if(ADC->reg4.DFS){	
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
}


void adcInitializeSettings(ADCvars_t *ADC){
	
	DREF32(ADC->controlComms) = ADC_HARDWARE_RESET;
	usleep(100000);
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(1000);
	
	ADC->reg0.SOFTWARE_RESET = 1;
	adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	
	ADC->reg0.SOFTWARE_RESET = 0;
	ADC->reg0.TGC_REGISTER_WREN = 1;
	adcIssueSerialCmd(ADC,ADC->reg0.adccmd);

	// WITHOUT INTERP_ENABLE=1, THIS ONLY ALLOWS COARSE_GAIN TO BE SET
	ADC->tgcreg0x99.STATIC_PGA = 1;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x99.adccmd);
	
	// NOT IN MANUAL!!!, NEEDED FOR FINE GAIN CONTROL
	ADC->tgcreg0x97.INTERP_ENABLE = 1;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x97.adccmd);
}


void adcSetGain(ADCvars_t *ADC, double gainVal){
	
	uint32_t coarseGain, fineGain;
	
	if(gainVal > 0){
		coarseGain = (uint32_t )(gainVal+5.0);
		fineGain = (uint32_t )((((gainVal+5.0)-(double )coarseGain))*8);
	} else {
		coarseGain = (uint32_t )(gainVal+5);
		fineGain = (uint32_t )((1.0-((int)gainVal-gainVal))*8);
	}
	//printf("gainVal: %g, coarseGain: %u, fineGain: %u \n",gainVal,coarseGain,fineGain);

	// set TGC_REG_WREN = 1 to access gain controls
	if( !(ADC->reg0.TGC_REGISTER_WREN) ){
		ADC->reg0.TGC_REGISTER_WREN = 1;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}

	ADC->tgcreg0x99.FINE_GAIN = fineGain;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x99.adccmd);
	
	ADC->tgcreg0x9A.COARSE_GAIN = coarseGain;
	adcIssueSerialCmd(ADC,ADC->tgcreg0x9A.adccmd);
	
	
}

void adcSetDTypeUnsignedInt(ADCvars_t *ADC, uint32_t val){
	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	// set dtype as signed (val=0), unsigned (val=1) int
	if(val){
		ADC->reg4.DFS = 1;
	} else {
		ADC->reg4.DFS = 0;
	}	
	//~ printf("dtype: %u\n",ADC->reg4.adccmd);
	adcIssueSerialCmd(ADC,ADC->reg4.adccmd);
}

void adcSetLowNoiseMode(ADCvars_t *ADC, uint32_t val){
	
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	if(val){
		ADC->reg7.VCA_LOW_NOISE_MODE = 1;
	} else {
		ADC->reg7.VCA_LOW_NOISE_MODE = 0;
	}
	
	adcIssueSerialCmd(ADC,ADC->reg7.adccmd);
}

void adcToggleChannelPwr(ADCvars_t *ADC, uint8_t pwrdOn){
	
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	ADC->reg1.PDN_CHANNEL = pwrdOn;	
	adcIssueSerialCmd(ADC,ADC->reg1.adccmd);
}

void adcSetFilterBW(ADCvars_t *ADC, uint8_t filter){	
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	ADC->reg7.FILTER_BW = 0x02;
	adcIssueSerialCmd(ADC,ADC->reg7.adccmd);
}

void adcSetInternalAcCoupling(ADCvars_t *ADC, uint8_t accoupling){	
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	ADC->reg7.INTERNAL_AC_COUPLING = accoupling;
	adcIssueSerialCmd(ADC,ADC->reg7.adccmd);
}

void adcSetReg1(ADCvars_t *ADC){
	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	ADC->reg1.GLOBAL_PDN = 0;
	ADC->reg1.OUTPUT_DISABLE = 0;
	ADC->reg1.PDN_CHANNEL = 0x00;
	ADC->reg1.STDBY = 0;
	ADC->reg1.LOW_FREQUENCY_NOISE_SUPRESSION = 0;
	ADC->reg1.EXTERNAL_REFERENCE = 0;
	ADC->reg1.OUTPUT_RATE_2X = 0;

	adcIssueSerialCmd(ADC,ADC->reg1.adccmd);
}

void adcSetReg7(ADCvars_t *ADC){

	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->reg0.TGC_REGISTER_WREN ){
		ADC->reg0.TGC_REGISTER_WREN = 0;
		adcIssueSerialCmd(ADC,ADC->reg0.adccmd);
	}
	
	ADC->reg7.VCA_LOW_NOISE_MODE = 0;
	ADC->reg7.FILTER_BW = 0x00;
	ADC->reg7.INTERNAL_AC_COUPLING = 1;
	
	adcIssueSerialCmd(ADC,ADC->reg7.adccmd);
}




