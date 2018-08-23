#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "mysocket.h"

int open_Socket() {
    //open udp socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        Sysdie("Could not create socket");
    } else {
        printf("Socket opened sucessfully | ");
    }

    return sockfd;
}

void close_Socket(int sockfd) { close(sockfd); return; }

void bind_Socket(int sockfd, addr_in *addr) {
    if (bind(sockfd, (struct sockaddr *) addr, sizeof(*addr)) == -1) {
        printf("\n"); Sysdie("Could not bind");
   } else { printf("Bind sucessfully\n"); }

   return;
}

void send_Packet(int sockfd, addr_in *addr, udp_pkt *packet) {
    size_t sizeof_pkt = sizeof(udp_pkt), sizeof_addr = sizeof(*addr);
    if (sendto(sockfd, packet, sizeof_pkt, 0, (struct sockaddr*)addr, sizeof_addr) == -1) {
        printf("\n"); Sysdie("Send packet error.");
    }

    return;
}

void recv_Packet(int sockfd, addr_in *addr, udp_pkt *packet) {
    size_t sizeof_pkt = sizeof(udp_pkt), sizeof_addr = sizeof(*addr);
    memset((char *)addr, 0 , sizeof_addr); memset((char *)packet, 0 , sizeof_pkt);

    int addrSize = sizeof_addr;
    if ( recvfrom(sockfd, packet, sizeof_pkt, 0, (struct sockaddr*)addr, &addrSize) < 0) {
        printf("\n"); Sysdie("Receive packet error.");
    }

    return;
}

int check_Recv(int sockfd) {
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
    fd_set rfd; FD_ZERO(&rfd); FD_SET(sockfd, &rfd);

    int nRet = select(sockfd+1, &rfd, NULL, NULL, &timeout);
    if (nRet == -1) {
        Sysdie("select() error");
    } else if (nRet == 0) {
        // timeout
        return 0;
    } else {
        return FD_ISSET(sockfd, &rfd);
    }
}
int check_addr(addr_in *a, addr_in *b) {
    int check = strcmp(inet_ntoa(a->sin_addr), inet_ntoa(b->sin_addr)) == 0;
        check = (a->sin_port   == b->sin_port  ) && check;
        check = (a->sin_family == b->sin_family) && check;
    
    return ( check )? 1:0;
}

void create_addr(addr_in *addr, char *ip, int port) {
    memset((char *) addr, 0 , sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    if (strcmp( ip, "INADDR_ANY" ) != 0) {
        addr->sin_addr.s_addr = inet_addr(ip);
    } else {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    }

    return;
}


void Errdie(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
void Sysdie(char *msg) {
    perror(msg);
    exit(1);
}

int hasEOF = 0; FILE *cfgfile = NULL;
char buf[1<<10]; char *p = buf;
int readChar() {
    static int N = 1<<10;
    static char *end = buf;
    if(p == end) {
        if((end = buf + fread(buf, 1, N, cfgfile)) == buf) {
            hasEOF = 1;
            return EOF;   
        }
        p = buf;
    }
    return *p++;
}
int read_cfgline(char line[]) {
    char c = 0; int i = 0;
    if((c = readChar()) == EOF ) { return 0; }
    do {
        line[i++] = c;
    } while( (c = readChar()) != '\n' && c != EOF );
    line[i] = '\0';
    return 1;
}
char *get_config(char name[]) {
    char *value = malloc(1024);
    cfgfile = fopen("config", "r");
    value[0] = '\0';
    if (cfgfile != NULL) {
        char cfgline[1024]; p = buf;
        while ( read_cfgline( cfgline ) ){
            char cfgname[1024], cfgvalue[1024];
            sscanf(cfgline, "%1023[^=]=%s", cfgname, cfgvalue);

            if (strcmp(cfgname, name) == 0){
                char *tempvalue = strtok(cfgvalue, "\'");
                strncpy(value, tempvalue, strlen(tempvalue)+1);
                break;
            }
        }
        fclose(cfgfile);
    } else { Sysdie("Open config file error"); }
    return value;
}
