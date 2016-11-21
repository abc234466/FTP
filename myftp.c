#include	"myftp.h"
static char server_IP[20];

int getDeviceName( int socketfd, char *device ) {
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list
	struct ifconf ifconf;
	struct ifreq ifr[10], freq;
	struct sockaddr_in addr;
	int conf_inf, addr_inf;
	
	bzero(&ifconf,sizeof(ifconf));
	ifconf.ifc_buf = (char *)(ifr);
	ifconf.ifc_len = sizeof(ifr);
	
	// !!
	conf_inf = ioctl(socketfd, SIOCGIFCONF, &ifconf);
	if(conf_inf<0)
		errCTL("SIOCGIFCONF");

	strcpy(device, ifr[1].ifr_name);
	strcpy(freq.ifr_name,device);
	
	//get device IP
	addr_inf = ioctl(socketfd, SIOCGIFADDR, &freq);
	if(addr_inf<0)
		errCTL("SIOCGIFCONF");
	memcpy(&addr, &freq.ifr_addr,sizeof(addr));
	strcpy(server_IP, inet_ntoa(addr.sin_addr));
	printf("Host IP : %s\n",server_IP);
	return 0;
}

int initServerAddr( int socketfd, int port, const char *device, struct sockaddr_in *addr ) {
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)
    
    struct ifreq freq;
    int sock_ser;
    
    bzero(addr, sizeof(struct sockaddr_in));
    
    //set device
    strcpy(freq.ifr_name, device);
    if(setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, &freq, sizeof(freq))<0)
    	errCTL("initServerAdr setsockopt error");

    //set status
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind socket
    sock_ser = bind(socketfd, (struct sockaddr*)addr, sizeof(struct sockaddr));
    
    if(sock_ser <0)
    	errCTL("initServerAddr bind error");
    
	return 0;
}

int startMyftpServer( int temp_port, struct sockaddr_in *clientaddr, const char *filename ) {

	int socketfd, sock_recv, fd, addrlen = sizeof(struct sockaddr);
	struct startServerInfo serverinfo;
	struct sockaddr_in servaddr;
	struct myFtphdr ftp_packet;
	
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd <0)
		errCTL("startMyftpServer");
	
	bzero(&servaddr, sizeof(struct sockaddr_in));
	bzero(&ftp_packet, sizeof(ftp_packet));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(temp_port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind socket
	if(bind(socketfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) <0)
		errCTL("startMyftpServer bind error");
	
	//set timeout 20 sec
	Timeout(socketfd, 20, 0);
	
	strcpy(serverinfo.servAddr, server_IP);
	strcpy(serverinfo.filename, filename);
	serverinfo.connectPort = temp_port;
	
	//send file information
	if(sendto(socketfd, &serverinfo, sizeof(serverinfo), 0, (struct sockaddr *)clientaddr, addrlen)<0 )
		errCTL("startMyftpServer sendto error");
		
	sock_recv = recvfrom(socketfd, &ftp_packet, 1+strlen(filename)+4, MSG_WAITALL, (struct sockaddr *)clientaddr, &addrlen);
	if(sock_recv <0)
	{
		if(errno == EAGAIN)
			puts("wait client time out");
		
		errCTL("startMyftpServer sock_recv error");	
	}
	
	if(in_cksum((unsigned short *)&ftp_packet, 4 + strlen(ftp_packet.mf_filename)+1)!=0)
	{
		errCTL("startMyftpServer ftp cksum error");
	}
		
	if(ftp_packet.mf_opcode != ntohs(FRQ))
	{
		errCTL("startMyftpServer not FRQ");
	}
	else 
	{
		printf("        [file transmission - start]\n");
	}
	
	//open file
	FILE *tran_file;
	unsigned long long ind =0;
	unsigned short packet;
	//read binary 
	tran_file = fopen(filename, "rb");
	
	for(packet = 1; !feof(tran_file); packet++)
	{
		if(packet ==65535)
			packet=1;
		
		ftp_packet.mf_cksum = 0;
		// data packet 
		ftp_packet.mf_opcode = htons(DATA);
		//https://www.tutorialspoint.com/c_standard_library/c_function_fread.htm
		fd = fread(ftp_packet.mf_data, 1, MFMAXDATA, tran_file);
		
		if(fd< MFMAXDATA && fd>=0)
		{
			ftp_packet.mf_block =0;
		}
		else if(fd ==MFMAXDATA)
		{
			ftp_packet.mf_block =packet;
		}
		else
		{
			errCTL("startMyftpServer fread error");
		}
		
		//en checksum
		ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, 6+fd);
		if(sendto(socketfd, &ftp_packet, 6+fd, 0, (struct sockaddr *)clientaddr,addrlen)<0)
			errCTL("startMyftpServer sendto error");
		
		//sum of byte
		ind +=fd;
		
		//ack
		fd = recvfrom(socketfd, &ftp_packet, sizeof(struct myFtphdr), 0, (struct sockaddr *)clientaddr, &addrlen);
		if(fd<0)
		{
			if(errno ==EAGAIN)
				puts("wait client timeout");
				
			errCTL("startMyftpServer recvfrom error");
		}
		if(in_cksum((unsigned short *)&ftp_packet,6) !=0)
		{
			errCTL("check sum error");
		}
	}
		
	// close file
	fclose(tran_file);
	printf("               send file : <%s> to %s\n",filename, inet_ntoa(clientaddr->sin_addr));
	printf("        [file transmission -finish]\n               %llu bytes received\n", ind);
		
	return 0;
}

//Initialize Client
int initClientAddr( int socketfd, int port, char *sendClient, struct sockaddr_in *addr ) {
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option
    
    //http://beej-zhtw.netdpi.net/09-man-manual/9-20-setsockopt-getsockopt
	int opt = 1;
	bzero(addr, sizeof(struct sockaddr_in));
	
	if(setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))<0)
		errCTL("initClientAddr error");
	
	
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	//http://beej-zhtw.netdpi.net/09-man-manual/9-13-inet_ntoa-inet_aton-inet_addr
	//addr->sin_addr.s_addr = inet_aton(sendClient);
	if(inet_aton(sendClient, &(addr->sin_addr)) ==0)
	{
		errCTL("initClientAddr inet_atonerror");
	}
	
	return 0;
}

int findServerAddr( int socketfd, char *filename, const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr ) {
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct startServerInfo as boradcast message
    //          Use setsockopt to set timeout
	struct startServerInfo packet;
	int sock_send, sock_recv, addrlen = sizeof(struct sockaddr_in);
	
	//store filename
	strcpy(packet.filename, filename);
	
	//find server
	sock_send = sendto(socketfd, &packet, sizeof(struct startServerInfo), 0, (struct sockaddr *)broadaddr, addrlen);
	if(sock_send<0)
		errCTL("findServerAddr sock_send error");
	
	//set timeout
	Timeout(socketfd, 20, 0);
	
	bzero(servaddr,sizeof(struct sockaddr_in));
	
	//receive information from server
	sock_recv = recvfrom(socketfd, &packet, sizeof(struct startServerInfo),MSG_WAITALL, (struct sockaddr*)servaddr, &addrlen);
	
	/* 
	 * http://beej-zhtw.netdpi.net/05-system-call-or-bust/5-8-sendto
	 * error -> http://man7.org/linux/man-pages/man3/errno.3.html 
	 */
	if(sock_recv<0)
	{   
		//Resource temporarily unavailable
		if(errno == EAGAIN)
			printf("No server answer!!");
		
		errCTL("findServerAddr error");
	}
	else
	{
		puts("[Receive Reply]");
		
		//Whether the file exist or not 
		//exist
		if(packet.filename[0] != '\0')
		{
			printf("               Get MyftpServer servAddr : %s\n               Myftp connectPort : %d\n", packet.servAddr, packet.connectPort);
			
			//process port number
			//servaddr->sin_port = htons(packet.connectPort);
			//servaddr->sin_addr.s_addr = inet_addr(packet.servAddr);
			//printf("%d",ntohs(servaddr->sin_port));
			//printf("%d",servaddr->sin_family);
		}
		//not exist
		else if(packet.filename[0] =='\0')
		{
			printf("Requested file : %s doesn't exist",filename);
			exit(1);
		}
		
	}
	return 0;
}


int startMyftpClient(struct sockaddr_in *servaddr, const char *filename ) {
	
	int socketfd, sock_send, sock_recv, addrlen = sizeof(struct sockaddr);
	unsigned long long ind;
	struct myFtphdr ftp_packet;
	
	//open socket
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd <0)
		errCTL("startMyftpClient socketfd error");
	
	//set timeout 20 sec
	Timeout(socketfd, 20, 0);
	
	bzero(&ftp_packet, sizeof(ftp_packet));
	
	//process myFtphdr
	ftp_packet.mf_cksum = htons(0);
	// file request 
	ftp_packet.mf_opcode = htons(FRQ);
	//get filename
	strcpy(ftp_packet.mf_filename, filename);
	//checksum
	ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, 4+strlen(ftp_packet.mf_filename) + 1);
	
	sock_send = sendto(socketfd, &ftp_packet, 4+strlen(ftp_packet.mf_filename) + 1, 0, (struct sockaddr *)servaddr, addrlen);
	if(sock_send <0)
		errCTL("startMyftpClient sock_send error");
		
	//process file
	FILE *download;
	char newfile[FNAMELEN];
	sprintf(newfile, "client_%s",filename);
	//create binary data
	download = fopen(newfile, "wb");
	
	printf("[file transmission - start]\n               download to file : %s\n               get file : %s from %s\n",newfile, filename,inet_ntoa(servaddr->sin_addr));
	
	while(1)
	{	
 		sock_recv = recvfrom(socketfd, &ftp_packet, sizeof(ftp_packet), 0, (struct sockaddr *)&servaddr, &addrlen);
		if(sock_recv <0)
		{
			if(errno ==EAGAIN)
				puts("wait server time out");
		
			puts("time out waiting ACK,send data again\n");
			
		}
		
		//write file	
		ind+=(sock_recv-6);
		fwrite(ftp_packet.mf_data, 1, sock_recv-6, download);	

		if(sock_recv < MFMAXDATA-6)
			ftp_packet.mf_block = 0;
		ftp_packet.mf_cksum=0;
		ftp_packet.mf_opcode = htons(ACK);
		ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, 6);

		if(sendto(socketfd, &ftp_packet, 6, 0, (struct sockaddr *)&servaddr, addrlen) < 0)
			errCTL("startMyftpClient_sendto_ACK error");

		if(ftp_packet.mf_block == 0)
			break;
	
	}
	
	fclose(download);
	printf("[file transmission - finish]");
	printf("               %llu bytes received",ind);
	
	return 0;
}

unsigned short in_cksum( unsigned short *addr, int len ) {
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while( nleft > 1 ) {
		sum += *w++;
		nleft -= 2;
	}
	
	if( nleft == 1 ) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

//http://pubs.opengroup.org/onlinepubs/7908799/xsh/systime.h.html
//https://www.freebsd.org/cgi/man.cgi?query=setsockopt&sektion=2
int Timeout(int socketfd, int sec, int usec)
{
	struct timeval time;
	
	//set time information
	//second
	time.tv_sec = sec;
	//microsecond
	time.tv_usec = usec;
	if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time, sizeof(struct timeval))!=0)
		errCTL("Request timeout ");
		
	return 0;
}

int listenClient(int socketfd, int port,int tmpPort, char *filename, struct sockaddr_in *clientaddr)
{
	int sock_recv, len=sizeof(struct sockaddr);
	struct startServerInfo file;
	bzero(&file,sizeof(struct startServerInfo));
	
	while(1)
	{
		sock_recv = recvfrom(socketfd, &file, sizeof(file), 0,(struct sockaddr*)clientaddr, &len);
		if(sock_recv < 0)
			errCTL("listenClient error");
		
		printf("        [Request]\n               file : %s\n               from : %s\n",file.filename, inet_ntoa(clientaddr->sin_addr));
		printf("        [Reply]\n               file : %s\n               connectPort : %d\n",filename, tmpPort);
		// file match -> 
		if(strcmp(file.filename,filename)==0)
		{
			
			return 0;
		}
		else
			errCTL("listenClient file not matched !");	
	}
}


