

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



// adc settings
#define CASE_RCV_RECORD_LENGTH 0
#define CASE_RCV_TRIGGER_DELAY 1
#define CASE_RCV_SET_LOCAL_STORAGE 2
#define CASE_RCV_TOGGLE_DATA_ACQ 3
#define CASE_RCV_DIRECT_CONTROL_COMMS 4
#define CASE_RCV_SET_PACKETSIZE 5

#define CASE_RESET_RCV_SYSTEM 6

#define CASE_ADC_POWER 10
#define CASE_ADC_SYNC 11
#define CASE_ADC_INITIALIZE 12		
#define CASE_ADC_GAIN 13
#define CASE_ADC_DIRECT_SERIAL_COMMAND 14

#define CASE_INTERRUPT_THYSELF 21
#define CASE_UNINTERRUPT_THYSELF 22
#define CASE_ADC_DIRECT_CONTROL_COMMS 23

void recvSysMain(int sv){

	FPGAvars_t FPGA;
	FPGA_init(&FPGA);
	
	RCVsys_t RCV;
	RCV_init(&FPGA, &RCV);
	
	char *data;
	data = *(RCV->data);
	
	int epfd;
	struct epoll_event ev;
	struct epoll_event events[MAX_RCV_SYS_SOCKETS];
	epfd = epoll_create(MAX_RCV_SYS_SOCKETS);
	
    // create ethernet socket to communicate with server and establish connection
    ENETsock_t *enetComm;
	ENETsettings_t enet_settings;
	RCV->ENET = (ENETsock_t **)calloc(1,sizeof(ENETsock_t *));
	ENET_init(RCV->ENET, &epfd, &ev, events, &enet_settings, RCV_COMM_PORT);
	
	IPCsock_t **IPC;
	IPC = (IPCsock_t **)calloc(1,sizeof(IPCsock_t *));
	IPC_init(IPC,&epfd,&ev,events,sv,0);
	
	SOCKgeneric_t *sock;
	
	int timeout_ms = 1000;
	int nfds;
	int n;
	int nrecv;
	uint32_t msg[10]={0};
    int retval;
	
	while(1){
		printf("into loop!\n");

		nfds = epoll_wait(epfd, events, MAX_RCV_SYS_SOCKETS, timeout_ms);
		if( nfds < 0 ){
			perror("error sending data:");
		}
		if( nfds > 0 ){
			
            printf("nfds = %d\n",nfds);
            for(n = 0; n < nfds; n++){
				
                sock = (SOCKgeneric_t *)events[n].data.ptr;
                nrecv = recv(sock->fd,&msg,10*sizeof(uint32_t),0);
				setsockopt(sock->fd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
				printf("bongo pongo = %d,%d\n",msg[0],nrecv);
				msg[0]++;
				send(IPC[0]->sock.fd, &msg, 10*sizeof(uint32_t), 0);
                
                switch(msg[0]){
                    case(CASE_RCV_RECORD_LENGTH):{
                        retval = RCV->setRecLen(RCV,msg[1]);
                        if(retval){
                            printf("illegal record length, defaulting to 2048.\n");
                        }

                        break;
                    }
                    case(CASE_RCV_TRIGGER_DELAY):{
                        RCV->setTrigDelay(RCV,msg[1]);
                        break;
                    }
                    case(CASE_RCV_SET_LOCAL_STORAGE):{
                        RCV->setLocalStorage(RCV,msg[1],msg[2]);
                        break;
                    }
                    case(CASE_RCV_TOGGLE_DATA_ACQ):{
                        break;
                    }
                    case(CASE_RCV_DIRECT_CONTROL_COMMS):{
                        break;
                    }
                    case(CASE_RESET_RCV_SYSTEM):{
                        RCV->stateResetFPGA(RCV);
                        RCV->resetVars(RCV);
                        break;
                    }
                    case(CASE_ADC_POWER):{
                        if(msg[1]){
                            RCV->ADC->powerOn(RCV);
                        } else {
                            RCV->ADC->powerOff(RCV);
                        }
                        break;
                    }
                    case(CASE_ADC_SYNC):{
                        RCV->ADC->sync(RCV);
                        break;
                    }
                    case(CASE_ADC_INITIALIZE):{
                        RCV->ADC->initializeSettings(RCV);
                        break;
                    }
                    case(CASE_ADC_GAIN):{
                        RCV->ADC->setGain(RCV,msg[1],msg[2]);
                        break;
                    }
                    case(CASE_ADC_DIRECT_SERIAL_COMMAND):{
                        RCV->ADC->issueDirectSerialCommand(RCV,msg[1],msg[2]);
                        break;
                    }
                    case(CASE_INTERRUPT_THYSELF):{
                        break;
                    }
                    case(CASE_UNINTERRUPT_THYSELF):{
                        break;
                    }
                    case(CASE_ADC_DIRECT_CONTROL_COMMS):{
                        DREF32(RCV->controlComms);
                        break;
                    }
                    default:{
                        break;
                    }

                }

			}
                
		}
		if(msg[0]>5){
			break;
		}
	}
	
	if(data != NULL){
		free(data);
		*(RCV->data) = NULL;
	}
	
	ENETsock_t *enet, *tmp;
	enet = (*(RCV->ENET))->commsock;
	while(enet->prev != NULL){
		enet = enet->prev;
	}
	while(enet != NULL){
		tmp = enet;
		enet=enet->next;
		free(tmp);
	}

	free(RCV->ENET);
	free(*IPC);
	free(IPC);
	
	//~ printf("childaaaa in\n");
	//~ send(sv, &msg, 10*sizeof(uint32_t), 0);
	//~ printf("child: sent '%d'\n", msg[0]);
	sleep(1);
	exit(0);
}



void launchRecvSys(IPCsock_t **IPC, int *epfd, struct epoll_event *ev, struct epoll_event *events){
	
	int sv[2];

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
		recvSysMain(sv[1]);
	} else { // parent
		close(sv[1]);
		IPC_init(IPC,epfd,ev,events,sv[0],0);
		IPC[0]->sock.is.ipcRx = 1;
		IPC[0]->ipc_pid = pid;
		sleep(1);
		send(IPC[0]->sock.fd, &enetmsg[0], 10*sizeof(uint32_t), 0);
	}
}


void recvSysHandler(IPCsock_t **IPC, int *epfd, struct epoll_event *ev, struct epoll_event *events, uint32_t *msg){
	pid_t cpid;
	int status;
	
	if( msg[0] != LAUNCH_RECV_SUBSYS ){
									
		if( IPC[0] == NULL ){
			launchRecvSys(IPC, epfd, ev, events);
			
		} else {
			// obliterate the child, wipe it from the history books
			// ...but only if it doesn't take any time.
			cpid = waitpid(IPC[0]->ipc_pid,&status,WNOHANG);
			
			if( !cpid ){
				printf("Can't launch, RCV_SYS still running\n");
				
			} else if (cpid>0){
				
				printf("child exited\n");
				launchRecvSys(IPC, epfd, ev, events);
				
			} else {
				
				printf("recv_sys exit() error\n");
				
			}
		}
	}
}




