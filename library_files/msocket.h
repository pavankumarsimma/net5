#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include <sys/time.h>



#define p 0.05
#define T 5

// Custom socket type for MTP
#define SOCK_MTP 3
#define MAX_SOCKETS 3
#define SEND_BUF_SIZE 10
#define RECV_BUF_SIZE 5
#define MSG_SIZE 1024

// errors
#define ENOTBOUND 99

typedef struct sbuf {
    char msg[MSG_SIZE];
    struct timeval sent_time;
    int sent;
    int occupied;
}sbuf;
typedef struct rbuf {
    char msg[MSG_SIZE];
    struct timeval recv_time;
    int occupied;
    int recvd;
}rbuf;
typedef struct swnd {
    int send_wnd_size;
    int start_index;
    int sequence[SEND_BUF_SIZE]; // msgs that are sent but not yet acknowledged
}swnd;
typedef struct rwnd{
    int recv_wnd_size;
    int start_index;
    int sequence[SEND_BUF_SIZE]; // msgs that are received but not yet acknowledged
}rwnd;

typedef struct mtp_socket_info {
    int alloted;
    pid_t pid;
    int udp_sock_id;
    struct sockaddr_in other;
    sbuf send_buffer[SEND_BUF_SIZE];
    rbuf recv_buffer[RECV_BUF_SIZE];
    swnd send_window;
    rwnd recv_window;
}mtp_socket_info;
typedef struct sock_info {
    int sock_id;
    char ip[INET_ADDRSTRLEN];
    int port;
    int err;
}sock_info;
// Global error variable
extern int m_errno;

// Function prototypes
int m_socket(int domain, int type, int protocol);
int m_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen, const struct sockaddr* dest, socklen_t destlen );
int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int m_close(int sockfd);
int dropMessage(float);
void set_curr_time(struct timeval*);
