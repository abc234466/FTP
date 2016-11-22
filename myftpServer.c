#include	"myftp.h"

// use ./myftpServer <port> <filename>
int main(int argc,char **argv)
{
	int socketfd;
	struct stat buf;
	struct sockaddr_in servaddr, clientaddr;
	char device[DEVICELEN];
	int tmp_port, port,lc;

	/* Usage information. */
	//port number : 33020 , Student ID:M053040020
	if(argc != 3)
	{
		printf("usage: ./myftpServer <port> <filename>\n");
		return 0;
	}

	// port we key in
	port = atoi(argv[1]);
	
	/* Check if file exist. */
	// lstat -> get file status
	if(lstat(argv[2], &buf) < 0)
	{
		printf("unknow file : %s\n", argv[2]);
		return 0;
	}
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	// set socket
	if(socketfd < 0)
		errCTL("socket error");

	// set device
	if(getDeviceName( socketfd, device ) )
		errCTL("Server getDeviceName error");

	/* Initial server address. */
	if( initServerAddr(socketfd, port, device, &servaddr) <0)
		errCTL("initServerAddr error");

    printf("network interface = %s\n",device);
	printf("network port %d\n",port);
	puts("MyFtp Server Start!!");
	printf("share file : %s\n", argv[2]);
	
	//Function: Server can serve multiple clients
    //Hint: Use loop, listenClient(), startMyFtpServer(), and ( fork() or thread )

	//Process Identification
    pid_t fpid; 
    
	while( 1 ) 
	{
		puts("wait client!");		
		//random port
		srand(time(NULL));
		tmp_port = port + (rand() % 999) +1;
		
		//wait to listen client				
		lc = listenClient(socketfd, tmp_port, argv[2], &clientaddr);
		
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
			//prepare to start ftp server
			startMyftpServer(tmp_port, &clientaddr , argv[2]);
			exit(0);
		}
		else
			;
	}

	return 0;
}
