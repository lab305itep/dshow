#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int OpenSock(const char *hname, int port)
{
	struct hostent *host;
	struct sockaddr_in hostname;
	int fd;

	host = gethostbyname(hname);
	if (!host) {
		printf("Host not found %s. %m\n", hname);
		return -100;
	}

	hostname.sin_family = AF_INET;
	hostname.sin_port = htons(port);
	hostname.sin_addr = *(struct in_addr *) host->h_addr;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("Can not create socket - %m\n");
		return -200;
	}

	if (connect(fd, (struct sockaddr *)&hostname, sizeof(hostname))) {
		printf("Setting default destination to %s:%d failed %m\n", hname, port);
		close(fd);
		return -300;
	}
	return fd;
}

int DoSelect(int fd)
{
	fd_set wset;
	struct timeval tm;
	int irc;
	
	tm.tv_sec = 2;		// 2 s
	tm.tv_usec = 0;
	FD_ZERO(&wset);
	FD_SET(fd, &wset);

	irc = select(FD_SETSIZE, NULL, &wset, NULL, &tm);

	if (irc < 0) {
		printf("Select Error %m\n");
	} else if (irc == 0) {
		printf("Timeout\n");
	} else if (irc > 1) {
		printf("Strange irc = %d\n", irc);
	} else if (!FD_ISSET(fd, &wset)) {
		printf("Logic error\n");
		irc = 2;
	}
	return irc;
}

int main()
{
	int *buf;
	int fd;
	int i, j, irc;

	buf = (int *)malloc(0x100000);
	if (!buf) return -10;

	fd = OpenSock("dserver.danss.local", 15629);
	if (fd < 0) return fd;

	j = 0;
	for(i=0;;i++) {
		buf[0] = i;
		irc = DoSelect(fd);
		if (irc == 1) {
			irc = write(fd, buf, 0xC000);
			if (irc == 0xC000) {
				if (!(j&0xFFFF)) printf("Written: %d/%d\n", j, i);
				j++;
			} else if (irc < 0) {
				printf("Write error %m\n");
			} else {
				printf("Strange number ov bytes written: %d\n", irc);
			}
		}
	}
	return 0;
}

