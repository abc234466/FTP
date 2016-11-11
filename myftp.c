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

int initClientAddr( int socketfd, int port, char *sendClient, struct sockaddr_in *addr ) {
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option
	int opt = 1;
	if(setsocket(socketfd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt))<0)
		errCTL("initClientAddr error");
		
	return 0;
}

int findServerAddr( int socketfd, char *filename, const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr ) {
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct startServerInfo as boradcast message
    //          Use setsockopt to set timeout
	struct startServerInfo packet;
	
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
}*/

//include sys/time.h
//reference : http://pubs.opengroup.org/onlinepubs/7908799/xsh/systime.h.html
int Timeout(int socketfd, int sec, int usec)
{
	struct timeval time;
	
	//set time information
	//second
	time.tv_sec = sec;
	//microsecond
	time.tv_usec = usec; 
	
	


}

int listenClient(int socketfd, int port, int *tempPort, char *filename, struct sockaddr_in *clientaddr)
{




}


