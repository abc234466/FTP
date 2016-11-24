#include	"myftp.h"
static char Host_IP[20];
int getDeviceName(int socketfd, char *device)
{
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list

	// get NIC information e.g. IP, interface
	struct ifconf ifcf;
	struct ifreq ifreqs[20], ifrq;
	struct sockaddr_in *ifraddr;
	int addr_inf, conf_inf;

	bzero(&ifcf, sizeof(ifcf));
	ifcf.ifc_buf = (char *)(ifreqs);
	ifcf.ifc_len = sizeof(ifreqs);
	
	conf_inf = ioctl(socketfd, SIOCGIFCONF, (char *)&ifcf);
	if( conf_inf< 0)
		errCTL("getIFname_ioctl_SIOCGIFCONF error");

	strcpy(device, ifreqs[1].ifr_name);
	strcpy(ifrq.ifr_name, device);
	
	addr_inf = ioctl(socketfd, SIOCGIFADDR, &ifrq);
	if( addr_inf< 0)
		errCTL("getIFname_ioctl_SIOCGIFADDR error");

	ifraddr = (struct sockaddr_in*)&ifrq.ifr_addr;
	strcpy(Host_IP, inet_ntoa(ifraddr->sin_addr));
	printf("Host IP : %s\n",Host_IP);
	
	return 0;
}

int initServerAddr(int socketfd, int port, const char *device,struct sockaddr_in *serveraddr)
{
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)

	int sock_server;
	struct ifreq dev;
	
	bzero(serveraddr, sizeof(struct sockaddr_in));
	
	// bind device with socket
	memcpy(&dev.ifr_name, device,sizeof(dev.ifr_name));
	if(setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, &dev, sizeof(dev)) < 0)
		errCTL("initServAddr_setsockopt");

	// Server bind socket 
	serveraddr->sin_family = AF_INET;
	serveraddr->sin_port = htons(port);
	serveraddr->sin_addr.s_addr = htonl(INADDR_ANY);
	

	sock_server = bind(socketfd, (struct sockaddr*)serveraddr, sizeof(struct sockaddr));
	if( sock_server < 0)
		errCTL("initServAddr bind error");

	return 0;
}

int initClientAddr(int socketfd, int port, char *sendClient, struct sockaddr_in *addr)
{
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option

	//reference : http://beej-zhtw.netdpi.net/09-man-manual/9-20-setsockopt-getsockopt
	int opt = 1;
	bzero(addr, sizeof(struct sockaddr_in));
	
	//set broadcast
	if(setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0)
		errCTL("initCliAddr setsockopt error");

	// process addr
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	
	//reference : http://beej-zhtw.netdpi.net/09-man-manual/9-13-inet_ntoa-inet_aton-inet_addr
	if(inet_aton(sendClient, &(addr->sin_addr)) == 0)
		errCTL("initClientAddr inet_aton error");
	//addr->sin_addr.s_addr = inet_addr(sendClient);
	return 0;
}

int findServerAddr(int socketfd, const struct sockaddr_in *broadaddr, struct startServerInfo *file)
{
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout

	int sock_send, sock_recv, len = sizeof(struct sockaddr);
	char f_name[20];
	
	
	//printf("%s",file->filename);
	strcpy(f_name, file->filename);
	// broadcast to find server
	sock_send = sendto(socketfd, file, sizeof(struct startServerInfo), 0, (struct sockaddr *)broadaddr, len);
	if(sock_send < 0)
		errCTL("findServerAddr sendto error");

	//set timeout 
	Timeout(socketfd, 20, 0);	
	
	// wait for server response 
	sock_recv = recvfrom(socketfd, file, sizeof(struct startServerInfo), 0, NULL, NULL);
	
	/* 
	 * http://beej-zhtw.netdpi.net/05-system-call-or-bust/5-8-sendto
	 * error -> http://man7.org/linux/man-pages/man3/errno.3.html 
	 */
	//printf("%s",file->filename);
	
	if(sock_recv < 0)
	{
		//Resource temporarily unavailable
		if(errno == EAGAIN)
			printf("no server answer!\n");

		errCTL("findServerAddr recvfrom error");
	}
	else
	{
		puts("[Receive Reply]");
		
		//Whether the previous and next file match or not
		//not exist
		if(strcmp(file->filename, f_name)!=0)
		{
			puts("  --special case--");
			printf("  Requested file : <%s> doesn't match the file : <%s> that server offers\n",f_name, file->filename);
			exit(1);
		}
		
		//exist
		else if((file->filename) !='\0')
		{
			printf("          Get MyftpServer servAddr : %s\n          Myftp connectPort : %d\n", file->servAddr, file->connectPort);
		}
	}
	return 0;
}

int listenClient(int socketfd, int tmp_port, char *filename, struct sockaddr_in *clientaddr)
{
    //Function: Wait for broadcast message from client
    //          As receive broadcast message, check file exist or not
    //          Set bootServerInfo with server address and new port, and send back to client

	int sock_recv, len = sizeof(struct sockaddr);
	struct startServerInfo file;
	
	//listen to client request
	while(1)
	{
		// get the client request
		sock_recv = recvfrom(socketfd, &file, sizeof(file), 0, (struct sockaddr *)clientaddr, &len);
		
		//NONE client request 
		
		if(sock_recv < 0)
			errCTL("listenClient_recvfrom");
		
		// check if server has file
		if(strcmp(file.filename, filename) == 0)
		{
			printf("     [Request]\n           file : %s\n           from : %s\n",file.filename, inet_ntoa(clientaddr->sin_addr));
			printf("     [Reply]\n           file : %s\n           connectPort : %d\n",filename, tmp_port);
			return 0;
		}
		//server doesn't have file
		else
		{
			puts("     [special case]");
			printf("           file <%s> requested from client %s doesn't exist\n",file.filename,inet_ntoa(clientaddr->sin_addr));
			return 0;
		}
	}

	// error message
	errCTL("listenClient error\n");
}

int startMyftpServer(int temp_port, struct sockaddr_in *clientaddr, const char *filename )
{
    //Function: Send file

	int socketfd, sock_recv, sock_recv2, addrlen=sizeof(struct sockaddr);
	unsigned short packet = 1;
	unsigned long long totalbyte = 0;
	struct myFtphdr ftp_packet;
	struct sockaddr_in servaddr;
	struct startServerInfo servInfo;
	
	// create new socket and bind
	if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		errCTL("startMyftpServer_socket");

	bzero(&servaddr, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(temp_port);

	if(bind(socketfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)) < 0)
		errCTL("startMyftpServer_bind");

	// Timeout 20 second
	Timeout(socketfd, 20, 0);

	// tell client server has file
	strcpy(servInfo.filename, filename);
	strcpy(servInfo.servAddr, Host_IP);
	servInfo.connectPort = temp_port;

	if(sendto(socketfd, &servInfo, sizeof(servInfo), 0, (struct sockaddr *)clientaddr, addrlen) < 0)
		errCTL("startMyftpServer_sendto");

	// get FRQ
	bzero(&ftp_packet, sizeof(ftp_packet));
	sock_recv = recvfrom(socketfd, &ftp_packet, sizeof(ftp_packet), 0, (struct sockaddr *)clientaddr, &addrlen);
	if( sock_recv < 0)
	{
		if(errno == EAGAIN)
			printf("wait client timeout\n");

		errCTL("startMyftpServer_recvfrom");
	}

	// check FRQ for check sum
	if(in_cksum((unsigned short *)&ftp_packet, 5 + strlen(ftp_packet.mf_filename)) != 0)
	{
		printf("Wrong FRQ check sum \n");
		return -1;
	}

	// check if it is FRQ
	if(ftp_packet.mf_opcode == ntohs(FRQ))
		printf("     [file transmission - start]\n");
	else
	{
		printf("Not FRQ\n");
		return -1;
	}

	// file open
	FILE *tran_file;
	tran_file = fopen(ftp_packet.mf_filename, "rb");
	
	while(!feof(tran_file))
	{
		
		int opcode;
		
		if(packet == 65535)
			packet = 1;
		
		// check whether overflow or not
		
		// read file
		ftp_packet.mf_cksum = 0;
		ftp_packet.mf_opcode = htons(DATA);
		sock_recv = fread(ftp_packet.mf_data, 1, MFMAXDATA, tran_file);
		if(sock_recv == MFMAXDATA)
			ftp_packet.mf_block = packet;
		else if(sock_recv < MFMAXDATA && sock_recv >= 0)
			ftp_packet.mf_block = 0;
		else
			errCTL("startMyftpServer_fread");

		//**********correct checksum**********//
		ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, 6 + sock_recv);
	
		//printf("%d\n",pacxxxxket);
		
		//**********make wrong checksum**********//
		/*if(packet%100==0)
		{
			ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, rand()%10);	
		}*/
		
		// send data
		if(sendto(socketfd, &ftp_packet, 6 + sock_recv, 0, (struct sockaddr *)clientaddr, addrlen) < 0)
		{
			errCTL("startMyftpServer_sendto");
		}
		
		// wait ACK
		if((sock_recv2 = recvfrom(socketfd, &ftp_packet, sizeof(struct myFtphdr), 0, (struct sockaddr *)clientaddr, &addrlen)) < 0)
		{
			if(errno == EAGAIN)
				printf("wait client timeout\n");
			
			errCTL("startMyftpServer_recvfrom");
		}
		
		//printf("%d",opcode);
		
		//ACK
		if(ntohs(ftp_packet.mf_opcode) == ACK)
		{
			// calculate total byte
			totalbyte += sock_recv;
			if(in_cksum((unsigned short *)&ftp_packet, 6) != 0)
			{
				printf("Wrong ACK check sum\n");
				return -1;
			}	
		}
		
		//ERROR
		else if(ntohs(ftp_packet.mf_opcode) == ERROR)
		{
			bzero(&ftp_packet, sizeof(struct myFtphdr));
			ftp_packet.mf_cksum = 0;
			ftp_packet.mf_opcode = htons(DATA);
			ftp_packet.mf_block = packet;
			ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, 6 + sock_recv);
			sock_recv = sendto(socketfd, &ftp_packet, 6 + sock_recv, 0, (struct sockaddr *)clientaddr, addrlen);
		}
		
		packet++;

	}

	// file close
	fclose(tran_file);

	printf("           send file : <%s> to %s \n", filename, inet_ntoa(clientaddr->sin_addr));
	printf("     [file transmission - finish]\n           %llu bytes sent\n", totalbyte);
	return 0;
}

int startMyftpClient(struct startServerInfo* file)
{
    //Function: Get file

	int socketfd, sock_recv, addrlen = sizeof(struct sockaddr);
	char newfile[FNAMELEN];
	unsigned short cksum=0;
	unsigned long block=0;
	unsigned long long totalbyte = 0;
	struct sockaddr_in servaddr;
	struct myFtphdr ftp_packet;
	

	// open socket
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd < 0)
		errCTL("startMyftpClient_socket");

	// Timeout 20 second
	Timeout(socketfd, 20, 0);

	// process FRQ
	bzero(&servaddr, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(file->connectPort);
	servaddr.sin_addr.s_addr = inet_addr(file->servAddr);
	
	
	//process myFtphdr , send FRQ
	bzero(&ftp_packet, sizeof(ftp_packet));
	ftp_packet.mf_cksum = 0;	
	ftp_packet.mf_opcode = htons(FRQ);
	strcpy(ftp_packet.mf_filename, file->filename);
	ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, strlen(ftp_packet.mf_filename) + 5);

	if(sendto(socketfd, &ftp_packet, strlen(ftp_packet.mf_filename) +5 , 0, (struct sockaddr *)&servaddr, addrlen) < 0)
		errCTL("startMyftpClient_sendto_FRQ");

	//process file
	FILE *download;
	sprintf(newfile, "client_%s",file->filename);
	//create binary data
	download = fopen(newfile, "wb");

	// receive data and write file
	printf("[file transmission - start]\n          download to file : <%s>\n", newfile);
	printf("          get file <%s> form %s\n", file->filename, file->servAddr);
	while(1)
	{
		// receive data
		sock_recv = recvfrom(socketfd, &ftp_packet, sizeof(ftp_packet), 0, (struct sockaddr *)&servaddr, &addrlen);
		if(sock_recv < 0)
		{
			if(errno == EAGAIN)
				printf("wait server time out\n");

			return -1;
		}
		
		block = ftp_packet.mf_block;
		cksum=ftp_packet.mf_cksum;
		ftp_packet.mf_cksum=0;
		
		//check whether checksum is error or not
		if(in_cksum((void *)&ftp_packet,sock_recv)!=cksum)
		{
			bzero(&ftp_packet,sizeof(struct myFtphdr));
            ftp_packet.mf_opcode=htons(ERROR);
            ftp_packet.mf_cksum=0;
            ftp_packet.mf_cksum=in_cksum((void*)&ftp_packet,ERRORHDRSIZE);
            sock_recv=sendto(socketfd,&ftp_packet,ERRORHDRSIZE,0,(struct sockaddr *)&servaddr,addrlen);
            printf("recevied data checksum error, the block is %lu\n",block);
            continue;
		}
		
		// calculate totalbyte in receive data
		totalbyte += (sock_recv - 6);

		// write file
		fwrite(ftp_packet.mf_data, 1, sock_recv - 6, download);

		// send ACK to server
		if(sock_recv < MFMAXDATA - 6)
			ftp_packet.mf_block = 0;
		ftp_packet.mf_cksum = 0;
		ftp_packet.mf_opcode = htons(ACK);
		ftp_packet.mf_cksum = in_cksum((unsigned short *)&ftp_packet, 6);

		if(sendto(socketfd, &ftp_packet, 6, 0, (struct sockaddr *)&servaddr, addrlen) < 0)
			errCTL("startMyftpClient_sendto_ACK");

		// end for while
		if(ftp_packet.mf_block == 0)
		{
			fclose(download);
			printf("[file transmission - finish]\n");
			printf("          %llu bytes received\n", totalbyte);
			break;
		}
	}

	// file close
	return 0;
}

static unsigned short in_cksum(unsigned short *addr,unsigned int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	
	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);

}

int Timeout(int socketfd, int sec, int usec)
{
	struct timeval tv;

	tv.tv_sec = sec;	// second
	tv.tv_usec = usec;	// microsecond

	// set timeout
	if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
		errCTL("Timeout error");

	return 0;
}
