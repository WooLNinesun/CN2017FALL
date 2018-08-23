#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

int open_socket();
void connect_irc_server(int sockfd);

void log_with_date(char line[]);

int read_cfgline(char *x);
char *get_config(char name[]);

int read_sockline(int sock, char buffer[]);
char *get_username(char line[]);
char *get_command(char line[]);
char *get_message(char line[]);
char *get_channel(char line[]);

char *get_botcmd(char message[], char *arg);
char *convert_hex_dec(char arg[]);
char **generate_vaild_IP(char* s, int* returnSize);

void set_nick(int sock, char nick[]);
void send_user_packet(int sock, char nick[]);
void join_channel(int sock, char channel[]);

void send_pong(int sock, char message[]);
void send_message(int sock, char to[], char message[]);

int main(int argc, char *argv[]) {
    /* ============= socket() ============= */
    int sockfd = open_socket();

    /* ============ connect() ============= */
    connect_irc_server( sockfd );

    /* =========== JOIN CHANNEL =========== */
    char *nick = get_config("NICK");
    char *user = get_config("USER");
    char *channels = get_config("CHANNEL");

    set_nick(sockfd, nick);
    send_user_packet(sockfd, user);
    join_channel(sockfd, channels);
    send_message(sockfd, channels, "Hello! I am robot.");

    free(nick); free(user); free(channels);

    /* =========== IRCBot Start =========== */
    while (1) {
        char line[512];
        read_sockline(sockfd, line);
        
        char *username = get_username(line);
        char *irccmd = get_command(line);
        char *message = get_message(line);

        if (strcmp(irccmd, "PING") == 0) {
            send_pong(sockfd, message);
            log_with_date("Got ping. Replying with pong...");
        } else if (strcmp(irccmd, "PRIVMSG") == 0) {
            char logline[512]; char *channel = get_channel(line);
            sprintf(logline, "%s/%s: %s", channel, username, message);
            log_with_date(logline);

            char arg[512] = {0}; char *botcmd = get_botcmd(message, arg);
            if (strcmp(botcmd, "@repeat") == 0) {
                send_message(sockfd, channel, arg);
            } else if (strcmp(botcmd, "@convert") == 0) {
                char *remsg = convert_hex_dec(arg);
                send_message(sockfd, channel, remsg);
                free(remsg);
            } else if (strcmp(botcmd, "@ip") == 0) {
                int inum; char snum[12];
                char **remsg = generate_vaild_IP(arg, &inum);

                sprintf(snum, "%d", inum);
                send_message(sockfd, channel, snum);
                for(int i = 0; i < inum; i++) {
                    send_message(sockfd, channel, remsg[i]);
                    free(remsg[i]);
                }
            } else if (strcmp(botcmd, "@help") == 0) {
                char *remsg[3] = {"@repeat <Message>", "@convert <Number>", "@ip <String>"};
                for(int i = 0; i < 3; i++) {
                    send_message(sockfd, channel, remsg[i]);
                }
            }

            free(channel); free(botcmd);
        } else if (strcmp(irccmd, "JOIN") == 0) {
            char logline[512]; char *channel = get_channel(line);
            sprintf(logline, "%s joined %s.", username, channel);
            log_with_date(logline);

            free(channel);
        } else if (strcmp(irccmd, "PART") == 0) {
            char logline[512]; char *channel = get_channel(line);
            sprintf(logline, "%s left %s.", username, channel);
            log_with_date(logline);

            free(channel);
        }

        free(username); free(irccmd); free(message);
    }
}

int open_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Could not create socket");
        exit(1);
    } else { printf("socket opened sucessfully | "); }

    return sockfd;
}

/* ============= connect() =============
 * first:   stores server name to connect and prints error message if host does not exist,
 * second:  copy server address structure ,and asign port to the server host structure,
 * final:   connect to server and prints error message if connect does not successful.
 */
void connect_irc_server(int sockfd) {
     char *ip = get_config("IP"), *port = get_config("PORT");
     
    struct hostent *server = gethostbyname(ip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    } else { printf("Found host | "); }

    struct sockaddr_in serv_addr; memset((char *) &serv_addr, 0 , sizeof(serv_addr));
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));

    free(ip); free(port);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Could not connect");
        exit(1);
    } else { printf("connection sucessful\n"); }
}

void log_with_date(char line[]) {
    char date[50];
    struct tm *current_time;

    time_t now = time(0);
    current_time = localtime(&now);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", current_time);

    printf("[%s] %s\n", date, line);
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
    } else {
        perror("Open config file error");
        exit(1);
    }
    return value;
}

int read_sockline(int sock, char buffer[]) {
    int length = 0;
    while (1){
        char data;
        int result = recv(sock, &data, 1, 0);
        if ((result <= 0) || (data == EOF)) {
            perror("Connection closed");
            exit(1);
        }
        buffer[length] = data;
        length++;
        if (length >= 2 && buffer[length-2] == '\r' && buffer[length-1] == '\n') {
            buffer[length-2] = '\0';
            return length;
        }
    }
}

char *get_username(char line[]) {
    char *username = malloc(512);
    char clone[512];
    strncpy(clone, line, strlen(line)+1);
    if (strchr(clone, '!') != NULL){
        char *splitted = strtok(clone, "!");
        if (splitted != NULL){
            strncpy(username, splitted+1, strlen(splitted)+1);
        }else{
            username[0] = '\0';
        }
    }else{
        username[0] = '\0';
    }
    return username;
}

char *get_command(char line[]) {
    char *command = malloc(512);
    char clone[512];
    strncpy(clone, line, strlen(line)+1);
    char *splitted = strtok(clone, " ");
    if (splitted != NULL){
        if (splitted[0] == ':'){
            splitted = strtok(NULL, " ");
        }
        if (splitted != NULL){
            strncpy(command, splitted, strlen(splitted)+1);
        }else{
            command[0] = '\0';
        }
    }else{
        command[0] = '\0';
    }
    return command;
}

char *get_message(char line[]) {
    char *msg = malloc(512);
    char clone[512];
    strncpy(clone, line, strlen(line)+1);
    char *splitted = strstr(clone, " :");
    if (splitted != NULL){
        strncpy(msg, splitted+2, strlen(splitted)+1);
    }else{
        msg[0] = '\0';
    }
    return msg;
}

char *get_channel(char line[]) {
    char *channel = malloc(512);
    char clone[512]; int argno = 1;
    strncpy(clone, line, strlen(line)+1);
    
    int current_arg = 0;
    char *splitted = strtok(clone, " ");
    while (splitted != NULL){
        if (splitted[0] != ':'){
            current_arg++;
        }
        if (current_arg == argno+1){
            strncpy(channel, splitted, strlen(splitted)+1);
            return channel;
        }
        splitted = strtok(NULL, " ");
    }
    
    if (current_arg != argno){
        channel[0] = '\0';
    }
    return channel;
}

char *get_botcmd(char message[], char *arg) {
    char *cmd = malloc(512);
    if ( message[0] == '@' ) {
        char *splitted = strchr(message, ' ');
        if (splitted != NULL){
            *splitted = '\0';
            strncpy(arg, splitted+1, strlen(splitted+1)+1);
        } else { arg[0] = '\0'; }
        strncpy(cmd, message, strlen(message)+1);
    } else {
        cmd[0] = '\0';
    }
    return cmd;
}

char *convert_hex_dec(char arg[]){
    char *remsg = malloc(512);
    if (arg[1] == 'x' || arg[1] == 'X') {
        // hex to dec
        int num;
        sscanf(arg, "%x", &num);
        sprintf(remsg, "%i", num);
    } else {
        // dec to hex
        int num;
        sscanf(arg, "%i", &num);
        sprintf(remsg, "0x%x", num);
    }
    return remsg;
}

int cmp(char *a, char *b, int n) {
    for(int i = 0; i < n; ++i) {
        if(a[i] != b[i]) {
            return a[i] - b[i];
        }
    }
    return 0;
}
int check_vaild_IP(char *arg, int len) {
    if ((len == 1 && cmp(arg,  "0", len) >= 0 && cmp(arg,  "9", len) <= 0) ||
        (len == 2 && cmp(arg, "10", len) >= 0 && cmp(arg, "99", len) <= 0) ||
        (len == 3 && cmp(arg,"100", len) >= 0 && cmp(arg,"255", len) <= 0)
    ) { return 1; }
    return 0;
}
void run_vaild_IP(char *arg, char *ip, int flag, char** ret, int* size) {
    int arglen = strlen(arg);
    if (flag == 3) {
        if( !check_vaild_IP(arg, arglen) ) { return; }

        strcat(ip, arg);
        ret[(*size)++] = strdup(ip);
        return ;
    }

    int iplen = strlen(ip);
    for(int n = 1; n <= 3; n++) {
        if (n > arglen) { break; }

        char *tmp = strndup(arg, n);
        if( !check_vaild_IP(tmp, n) ) { continue; }

        strcat(ip, tmp);
        strcat(ip, ".");
        run_vaild_IP(arg+n, ip, flag+1, ret, size);
        *(ip + iplen) = 0;
        free(tmp);
    }
}
char **generate_vaild_IP(char* arg, int* returnSize){
    char *ip = (char *)malloc(sizeof(char) * 16);
    memset(ip, 0, sizeof(char)*16);

    char** ret = (char **)malloc(sizeof(char*)*1000);

    int size = 0;
    run_vaild_IP(arg, ip, 0, ret, &size);
    *returnSize = size;

    ret = (char **)realloc(ret, sizeof(char *)*size);
    return ret;
}

void set_nick(int sockfd, char nick[]) {
    char nick_packet[512];
    sprintf(nick_packet, "NICK %s\r\n", nick);
    send(sockfd, nick_packet, strlen(nick_packet), 0);
}

void send_user_packet(int sockfd, char user[]) {
    char user_packet[512];
    sprintf(user_packet, "USER %s 0 * :%s\r\n", user, user);
    send(sockfd, user_packet, strlen(user_packet), 0);
}

void join_channel(int sockfd, char channel[]) {
    char join_packet[512];
    sprintf(join_packet, "JOIN %s\r\n", channel);
    send(sockfd, join_packet, strlen(join_packet), 0);
}

void send_pong(int sockfd, char message[]) {
    char pong_packet[512];
    sprintf(pong_packet, "PONG :%s\r\n", message);
    send(sockfd, pong_packet, strlen(pong_packet), 0);
}

void send_message(int sockfd, char to[], char message[]) {
    char message_packet[512];
    sprintf(message_packet, "PRIVMSG %s :%s\r\n", to, message);
    send(sockfd, message_packet, strlen(message_packet), 0);
}