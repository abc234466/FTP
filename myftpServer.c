#include	"myftp.h"

// use ./myftpServer <port> <filename>
int main( int argc,char **argv ) {
	int socketfd;
	struct stat buf;
	struct sockaddr_in servaddr, clientaddr;
	char device[DEVICELEN];
	int tmpPort, port,lc;
	
	/* Usage information. */
	//port number : 33020
	if( argc != 3 ) {
		printf( "usage: ./myftpServer <port> <filename>\n" );
		return 0;
	}
	// port we key in
	port = atoi(argv[1]);
	//random port
	srand(time(NULL));
	tmpPort = port + (rand() % 999) +1;	
	
	/* Check if file exist. */
	// lstat -> get file status
	if( lstat( argv[2], &buf ) < 0 ) {
		printf( "unknow file : %s\n", argv[2] );
		return 0;
	}

	/* Open socket. */
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd<0)
		errCTL("socket error");

	/* Get NIC device name. */
	if( getDeviceName( socketfd, device ) )
		errCTL("getDeviceName error");
	
	/* Initial server address. */
	if( initServerAddr(socketfd, atoi(argv[1]), device, &servaddr))
		errCTL("initServerAddr error");
	
	printf("network interface = %s\n",device);
	printf("network port %d\n",tmpPort);
	puts("MyFtp Server Start!!");

	//Function: Server can serve multiple clients
    //Hint: Use loop, listenClient(), startMyFtpServer(), and ( fork() or thread )
    
    //Process Identification
    pid_t fpid; 
    
	while( 1 ) 
	{
			
		lc = listenClient(socketfd, port, &tmpPort, argv[2], &clientaddr);
		
		if(lc <0)
		{
			continue;
		}
			
		fpid = fork();
		
		if(fpid < 0)
		{
			errCTL("fork error");
		}
			
		else if( fpid == 0 ) 
		{
					
			startMyftpServer(tmpPort, &clientaddr , argv[2]);
		}
		else
			;
	}

	return 0;
}
