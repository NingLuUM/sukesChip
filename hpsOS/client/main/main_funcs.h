

#define IPCSOCK "./arm_ipc"
#define MAX_IPC 5

// Done
void setnonblocking(int sock){
    int opts;
    if((opts=fcntl(sock,F_GETFL))<0) perror("GETFL nonblocking failed");
    
    opts = opts | O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0) perror("SETFL nonblocking failed");
}


void addIpcSock(IPCsock_t **IPC){
	IPCsock_t *ipc;
    ipc = (IPCsock_t *)calloc(1,sizeof(IPCsock_t));
    
    ipc->sock.isFlags = 0;
    ipc->sock.is.active = 1;
    ipc->sock.is.ipcGeneric = 1;
    ipc->sock.parent = ipc;
    
    ipc->next = *IPC;
    ipc->prev = NULL;
    (*IPC)->prev = ipc;
    *IPC = ipc;
}


void acceptIPCconnection(IPCsock_t **IPC, IPCsock_t *tmp){ /* function to accept incoming ipc connections (from python server) */
	IPCsock_t *ipc;
    struct sockaddr_un remote;
    socklen_t clilen;
    
    clilen = sizeof(remote);
    memset(&remote,0,sizeof(struct sockaddr_un));

    addIpcSock(IPC);
    ipc = (*IPC);
    
    ipc->sock.fd = accept(tmp->sock.fd, (struct sockaddr *)&remote, &clilen);
    setnonblocking(ipc->sock.fd);
    ipc->ev->data.ptr = &(ipc->sock);
    ipc->ev->events = EPOLLIN;
    epoll_ctl(*(ipc->epfd),EPOLL_CTL_ADD,ipc->sock.fd,ipc->ev);
    printf("IPC socket accepted %d\n",ipc->sock.fd);
}


void setupIPCserver(IPCsock_t **IPC, int *epfd, struct epoll_event *ev, struct epoll_event *events){ /* function to open listening ipc socket for connections from other processes (python server) */
    
    IPCsock_t *ipc;
    struct sockaddr_un local;
    
    ipc = (IPCsock_t *)calloc(1,sizeof(IPCsock_t));
    *IPC = ipc;
    
	ipc->sock.isFlags = 0;
    ipc->sock.is.ipcServer = 1;
    ipc->sock.is.active = 1;
    ipc->sock.parent = ipc;
    ipc->next = NULL;
    ipc->prev = NULL;
    ipc->server = ipc;
    
    ipc->sock.fd = socket(AF_UNIX, SOCK_STREAM, 0);
    setnonblocking_ipc(ipc->sock.fd);

	ipc->epfd = epfd;
	ipc->ev = ev;
	ipc->events = events;
	
    ipc->ev->data.ptr = &(ipc->sock);
    ipc->ev->events = EPOLLIN;
    epoll_ctl(*(ipc->epfd),EPOLL_CTL_ADD,ipc->sock.fd,ipc->ev);
    
    memset(&local,0,sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path,IPCSOCK);
    unlink(local.sun_path);
    
    if( bind( ipc->sock.fd, (struct sockaddr *)&local, sizeof(struct sockaddr_un)) < 0){
        perror("ERROR binding IPCsock");
        exit(1);
    }
    if( listen(ipc->sock.fd,MAX_IPC)){
        perror("ERROR listening IPCsock");
        exit(1);
    }
}









