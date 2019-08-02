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


#define DREF8(X)	( *( uint8_t *) X )
#define DREF16(X)	( *( uint16_t *) X )
#define DREF32(X)	( *( uint32_t *) X )
#define DREFP8(X)	( ( uint8_t *) X )
#define DREFP16(X)	( ( uint16_t *) X )
#define DREFP32(X)	( ( uint32_t *) X )
#define DREFP32S(X)	( ( int32_t *) X )



uint32_t g_boardData[10] = {0};
uint32_t g_boardNum;
uint32_t enetmsg[10] = {0}; // messaging variable to handle messages from cServer
uint32_t emsg[10] = {0}; // messaging variable to send to messages to cServer




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
#include "ipc_funcs.h"


//~ ADCchip ADC;
RCVsys RCV;
ENETsettings_t enet_settings;
//~ TXvars TX;
//~ extern void recvSysMain(int);


void launchRecvSys(IPCsock_t *IPC, int *epfd, struct epoll_event *ev, struct epoll_event *events){
	
	int sv[2];
	char buf;
	
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
		perror("socketpair");
		exit(1);
	}
	
	pid_t pid;
	
	pid = fork();
	if(pid == -1){
		printf("forking error!\n");
	} else if (!pid) { // child process
		close(sv[0]);
		//~ recvSysMain(sv[1]);
	} else {
		close(sv[1]);
		IPC->sock.fd = sv[0];
		
		IPC->epfd = epfd;
		IPC->ev = ev;
		IPC->events = events;
		
		IPC->ev->data.ptr = &(IPC->sock);
		IPC->ev->events = EPOLLIN;
		epoll_ctl(*(IPC->epfd), EPOLL_CTL_ADD, IPC->sock.fd, IPC->ev);
	}
}


void recvSysHandler(IPCsock_t **IPC, int *epfd, struct epoll_event *ev, struct epoll_event *events, uint32_t *msg){
	pid_t cpid;
	int status;
	
	if( msg[0] != LAUNCH_RECV_SUBSYS ){
									
		if( IPC[0] == NULL ){
			IPC[0] = (IPCsock_t *)calloc(1,sizeof(IPCsock_t));
			//~ launchRecvSys(IPC[0], epfd, ev, events);
			
		} else {
			cpid = waitpid(IPC[0]->ipc_pid,&status,WNOHANG);
			
			if( !cpid ){
				printf("Can't launch, RCV_SYS still running\n");
				
			} else if (cpid>0){
				
				printf("child exited\n");
				//~ launchRecvSys(IPC[0], epfd, ev, events);
				
			} else {
				
				printf("recv_sys exit() error\n");
				
			}
		}
	}
}



int main(int argc, char *argv[]) { printf("into main!\n");
	g_serverIP=argv[1];
	
	FPGAvars FPGA;
	FPGA_init(&FPGA);
	
	BOARDdata BOARD;
	loadBoardData(&BOARD);
	
	// create the function to poll sockets for activity
	int epfd;
	struct epoll_event ev;
	struct epoll_event events[MAX_SOCKETS];
	epfd = epoll_create(MAX_SOCKETS);
	
    // create ethernet socket to communicate with server and establish connection
	ENETsock_t **ENET;
	ENET = (ENETsock_t **)calloc(1,sizeof(ENETsock_t *));
	ENET_init(ENET, &epfd, &ev, events, &enet_settings, COMM_PORT);
	
	IPCsock_t **IPC;
	IPC = (IPCsock_t **)calloc(1,sizeof(IPCsock_t *));
	setupIPCserver(IPC, &epfd, &ev, events);
	
	// create connections to the interrupt lines
	//~ ENETsock *INTR = NULL;
	//~ connectInterrupt_intr(&INTR,"gpio@0x100000000",0); // tx interrupt 0
	//~ connectInterrupt_intr(&INTR,"gpio@0x100000010",1); // tx interrupt 1
	
	SOCKgeneric_t *sock;
	
	int timeout_ms = 1000;
	int nfds;
	int n;
	int nrecv;
	
	
	while(RUN_MAIN == 1){
		printf("into loop!\n");
		if (IPC[0] == NULL)
			recvSysHandler(IPC, &epfd, &ev, events, &enetmsg[1]);
		nfds = epoll_wait(epfd, events, MAX_SOCKETS, timeout_ms);
		if( nfds < 0 ){
			perror("error sending data:");
		}
		if( nfds > 0 ){
			
            printf("nfds = %d\n",nfds);
            for(n = 0; n < nfds; n++){
				
                sock = (SOCKgeneric_t *)events[n].data.ptr;
                nrecv = recv(sock->fd,&enetmsg,enetMsgSize,0);
				setsockopt(sock->fd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
				
				if(nrecv > 0){
					printf("nrecv %d\n\tmsg[0]:%d\n\tmsg[1]:%d\n\tmsg[2]:%d\n\tmsg[3]:%d\n\tmsg[4]:%d\n\tmsg[5]:%d\n\tmsg[6]:%d\n\tmsg[7]:%d\n\tmsg[8]:%d\n\tmsg[9]:%d\n",nrecv,enetmsg[0],enetmsg[1],enetmsg[2],enetmsg[3],enetmsg[4],enetmsg[5],enetmsg[6],enetmsg[7],enetmsg[8],enetmsg[9]);
					
					if (sock->is.enetCommSock){
						switch(enetmsg[0]){
							case(CASE_RECV_SYS_ACTIONS):{
								
								recvSysHandler(IPC, &epfd, &ev, events, &enetmsg[1]);
								
								break;
							}
							
							case(CASE_TX_SYS_ACTIONS):{
								
								break;
							}
							
							case(CASE_CLOSE_PROGRAM):{
								break;
							}
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

 

 









 






