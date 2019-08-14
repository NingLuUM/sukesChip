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
#include "hps_0_fancy.h"

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




#define MAX_ENET_TRANSMIT_SIZE ( 1460 )
#define ADC_CLK		(25)	// MHz
#define ADC_NBITS	(12)
#define ADC_NCHAN	(8)
#define BITS_PER_BYTE (8)

#define TX_CLK		(100)
#define TX_NCHAN	(8)
#define MAX_HPS_MAIN_SOCKETS (10)
#define MAX_RCV_SYS_SOCKETS (64)
#define MAX_TX_SYS_SOCKETS (4)
#define ENET_MSG_SIZE ( 10 )



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


// interrupt messages from adc
#define ADC_INTERRUPT_DATA_IS_READY (0x01)
#define RCV_UNSET_INTERRUPT (0x90) //0b10010000

// case flags for switch statement in FPGA_dataAcqController

#define CASE_CLOSE_PROGRAM 100
#define CASE_KILLPROGRAM 101


#define MAX_RECLEN 8192
#define MIN_PACKETSIZE 128

#define COMM_PORT 0
#define RCV_COMM_PORT 100
#define TX_COMM_PORT 200

#define LAUNCH_RECV_SUBSYS 1
#define CASE_RECV_SYS_ACTIONS 1
#define CASE_TX_SYS_ACTIONS 2
//~ #define IPC_SHARED_MEM_KEY 1234

uint32_t g_boardData[10] = {0};
uint32_t g_boardNum;
uint32_t enetmsg[10] = {0}; // messaging variable to handle messages from cServer
uint32_t emsg[10] = {0}; // messaging variable to send to messages to cServer




int RUN_MAIN = 1;
const int ONE = 1;
const int ZERO = 0;	
key_t g_shmkey_fpga = 1000;
key_t g_shmkey_board = 2000;
key_t g_shmkey_rcv = 3000;

size_t enetMsgSize = ENET_MSG_SIZE*sizeof(uint32_t);

int g_dataAcqGo=0;

const char *g_serverIP;

struct timespec gstart, gend;

// load user defined functions 
#include "structure_defs.h"

#include "enet_funcs.h"
#include "ipc_funcs.h"
#include "adc_funcs.h"
#include "recv_funcs.h"
#include "init_funcs.h"


#include "recv_sys.h"


//~ TXvars TX;
//~ extern void recvSysMain(int);


int main(int argc, char *argv[]) { printf("into main!\n");
	g_serverIP=argv[1];
	
	// variables shared between sub-processes
	// TODO: should probably throw in some mutexs
	FPGAvars_t FPGA;
	FPGA_init(&FPGA);
	
	BOARDdata_t *BOARD;
	BOARD = BOARD_init(1);
	
	// variables req'd to setup epoll to monitor sockets/interrupts for activity
	int epfd;
	struct epoll_event ev;
	struct epoll_event events[MAX_HPS_MAIN_SOCKETS];
	epfd = epoll_create(MAX_HPS_MAIN_SOCKETS);
	
    // create ethernet socket to communicate with server and establish connection
	ENETsock_t **ENET;
	ENETsettings_t enet_settings;
	ENET = (ENETsock_t **)calloc(1,sizeof(ENETsock_t *));
	ENET_init(ENET, &epfd, &ev, events, &enet_settings, COMM_PORT);
	
	// create ipc sockets to communicate with tx/rcv subsystems
	IPCsock_t **IPC;
	IPC = (IPCsock_t **)calloc(2,sizeof(IPCsock_t *));
	IPC[0]=NULL; // rcv subsys ipc
	IPC[1]=NULL; // tx subsys ipc
	
	
	// create connections to the interrupt lines
	//~ ENETsock *INTR = NULL;
	//~ connectInterrupt_intr(&INTR,"gpio@0x100000000",0); // tx interrupt 0
	//~ connectInterrupt_intr(&INTR,"gpio@0x100000010",1); // tx interrupt 1
	
	
	/* contains: fd, socket type, (void *)ptr to parent
	 * makes it easier to poll variables that have a common type
	 * to access the fields of each specific type of socket
	 * cast ptr to parent to appropriate type and dereference it
	*/
	SOCKgeneric_t *sock;
	
	int timeout_ms = 1000;
	int nfds;
	int n;
	int nrecv;
	pid_t cpid;
	
	while(RUN_MAIN == 1){
		printf("into loop!\n");
		sleep(1);
	
        // TODO: THIS IS JUST FOR TESTING. DELETE IT LATER	
		if (IPC[0] == NULL)
			recvSysHandler(IPC, &epfd, &ev, events, &enetmsg[1]);
		
		nfds = epoll_wait(epfd, events, MAX_HPS_MAIN_SOCKETS, timeout_ms);
		if( nfds < 0 ){
			perror("error sending data:");
		}
		if( nfds > 0 ){
			
            printf("nfds = %d\n",nfds);
            for(n = 0; n < nfds; n++){
				
                sock = (SOCKgeneric_t *)events[n].data.ptr;
                nrecv = recv(sock->fd,&enetmsg,enetMsgSize,0);
				setsockopt(sock->fd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
				printf("elbows = %d\n",FPGA->elbows);
				if(nrecv > 0){
					
					printf("nrecv %d\n",nrecv);
					for(int i=0;i<10;i++) printf("\tmsg[%d]:%d\n",i,enetmsg[i]);
					
					if (sock->is.enetCommSock){
						switch(enetmsg[0]){
							case(CASE_RECV_SYS_ACTIONS):{
								
								//~ recvSysHandler(IPC, &epfd, &ev, events, &enetmsg[1]);
								
								break;
							}
							
							case(CASE_TX_SYS_ACTIONS):{
								
								break;
							}
							
							case(CASE_CLOSE_PROGRAM):{
								break;
							}
						}	
					} else {
						printf("nobgo nopgo = %d\n",enetmsg[0]);
						enetmsg[0]++;
						send(IPC[0]->sock.fd, &enetmsg, 10*sizeof(uint32_t), 0);
					}
					
				} else {
					if(sock->is.ipcRx){
						epoll_ctl(epfd, EPOLL_CTL_DEL, sock->fd, &ev);
						close(sock->fd);
						cpid = waitpid( ((IPCsock_t *)(sock->parent))->ipc_pid, NULL , 0 );
						printf("the child is dead, a, %d, %p\n",cpid,(sock->parent));
						if(cpid>0){
							free(sock->parent);
							IPC[0]=NULL;
						}
							
					}	
				}
			}
                
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

 

 









 






