exe=gcc

OBJS1=myftp.c myftpServer.c  
OBJS2=myftp.c myftpClient.c

all:	myftpServer myftpClient

myftpServer: $(OBJS1)
	$(exe) -o $@ $(OBJS1)

myftpClient: $(OBJS2)
	$(exe) -o $@ $(OBJS2)
clean:
	-rm myftpServer
	-rm myftpClient
	-rm client_*
