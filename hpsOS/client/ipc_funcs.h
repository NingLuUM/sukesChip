


void IPC_init(IPCsock_t **IPC, int *epfd, struct epoll_event *ev, struct epoll_event *events, int ipc_fd, int idx){
	if(IPC[idx]==NULL){
		printf("AGDSG\n");
		IPC[idx] = (IPCsock_t *)calloc(1,sizeof(IPCsock_t));
	}
	IPC[idx]->sock.fd = ipc_fd;
	IPC[idx]->sock.parent = IPC[idx];
	IPC[idx]->epfd = epfd;
	IPC[idx]->ev = ev;
	IPC[idx]->events = events;
	IPC[idx]->ev->data.ptr = &(IPC[idx]->sock);
	IPC[idx]->ev->events = EPOLLIN;
	setnonblocking(IPC[idx]->sock.fd);
	epoll_ctl(*(IPC[idx]->epfd), EPOLL_CTL_ADD, IPC[idx]->sock.fd, IPC[idx]->ev);
}







