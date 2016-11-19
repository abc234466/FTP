exe=gcc

OBJS=myftp.c myftpClient.c myftpServer.c  

ftp: $(OBJS)
	$(exe) -o $@ $(OBJS)

clean:
	-rm ftp
	-rm *.o
