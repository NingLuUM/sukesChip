// Is black box, comments are for losers. 
// Abandon all hope ye who enter.


#include <sys/mman.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // unused
#include <unistd.h> 
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <error.h>
#include <errno.h>

#define soc_cv_av

#include "hwlib.h"  // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/hwlib.h
#include "soc_cv_av/socal/socal.h" // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/socal.h
#include "soc_cv_av/socal/hps.h" // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/hps.h
#include "soc_cv_av/socal/alt_gpio.h"       
#include "hps_0_fancy.h"


#define HW_REGS_BASE ( ALT_STM_OFST )  
#define HW_REGS_SPAN ( 0x04000000 )   
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

// HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, FPGA->fd,ALT_AXI_FPGASLVS_OFST
//setting for the HPS2FPGA AXI Bridge
#define ALT_AXI_FPGASLVS_OFST ( 0xC0000000 ) // axi_master
#define HW_FPGA_AXI_SPAN ( 0x40000000 ) // Bridge span 1GB
#define HW_FPGA_AXI_MASK ( HW_FPGA_AXI_SPAN - 1 )

#define MAX_ENET_TRANSMIT_SIZE ( 1460 )
#define ADC_CLK		(25)	// MHz
#define ADC_NBITS	(12)
#define ADC_NCHAN	(8)

#define ADC_BYTES_PER_TIMEPOINT ((ADC_NBITS*ADC_NCHAN)/8)

#define TX_CLK		(100)
#define TX_NCHAN	(8)
#define MAX_SOCKETS (64)
#define ENET_MSG_SIZE ( 10 )

#define DREFPTV(X)	( (struct timeval *) X )
#define DREFTV(X)	( *(struct timeval *) X )
#define DREF8(X)	( *(uint8_t *) X )
#define DREF16(X)	( *( uint16_t *) X )
#define DREF32(X)	( *( uint32_t *) X )
#define DREFP8(X)	( ( uint8_t *) X )
#define DREFP16(X)	( ( uint16_t *) X )
#define DREFP32(X)	( ( uint32_t *) X )
#define DREFP32S(X)	( ( int32_t *) X )

#define ADC_NBITS	(12)
#define ADC_NCHAN	(8)
#define ADC_BYTES_PER_TIMEPOINT ((ADC_NBITS*ADC_NCHAN)/8)

#define ADC_SERIAL_CLOCK_RATE		( 1 )

// states defined in the lvds.v module
#define ADC_POWER_OFF				( 0x00 )
#define ADC_POWER_ON				( 0x80 )

// lvds needs to be powered on to issue commands so all states other 
// than power off are defined as ( ADC_POWER_ON | STATE )
#define ADC_IDLE_STATE				( ADC_POWER_ON | 0x00 )
#define ADC_BUFFER_SERIAL_COMMAND	( ADC_POWER_ON | 0x01 )
#define ADC_ISSUE_SERIAL_COMMAND	( ADC_POWER_ON | 0x02 )
#define ADC_SYNC_COMMAND			( ADC_POWER_ON | 0x04 )


// address field of the lvds serial commands
#define ADC_SOFTWARE_RESET_ADDR 	( 0x00 )
#define ADC_SET_TGC_ADDR 			( 0x00 )
#define ADC_SET_UNSIGNED_INT_ADDR	( 0x04 )
#define ADC_SET_FIXED_GAIN_ADDR 	( 0x99 )
#define ADC_SET_COARSE_GAIN_ADDR 	( 0x9a )
#define ADC_SET_FINE_GAIN_ADDR		( 0x99 )

// command field of the lvds serial commands
#define ADC_SOFTWARE_RESET_CMD		( 0x0001 )
#define ADC_SET_TGC_CMD 			( 0x0004 )
#define ADC_SET_UNSIGNED_INT_CMD	( 0x0008 )
#define ADC_SET_FIXED_GAIN_CMD		( 0x0008 ) 
#define ADC_SET_COARSE_GAIN_CMD(X)	( (X) & 0x003f ) 
#define ADC_SET_FINE_GAIN_CMD(X) 	( ( (X) & 0x0007 ) | 0x0008 )

// commands for Output_Control_Module
#define	TX_HARD_RESET 			( 0x00 )
#define	TX_SOFT_RESET 			( 0x01 )
#define TX_LOAD_PROGRAM			( 0x02 )
#define TX_SET_TRIG_OUTPUT		( 0x04 )
#define TX_SET_TRIG_REST_LEVEL	( 0x05 )
#define TX_TEST_TRIG_OUTPUT		( 0x06 )
#define TX_SET_LED_OUTPUT		( 0x08 )
#define TX_RUN_PROGRAM			( 0x80 )


#define INIT_PORT 3400

#define CASE_BOARD_SETTINGS 0
#define CASE_ADC_SETTINGS 1
#define CASE_TX_SETTINGS 2

// adc settings
#define CASE_ADC_RECORD_LENGTH 0
#define CASE_ADC_TRIGGER_DELAY 1
#define CASE_ADC_POWER 2
#define CASE_ADC_SYNC 3
#define CASE_ADC_INITIALIZE 4		
#define CASE_ADC_GAIN 5
#define CASE_ADC_DIRECT_SERIAL_COMMAND 6
#define CASE_INTERRUPT_THYSELF 7
#define CASE_UNINTERRUPT_THYSELF 8
#define CASE_ADC_DIRECT_CONTROL_COMMS 9

// interrupt messages from adc
#define ADC_INTERRUPT_DATA_IS_READY (0x01)
#define RCV_UNSET_INTERRUPT (0x90) //0b10010000

// case flags for switch statement in FPGA_dataAcqController


//~ #define CASE_ADC_STATE 5
//~ #define CASE_ADC_STATE_RESET 6
//~ #define CASE_ADC_RECORD_LENGTH 7
//~ #define CASE_ADC_TRIGGER_DELAY 8
//~ #define CASE_ADC_TOGGLE_DATA_ACQ 9
//~ #define CASE_QUERY_ADC_FOR_DATA 10
//~ #define CASE_SET_QUERY_DATA_TIMEOUT 11
//~ #define CASE_QUERY_BOARD_INFO 12

//~ #define CASE_ENET_INTERLEAVE_DEPTH_AND_TIMER 13
//~ #define CASE_ENET_SET_PACKETSIZE 14

//~ #define CASE_ADC_DIRECT_SERIAL_COMMAND 15
//~ #define CASE_ADC_SYNC 16

//~ #define CASE_TX_CONTROL_COMMS 20
//~ #define CASE_TX_RECV_OUTPUT_CONTROL_REG_SINGLE 21
//~ #define CASE_TX_RECV_TIMING_REG_SINGLE 22
//~ #define CASE_TX_RECV_LOOP_CONTROL_REG_SINGLE 23

//~ #define CASE_RESET_TX_REGISTERS_ALL 27

//~ #define CASE_TX_RECV_PHASE_DELAYS_SINGLE 28
//~ #define CASE_TX_RECV_CHARGE_TIMES_SINGLE 29
//~ #define CASE_TX_INIT_PROGRAM_UPLOAD 30

//~ #define CASE_TX_SET_PHASE_DELAY 31
//~ #define CASE_TX_SET_CHARGE_TIME 32
//~ #define CASE_TX_FIRE_COMMAND 33

//~ #define CASE_TX_USER_MASK 34

//~ #define CASE_INTERRUPT_THYSELF 55
//~ #define CASE_UNINTERRUPT_THYSELF 56

#define CASE_CLOSE_PROGRAM 100
#define CASE_KILLPROGRAM 101


#define MAX_RECLEN 8192
#define MIN_PACKETSIZE 128

#define COMM_PORT 0
#define TX_RECV_PORT 500

uint32_t g_boardData[10] = {0};
uint32_t g_boardNum;


int epfd;
struct epoll_event ev;
struct epoll_event events[MAX_SOCKETS];


int RUN_MAIN = 1;
const int ONE = 1;
const int ZERO = 0;	


size_t enetMsgSize = ENET_MSG_SIZE*sizeof(uint32_t);

int g_dataAcqGo=0;

const char *g_serverIP;

struct timespec gstart, gend;

// load user defined functions 
#include "structure_defs.h"
#include "enet_funcs.h"

FPGAvars FPGA;
BOARDdata BOARD;

//~ ADCchip ADC;
//~ RCVsys RCV;
//~ TXvars TX;


//~ int epfd;
//~ struct epoll_event ev;
//~ struct epoll_event events[MAX_SOCKETS];

//~ const int ONE = 1;
//~ const int ZERO = 0;	




int main(int argc, char *argv[]) { printf("into main!\n");
	
	g_serverIP=argv[1];
	
	FPGA_init(&FPGA); 
	loadBoardData(&BOARD);
	
	// create the function to poll sockets for activity
	epfd = epoll_create(MAX_SOCKETS);
	
    // create ethernet socket to communicate with server and establish connection
	ENETsock *ENET = NULL;
	ENET_init(&ENET);
	
	// create connections to the interrupt lines
	//~ ENETsock *INTR = NULL;
	//~ connectInterrupt_intr(&INTR,"gpio@0x100000000",0); // tx interrupt 0
	//~ connectInterrupt_intr(&INTR,"gpio@0x100000010",1); // tx interrupt 1
	
	
	int timeout_ms = 1000;
	int nfds;
	
	while(RUN_MAIN == 1){
		printf("into loop!\n");
		nfds = epoll_wait(epfd, events, MAX_SOCKETS, timeout_ms);
		if( nfds < 0 ){
			perror("error sending data:");
		}
		if( nfds > 0 ){
			
            printf("nfds = %d\n",nfds);
		}

	}
	
	
	
	
    //~ uint32_t enetmsg[ENET_MSG_SIZE] = {0}; // messaging variable to handle messages from cServer
    //~ uint32_t emsg[ENET_MSG_SIZE] = {0}; // messaging variable to send to messages to cServer
	
    //~ // declare and initialize variables for the select loop
	//~ int n;
	//~ char dummybuff[100];
	//~ int nready,nrecv;
	//~ int nrecv_reg;
	//~ int nfds;

	
	
	
	
	//~ ENETsock *enet;

    //~ ENET->addPollSock(&ENET,TX_RECV_PORT);
    //~ ENET->addPollSock(&ENET,TX_RECV_PORT);

	//~ int timeout_ms = 1000;
	//~ int ummmm=0;
	//~ struct timespec difftime;
	
	//~ while(RUN_MAIN == 1){

		//~ nfds = epoll_wait(epfd, events, MAX_SOCKETS, timeout_ms);
		//~ if( nfds < 0 ){
			//~ perror("error sending data:");
		//~ }
		//~ if( nfds > 0 ){
			
            //~ for(n = 0; n < nfds; n++){

                //~ enet = (ENETsock *)events[n].data.ptr;
                
			    //~ if ( enet->is_tx_interrupt ) {
					//~ clock_gettime(CLOCK_MONOTONIC, &gend);
					//~ difftime = diff(gstart,gend);
					
					
				//~ } else if ( enet->is_rcv_interrupt ) {
					//~ if( RCV.getInterruptMsg(&RCV) == ADC_INTERRUPT_DATA_IS_READY ){
                        //~ if( RCV.isLocal ){
                            //~ if ( RCV.copyDataToMem( &RCV ) ){
                                //~ RCV.currentPulse++;
                                //~ DREF32(RCV.controlComms) = RCV_UNSET_INTERRUPT; 
                            //~ } else {
                                //~ printf("error copying data from RCV to local\n");
                            //~ }
                            //~ // TODO: set up actions for when data is done being captured locally
                            //~ if ( RCV.currentPulse == RCV.nPulses ) {
                                //~ printf("now what?\n");
                            //~ }
                        //~ } else {
						    //~ ENET->sendAcqdData(&ENET,&RCV,0);
                        //~ }
					//~ }
					
				//~ } else if ( enet->is_commsock ) {
					//~ nrecv = recv(enet->sockfd,&enetmsg,enetMsgSize,0);	
					//~ setsockopt(enet->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
					
					//~ if(nrecv > 0){
						
						//~ switch(enetmsg[0]){
							//~ case(CASE_BOARD_SETTINGS):{
								//~ BOARD_settings(&RCV, &ENET, &enetmsg[1]);
								//~ break;
							//~ }
							
							//~ case(CASE_ADC_SETTINGS):{
								//~ ADC_Settings(&ADC, &ENET, INTR, &enetmsg[1]);							
								//~ break;
							//~ }
							
							//~ case(CASE_TX_SETTINGS):{
                                //~ TX_Settings(&TX, &ENET, &enetmsg[1]); 
								
								//~ break;
							//~ }
															
							//~ case(CASE_CLOSE_PROGRAM):{
								//~ RUN_MAIN = 0;
								//~ break;
							//~ }
							
							//~ case(CASE_KILLPROGRAM):{
								//~ RUN_MAIN = 0;
								//~ break;
							//~ }
							
							//~ default:{
								//~ ADC_Controller(&FPGA, &ADC, &TX, &ENET, phaseDelays, chargeTimes);
								//~ break;
							//~ }
						//~ }
					//~ } else {
						//~ printf("nrecv = %d\n",nrecv);
						//~ RUN_MAIN = 0;
					//~ }

				//~ } else if ( enet->is_tx_recvsock ) {
                    //~ ntxrecv = recv(enet->sockfd,&txrcv_buff,TX->recvBuff,MSG_WAITALL); // grr... waitall. much inefficient.
                    //~ setsockopt(enet->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
                    //~ TX->parseRecvdInstructions(TX);
                //~ } else {
					//~ DREF32(TX.controlComms) = 0;
					//~ nrecv = recv(enet->sockfd,&enetmsg,enetMsgSize,0);
                    //~ if(nrecv == 0){
                        //~ disconnectSock(&ENET, enet->portNum);
                    //~ } else if (nrecv == -1){
                        //~ perror("recv being dumb\n");
                    //~ } else {
                        //~ printf("illegal recv (n = %d) on port %d, msg = [%lu, %lu, %lu, %lu]\n, shutting down client\n",nrecv,enet->portNum,(unsigned long)enetmsg[0],(unsigned long)enetmsg[1],(unsigned long)enetmsg[2],(unsigned long)enetmsg[3]);
                        //~ RUN_MAIN = 0;
                    //~ }
				//~ }
			//~ }
		//~ }
	//~ }
    
    //~ while( ENET != NULL ){
        //~ enet = ENET;
        //~ ENET = enet->next;
        //~ free(enet);
    //~ }
    
    //~ free(*(ADC.data));
    //~ free(ADC.data);
    //~ free(txOutputControlReg); free(txTimingReg);
    //~ free(txLoopAddressReg); free(txLoopCounterReg);
    //~ free(phaseDelays); free(chargeTimes);
    
    //~ FPGAclose(&FPGA);
	//~ sleep(1);
    //~ return( 0 );
}

 

 









 






