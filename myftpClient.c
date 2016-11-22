#include	"myftp.h"

// use ./myftpClient <port> <filename>
int main(int argc, char **argv)
{
	int socketfd, port = atoi(argv[1]);
	char device[DEVICELEN];
	struct sockaddr_in broadaddr;
	struct startServerInfo packet;

	/* Usage information.	*/
	if(argc != 3)
	{
		printf("usage: ./myftpClient <port> <filename>\n");
		return 0;
	}
	
	/* Open socket.	*/
	socketfd = socket(AF_INET, SOCK_DGRAM, 0 );
	if(socketfd<0)
		errCTL("Clinet socket error");
	
	/* Get NIC device name. */
	if( getDeviceName( socketfd, device ) )
		errCTL("Client getDeviceName error");
	
	/* Initial client address information. */
	//set boradcast socket for mininet
	if(initClientAddr(socketfd, port, "10.255.255.255", &broadaddr))
		errCTL("initClientAddr error");
	
	//WAN broadcast
	//if( initClientAddr(socketfd, atoi(argv[1]), "255.255.255.255", &broadaddr ) )
	//	errCTL("initClientAddr error");	
	
	// send broadcast to find server 
	strcpy(packet.filename, argv[2]);
	if(findServerAddr(socketfd, &broadaddr, &packet))
		errCTL("findServerAddr");

	// no more broadcast
	close(socketfd);

	// send request and receive data
	if(startMyftpClient(&packet))
		errCTL("startMyftpClient");

	return 0;
}
