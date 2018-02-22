extern "C"{           
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
}

extern "C" int set_io_blockability(int fd, int nonblock)
{
    int val;
    if (nonblock) {
        val = (O_NONBLOCK | fcntl(fd, F_GETFL));
    } else {
        val = (~O_NONBLOCK & fcntl(fd, F_GETFL));
    }
    return fcntl(fd, F_SETFL, val);
}

extern "C" int safe_tcp_connect(const char* ipaddr, int port, int nonblock)
{
    struct sockaddr_in peer;
    
    memset(&peer, 0, sizeof(peer));
    peer.sin_family  = AF_INET;
    peer.sin_port    = htons(port);
    if (inet_pton(AF_INET, ipaddr, &peer.sin_addr) <= 0) {
        return -1;
    }
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if ( sockfd == -1 ) {
        return -1;
    }
    
    if (connect(sockfd, (const struct sockaddr *)&peer, sizeof(peer)) == -1) {
        close(sockfd);
        return -1;
    }
    set_io_blockability(sockfd, nonblock);
    return sockfd;
}


int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s ipaddr port\n", argv[0]);
		return -1;
	}
	int sockfd = safe_tcp_connect(argv[1], atoi(argv[2]), 0);
	if (sockfd == -1) {
		perror("connect: ");
		return -1;
	}
    char buf[4096];
    char recv_buf[4096];
	while (fgets(buf, 4096, stdin) != NULL) {
        if(buf[strlen(buf) -1] == '\n')
            buf[strlen(buf) -1] = 0;
        write(sockfd, buf, strlen(buf));
        memset(recv_buf, 0, 4096);
        read(sockfd, recv_buf, 4096);
        fputs(recv_buf, stdout);
        printf("\n");
        //printf("%s\n", recv_buf);
	}
	return 0;
}
