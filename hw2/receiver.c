#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "mysocket.h"

int main(int argc, char *argv[]) {
    /* ========= argument and configs ========= */
    char *Filedir; 
    if ( argc != 2 ) {
        char msg[1024];
        sprintf(msg, "Usage: %s <file's dir>", argv[0]);
        Errdie(msg);
    } else { Filedir = argv[1]; }

    char *AGENT_IP      = get_config("AGENT_IP");
    char *AGENT_PORT    = get_config("AGENT_PORT");
    char *RECEIVER_IP   = get_config("RECEIVER_IP");
    char *RECEIVER_PORT = get_config("RECEIVER_PORT");
    char *BUFFERSIZE    = get_config("BUFFERSIZE");
    int buf_size = atoi(BUFFERSIZE); free(BUFFERSIZE);

    /* ============ create address ============ */
    struct sockaddr_in my_addr, agent_addr, tmp_addr;
    create_addr(&agent_addr, AGENT_IP, atoi(AGENT_PORT));
    free(AGENT_IP); free(AGENT_PORT);

    create_addr(&my_addr, RECEIVER_IP, atoi(RECEIVER_PORT));
    free(RECEIVER_IP); free(RECEIVER_PORT);

    create_addr(&tmp_addr, "INADDR_ANY", 0);

    /* ============ socket and bind =========== */
    int sockfd = open_Socket();  
    bind_Socket( sockfd, &my_addr );

    /* ============= transmission ============= */
    udp_pkt Packet = { .seq_num = 0, .ack_num = 0 };
    udp_pkt buf_data[buf_size];
    FILE *fptr = NULL;
    int ack = 0, buf_index = 0;
    while(1) {
        if (check_Recv(sockfd)) {
            memset(Packet.data, 0, 1024);
            recv_Packet(sockfd, &tmp_addr, &Packet);
            if(!check_addr(&tmp_addr, &agent_addr)) { continue; }

            int pkt_ack = Packet.ack_num, pkt_seq = Packet.seq_num;
            if (pkt_ack == 0 && pkt_seq != -1){
                int pkt_seq = Packet.seq_num;
                printf("recv\tdata\t#%d\n", pkt_seq);
    
                if ( ack + 1 == pkt_seq ) {
                    if(pkt_seq == 1) {
                        char FilePath[1024];
                        sprintf(FilePath, "%sresult%s", Filedir, Packet.data);
                        
                        if((fptr = fopen(FilePath, "wb"))==NULL) {
                            char msg[1024];
                            sprintf(msg, "Fail To Open File: %s", FilePath);
                            Errdie(msg);
                        }
                    } else {
                        if( buf_index+1 != buf_size) {
                            memset(buf_data[buf_index].data, 0, 1024);
                            memcpy(buf_data[buf_index].data, Packet.data, Packet.data_len);
                            buf_data[buf_index].data_len = Packet.data_len;

                            buf_index++;
                        } else {
                            ack--;
                            printf("drop\tdata\t#%d\n", pkt_seq);

                            printf("flush\n");
                            for(int i = 0; i < buf_index; i++) {
                                fwrite(buf_data[i].data, 1, buf_data[i].data_len, fptr);
                            }
                            buf_index = 0;
                        }
                    }
                    ack++;
                }
    
                Packet.seq_num = 0; Packet.ack_num = ack;
                memset(Packet.data, 0, 1024);
                Packet.data_len = 0;
                send_Packet(sockfd, &tmp_addr, &Packet);
                printf("send\tack\t#%d\n", ack);
            } else if (pkt_ack == 0 && pkt_seq == -1) {
                printf("recv\tfin\n");

                Packet.seq_num = 0; Packet.ack_num = -1;
                memset(Packet.data, 0, 1024);
                Packet.data_len = 0;
                send_Packet( sockfd, &agent_addr, &Packet);
                printf("send\tfinack\n");

                printf("flush\n");
                for(int i = 0; i < buf_index; i++) {
                    fwrite(buf_data[i].data, 1, buf_data[i].data_len, fptr);
                }
                break;
            }
        }
    }

    fclose(fptr);
    close_Socket(sockfd);
    return 0;
}