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
	printf("%s",server_IP);
	return 0;
}

int initServerAddr( int socketfd, int port, const char *device, struct sockaddr_in *addr ) {
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)
	return 0;
}

int startMyftpServer( int tempPort, struct sockaddr_in *clientaddr, const char *filename ) {

	return 0;
}

//Initialize Client
int initClientAddr( int socketfd, int port, char *sendClient, struct sockaddr_in *addr ) {
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option
    
    //http://beej-zhtw.netdpi.net/09-man-manual/9-20-setsockopt-getsockopt
	int opt = 1;
	bzero(addr, sizeof(struct sockaddr_in));
	
	if(setsocket(socketfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))<0)
		errCTL("initClientAddr error");
	
	
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	//http://beej-zhtw.netdpi.net/09-man-manual/9-13-inet_ntoa-inet_aton-inet_addr
	//addr->sin_addr.s_addr = inet_aton(sendClient);
	if(inet_aton(sendClient, &(addr->sin_addr) ==0)
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
	int sock_send, sock_recv, addrlen = sizeof(struct sockaddr);
	
	//store filename
	strcpy(packet.filename, filename);
	
	//find server
	sock_send = sendto(socketfd, &packet, sizeof(struct bootServerInfo), 0, (struct sockaddr *)broadaddr, addrlen)
	if(sock_send<0)
		errCTL("findServerAddr sock_send error");
	
	Timeout(socketfd, 20);
	
	bzero(servaddr,sizeof(struct sockaddr_in));
	
	sock_recv = recvfrom(socketfd, &packet, sizeof(struct bootServerInfo), 0, NULL, NULL)
	
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
		if(strcpy(packet.filename[0],'\0')!=0)
		{
			printf("               Get MyftpServer servAddr : %s\n               Myftp connectPort : %d\n", packet.servAddr, packet.connectPort);
		}
		//not exist
		else if(strcpy(packet.filename[0],'\0')==0)
		{
			puts("Requested file : %s doesn't exist",filename);
		}
		
	}
	return 0;
}


int startMyftpClient( int socketfd, struct sockaddr_in *servaddr, const char *filename ) {
	
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
int Timeout(int socketfd, int sec)
{
	struct timeval time;
	
	//set time information
	//second
	time.tv_sec = sec;
	
	if(setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct time)) <0)
		errCTL("Request timeout");
}

int listenClient(int socketfd, int port, int *tempPort, char *filename, struct sockaddr_in *clientaddr)
{




}


