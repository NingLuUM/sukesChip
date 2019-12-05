


void setnonblocking(int sockfd){
    int opts;
    if((opts=fcntl(sockfd,F_GETFL))<0) perror("GETFL nonblocking failed");

    opts = opts | O_NONBLOCK;
    if(fcntl(sockfd,F_SETFL,opts)<0) perror("SETFL nonblocking failed");
}

void setblocking(int sockfd){
    int opts;
    if((opts=fcntl(sockfd,F_GETFL))<0) perror("GETFL blocking failed");

    opts &= ~O_NONBLOCK;
    if(fcntl(sockfd,F_SETFL,opts)<0) perror("SETFL blocking failed");
}

void addEnetServerSock(POLLserver_t *ps, SOCK_t *sock, int portNum){
    
    struct sockaddr_in server;

    sock->is.flags = 0;
    sock->is.listener = 1;
    sock->portNum = portNum;
    sock->ps = ps;

    sock->fd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(sock->fd);
    ps->ev.data.ptr = sock;
    ps->ev.events = EPOLLIN;
    epoll_ctl(ps->epfd,EPOLL_CTL_ADD,sock->fd,&(ps->ev));

    memset(&server,0,sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(portNum);

    setsockopt(sock->fd,SOL_SOCKET,SO_REUSEADDR, &ONE, sizeof(int));

    if( bind(sock->fd, (struct sockaddr *)&server, sizeof(struct sockaddr_in))<0){
        perror("ERROR binding socket");
        exit(1);
    }
    if( listen(sock->fd, MAX_SERVER_QUEUE) ){
        perror("ERROR listening sock->fd");
        exit(1);
    }

}

void acceptEnetClientSock(SOCK_t *servsock){
    struct sockaddr_in client, peername;
    socklen_t clilen, peerlen;
    
    POLLserver_t *ps;
    ps = servsock->ps;
    
    SOCK_t *clisock;
    clisock = servsock->partner;

    memset(&client,0,sizeof(struct sockaddr_in));
    clilen = sizeof(client);
    memset(&peername,0,sizeof(struct sockaddr_in));
    peerlen = sizeof(peername);

    clisock->fd = accept(servsock->fd, (struct sockaddr *)&client, &clilen);

    if( getpeername(clisock->fd, (struct sockaddr *)&peername, &peerlen) ){
        perror("ERROR getting peername");
    }

    clisock->is.flags = 0;
    clisock->portNum = servsock->portNum;
    if(clisock->portNum == INIT_PORT){
        clisock->is.commsock = 1;
    } else if (clisock->portNum == ADC_CONTROL_PORT){
        clisock->is.adc_control = 1;
    } else if (clisock->portNum == TX_CONTROL_PORT){
        clisock->is.tx_control = 1;
    }

    if(clisock->portNum != ADC_CONTROL_PORT){
        setnonblocking(clisock->fd);
    }
    setsockopt(clisock->fd,IPPROTO_TCP,TCP_NODELAY,&ONE,sizeof(int));

    clisock->ps = ps;
    ps->ev.data.ptr = clisock;
    ps->ev.events = EPOLLIN;

    epoll_ctl(ps->epfd,EPOLL_CTL_ADD,clisock->fd,&(ps->ev));
}

void disconnectPollSock(SOCK_t *tmp){

	epoll_ctl(tmp->ps->epfd, EPOLL_CTL_DEL, tmp->fd, &(tmp->ps->ev));
	close(tmp->fd);
    tmp->is.flags = 0;
    tmp->portNum = 0;
    tmp->ps = NULL;
    tmp->fd = 0;
}

void connectPollInterrupter(POLLserver_t *ps, SOCK_t *interrupt, char *gpio_lab, int isRcvInterrupt){
	 
	// CHANGE THIS TO DESIRED LABEL
	const char *gpio_label = gpio_lab;
	const char *edge_type = "rising";

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

	//~ printf("path: ");
	//~ printf("%s",path);
	//~ printf("\n");

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

	interrupt->fd = file_fd;
    interrupt->ps = ps;
    interrupt->is.flags = 0;
    if(isRecvInterrupt){
        interrupt->is.rcv_interrupt = 1;
    } else {
        interrupt->is.tx_interrupt = 1;
    }
    interrupt->portNum = 0;
	
	result = read(interrupt->fd, buffer, PATH_MAX);
	
    ps->ev.data.ptr = interrupt;
    ps->ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(ps->epfd, EPOLL_CTL_ADD, interrupt->fd, &(ps->ev));
    
    // need to have epoll_wait once before interrupt can really be used
    epoll_wait(ps->epfd, ps->events, MAX_POLL_EVENTS, 10); // 10 ms
}


