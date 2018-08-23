#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>

#include "mysocket.h"

int main(int argc, char *argv[]) {
    /* ========= argument and configs ========= */
    char *SENDER_IP     = get_config("SENDER_IP");
    char *SENDER_PORT   = get_config("SENDER_PORT");
    char *AGENT_IP      = get_config("AGENT_IP");
    char *AGENT_PORT    = get_config("AGENT_PORT");
    char *RECEIVER_IP   = get_config("RECEIVER_IP");
    char *RECEIVER_PORT = get_config("RECEIVER_PORT");
    char *CHANCE        = get_config("CHANCE");
    int chance = atoi(CHANCE); free(CHANCE);

    /* ============ create address ============ */
    struct sockaddr_in my_addr, recv_addr, send_addr, tmp_addr;
    create_addr(&send_addr, SENDER_IP, atoi(SENDER_PORT));
    free(SENDER_IP); free(SENDER_PORT);

    create_addr(&my_addr, AGENT_IP, atoi(AGENT_PORT));
    free(AGENT_IP); free(AGENT_PORT);

    create_addr(&recv_addr, RECEIVER_IP, atoi(RECEIVER_PORT));
    free(RECEIVER_IP); free(RECEIVER_PORT);

    create_addr(&tmp_addr, "INADDR_ANY", 0);

    /* ============ socket and bind =========== */
    int sockfd = open_Socket();  
    bind_Socket( sockfd, &my_addr );

    /* ============== set random ============== */
    srand(time(NULL));

    /* ============= transmission ============= */
    udp_pkt Packet = { .seq_num = 0, .ack_num = 0 };
    float loss_rate = 0.0, drop_num = 0, total_num = 0;
    while(1) {
        if (check_Recv(sockfd)) {
            recv_Packet(sockfd, &tmp_addr, &Packet);
            if(
                !check_addr(&tmp_addr, &send_addr) && 
                !check_addr(&tmp_addr, &recv_addr)
            ) { continue; }

            int pkt_ack = Packet.ack_num, pkt_seq = Packet.seq_num;
            if (pkt_ack == 0 && pkt_seq != -1) {
                printf("get\tdata\t#%d\n", pkt_seq);
 
                char *act = NULL;
                int chose = (rand()%1000)+1;
                if ( chose <= chance ) {
                    drop_num = drop_num + 1;
                    act = "drop";
                } else {
                    send_Packet( sockfd, &recv_addr, &Packet);
                    act = "fwd";
                }
                
                total_num = total_num + 1;
                loss_rate = drop_num/total_num;
                printf("%s\tdata\t#%d,\tloss rate = %f\n", act, pkt_seq, loss_rate);
            } else if (pkt_ack == 0 && pkt_seq == -1) {
                printf("get\tfin\n");
                
                send_Packet( sockfd, &recv_addr, &Packet);
                printf("fwd\tfin\n");
            } else if (pkt_ack != -1 && pkt_seq == 0) {
                printf("get\tack\t#%d\n", Packet.ack_num);
                
                send_Packet( sockfd, &send_addr, &Packet);
                printf("fwd\tack\t#%d\n", Packet.ack_num);
            } else if (pkt_ack == -1 && pkt_seq == 0) {
                printf("get\tfinack\n");
                
                send_Packet( sockfd, &send_addr, &Packet);
                printf("fwd\tfinack\n");
                break;
            }
        }
    }

    close_Socket(sockfd);
    return 0;
}