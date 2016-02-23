#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define UDPPORT	0xA230
#define BSIZE 0x800000

/*	Create socket for UDP write						*/
int OpenUDP(void)
{
	struct sockaddr_in host;
	int fd;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("Can not create socket - %m\n");
		return -1;
	}

	host.sin_family = AF_INET;
	host.sin_port = htons(UDPPORT);
	host.sin_addr.s_addr = htonl(INADDR_ANY);

	if (connect(fd, (struct sockaddr *)&host, sizeof(host))) {
		printf("Setting default UDP destination failed - %m\n");
		close(fd);
		return -1;
	}
	return fd;
}

int main(int argc, char **argv)
{
	FILE *f;
	int fd;
	int *buf;
	int irc;

	if (argc < 2) {
		printf("Usage: dplay <data_file_name>\n");
		return 10;
	}

	f = fopen(argv[1], "rb");
	if (!f) {
		printf("File %s not found - %m.\n", argv[0]);
		return 20;
	}	

	fd = OpenUDP();
	if (fd <= 0) {
		fclose(f);
		return 30;
	}

	buf = (int *) malloc(BSIZE);
	if (!buf) {
		printf("Allocation failure %m.\n");
		fclose(f);
		close(fd);
		return 40;

	}

	for(;!feof(f);) {
		irc = fread(buf, sizeof(int), 1, f);
		if (irc < 0 || irc > 1) {
			printf("File read error - %m\n");
			break;
		} else if (irc == 0) continue;
		if (buf[0] > BSIZE || buf[0] < 20) {
			printf("Odd block size: %d\n", buf[0]);
			break;
		}
		irc = fread(&buf[1], buf[0] - sizeof(int), 1, f);
		if (irc != 1) {
			printf("Unexpected EOF - %m\n");
			break;
		}
		write(fd, buf, buf[0]);
	}

	fclose(f);
	close(fd);
	free(buf);
	return 0;
}

