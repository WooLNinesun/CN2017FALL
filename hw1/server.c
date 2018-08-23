#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

typedef struct	t_env {
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int newfd;
    int sockfd;
    int fdmax;
    fd_set master;
    fd_set fdreads;
} s_env;


void send_to_client(s_env* env, int i, char* buff) {
  
}

void send_all_client(s_env* env,int	nbytes, char* buff, int i) {
    int	j;

    printf("Server recv %d bytes\n", nbytes);
    printf("%s \n",buff);
    j = 0;
    while (j <= env->fdmax) {
        if (FD_ISSET(j,&env->master)) {
            if (j != env->sockfd && j != i) {
                send(j,buff,sizeof buff,0);
            }
        }
        j++;
    }
}

void new_client(s_env* env) {
    int	newfd;
    char	buff[1024];
    char	welcom[] = "Welcome to the Internet Relay Network nick!user@host\n";
    char 	yourhost[] = "Your host is servername, running version version\n";
    char	create[] = "server_name version user_modes chan_modes\n";

    env->addr_size = sizeof env->their_addr;
    newfd = accept(env->sockfd, (struct sockaddr *)&env->their_addr,&env->addr_size);
    printf("connex %d\n", newfd);
    if (newfd > env->fdmax) {
        env->fdmax = newfd;
    }
    FD_SET(newfd, &env->master);
    recv(newfd, buff, sizeof buff, 0);
    printf("%s\n",buff);
    send(newfd,welcom,sizeof welcom,0);
    send(newfd,yourhost,sizeof yourhost,0);
    send(newfd,create,sizeof create,0);

}

void read_fd(s_env* env) {
    int	i;
    char 	buff[2048];
    int	nbytes;

    i = 0;
    while (i <= env->fdmax) {
        if (FD_ISSET(i,&env->fdreads)) {
            if (i == env->sockfd) {
                new_client(env);
            } else {
                nbytes = recv(i, buff, 2048, 0);
                if(nbytes <= 0) {
                    close(i);
                    FD_CLR(i,&env->master);
                } else {
                    send_all_client(env,nbytes,buff,i);
                }
            }
        }
        i++;
    }
}

int	main(void) {
    struct addrinfo	hints;
    struct addrinfo	*res;
    s_env env;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo("127.0.0.1", "3490", &hints, &res);
    env.sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bind(env.sockfd, res->ai_addr, res->ai_addrlen);
    listen(env.sockfd, 10);
    FD_ZERO(&env.fdreads);
    FD_ZERO(&env.master);
    FD_SET(env.sockfd,&env.master);
    env.fdmax = env.sockfd;
    while (1) {
        env.fdreads = env.master;
        printf("AVANT\n");
        select(env.fdmax + 1, &env.fdreads, NULL, NULL, NULL);
        printf("APRES\n");
        read_fd(&env);
    }
}