// Is black box, comments are for losers. 
// Abandon all hope ye who enter.


#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // unused
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
//#include <signal.h>
//#include <sys/time.h>
//#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <error.h>
#include <errno.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
#include <math.h>

#define soc_cv_av

#include "hwlib.h"  // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/hwlib.h
#include "soc_cv_av/socal/socal.h" // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/socal.h
#include "soc_cv_av/socal/hps.h" // /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/hps.h
#include "soc_cv_av/socal/alt_gpio.h"       
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
#define ADC_CONTROL_PORT 3500
#define MAX_PACKETSIZE 8192

#define MAX_RECLEN 32768
#define COMM_PORT 0
#define MAX_SOCKETS (10)
#define ENET_MSG_SIZE (10)
#define MAX_POLL_EVENTS (5)
#define MAX_SERVER_PORTS (3)
#define MAX_SERVER_QUEUE_LENGTH (5)

#define ADC_CLK		(25)	// MHz
#define ADC_NBITS	(12)
#define ADC_NCHAN	(8)
#define BITS_PER_BYTE (8)

// states defined in the lvds.v module
#define ADC_HARDWARE_RESET				( 0xFF )

#define ADC_IDLE_STATE				( 0x00 )
#define ADC_BUFFER_SERIAL_COMMAND	( 0x01 )
#define ADC_ISSUE_SERIAL_COMMAND	( 0x02 )
#define ADC_SYNC_COMMAND			( 0x04 )

#define CASE_ADD_DATA_SOCKET 0


#define CASE_SET_RECLEN 0
#define CASE_SET_PIO_VAR_GAIN 1
#define CASE_SET_LEDS 2
#define CASE_QUERY_DATA 3
#define CASE_ADC_SET_GAIN 4
#define CASE_ADC_SET_UNSIGNED 5
#define CASE_ADC_SET_LOW_NOISE_MODE 6
#define CASE_ADC_TOGGLE_CHANNEL_POWER 7
#define CASE_ADC_SET_FILTER_BW 8
#define CASE_ADC_SET_INTERNAL_AC_COUPLING 9
#define CASE_ADC_ISSUE_DIRECT_CMD 10
#define CASE_CONNECT_INTERRUPT 11
#define CASE_SETUP_LOCAL_STORAGE 12
#define CASE_ADC_SET_DEFAULT_SETTINGS 13
#define CASE_SET_QUERY_MODE 14
#define CASE_UPDATE_AUTO_SHUTDOWN_SETTING 15
#define CASE_EXIT_PROGRAM 100

const int ONE = 1;
const int ZERO = 0;
int g_auto_shutdown_enabled = 1;
struct timespec gstart, gend, gdifftime;

#include "client_funcs.h"
#include "adc_funcs.h"

int main(int argc, char *argv[]) { printf("\ninto main!\nargcount:%d\n\n",argc);
	
    int n;
	
    POLLserver_t PS;
    PS.epfd = epoll_create(MAX_POLL_EVENTS);
    
    FPGAvars_t FPGA;
	FPGA_init(&FPGA);
	
    ADCvars_t ADC;
	ADC_init(&FPGA,&ADC);
    connectPollInterrupter(&PS,&ADC,"gpio@0x100000000");
    setLEDS(&ADC,0x1f);

    SOCK_t ENETserver[MAX_SERVER_PORTS];
    addEnetServerSock(&PS,&ENETserver[0],INIT_PORT);
    addEnetServerSock(&PS,&ENETserver[1],ADC_CONTROL_PORT);
    
    SOCK_t ENETclient[MAX_SERVER_PORTS];
    for(n=0;n<MAX_SERVER_PORTS;n++){
        ENETclient[n].ps = NULL;
        ENETclient[n].partner = &ENETserver[n];
        ENETserver[n].partner = &ENETclient[n];
    }

    SOCK_t *sock;
    FMSG_t msg;
    int timeout_ms = 100;
    int nfds;
    int nrecv;
    int tmp = 0;
    int runner = 1;

   while(runner==1){
        
        nfds = epoll_wait(PS.epfd,PS.events,MAX_POLL_EVENTS,timeout_ms);
		
        if( nfds > 0 ){
            for(n=0;n<nfds;n++){
                sock = (SOCK_t *)PS.events[n].data.ptr;

                if( sock->is.listener ){
                    if( sock->partner->ps != NULL ){
                        disconnectPollSock(sock->partner);
                    }
                    acceptEnetClientSock(sock);
                
                } else if ( sock->is.rcv_interrupt ) {
                    queryData(&ADC,&ENETclient[1]); 
                
                } else if ( PS.events[n].events & EPOLLIN ){
                    nrecv = recv(sock->fd,&msg,10*sizeof(uint32_t),0);
                    
                    if( nrecv < 0 ){
                        perror("RECV ERROR:");
                        runner = 0;
                        break;
                    
                    } else if ( nrecv == 0 ) {
                        disconnectPollSock(sock);
                    
                    } else if ( sock->is.commsock ){
                        recvSysMsgHandler(&PS, &ADC, &msg, &runner);
                    }
                }
            }
        } else if ( ( nfds == 0 ) && DREF32(ADC.dataReadyFlag) ){
            DREF32(ADC.stateReset)=1;
            usleep(5);
        }

        if(!(tmp%10)){
            printf("query data waiting... (%d s)\n",tmp/10);
            setLEDS(&ADC,(tmp/10));
        }
        tmp++;

        if( g_auto_shutdown_enabled && ( tmp>300 ) ){
            runner = 0;
            break;
        }
    }

    for(n=0;n<MAX_SERVER_PORTS;n++){
        if(ENETserver[n].ps!=NULL){
            disconnectPollSock(&ENETserver[n]);
        }
        if(ENETclient[n].ps!=NULL){
            disconnectPollSock(&ENETclient[n]);
        }
    }
    if(ADC.interrupt.ps!=NULL){
        disconnectPollSock(&ADC.interrupt);
    }
    
    free(ADC.reg);
    free(ADC.reg_dict[0]);
    free(ADC.reg_dict[1]);
    free(ADC.reg_dict);
    for(n=0;n<2;n++){
        if( ADC.data[n] != NULL){
            free(ADC.data[n]);
        }
    }
    free(ADC.data);
	FPGAclose(&FPGA);
	return(0);
}

 

 









 






