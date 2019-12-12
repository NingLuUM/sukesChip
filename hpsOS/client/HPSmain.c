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
#include <string.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <error.h>
#include <errno.h>
#include <math.h>
//#include <sys/ipc.h>
//#include <sys/shm.h>
//#include <signal.h>
//#include <sys/time.h>
//#include <sys/wait.h>
// TODO: remove unused headers


/*****************************************************************************/
/*** ALTERA/INTEL-SPECIFIC HEADERS/MACROS -- DON'T TOUCH!!! ******************/
/*****************************************************************************/

/*** headers required to compile code to run on SoCs ***/
#define soc_cv_av // define device family (req'd by headers)
#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
/* (loc: /intelFPGA/17.1/embedded/ip/altera/hps/altera_hps/hwlib/include) */

/*** addresses of memory mapped FPGA-ARM shared hardware registers ***/
/* GPIO registers (slow-bridge, registers) */
#define HW_REGS_BASE ( ALT_STM_OFST )  
#define HW_REGS_SPAN ( 0x04000000 )   
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
// ALT_STM_OFST defined in headers

/* RAM registers (HPS2FPGA AXI, fast-bridge, registers) */
#define ALT_AXI_FPGASLVS_OFST ( 0xC0000000 ) // axi_master
#define HW_FPGA_AXI_SPAN ( 0x40000000 ) // Bridge span 1GB
#define HW_FPGA_AXI_MASK ( HW_FPGA_AXI_SPAN - 1 )
// ^ none of these were defined in headers or manuals...

/*****************************************************************************/
/*** ALTERA/INTEL-SPECIFIC HEADERS/MACROS -- END *****************************/
/*****************************************************************************/

// sopc-create-header-files: contains addresses of pio/rams defined in qsys 
//~ #include "hps_0_ramclks.h"
//~ #include "hps_0_16bitADCdata.h"
//~ #include "hps_0_rambank128bit.h"
// #include "hps_0_txrx_test.h"
#include "hps_0_txPioReg.h"

// macros to dereference memory-mapped variables from FPGA
#define DREF8(X)	    ( *( uint8_t  *) X )
#define DREF16(X)	    ( *( uint16_t *) X )
#define DREF32(X)	    ( *( uint32_t *) X )
#define DREFP8(X)	    (  ( uint8_t  *) X )
#define DREFP16(X)	    (  ( uint16_t *) X )
#define DREFP32(X)	    (  ( uint32_t *) X )
#define DREFP32S(X)	    (  ( int32_t  *) X )
#define DREFP64(X)	    (  ( uint64_t *) X )
#define DREFP64S(X)	    (  ( int64_t  *) X )
#define DREFPCHAR(X)    (  ( char     *) X )


// ETHERNET SETUP
#define INIT_PORT           ( 3400 )
#define ADC_CONTROL_PORT    ( 3500 )
#define TX_CONTROL_PORT     ( 3600 )
#define TX_DATA_UPLOAD_PORT ( 3700 )

#define MAX_RECLEN          ( 16384 )
#define MAX_PACKETSIZE      ( 8192 )

#define MAX_SOCKETS         ( 10 )
#define ENET_MSG_SIZE       ( 10 )
#define MAX_POLL_EVENTS     ( 8 )
#define MAX_SERVER_PORTS    ( 6 )
#define MAX_SERVER_QUEUE    ( 8 )
#define FMSG_SIZE           ( 1024 )

// UNUSED, but potentially useful (see 'cServer' for use examp) 
#define ADC_CLK             ( 25 )	// MHz
#define ADC_NBITS           ( 12 )
#define ADC_NCHAN           ( 8 )

// states defined in the lvds.v module
#define ADC_HARDWARE_RESET          ( 0xFF )
#define ADC_IDLE_STATE				( 0x00 )
#define ADC_BUFFER_SERIAL_COMMAND	( 0x01 )
#define ADC_ISSUE_SERIAL_COMMAND	( 0x02 )
#define ADC_SYNC_COMMAND			( 0x04 )
// TODO: make these into structs with initialized vars (like adc registers)?


#define TX_INTERRUPT_ID             ( 0 )
#define RCV_INTERRUPT_ID            ( 1 )
// case defs for main controller
#define CASE_ADD_DATA_SOCKET 0
// TODO: separate recvSys and control handlers

// case defs for recvSysMsgHandler
#define CASE_RCV_SET_RECLEN                 ( 0 )
#define CASE_RCV_SET_PIO_VAR_GAIN           ( 1 )
#define CASE_SET_LEDS                       ( 2 )
#define CASE_RCV_STATE_RESET                ( 3 )
#define CASE_ADC_SET_GAIN                   ( 4 )
#define CASE_ADC_SET_UNSIGNED               ( 5 )
#define CASE_ADC_SET_LOW_NOISE_MODE         ( 6 )
#define CASE_ADC_TOGGLE_CHANNEL_POWER       ( 7 )
#define CASE_ADC_SET_FILTER_BW              ( 8 )
#define CASE_ADC_SET_INTERNAL_AC_COUPLING   ( 9 )
#define CASE_ADC_ISSUE_DIRECT_CMD           ( 10 )
#define CASE_RCV_CONNECT_INTERRUPT          ( 11 )
#define CASE_RCV_SETUP_LOCAL_STORAGE        ( 12 )
#define CASE_ADC_SET_DEFAULT_SETTINGS       ( 13 )
#define CASE_RCV_SET_QUERY_MODE             ( 14 )
#define CASE_UPDATE_AUTO_SHUTDOWN_SETTING   ( 15 )
#define CASE_RCV_SET_NPULSES				( 16 )
#define CASE_RCV_SET_CLOCK_DIVISOR			( 17 )
#define CASE_RCV_SET_SAMPLING_MODE			( 18 )
#define CASE_RCV_SET_COMPRESSOR_MODE		( 19 )
#define CASE_RCV_INTERRUPT_THYSELF          ( 20 )

// TODO: delete these from rcv sys handler
#define	CASE_ADC_SET_FCLOCK_DELAY			( 52 )

// TODO: move to header containing handler(?) 
#define CASE_TX_SET_CONTROL_STATE           ( 0 )
#define CASE_TX_SET_TRIG_REST_LVLS          ( 1 )
#define CASE_TX_SET_ACTIVE_TRANSDUCERS      ( 2 ) 
#define CASE_TX_MAKE_PIO_CMD                ( 3 )
#define CASE_TX_END_PIO_CMD                 ( 4 )
#define CASE_TX_MAKE_LOOP_START             ( 5 )
#define CASE_TX_MAKE_LOOP_END               ( 6 )
#define CASE_TX_MAKE_STEERING_LOOP_START    ( 7 )
#define CASE_TX_MAKE_STEERING_LOOP_END      ( 8 )
#define CASE_TX_BUFFER_TRIG_TIMINGS         ( 10 )
#define CASE_TX_BUFFER_CHARGE_TIME          ( 11 ) 
#define CASE_TX_BUFFER_PHASE_DELAYS         ( 12 )
#define CASE_TX_BUFFER_FIRE_CMD             ( 13 )
#define CASE_TX_BUFFER_RECV_TRIG            ( 14 )
#define CASE_TX_BUFFER_ASYNC_WAIT           ( 15 )
#define CASE_TX_WAIT_CMD                    ( 16 )
#define CASE_TX_SET_NSTEERING_LOCS          ( 17 )
#define CASE_TX_CONNECT_INTERRUPT           ( 18 )

#define CASE_EXIT_PROGRAM 100

const int ONE = 1;
const int ZERO = 0;

// system automatically shuts down if not interacted with within 30s of launch
int g_auto_shutdown_enabled = 1;

// timer variables for cross-function timing tests
struct timespec gstart, gend, gdifftime;

#include "interrupt_readout_funcs.h"
#include "adc_register_defs.h"
#include "tx_pio_register_defs.h"
#include "structure_defs.h"
#include "adc_funcs.h"
#include "tx_funcs.h"
#include "epoll_server_funcs.h"
#include "subsys_setup_funcs.h"
#include "rcv_funcs.h"
#include "recvSys_handlerFuncs.h"
#include "txProgramExecutionHandler.h"
#include "txSys_enetMsgHandlerFuncs.h"


int main(int argc, char *argv[]) { printf("\ninto main!\nargcount:%d\n\n",argc);
	
    POLLserver_t *PS = (POLLserver_t *)malloc(sizeof(POLLserver_t));
    FPGAvars_t *FPGA = (FPGAvars_t *)malloc(sizeof(FPGAvars_t));
    ADCvars_t *ADC = (ADCvars_t *)malloc(sizeof(ADCvars_t));
    RCVsys_t *RCV = (RCVsys_t *)malloc(sizeof(RCVsys_t));
    TXsys_t *TX = (TXsys_t *)malloc(sizeof(TXsys_t));
    SOCK_t *ENETserver = (SOCK_t *)malloc(MAX_SERVER_PORTS*sizeof(SOCK_t));
    SOCK_t *ENETclient = (SOCK_t *)malloc(MAX_SERVER_PORTS*sizeof(SOCK_t));

    PS->epfd = epoll_create(MAX_POLL_EVENTS);
	
    FPGA_init(FPGA);	
	ADC_init(FPGA,ADC);
    RCV_init(FPGA,ADC,RCV,PS);
    TX_init(FPGA,TX,PS);

    connectPollInterrupter(PS,RCV->interrupt,"gpio@0x100000000",RCV_INTERRUPT_ID);
    connectPollInterrupter(PS,TX->interrupt,"gpio@0x100000010",TX_INTERRUPT_ID);
    //disconnectPollSock(RCV->interrupt);
    //disconnectPollSock(TX->interrupt);
    addEnetServerSock(PS,&ENETserver[0],INIT_PORT);
    addEnetServerSock(PS,&ENETserver[1],ADC_CONTROL_PORT);
    addEnetServerSock(PS,&ENETserver[2],TX_CONTROL_PORT);
    addEnetServerSock(PS,&ENETserver[3],TX_DATA_UPLOAD_PORT);
    
    for(int i=0;i<MAX_SERVER_PORTS;i++){
        ENETclient[i].ps = NULL;
        ENETclient[i].partner = &ENETserver[i];
        ENETserver[i].partner = &ENETclient[i];
    }
    
    int n;
    SOCK_t *sock;
    FMSG_t msg;
    int timeout_ms = 100;
    int nfds;
    int nrecv,pd_size;
    int tmp = 0;
    int runner = 1;
    int activeSocks = 0;
    TXpiocmd_t *pio_cmd;
    uint32_t interrupt_readout;

    while(runner==1){
        
        nfds = epoll_wait(PS->epfd,PS->events,MAX_POLL_EVENTS,timeout_ms);
	    //printf("rcv state reset = %d\n",DREF32(RCV->stateReset));	
	    //printf("data ready flag = %d\n",DREF32(RCV->dataReadyFlag));	
        //printf("rcv interrupt reg: %u (%d)\n",DREF32(RCV->interrupt_reg),DREF32(RCV->dataReadyFlag));
        //printf("tx interrupt reg: %u ()\n",DREF32(TX->interrupt_reg));
        if( nfds > 0 ){
            for(n=0;n<nfds;n++){
                sock = (SOCK_t *)PS->events[n].data.ptr;

                if( sock->is.listener ){
                    if( sock->partner->ps != NULL ){
                        disconnectPollSock(sock->partner);
                    }
                    acceptEnetClientSock(sock);
                    sock->is.alive = 1;
                    activeSocks++;

                    if( sock->partner->is.tx_control ){
                        TX->comm_sock = sock->partner;
                    } else if ( sock->partner->is.tx_incoming_data ){
                        TX->pd_data_sock = sock->partner;
                    } else if ( sock->partner->is.commsock ){
                        RCV->comm_sock = sock->partner;
                        RCV->setStateReset(RCV,0);
                    } else if ( sock->partner->is.adc_control ){
                        RCV->data_sock = sock->partner;
                    }
                
                } else if ( sock->is.rcv_interrupt ) {
                    printf("rcv program execution interrupt\n");
                    interrupt_readout = DREF32(RCV->interrupt_reg);
                    printf("rcv interrupt reg:\n");
                    printBinaryInterrupt(interrupt_readout);
                    RCV->queryData(RCV); 
                    TX->resetRcvTrig(TX);
                    //RCV->setStateReset(RCV,0);
               
                } else if ( sock->is.tx_interrupt ){
                    //printf("tx program execution interrupt\n");
                    //interrupt_readout = DREF32(TX->interrupt_reg);
                    //printf("tx interrupt reg:\n");
                    //printBinaryInterrupt(interrupt_readout);
                    TX->programExecutionHandler(TX);

                } else if ( PS->events[n].events & EPOLLIN ){
                    //printf("sock->portNum = %d\n",sock->portNum);
                    if ( !(sock->is.tx_incoming_data) ){
                        nrecv = recv(sock->fd,&msg,10*sizeof(uint32_t),0);
                        
                    } else {
                        pd_size = TX->nSteeringLocs*8*sizeof(uint16_t) - TX->nPhaseDelaysWritten;
                        if(pd_size>FMSG_SIZE){
                            nrecv = recv(sock->fd,&msg,FMSG_SIZE*sizeof(uint8_t),0);
                        } else {
                            nrecv = recv(sock->fd,&msg,pd_size*sizeof(uint8_t ),0);
                        }
                    }
                    if( nrecv < 0 ){
                        perror("RECV ERROR:");
                        runner = 0;
                        break;
                    
                    } else if ( nrecv == 0 ) {
                        printf("nrecv = 0, disconnecting sock\n");
                        disconnectPollSock(sock);
                        sock->is.alive=0;
                        activeSocks--;
                        if( activeSocks <= 0){
                            printf("deleting TX->pio_cmd_list\n");
                            TX->setControlState(TX,0);
                            printf("exiting program\n");
                            runner = 0;
                        }
                    
                    } else if ( sock->is.commsock ){
                        //printf("rcv sock_msg: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",msg.u[0],msg.u[1],msg.u[2],msg.u[3],msg.u[4],msg.u[5],msg.u[6],msg.u[7],msg.u[8],msg.u[9]);
                        RCV->enetMsgHandler(RCV, &msg, &runner);
                    
                    } else if ( sock->is.tx_control ){
                        //printf("tx sock_msg: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",msg.u[0],msg.u[1],msg.u[2],msg.u[3],msg.u[4],msg.u[5],msg.u[6],msg.u[7],msg.u[8],msg.u[9]);
                        TX->enetMsgHandler(TX, &msg, nrecv, &runner);
                    
                    } else if ( sock->is.tx_incoming_data ){
                        TX->storePhaseDelays(TX,nrecv,&msg);
                    
                    }
                }
            }
        } else if ( ( nfds == 0 ) && DREF32(RCV->dataReadyFlag) ){
            RCV->setStateReset(RCV,1);
        }

        if(!(tmp%10)){
            printf("query data waiting... (%d s)\n",tmp/10);
            RCV->setLEDs(RCV,(tmp/10));
        }
        tmp++;

        if( g_auto_shutdown_enabled && ( tmp>3000 ) ){
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

    if(RCV->interrupt->ps != NULL){
        disconnectPollSock(RCV->interrupt);
    }
    free(RCV->interrupt);

    if(TX->interrupt->ps != NULL){
        disconnectPollSock(TX->interrupt);
    }
    free(TX->interrupt);

    free(ADC->reg); printf("free(ADC->reg)\n");
    free(ADC->reg_dict[0]); printf("free(ADC->reg_dict[0])\n");
    free(ADC->reg_dict[1]); printf("free(ADC->reg_dict[1])\n");
    free(ADC->reg_dict); printf("free(ADC->reg_dict)\n");
    for(n=0;n<2;n++){
        if( RCV->data[n] != NULL){
            free(RCV->data[n]); printf("free(RCV->data[%d])\n",n);
        }
    }
    free(RCV->data); printf("free(RCV->data)\n");
    free(TX->phaseDelays[0]); printf("free(TX->phaseDelays[0])\n");
    free(TX->phaseDelays); printf("free(TX->phaseDelays)\n");
    
    pio_cmd = *(TX->pio_cmd_list);
    if( pio_cmd != NULL ){
        pio_cmd = pio_cmd->top;
        while(pio_cmd->next != NULL){
            TX->delCmd(TX,0);
        }
        if(pio_cmd->reg9_24 != NULL) free(pio_cmd->reg9_24);
        free(pio_cmd);
        printf("free(TX->pio_cmd_list[cmdNumber: %d])\n", pio_cmd->cmdNumber);
    }
    free(TX->pio_cmd_list);
    printf("free(TX->pio_cmd_list)\n");
	
    FPGAclose(FPGA);

    free(FPGA);
    free(ENETserver);
    free(ENETclient);
    free(PS);
    free(TX);
    free(ADC);
    free(RCV);
	return(0);
}

 

 









 






