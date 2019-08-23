// Is black box, comments are for losers. 
// Abandon all hope ye who enter.


#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // unused
#include <unistd.h> 
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <error.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define soc_cv_av

#include "hwlib.h"  // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/hwlib.h
#include "soc_cv_av/socal/socal.h" // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/socal.h
#include "soc_cv_av/socal/hps.h" // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/hps.h
#include "soc_cv_av/socal/alt_gpio.h"       
//~ #include "hps_0_receiveOnly.h"
#include "hps_0_largeRecv.h"

#define HW_REGS_BASE ( ALT_STM_OFST )  
#define HW_REGS_SPAN ( 0x04000000 )   
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

// HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, FPGA->fd,ALT_AXI_FPGASLVS_OFST
//setting for the HPS2FPGA AXI Bridge
#define ALT_AXI_FPGASLVS_OFST ( 0xC0000000 ) // axi_master
#define HW_FPGA_AXI_SPAN ( 0x40000000 ) // Bridge span 1GB
#define HW_FPGA_AXI_MASK ( HW_FPGA_AXI_SPAN - 1 )

#define DREFPTV(X)	( (struct timeval *) X )
#define DREFTV(X)	( *(struct timeval *) X )
#define DREF8(X)	( *(uint8_t *) X )
#define DREF16(X)	( *( uint16_t *) X )
#define DREF32(X)	( *( uint32_t *) X )
#define DREFP8(X)	( ( uint8_t *) X )
#define DREFP16(X)	( ( uint16_t *) X )
#define DREFP32(X)	( ( uint32_t *) X )
#define DREFP32S(X)	( ( int32_t *) X )
#define DREFP64(X)	( ( uint64_t *) X )
#define DREFP64S(X)	( ( int64_t *) X )
#define DREFPCHAR(X)	( ( char *) X )

#define INIT_PORT 3400
#define MAX_RECLEN 32768
#define COMM_PORT 0
#define MAX_SOCKETS (10)
#define ENET_MSG_SIZE (10)

#define ADC_CLK		(25)	// MHz
#define ADC_NBITS	(12)
#define ADC_NCHAN	(8)
#define BITS_PER_BYTE (8)

// states defined in the lvds.v module
#define ADC_HARDWARE_RESET				( 0xFF )

#define GEN_SERIAL_CMD(ADDR,CMD) ( ( ADDR<<16 ) | CMD )

#define ADC_IDLE_STATE				( 0x00 )
#define ADC_BUFFER_SERIAL_COMMAND	( 0x01 )
#define ADC_ISSUE_SERIAL_COMMAND	( 0x02 )
#define ADC_SYNC_COMMAND			( 0x04 )

struct timespec gstart, gend;

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
	
	//~ clock_gettime(CLOCK_MONOTONIC, &gstart);
	//~ something();
	//~ clock_gettime(CLOCK_MONOTONIC, &gend);
	//~ difftime = diff(gstart,gend);
	//~ printf("adcmemcpy: %ld us\n",difftime.tv_nsec/1000);
};

#include "client_funcs.h"

void cmdFileReader(ADCvars_t *ADC){
	ADCREG_t cmdreg;
	FILE *cmdfile = fopen("command_file.txt","r");
	char line[256];
	
    while( fgets(line, sizeof(line), cmdfile) ){
        cmdreg.adccmd = (uint32_t )atoi(line);
        printf("fileread: %u\n",cmdreg.adccmd);
        if(cmdreg.adccmd){
			adcIssueSerialCmd(ADC,(cmdreg.adccmd & 0x00ffffff) );
		}
        usleep(1000);
    }  
    fclose(cmdfile);
	
}



int main(int argc, char *argv[]) { printf("into main! %d\n",argc);
	
	remove("data_file.dat");
	
	FPGAvars_t FPGA;
	ADCvars_t ADC;
	
	FPGA_init(&FPGA);
	ADC_init(&FPGA,&ADC);

	setLEDS(&ADC,0x1f);

	uint32_t recLen = 2048;
	uint32_t varGain = 0;
	double gain = 0.0;
	uint32_t set_unsigned = 0;
	uint32_t set_lownoise = 0;
	
	if(argc>1) recLen = (uint32_t )atoi(argv[1]);
	if(argc>2) varGain = (uint32_t )atoi(argv[2]);
	if(argc>3) gain = atof(argv[3]);
	if(argc>4) set_unsigned = (uint32_t )atoi(argv[4]);
	if(argc>5) set_lownoise = (uint32_t )atoi(argv[5]);
	
	setRecLen(&ADC,recLen);
	setVarGain(&ADC,varGain);
	
	adcInitializeSettings(&ADC);
	usleep(100);
	
	adcSetGain(&ADC,gain);
	usleep(100);
	adcSetDTypeUnsignedInt(&ADC, set_unsigned);
	usleep(100);
	adcSetLowNoiseMode(&ADC, set_lownoise);

	adcSetReg1(&ADC);
	usleep(100);
	adcSetReg7(&ADC);
	usleep(100);
	
	//~ cmdFileReader(&ADC);

	queryData(&ADC);
	
	FPGAclose(&FPGA);
	return(0);
}

 

 









 






