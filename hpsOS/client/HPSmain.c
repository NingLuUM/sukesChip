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
#define MAX_RECLEN 32768
#define COMM_PORT 0
#define MAX_SOCKETS (10)
#define ENET_MSG_SIZE (10)
#define MAX_POLL_EVENTS (5)
#define MAX_SERVER_CONNECTIONS (5)

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

const int ONE = 1;
const int ZERO = 0;
struct timespec gstart, gend;

typedef struct FMSG_{
    union{
        uint32_t u[10];
        float f[10];
        double d[5];
    };
} FMSG_t;

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



int main(int argc, char *argv[]) { printf("\ninto main!\nargcount:%d\n\n",argc);
	
	remove("data_file.dat");
	
	FPGAvars_t FPGA;
	ADCvars_t ADC;
    POLLSERVER_t PS;
    SOCK_t ENETserver;
    SOCK_t ENETclient;
    SOCK_t *sock;

    PS.epfd = epoll_create(MAX_POLL_EVENTS);
    addEnetServerSock(&PS,&ENETserver);

	FPGA_init(&FPGA);
	ADC_init(&FPGA,&ADC);
	
    adcInitializeSettings(&ADC);
	usleep(100);
    
    /*
    uint32_t recLen = 2048;
	uint32_t pioVarGain = 0;
	double gain = 0.0;
	uint32_t set_unsigned = 0;
	uint32_t set_lownoise = 0;
    uint32_t filter_bw = 0;
	
	if(argc>1) recLen = (uint32_t )atoi(argv[1]);
	if(argc>2) pioVarGain = (uint32_t )atoi(argv[2]);
	if(argc>3) gain = atof(argv[3]);
	if(argc>4) set_unsigned = (uint32_t )atoi(argv[4]);
	if(argc>5) set_lownoise = (uint32_t )atoi(argv[5]);
	if(argc>6) filter_bw = (uint32_t )atoi(argv[6]);
	
	setRecLen(&ADC,recLen);
	setPioVarGain(&ADC,pioVarGain);
	
	
	adcSetGain(&ADC,gain);
	usleep(100);
	adcSetDTypeUnsignedInt(&ADC, set_unsigned);
	usleep(100);
	adcSetLowNoiseMode(&ADC, set_lownoise);
    usleep(100);
    adcSetFilterBW(&ADC,filter_bw);

	adcSetReg1(&ADC);
	usleep(100);
	adcSetReg7(&ADC);
	usleep(100);
	
	DREF32(ADC.stateReset)=1;
	usleep(5);
	DREF32(ADC.stateReset)=0;
	usleep(5);
    
    connectPollInterrupter(&PS,&ADC,"gpio@0x100000000");
	*/
    
    setLEDS(&ADC,0x1f);
    printf("%d,%d,%d\n",sizeof(uint32_t),sizeof(float),sizeof(double));
    FMSG_t msg;
    int timeout_ms = 10;
    int nfds;
    int nrecv;
    int n,m;
    int tmp = 0;
    int runner = 1;

    while(runner==1){
		tmp++;
		
        if(!(tmp%100)){
			printf("query data waiting... (%d s)\n",tmp/100);
			DREF32(ADC.leds) = ( ( tmp/100 ) & 0x1F );
		}

        if(tmp>3000){
            runner = 0;
            break;
        }
        
        nfds = epoll_wait(PS.epfd,PS.events,MAX_POLL_EVENTS,timeout_ms);
		
        if(DREF32(ADC.dataReadyFlag)){
			printf("nfds %d, dataReadyFlag %u\n\n",nfds,DREF32(ADC.dataReadyFlag));
		}

        if( nfds > 0 ){
            for(n=0;n<nfds;n++){
                sock = (SOCK_t *)PS.events[n].data.ptr;
                if(sock->is.listener){

                   if(ENETclient.ps != NULL) disconnectPollSock(&ENETclient);
                   acceptEnetClientSock(&PS, &ENETserver, &ENETclient);
                } else if (sock->is.interrupt) {
                    queryDataOuter(&ADC,&ENETclient);
                    //char buff[100] = "hello billy";
                    //printf("sent: %d\n", send(ENETclient.fd,buff,100*sizeof(char),0));
            //        runner = 0;
              //      break;
                } else if ( PS.events[n].events & EPOLLIN ){
                    nrecv = recv(sock->fd,&msg,10*sizeof(uint32_t),0);
                    if(nrecv < 0){
                        printf("ERROR: recv<0\n");
                        runner = 0;
                        break;
                    } else if (nrecv == 0) {
                        disconnectPollSock(sock);
                    } else {
                        switch(msg.u[0]){
                            case(CASE_SET_RECLEN):{
                                setRecLen(&ADC,msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_SET_PIO_VAR_GAIN):{
                                setPioVarGain(&ADC,msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_SET_LEDS):{
                                setLEDS(&ADC,msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_QUERY_DATA):{
                                DREF32(ADC.stateReset)=1;
                                usleep(5);
                                DREF32(ADC.stateReset)=0;
                                usleep(5);
                                break;
                            }
                            case(CASE_ADC_SET_GAIN):{
                                adcSetGain(&ADC,msg.d[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_ADC_SET_UNSIGNED):{
                                adcSetDTypeUnsignedInt(&ADC, msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_ADC_SET_LOW_NOISE_MODE):{
                                adcSetLowNoiseMode(&ADC, msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_ADC_TOGGLE_CHANNEL_POWER):{
                                adcToggleChannelPwr(&ADC,msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_ADC_SET_FILTER_BW):{
                                adcSetFilterBW(&ADC,msg.u[1]);
                                usleep(100);
                                break;
                            }
                            case(CASE_ADC_SET_INTERNAL_AC_COUPLING):{
                                adcSetInternalAcCoupling(&ADC,msg.u[1]);
                                break;
                            }
                            case(CASE_ADC_ISSUE_DIRECT_CMD):{
                                break;
                            }
                            case(CASE_CONNECT_INTERRUPT):{
                                if(msg.u[1] && ( ADC.interrupt.ps == NULL ) ){
                                    connectPollInterrupter(&PS,&ADC,"gpio@0x100000000");
                                } else if ( !msg.u[1] && ( ADC.interrupt.ps != NULL ) ){
                                    disconnectPollSock(&(ADC.interrupt));
                                }
                                break;
                            }
                            default:{
                                runner=0;
                                break;
                            }

                        }
     //                   printf("fmsg: \n");
       //                 for(m=0;m<10;m++){
         //                   printf("[%d]: %u\n",m,msg.u[m]);
           //             }
                    }
                }
            }
        }
    }
    printf("1\n");
    if(ENETserver.ps!=NULL)
        disconnectPollSock(&ENETserver);
    printf("2\n");
    if(ENETclient.ps!=NULL)
        disconnectPollSock(&ENETclient);
    printf("3\n");
    if(ADC.interrupt.ps!=NULL)
        disconnectPollSock(&ADC.interrupt);
    
    free(ADC.reg);
	FPGAclose(&FPGA);
	return(0);
}

 

 









 






