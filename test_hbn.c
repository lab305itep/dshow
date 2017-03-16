#include <netdb.h>
#include <stdio.h>

int main(int argc, char **argv) 
{
        struct hostent *host;
        
    	if (argc < 2) return 10;
        host = gethostbyname(argv[1]);
        if (!host) {
                printf("Host %s not found. %m\n", argv[1]);
                return 20;
        } else {
		printf("OK\n");
    	}
}
