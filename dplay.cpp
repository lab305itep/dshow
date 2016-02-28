//  Play data file for dshow

#define _FILE_OFFSET_BITS 64
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define BSIZE 0x800000

/*	Create socket for UDP write						*/
int OpenUDP(char *hostname, int port)
{
	struct sockaddr_in host;
	struct hostent *hostbyname;

	int fd;
	
	if (hostname) {
		hostbyname = gethostbyname(hostname);
		if (!hostbyname) {
			printf("host %s not found.\n", hostname);
			return -1;
		}
	}

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		printf("Can not create socket - %m\n");
		return -1;
	}

	host.sin_family = AF_INET;
	host.sin_port = htons(port);
	if (hostname) {
		host.sin_addr = *(struct in_addr *) hostbyname->h_addr;
	} else{
		host.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	
	if (connect(fd, (struct sockaddr *)&host, sizeof(host))) {
		printf("Setting default UDP destination failed - %m\n");
		close(fd);
		return -1;
	}
	return fd;
}

void Help(void)
{
	printf("Play data file for dshow. Does not prevent online data to go into the same analysis.\n");
	printf("Usage: ./dplay [-h] [-p port] [-s events] [-t host] file.dat ...\n");
	printf("Several files could be given. Options:\n");
	printf("   -h - print this message;\n");
	printf("   -p port - send data to the UDP port port (0xA230 - default);\n");
	printf("   -s number - insert 1 second sleep after number blocks;\n");
	printf("   -t host - target host to which send data.\n");
}

int main(int argc, char **argv)
{
	FILE *f;
	int fd;
	int *buf;
	int irc;
	char *hostname;
	int port;
	int c;
	int iNum;
	int Cnt;
	int i;

	hostname = NULL;
	port = 0xA230;
	iNum = 0;
	for (;;) {
		c = getopt(argc, argv, "hp:t:s:");
		if (c == -1) break;
		switch (c) {
		case 'p':
			port = strtol(optarg, NULL, 0);
			break;
		case 't':
			hostname = optarg;
			break;
		case 's':
			iNum = strtol(optarg, NULL, 0);
			break;		
		case 'h':
		default:
			Help();
			return 0;
		}
	}

	if (argc <= optind) {
		Help();
		return 10;
	}

	fd = OpenUDP(hostname, port);
	if (fd <= 0) return 30;

	buf = (int *) malloc(BSIZE);
	if (!buf) {
		printf("Allocation failure %m.\n");
		close(fd);
		return 40;

	}

	for (i=optind; i<argc; i++) {	

		f = fopen(argv[i], "rb");
		if (!f) {
			printf("File %s not found - %m.\n", argv[i]);
			continue;
		}	

		Cnt = 0;
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
			
			Cnt++;
			if (iNum > 0 && Cnt == iNum) {
				sleep(1);
				Cnt = 0;
			}
		}
		if (f) fclose(f);
	}
	
	close(fd);
	free(buf);
	return 0;
}

