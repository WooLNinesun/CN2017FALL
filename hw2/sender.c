#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "mysocket.h"

int timeout = 0;
int check_timeout(long long start);
long long get_Nowtime(void);

int main(int argc, char *argv[]) {
    /* ========= argument and configs ========= */
    char *FilePath;
    if ( argc != 2 ) {
        char msg[1024];
        sprintf(msg, "Usage: %s <file's path>", argv[0]);
        Errdie(msg);
    } else { FilePath = argv[1]; }

    char *SENDER_IP     = get_config("SENDER_IP");
    char *SENDER_PORT   = get_config("SENDER_PORT");
    char *AGENT_IP      = get_config("AGENT_IP");
    char *AGENT_PORT    = get_config("AGENT_PORT");
    char *THRESHOLD     = get_config("THRESHOLD");
    char *PKTDATASIZE   = get_config("PKTDATASIZE");
    char *TIMEOUT       = get_config("TIMEOUT");
    int threshold       = atoi(THRESHOLD);      free(THRESHOLD);
    int pkt_datasize    = atoi(PKTDATASIZE);    free(PKTDATASIZE);
        timeout         = atoi(TIMEOUT);        free(TIMEOUT);

    /* ============== open file =============== */
    FILE *fptr = fopen(FilePath, "rb");
    if(fptr==NULL) {
        char msg[1024];
        sprintf(msg, "Fail To Open File: %s", FilePath);
        Errdie(msg);
    }
    if(fseek(fptr, 0, SEEK_END) == -1) { Errdie("Fail to seek offset"); }
    int file_size = ftell(fptr),
        pkt_maxnum  = (file_size/pkt_datasize)+ 1;
        pkt_maxnum += (file_size%pkt_datasize)? 1:0;
    
    int dotindex = 0;
    char Extension[1024]; memset(Extension, 0, 1024);
    for(int i = strlen(FilePath)-1; i >= 0; i--) {
        if(FilePath[i] == '.') {
            strcpy(Extension, &FilePath[i]);
            dotindex = i; break;
        }
    } Extension[strlen(FilePath)-dotindex] = '\0';

    /* ============ create address ============ */
    struct sockaddr_in my_addr, agent_addr, tmp_addr;
    create_addr(&my_addr, SENDER_IP, atoi(SENDER_PORT));
    free(SENDER_IP); free(SENDER_PORT);

    create_addr(&agent_addr, AGENT_IP, atoi(AGENT_PORT));
    free(AGENT_IP); free(AGENT_PORT);

    create_addr(&tmp_addr, "INADDR_ANY", 0);
    
    /* ============ socket and bind =========== */
    int sockfd = open_Socket();  
    bind_Socket( sockfd, &my_addr );

    /* ============== set timeout var ========= */
    long long start = 0;

    /* ============= transmission ============= */
    udp_pkt Packet = { .seq_num = 0, .ack_num = 0 };
    int winsize = 1, sent = 1, ack = 0, rsend_seq = 0;
	while (1) {
        int bound = (ack + winsize < pkt_maxnum)? ack + winsize:pkt_maxnum;
		for(; sent <= bound; sent++) {
            char *act = (rsend_seq > sent)? "rsend":"send";

            int read_count = 0;
            char file_data[pkt_datasize];
            memset(file_data, 0, pkt_datasize);
            if(sent == 1) {
                strcpy(file_data, Extension);
                read_count = strlen(Extension);
            } else if (sent > 1) {
                if(fseek(fptr, (sent-2)*pkt_datasize, SEEK_SET) == -1) {
                    Errdie("Fail to seek offset");
                }
                read_count = fread(file_data, 1, pkt_datasize, fptr);
            }

            
            Packet.seq_num = sent; Packet.ack_num = 0;
            memset(Packet.data, 0, 1024);
            memcpy(Packet.data, file_data, read_count);
            Packet.data_len = read_count;
            send_Packet( sockfd, &agent_addr, &Packet);
            printf("%s\tdata\t#%d,\twinSize = %d\n", act, Packet.seq_num, winsize);
        }

        start = get_Nowtime();
        while(1) {
            if (check_Recv(sockfd)) {
                recv_Packet(sockfd, &tmp_addr, &Packet);
                if(!check_addr(&tmp_addr, &agent_addr)) { continue;}
                
                if( Packet.seq_num == 0 ) {
                    printf("recv\tack\t#%d\n", Packet.ack_num);
                    if (ack+1 == Packet.ack_num) {
                        ack++;
                        start = get_Nowtime();
                    }
                }

                if(ack == bound) {
                    winsize =  (winsize >= threshold)? winsize+1: winsize*2;
                    break;
                }
            }

            if ( check_timeout(start) ) {
                threshold = (winsize/2 > 1)? winsize/2:1;
                printf("time\tout,\t\tthreshold = %d\n", threshold);
                winsize = 1;
                rsend_seq = sent;
                sent = ack+1;
                break;
            }
        }

        if(ack == pkt_maxnum) {  break; }
    }

    // send final
    int break_flag = 0;
    while(1) {
        Packet.seq_num = -1; Packet.ack_num = 0; Packet.data_len = 0;
        memset(Packet.data, 0, 1024);
        send_Packet( sockfd, &agent_addr, &Packet);

        printf("send\tfin\n");

        start = get_Nowtime();
        while(1) {
            if (check_Recv(sockfd)) {
                recv_Packet(sockfd, &tmp_addr, &Packet);
                if(!check_addr(&tmp_addr, &agent_addr)) { continue; }
    
                if( Packet.ack_num == -1 && Packet.seq_num == 0) {
                    printf("recv\tfinack\n");
                    break_flag = 1; break; 
                }
            }

            if ( check_timeout(start) ) { break; }
        }
        if(break_flag) { break; }
    }
    
    fclose(fptr);
    close_Socket(sockfd);
    return 0;
}

int check_timeout(long long start) {
    long long end = get_Nowtime();
    return (end - start > timeout)? 1:0;
}

long long get_Nowtime(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}