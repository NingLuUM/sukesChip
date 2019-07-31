/**** function prototypes for ENET ****
void setnonblocking_enet(int sock);
void connectInterrupt_intr(ENETsock **INTR, char *gpio_lab, int portnum);
void disconnectInterrupt_intr(ENETsock **INTR);
void addPollSock_enet(ENETsock **ENET, int portNum);
void connectPollSock_enet(ENETsock **ENET, int portNum);
void disconnectSock_enet(ENETsock **ENET, int portNum);
void setPacketSize_enet(ENETsock *ENET, uint32_t packetsize);
void sendAcqdData_enet(ENETsock **ENET, RCVsys *ADC, int portNum);
*/

// Done
void setnonblocking_enet(int sock){
    int opts;
    if((opts=fcntl(sock,F_GETFL))<0) perror("GETFL nonblocking failed");
    
    opts = opts | O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0) perror("SETFL nonblocking failed");
}


// Done
void connectInterrupt_intr(ENETsock **INTR, char *gpio_lab, int portnum){
	
	ENETsock* enet;	
	enet = (ENETsock *)malloc(sizeof(ENETsock));
    setupENETsockFunctionPointers_intr(enet);
    enet->next = *INTR;
	enet->prev = NULL;
    enet->sockfd = 0;
	enet->portNum = portnum;
	if(portnum < 2){
		enet->type.isInterruptRcv = 0;
		enet->type.isInterruptTx = 1;
	} else {
		enet->type.isInterruptRcv = 1;
		enet->type.isInterruptTx = 0;
	}
    enet->is_active = 1;
	enet->type.isEnetCommSock = 0;
	
	if(*INTR != NULL){
		(*INTR)->prev = enet;
    }
    
	// CHANGE THIS TO DESIRED LABEL
	const char *gpio_label = gpio_lab;//"gpio@0x100000080";
	const char *edge_type = "rising";
	//~ int poll_interval = 1000;

	// Other vars
	DIR *gpio_dir;
	const char *gpio_dir_path = "/sys/class/gpio";
	const char *gpiochip_str = "gpiochip";
	size_t gpiochip_str_len = strlen(gpiochip_str);
	int result;
	struct dirent *dir_entry;
	char path[PATH_MAX];
	int path_length;
	int file_fd;
	char buffer[PATH_MAX + 1] = {0};
	char gpio_number_buffer[PATH_MAX + 1] = {0};
	char *str_result = NULL;
	char *newline_ptr;
	//~ struct pollfd pollfd_struct;

	// Open the sysfs gpio directory
	gpio_dir = opendir(gpio_dir_path);
	if (gpio_dir == NULL)
		error(1, errno, "could not open directory '%s'", gpio_dir_path);

	// Find the gpio controller for given gpio_label
	while (1)
	{
		// Read next dir
		errno = 0;
		dir_entry = readdir(gpio_dir);
		if (dir_entry == NULL)
		{
			if (errno != 0)
				error(1, errno, "reading directory '%s'",
					  gpio_dir_path);
			else
				break;
		}

		// check if this is a gpio controller entry
		result = strncmp(dir_entry->d_name, gpiochip_str,
						 gpiochip_str_len);
		if (result != 0)
			continue;

		// open the gpio controller label file and read label value
		path_length = snprintf(path, PATH_MAX, "%s/%s/label",
							   gpio_dir_path, dir_entry->d_name);
		if (path_length < 0)
			error(1, 0, "path output error");
		if (path_length >= PATH_MAX)
			error(1, 0, "path length overflow");

		file_fd = open(path, O_RDONLY | O_SYNC);
		if (file_fd < 0)
			error(1, errno, "could not open file '%s'", path);

		result = read(file_fd, buffer, PATH_MAX);
		if (result < 0)
			error(1, errno, "reading from '%s'", path);
		if (result == PATH_MAX)
			error(1, errno, "buffer overflow reading '%s'", path);

		result = close(file_fd);
		if (result < 0)
			error(1, errno, "could not close file '%s'", path);

		buffer[PATH_MAX] = 0;
		// test the gpio controller label value for our gpio controller
		str_result = strstr(buffer, gpio_label);
		if (str_result != NULL)
			break;
	}

	if (str_result == NULL)
		error(1, 0, "unable to locate gpio controller");

	// Create path to read base value
	path_length = snprintf(path, PATH_MAX, "%s/%s/base",
						   gpio_dir_path, dir_entry->d_name);
	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	// Open file and read base value
	file_fd = open(path, O_RDONLY | O_SYNC);
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	result = read(file_fd, gpio_number_buffer, PATH_MAX);
	if (result < 0)
		error(1, errno, "reading from '%s'", path);
	if (result == PATH_MAX)
		error(1, errno, "buffer overflow reading '%s'", path);

	result = close(file_fd);
	if (result < 0)
		error(1, errno, "could not close file '%s'", path);

	gpio_number_buffer[PATH_MAX] = 0;

	// Remove the newline at the end of the string
	newline_ptr = strchr(gpio_number_buffer, '\n');
	if (newline_ptr != NULL)
		memset(newline_ptr, '\0', 1);

	// Open the gpio export file and write our gpio number
	path_length = snprintf(path, PATH_MAX, "%s/export",
						   gpio_dir_path);
	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	file_fd = open(path, O_WRONLY | O_SYNC);
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	result = write(file_fd, gpio_number_buffer, strlen(gpio_number_buffer));
	// Don't check errors because device could already exist

	result = close(file_fd);
	if (result < 0)
		error(1, errno, "could not close file '%s'", path);

	// Open the gpio edge file and write edge type (falling, both, etc)
	path_length = snprintf(path, PATH_MAX, "%s/gpio%s/edge",
						   gpio_dir_path, gpio_number_buffer);

	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	file_fd = open(path, O_WRONLY | O_SYNC);
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	// Write edge type
	result = write(file_fd, edge_type, 7);
	if (result < 0)
		error(1, errno, "writing to '%s'", path);
	if (result != 7)
		error(1, errno, "buffer underflow writing '%s'", path);

	result = close(file_fd);
	if (result < 0)
		error(1, errno, "could not close file '%s'", path);

	// Open the gpio value file and poll the file
	path_length = snprintf(path, PATH_MAX, "%s/gpio%s/value",
						   gpio_dir_path, gpio_number_buffer);
	if (path_length < 0)
		error(1, 0, "path output error");
	if (path_length >= PATH_MAX)
		error(1, 0, "path length overflow");

	printf("path: ");
	printf("%s",path);
	printf("\n");

	// path = /sys/class/gpio/gpio331/edge

	file_fd = open(path, O_RDONLY | O_RSYNC | O_NONBLOCK ); // Set to nonblocking
	
	int flags = fcntl(file_fd, F_GETFL, 0);
	fcntl(file_fd, F_SETFL, flags | O_NONBLOCK);
		
	if (file_fd < 0)
		error(1, errno, "could not open file '%s'", path);

	// Need to read the file before we can poll it
	// poll will keep returning 1 otherwise
	result = read(file_fd, buffer, PATH_MAX);
	if (result < 0)
		error(1, errno, "reading from '%s'", path);
	if (result == PATH_MAX)
		error(1, errno, "buffer overflow reading '%s'", path);

	enet->sockfd = file_fd;

	*INTR = enet;
		
    ev.data.ptr = enet;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, enet->sockfd, &ev);
    
}

// Done
void disconnectInterrupt_intr(ENETsock **INTR){
	
	ENETsock **enet, *prev, *next;
    enet = INTR;
    while((*enet) != NULL){
        next = (*enet)->next;
        prev = (*enet)->prev;
        epoll_ctl(epfd, EPOLL_CTL_DEL, (*enet)->sockfd, &ev);
        close((*enet)->sockfd);
        
        
		free(*enet);
		if ( prev == NULL && next == NULL ){
			break;
		} else if(next != NULL){
			next->prev = prev;
			(*enet)=next; 
		} else if (prev != NULL){
			prev->next = next;
			(*enet)=next; 
		} 
    }
}


// Done
void addPollSock_enet(ENETsock **ENET, int portNum){
    ENETsock* enet;	
	enet = (ENETsock *)malloc(sizeof(ENETsock));
    setupENETsockFunctionPointers_enet(enet);
    enet->next = *ENET;
	enet->prev = NULL;
    enet->sock.fd = 0;
	enet->portNum = portNum;
	enet->sock.type.isInterruptTx = 0;
	enet->sock.type.isInterruptTx = 0;
    enet->is_active = 0;
    enet->sock.type.isEnetCommSock = 0;
    enet->sock.parent = enet;
	if(portNum == COMM_PORT){
		enet->sock.type.isEnetCommSock = 1;
        enet->commsock = enet;
	} else if (portNum == TX_RECV_PORT){
		enet->sock.type.isEnetRecvLargeBuffer = 1;
	}
	if(*ENET != NULL){
		(*ENET)->prev = enet;
        enet->commsock = (*ENET)->commsock;
    }
	*ENET = enet;
}

// Done
void connectPollSock_enet(ENETsock **ENET, int portNum){ /* function to accept incoming ethernet connections from the socs */
    struct sockaddr_in server_sockaddr;	
	struct timeval t0,t1;
	int diff;
	ENETsock *enet, *enet0;

	enet = (*ENET)->commsock;
    while(enet->portNum != portNum){
		enet = enet->prev;
	}		
    enet->sock.fd = socket(AF_INET, SOCK_STREAM, 0);
    server_sockaddr.sin_port = htons(enet->portNum+INIT_PORT);
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = inet_addr(g_serverIP);
    
    gettimeofday(&t0,NULL); 
    if(connect(enet->sock.fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))  == -1){		
        while(connect(enet->sock.fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))  == -1){
            gettimeofday(&t1,NULL);
            diff = (t1.tv_sec-t0.tv_sec);
            if(diff>(600)){
                printf("NO CONNECT!!!!\n");
                break;
            }	
        }
    }  
    setsockopt(enet->sock.fd,IPPROTO_TCP,TCP_NODELAY,&ONE,sizeof(int));
    setsockopt(enet->sock.fd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
	setnonblocking_enet(enet->sock.fd);

    enet->is_active = 1;	
    
    ev.data.ptr = &(enet->sock);
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, enet->sock.fd, &ev);
    
	while(enet != NULL){
        if(enet->is_active){
            enet0 = enet;
        }
		enet = enet->prev;
	}
	*ENET = enet0;
	printf("board %d, port %d is active\n", (*ENET)->sock.fd, (*ENET)->portNum); 
}

// Done
void disconnectSock_enet(ENETsock **ENET, int portNum){ 
    ENETsock *enet, *prev, *next;
    int sockfd = -1;
    enet = (*ENET);
    while(enet != NULL){
        next = enet->next;
        prev = enet->prev;
        if(enet->portNum == portNum){
			sockfd = enet->sock.fd;
            if(next != NULL){
                next->prev = prev;
            }
            if(prev != NULL){
                prev->next = next;
            }
            if(prev == NULL){
                (*ENET) = next;
            }
            free(enet);
            break;
        }
        enet = enet->next;
    }
    
    if(sockfd>(-1)){
		epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &ev);
		close(sockfd);
	}
}


void ENET_init(ENETsock **ENET){
	addPollSock_enet(ENET, COMM_PORT);
	(*ENET)->connectPollSock(ENET, COMM_PORT);
}

//~ void sendAcqdData_enet(ENETsock **ENET, RCVsys *RCV, int portNum){
	//~ struct ENETsock *enet0, *commsock;
	//~ commsock = (*ENET)->commsock;
	//~ static int tmp = 0;
    //~ int nsent;
    
	//~ printf("sendAcqdData, tmp=%d\n",tmp);
	//~ if( RCV->board->queryMode == 0 || tmp == 0 ){
		//~ RCV->copyDataToMem(RCV);
	//~ }
	//~ printf("sendAcqdData, tmp=%d\n",tmp);
	//~ printf("g_queryMode=%d\n",RCV->board->queryMode);
	//~ if(RCV->board->queryMode == 0){
		//~ enet0 = commsock->prev;
		//~ while( enet0 != NULL && enet0->is_active ){
			//~ setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ONE,sizeof(int));
			//~ nsent = send(enet0->sockfd, enet0->dataAddr, enet0->bytesInPacket,0);	
			//~ setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ZERO,sizeof(int));
			//~ if( nsent < 0 )
				//~ perror("error sending data:");
			//printf("oh my, data sent. enetmsg1 = 0\n");
			//~ setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
			//~ enet0 = enet0->prev;
			//~ usleep(RCV->board->packetWait);
		//~ }
	//~ } else {
		//~ tmp += 1;
		//~ if(portNum==0){
			//~ portNum=commsock->prev->portNum;
		//~ }
		//~ if(portNum <= RCV->board->numPorts){
			//~ enet0 = commsock->prev;
			//~ while( enet0->portNum != portNum )
				//~ enet0 = enet0->prev;
			
			//~ printf("%d\n",enet0->dataAddr);
			//~ setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ONE,sizeof(int));
			//~ nsent = send(enet0->sockfd, enet0->dataAddr, enet0->bytesInPacket,0);	
			//~ setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_CORK,&ZERO,sizeof(int));
			//~ if( nsent < 0 )
				//~ perror("error sending data:");
			//printf("oh my, data sent. enetmsg1 != 0\n");
			//~ setsockopt(enet0->sockfd,IPPROTO_TCP,TCP_QUICKACK,&ONE,sizeof(int));
		//~ }
		//~ printf("tmp %d, portNum %d, g_numPorts %d\n",tmp,portNum, RCV->board->numPorts);
		//~ if(portNum == RCV->board->numPorts){
			//~ tmp = 0;
			//~ usleep(10);
		//~ }
	//~ }

//~ }


//~ void setPacketSize_enet(ENETsock *ENET, uint32_t packetsize){
	//~ ENET->board->packetsize = packetsize;
//~ }


void setupENETsockFunctionPointers_intr(ENETsock *tmp){
    tmp->connectInterrupt = &connectInterrupt_intr;
    tmp->disconnectInterrupt = &disconnectInterrupt_intr;
}


void setupENETsockFunctionPointers_enet(ENETsock *tmp){
    tmp->addPollSock = &addPollSock_enet;
    tmp->connectPollSock = &connectPollSock_enet;
    tmp->disconnectSock = &disconnectSock_enet;
    //~ tmp->setPacketSize = &setPacketSize_enet;
    //~ tmp->sendAcqdData = &sendAcqdData_enet;
}


//~ void ENET_Settings(ENETsock **ENET, uint32_t *msg){

    //~ ENETsock *commsock;
    //~ commsock = (*ENET)->commsock;

    //~ switch(msg[0]){
        
        //~ case(CASE_ENET_SET_PACKETSIZE):{
			//~ if( msg[1]>=MIN_PACKETSIZE && msg[1]<=MAX_RECLEN ){
				//~ commsock->setPacketSize(commsock,msg[1]);
			//~ } else {
				//~ commsock->setPacketSize(commsock,2048);
				//~ printf("invalid recLen, defaulting to 2048, packetsize to 512\n");
			//~ }
			//~ break;
        //~ }

        //~ case(CASE_ENET_INTERLEAVE_DEPTH_AND_TIMER):{
			//~ if( msg[1] > 0 ){
                //~ commsock->board->moduloBoardNum = msg[1];
				//~ if( msg[2] < 5000 ){
					//~ commsock->board->moduloTimer = msg[2];
				//~ } else {
					//~ commsock->board->moduloTimer = 0;
				//~ }
				//~ if( msg[3] < 5000 ){
					//~ commsock->board->packetWait = msg[3];
				//~ } else {
					//~ commsock->board->packetWait = 0;
				//~ }
			//~ } else {
                //~ commsock->board->moduloBoardNum = 1;
                //~ commsock->board->moduloTimer = 0;
                //~ commsock->board->packetWait = 0;
            //~ }
            //~ break;
        //~ }
        
    //~ }
//~ }








