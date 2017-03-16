#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int bind_my_socket(int port)
{
	int fd;
	struct sockaddr_in name;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return fd;
	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
		printf("Bind failre %m\n");
		close(fd);
		return -100;
	}
	return fd;
}

int main()
{
	int *buf;
	int fd;
	int i;
	int irc;

	buf = (int *)malloc(0x10000);
	if (!buf) return -10;

	fd = bind_my_socket(15629);
	if (fd < 0) return fd;

	for(i=0;;i++) {
		irc = read(fd, buf, 0x10000);
		printf(" %d: irc = %d buf = %d %d %d %d %d\n", i, irc, buf[0], buf[1], buf[2], buf[3], buf[4]);
	}

	return 0;
}

