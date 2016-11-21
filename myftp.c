#include	"myftp.h"

int getDeviceName(int socketfd, char *device)
{
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list

	// get device
	struct ifreq ifreqs[20];
	struct ifconf ifc;

	bzero(&ifc, sizeof(ifc));
	ifc.ifc_buf = (char *)(ifreqs);
	ifc.ifc_len = sizeof(ifreqs);

	if(ioctl(socketfd, SIOCGIFCONF, (char *)&ifc) < 0)
		errCTL("getIFname_ioctl_SIOCGIFCONF");

	strcpy(device, ifreqs[1].ifr_name);

	// get device's IP
	struct ifreq ifr;
	struct sockaddr_in *ifraddr;

	strcpy(ifr.ifr_name, device);
	if(ioctl(socketfd, SIOCGIFADDR, &ifr) < 0)
		errCTL("getIFname_ioctl_SIOCGIFADDR");

	ifraddr = (struct sockaddr_in*)&ifr.ifr_addr;
	strcpy(servIP, inet_ntoa(ifraddr->sin_addr));

	return 0;
}

int initServerAddr(int socketfd, int port, const char *device,struct sockaddr_in *servaddr)
{
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)

	struct ifreq dev;

	// bind device with socket
	strcpy(dev.ifr_name, device);
	if(setsockopt(socketfd, SOL_SOCKET, SO_BINDTODEVICE, &dev, sizeof(dev)) < 0)
		errCTL("initServAddr_setsockopt");

	// bind socket with address
	bzero(servaddr, sizeof(struct sockaddr_in));
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr->sin_port = htons(port);

	if(bind(socketfd, (struct sockaddr*)servaddr, sizeof(struct sockaddr)) < 0)
		errCTL("initServAddr_bind");

	return 0;
}

int initCliAddr(int socketfd, int port, char *sendClient, struct sockaddr_in *addr)
{
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option

	int boradcast = 1;

	// set broadcast
	if(setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &boradcast, sizeof(int)) < 0)
		errCTL("initCliAddr_setsockopt");

	// set timeout with 3 second
	setTimeout(socketfd, 3, 0);

	// fill *addr
	bzero(addr, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(sendClient);
	addr->sin_port = htons(port);

	return 0;
}

int findServerAddr(int socketfd, const struct sockaddr_in *broadaddr, struct startServerInfo *fileInfo)
{
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout

	int n, len = sizeof(struct sockaddr);

	// broadcast find server
	if(sendto(socketfd, fileInfo, sizeof(struct startServerInfo), 0, (struct sockaddr *)broadaddr, len)<0)
		errCTL("findServerAddr_sendto");

	// get server's response (server will tell client he has file)
	if((n = recvfrom(socketfd, fileInfo, sizeof(struct startServerInfo), 0, NULL, NULL)) < 0)
	{
		if(errno == EAGAIN)
			printf("no server answer!\n");

		errCTL("findServerAddr_recvfrom");
	}

	// print server's message
	printf("find myftpServer servaddr : %s\nMyftp connectPort : %d\n", fileInfo->servAddr, fileInfo->connectPort);

	return 0;
}

int listenClient(int socketfd, char *filename, struct sockaddr_in *clientaddr)
{
    //Function: Wait for broadcast message from client
    //          As receive broadcast message, check file exist or not
    //          Set bootServerInfo with server address and new port, and send back to client


	struct startServerInfo fileInfo;
	int n, len = sizeof(struct sockaddr);

	while(1)
	{
		// get request
		if((n = recvfrom(socketfd, &fileInfo, sizeof(fileInfo), 0, (struct sockaddr *)clientaddr, &len)) < 0)
			errCTL("listenClient_recvfrom");

		// check if server has file
		if(strcmp(fileInfo.filename, filename) == 0)
		{
			printf("client from %s connect!!\n", inet_ntoa(clientaddr->sin_addr));
			return 0;
		}
	}

	// error message
	printf("listen stop\n");
	return -1;
}

int startMyftpServer(int port, struct sockaddr_in *clientaddr, const char *filename )
{
    //Function: Send file

	struct myFtphdr ftpPackage;
	struct sockaddr_in servaddr;
	struct startServerInfo fileInfo;
	int socketfd,n;
	int len = sizeof(struct sockaddr);

	// create new socket and bind
	if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		errCTL("startMyftpServer_socket");

	bzero(&servaddr, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if(bind(socketfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr)) < 0)
		errCTL("startMyftpServer_bind");

	// set timeout with 5 second
	setTimeout(socketfd, 5, 0);

	// tell client server has file
	strcpy(fileInfo.filename, filename);
	strcpy(fileInfo.servAddr, servIP);
	fileInfo.connectPort = port;
	printf("myftp connect port : %d\n", port);

	if(sendto(socketfd, &fileInfo, sizeof(fileInfo), 0, (struct sockaddr *)clientaddr, len) < 0)
		errCTL("startMyftpServer_sendto");

	// get FRQ
	bzero(&ftpPackage, sizeof(ftpPackage));
	if((n = recvfrom(socketfd, &ftpPackage, sizeof(ftpPackage), 0, (struct sockaddr *)clientaddr, &len)) < 0)
	{
		if(errno == EAGAIN)
			printf("wait client timeout\n");

		errCTL("startMyftpServer_recvfrom");
	}

	// check FRQ for check sum
	if(in_cksum((unsigned short *)&ftpPackage, 4 + strlen(ftpPackage.mf_filename)) != 0)
	{
		printf("Wrong check sum: package FRQ\n");
		return -1;
	}

	// check if it is FRQ
	if(ftpPackage.mf_opcode == ntohs(FRQ))
		printf("file transmission start\n");
	else
	{
		printf("Wrong package: not FRQ\n");
		return -1;
	}

	// file open
	FILE *infile;
	unsigned short pkg;
	unsigned long long index = 0;
	infile = fopen(filename, "rb");

	for(pkg = 1; !feof(infile); pkg++)
	{
		// check overflow
		if(pkg == 65535)
			pkg = 1;

		// read file
		ftpPackage.mf_cksum = 0;
		ftpPackage.mf_opcode = htons(DATA);
		n = fread(ftpPackage.mf_data, 1, MFMAXDATA, infile);
		if(n == MFMAXDATA)
			ftpPackage.mf_block = pkg;
		else if(n < MFMAXDATA && n >= 0)
			ftpPackage.mf_block = 0;
		else
			errCTL("startMyftpServer_fread");

		// add check sum
		ftpPackage.mf_cksum = in_cksum((unsigned short *)&ftpPackage, 6 + n);

		// send data
		if(sendto(socketfd, &ftpPackage, 6 + n, 0, (struct sockaddr *)clientaddr, len) < 0)
			errCTL("startMyftpServer_sendto");

		// calculate total byte
		index += n;

		// wait ACK
		if((n = recvfrom(socketfd, &ftpPackage, sizeof(struct myFtphdr), 0, (struct sockaddr *)clientaddr, &len)) < 0)
		{
			if(errno == EAGAIN)
				printf("wait client timeout\n");
			
			errCTL("startMyftpServer_recvfrom");
		}

		// check ACK for check sum
		if(in_cksum((unsigned short *)&ftpPackage, 6) != 0)
		{
			printf("Wrong check sum: package ACK\n");
			return -1;
		}

	}

	// file close
	fclose(infile);

	printf("send file : <%s> to %s \n", filename, inet_ntoa(clientaddr->sin_addr));
	printf("%llu bytes sent\n", index);
	printf("file transmission finish!!\n");
	return 0;
}

int startMyftpClient(struct startServerInfo* fileInfo)
{
    //Function: Get file

	int socketfd, len = sizeof(struct sockaddr);
	unsigned long block=0;
	unsigned short cksum=0;
	struct myFtphdr ftpPackage;
	struct sockaddr_in servaddr;
	

	// create new socket
	if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		errCTL("startMyftpClient_socket");

	// set timeout with 3 second
	setTimeout(socketfd, 3, 0);

	// send FRQ
	bzero(&servaddr, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(fileInfo->servAddr);
	servaddr.sin_port = htons(fileInfo->connectPort);

	bzero(&ftpPackage, sizeof(ftpPackage));
	ftpPackage.mf_cksum = 0;	
	ftpPackage.mf_opcode = htons(FRQ);
	strcpy(ftpPackage.mf_filename, fileInfo->filename);
	ftpPackage.mf_cksum = in_cksum((unsigned short *)&ftpPackage, 4 + strlen(ftpPackage.mf_filename) + 1);

	if(sendto(socketfd, &ftpPackage, 4 + strlen(ftpPackage.mf_filename) + 1, 0, (struct sockaddr *)&servaddr, len) < 0)
		errCTL("startMyftpClient_sendto_FRQ");

	// file open
	FILE *outfile;
	char newFilename[FNAMELEN];
	strcpy(newFilename, "client_");
	strcat(newFilename, fileInfo->filename);
	int n = 0;
	unsigned long long index = 0;
	outfile = fopen(newFilename, "wb");

	// receive data and write file
	printf("file transmission start!!\ndownload to file : <%s>\n", newFilename);
	printf("get file <%s> form %s\n", fileInfo->filename, fileInfo->servAddr);
	while(1)
	{
		// receive data
		if((n = recvfrom(socketfd, &ftpPackage, sizeof(ftpPackage), 0, (struct sockaddr *)&servaddr, &len)) < 0)
		{
			if(errno == EAGAIN)
				printf("wait server time out\n");

			return -1;
		}
		
		if(n==DATAHDRSIZE)
		{
			fclose(outfile);
			printf("%llu bytes received\n", index);
			printf("file transmission finish!!\n");
		}
		
		block = ntohs(ftpPackage.mf_block);
		cksum=ftpPackage.mf_cksum;
		ftpPackage.mf_cksum=0;
		
		if(in_cksum((void *)&ftpPackage,n)!=cksum)
		{
			bzero(&ftpPackage,1024);
            ftpPackage.mf_opcode=htons(ERROR);
            ftpPackage.mf_cksum=0;
            ftpPackage.mf_cksum=in_cksum((void*)&ftpPackage,ERRORHDRSIZE);
            n=sendto(socketfd,&ftpPackage,ERRORHDRSIZE,0,(struct sockaddr *)&servaddr,len);
            printf("recevied data checksum error, the block is %lu\n",block);
            continue;
		
		}
		
		// calculate how many byte in receive data
		index += (n - 6);

		// write file
		fwrite(ftpPackage.mf_data, 1, n - 6, outfile);

		// send ACK to server
		if(n < MFMAXDATA - 6)
			ftpPackage.mf_block = 0;
		ftpPackage.mf_cksum = 0;
		ftpPackage.mf_opcode = htons(ACK);
		ftpPackage.mf_cksum = in_cksum((unsigned short *)&ftpPackage, 6);

		if(sendto(socketfd, &ftpPackage, 6, 0, (struct sockaddr *)&servaddr, len) < 0)
			errCTL("startMyftpClient_sendto_ACK");

		// end for while
		if(ftpPackage.mf_block == 0)
			break;
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

int setTimeout(int socketfd, int sec, int usec)
{
	struct timeval tv;

	tv.tv_sec = sec;	// second
	tv.tv_usec = usec;	// microsecond

	// set timeout
	if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
		errCTL("setTimeout");

	return 0;
}
