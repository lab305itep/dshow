#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BSIZE	0x1000000



int bind_my_socket(int port)
{
	struct hostent *host;
	struct sockaddr_in hostname;
	int fd, flags, irc;
	
	host = gethostbyname("dserver.danss.local");
	if (!host) {
		printf("Host not found\n");
		return -10;
	}
	
	hostname.sin_family = AF_INET;
	hostname.sin_port = htons(port);
	hostname.sin_addr = *(struct in_addr *) host->h_addr;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("Can not open socket %m\n");
		return -20;
		}
	
	if (connect(fd, (struct sockaddr *)&hostname, sizeof(hostname))) {
		close(fd);
		printf("Can not connect the socket %m\n");
		return -30;
	}
	return fd;
}

int main()
{
	int *buf;
	int fd;
	int i, j;
	int irc;
	long long Cnt;

	buf = (int *)malloc(BSIZE);
	if (!buf) {
		printf("Malloc failed %m\n");
		return 10;
	}

	fd = bind_my_socket(15629);
	if (fd < 0) {
		printf("Open receive socket failed %m\n");
		return 20;
	}

	Cnt = 0;
	for(i=0;;i++) {
		irc = read(fd, buf, BSIZE);
		printf("Read irc = %d [Cnt = %d]\n", irc, (int)(Cnt / BSIZE));
		if (irc < 0) {
			close(fd);
			return 0;
		}
		Cnt += irc;
		for (j=0; j < irc/sizeof(int); j++) if (buf[j]) {
			printf("buf[%d:%d] = %d(0x%X)\n", i, j, buf[j], buf[j]);
			break;
		}
	}

	return 0;
}

