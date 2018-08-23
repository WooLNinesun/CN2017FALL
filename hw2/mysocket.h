// packet struct
struct udp_pkt_s {
    int     seq_num;
    int     ack_num;
    size_t  data_len;
    char    data[1024];
};

typedef struct udp_pkt_s    udp_pkt;
typedef struct sockaddr_in  addr_in;

// Socket
int  open_Socket();
void close_Socket(int sockfd);
void bind_Socket(int sockfd, addr_in *addr);

void send_Packet(int sockfd, addr_in *addr, udp_pkt *packet);
void recv_Packet(int sockfd, addr_in *addr, udp_pkt *packet);

int check_Recv(int sockfd);
int check_addr(addr_in *a, addr_in *b);

void create_addr(addr_in *addr, char *ip, int port) ;

// error exit msg
void Sysdie(char *msg);
void Errdie(char *msg);

// read config
int read_cfgline(char *x);
char *get_config(char name[]);
