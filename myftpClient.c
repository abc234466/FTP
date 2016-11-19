#include	"myftp.h"

// use ./myftpClient <port> <filename>
int main( int argc, char **argv ) {
	int socketfd;
	struct sockaddr_in servaddr,broadaddr;
	int result;

	/* Usage information.	*/
	if( argc != 3 ) {
		printf( "usage: ./myftpClient <port> <filename>\n" );
		return 0;
	}
	
	/* Open socket.	*/
	socketfd = socket(AF_INET, SOCK_DGRAM, 0 );
	if(socketfd<0)
		errCTL("Clinet socket error");
		
	/* Initial client address information. */
	//set boradcast socket
	if( initClientAddr(socketfd, atoi(argv[1]), "255.255.255.255", &broadaddr ) )
		errCTL("initClientAddr error");
	
	//find server 
	if(findServerAddr(socketfd, argv[2], &broadaddr, &servaddr))
		errCTL("findServerAddr error");
		
	//stop the broadcast socket
	close(socketfd);
	
	/* Start ftp client */
	if( startMyftpClient(socketfd, &servaddr, argv[2] ))
		errCTL("startMyftpClient error");
	
	return 0;
}
