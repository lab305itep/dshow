#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int Rfd;
int Wfd;
int *Buf;
#define BSIZE	0x10000000
int Cnt;

int OpenListenSock(int port)
{
	int fd, i, irc;
	struct sockaddr_in name;
	
	fd = socket (PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("Can not create socket.: %m\n");
		return fd;
	}

	i = 1;
	irc = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &i, sizeof(i));
	if (irc) {
		printf("Setsockopt error: %m\n");
		close(fd);
		return -1;
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
		printf("Can not bind to port %d: %m\n", port);
		close(fd);
		return -1;
	}
	
	if (listen(fd, 1) < 0) {
		printf("Can not listen to port %d: %m\n", port);
		close(fd);
		return -1;
	}
	
	return fd;
}

void DoRead(void)
{
	struct sockaddr_in addr;
	socklen_t len;
	int i, irc, descr;

	len = sizeof(addr);
	irc = accept(Rfd, (struct sockaddr *)&addr, &len);
	if (irc <= 0) {
		printf("Client connection accept error: %m\n");
		return;
	}
	if (Wfd > 0) {
		close(irc);
		printf("Connection already opened\n");
		return;
	}

	Wfd = irc;

	// Set client non-blocking
	irc = fcntl(Wfd, F_GETFL);
	if (irc < 0) {
		close(Wfd);
		Wfd = 0;
		printf("Client fcntl(F_GETFD) error: %m\n");
		return;
	}

	descr = irc | O_NONBLOCK;
	irc = fcntl(Wfd, F_SETFL, descr);
	if (irc < 0) {
		close(Wfd);
		Wfd = 0;
		printf("Client fcntl(F_SETFD) error: %m\n");
		return;
	}

	irc = fcntl(Wfd, F_GETFL);
	printf("Resulting flags: 0x%X (%X/%X)\n", irc, descr, O_NONBLOCK);

	printf("Client connection accepted\n");
}

void DoWrite(void)
{
	int flen, irc;
	
	flen = BSIZE;
	irc = write(Wfd, Buf, flen);
	if (irc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		printf("Block sent\n");
	} else if (irc < 0) {
		printf("Client send error: %m\n");
		shutdown(Wfd, 2);
		Wfd = 0;
		Cnt = 0;
	} else {
		printf("%d bytes sent [cnt = %d]\n", irc, Cnt);
	}
	Cnt++;
	irc = write(Wfd, Buf, flen);
	if (irc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		printf("EAGAIN\n");
		return;
	} else if (irc < 0) {
		printf("Client send other error: %m\n");
		shutdown(Wfd, 2);
		Wfd = 0;
		Cnt = 0;
		return;
	} else {
		printf("Another %d bytes sent [cnt = %d]\n", irc, Cnt);
	}
	Cnt++;
}

void DoSelect(void)
{
	fd_set wset;
	fd_set rset;

	struct timeval tm;
	int irc;

	tm.tv_sec = 2;		// 2 s
	tm.tv_usec = 0;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(Rfd, &rset);
	if (Wfd > 0) {
		FD_SET(Wfd, &wset);
	}

	irc = select(FD_SETSIZE, &rset, &wset, NULL, &tm);

	if (irc < 0) {
		printf("Select Error %m\n");
	} else if (irc == 0) {
		printf("Timeout\n");
	} else {
		if (Wfd > 0 && FD_ISSET(Wfd, &wset)) DoWrite();
		if (FD_ISSET(Rfd, &rset)) DoRead();
	}
}

int main()
{
	int *buf;
	int fd;
	int i, irc;

	Buf = (int *)malloc(BSIZE);
	if (!Buf) {
		printf("Malloc failed %m\n");
		return 10;
	}
	memset(Buf, 0, BSIZE);

	Rfd = OpenListenSock(15629);
	if (Rfd < 0) {
		printf("Open listen sock failed %m\n");
		return 20;
	}
	Wfd = 0;
	Cnt = 0;

	for(i=0;;i++) {
		memset(Buf, 0, BSIZE);
		Buf[0] = i;
		DoSelect();
		memset(Buf, 5, BSIZE);
	}
	return 0;
}

