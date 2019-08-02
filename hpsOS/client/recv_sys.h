





void recvSysMain(int sv){
	FPGAvars *FPGA;
	FPGA = getSharedFpga();
	
	RCVsys *RCV;
	RCV = getSharedRcv();
	
	char *data;
	data = *(RCV->data);
	
	int epfd;
	struct epoll_event ev;
	struct epoll_event events[MAX_RCV_SYS_SOCKETS];
	epfd = epoll_create(MAX_RCV_SYS_SOCKETS);
	
    // create ethernet socket to communicate with server and establish connection
	ENETsock_t **ENET;
	ENETsettings_t enet_settings;
	ENET = (ENETsock_t **)calloc(1,sizeof(ENETsock_t *));
	ENET_init(ENET, &epfd, &ev, events, &enet_settings, RCV_COMM_PORT);
	
	IPCsock_t **IPC;
	IPC = (IPCsock_t **)calloc(1,sizeof(IPCsock_t *));
	IPC_init(IPC,&epfd,&ev,events,sv,0);
	
	SOCKgeneric_t *sock;
	
	int timeout_ms = 1000;
	int nfds;
	int n;
	int nrecv;
	uint32_t buf[10]={0};
	
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
                nrecv = recv(sock->fd,&buf,10*sizeof(uint32_t),0);
				setsockopt(sock->fd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
				printf("bongo pongo = %d,%d\n",buf[0],nrecv);
				buf[0]++;
				send(IPC[0]->sock.fd, &buf, 10*sizeof(uint32_t), 0);

			}
                
		}
		if(buf[0]>5){
			break;
		}
	}
	
	if(data != NULL){
		free(data);
		*(RCV->data) = NULL;
	}
	
	ENETsock_t *enet, *tmp;
	enet = (*ENET)->commsock;
	while(enet->prev != NULL){
		enet = enet->prev;
	}
	while(enet != NULL){
		tmp = enet;
		enet=enet->next;
		free(tmp);
	}

	free(ENET);
	free(*IPC);
	free(IPC);
	FPGA->elbows=35;
	
	//~ printf("childaaaa in\n");
	//~ send(sv, &buf, 10*sizeof(uint32_t), 0);
	//~ printf("child: sent '%d'\n", buf[0]);
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




