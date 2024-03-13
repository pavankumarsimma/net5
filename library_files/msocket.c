#include "msocket.h"

sock_info* SOCK_INFO;
mtp_socket_info* SM;

int m_socket(int domain, int type, int protocol){
    int shm_SOCK_INFO = shmget(3500, sizeof(sock_info), IPC_CREAT|0777);
    SOCK_INFO = (sock_info*)shmat(shm_SOCK_INFO, 0, 0 );
    int shm_SM = shmget(3501, sizeof(mtp_socket_info)*MAX_SOCKETS, 0777|IPC_CREAT);
    SM = (mtp_socket_info*) shmat(shm_SM, 0, 0);
    int sem1, sem2;
    sem1 = semget(4100, 1, 0777|IPC_CREAT);
    sem2 = semget(4101, 1, 0777|IPC_CREAT);
    struct sembuf sem1_op;
    struct sembuf sem2_op;
    sem1_op.sem_flg = 0;
    sem1_op.sem_num = 0;
    sem2_op.sem_flg = 0;
    sem2_op.sem_num = 0;
    int mutex;
    mutex = semget(5000, 1, 0777|IPC_CREAT);
    struct sembuf mtx_op;
    mtx_op.sem_flg = 0;
    mtx_op.sem_num = 0;

    int return_value = -1;
    int i=0;
    for (i=0; i<MAX_SOCKETS; i++){
        if (SM[i].alloted == 0){
            // not alloted
            sem1_op.sem_op = 1;
            semop(sem1, &sem1_op, 1);   // signal sem1
            sem2_op.sem_op = -1;
            semop(sem2, &sem2_op, 1);   // wait sem2
            mtx_op.sem_op = -1;
            semop(mutex, &mtx_op, 1);
            
            if (SOCK_INFO->sock_id == -1){
                // error
                errno = SOCK_INFO->err;
                return_value = -1;
            }
            else {
                SM[i].alloted = 1;
                return_value = i;
                SM[i].pid = getpid();
                SM[i].udp_sock_id = SOCK_INFO->sock_id;
            }
            mtx_op.sem_op = 1;
            semop(mutex, &mtx_op, 1);
            break;
        }
    }
    if (i == MAX_SOCKETS){
        printf("m_socket: No free mtp scoket available\n");
        errno = ENOBUFS;
        return_value = -1;
    }
    SOCK_INFO->err=0;
    SOCK_INFO->port = 0;
    SOCK_INFO->sock_id = 0;
    memset(SOCK_INFO->ip, 0, INET_ADDRSTRLEN);
    shmdt(SOCK_INFO);
    shmdt(SM);
    return return_value;
}
int m_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen, const struct sockaddr* dest, socklen_t destlen ){
    int shm_SOCK_INFO = shmget(3500, sizeof(sock_info), IPC_CREAT|0777);
    SOCK_INFO = (sock_info*)shmat(shm_SOCK_INFO, 0, 0 );
    int shm_SM = shmget(3501, sizeof(mtp_socket_info)*MAX_SOCKETS, 0777|IPC_CREAT);
    SM = (mtp_socket_info*) shmat(shm_SM, 0, 0);
    int sem1, sem2;
    sem1 = semget(4100, 1, 0777|IPC_CREAT);
    sem2 = semget(4101, 1, 0777|IPC_CREAT);
    struct sembuf sem1_op;
    struct sembuf sem2_op;
    sem1_op.sem_flg = 0;
    sem1_op.sem_num = 0;
    sem2_op.sem_flg = 0;
    sem2_op.sem_num = 0;
    int mutex;
    mutex = semget(5000, 1, 0777|IPC_CREAT);
    struct sembuf mtx_op;
    mtx_op.sem_flg = 0;
    mtx_op.sem_num = 0;

    int return_value = -1;

    if (sockfd >= MAX_SOCKETS){
        // no such mtp socket is there
        printf("m_bind: No such mtp socket created\n");
        errno = EBADF;
    }
    else {
        int udp_socket = SM[sockfd].udp_sock_id;
        const struct sockaddr_in *addr_in = (const struct sockaddr_in *) addr;
        int port = ntohs(addr_in->sin_port);
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr_in->sin_addr), ip, INET_ADDRSTRLEN);
        SOCK_INFO->port = port;
        SOCK_INFO->sock_id = udp_socket;
        sprintf(SOCK_INFO->ip, "%s", ip);

        sem1_op.sem_op = 1;
        semop(sem1, &sem1_op, 1);   // signal sem1
        sem2_op.sem_op = -1;
        semop(sem2, &sem2_op, 1);   // wait sem2

        if (SOCK_INFO->sock_id == -1){
            return_value = -1; // error
            errno = SOCK_INFO->err;
        }
        else {
            return_value = 0; // success
            mtx_op.sem_op = -1;
            semop(mutex, &mtx_op, 1);

            SM[sockfd].other = *(struct sockaddr_in *) dest;

            mtx_op.sem_op = 1;
            semop(mutex, &mtx_op, 1);
        }
    }

    SOCK_INFO->err=0;
    SOCK_INFO->port = 0;
    SOCK_INFO->sock_id = 0;
    memset(SOCK_INFO->ip, 0, INET_ADDRSTRLEN);
    shmdt(SOCK_INFO);
    shmdt(SM);
    return return_value;
}
int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen){
    int shm_SM = shmget(3501, sizeof(mtp_socket_info)*MAX_SOCKETS, 0777|IPC_CREAT);
    SM = (mtp_socket_info*) shmat(shm_SM, 0, 0);
    int mutex;
    mutex = semget(5000, 1, 0777|IPC_CREAT);
    struct sembuf mtx_op;
    mtx_op.sem_flg = 0;
    mtx_op.sem_num = 0;

    int return_value = -1;

    struct sockaddr_in dest = *(struct sockaddr_in *)addr;
    mtx_op.sem_op = -1;
    semop(mutex, &mtx_op, 1);
    if (SM[sockfd].alloted != 1){
        return_value = -1;
        errno = EBADF;
    }
    else if (strcmp(SM[sockfd].other.sin_addr.s_addr, dest.sin_addr.s_addr)==0 && SM[sockfd].other.sin_port==dest.sin_port){
        // bound
        if (SM[sockfd].send_window.send_wnd_size < SEND_BUF_SIZE){
            // space is there
            int index = (SM[sockfd].send_window.start_index + SM[sockfd].send_window.send_wnd_size)%SEND_BUF_SIZE;
            int i=index;
            while(i<SM[sockfd].send_window.start_index || i>=index){
                if (SM[sockfd].send_buffer[i].occupied==0){
                    // means a space for the message
                    memset(SM[sockfd].send_buffer, 0, min(len, MSG_SIZE));
                    strncpy(SM[sockfd].send_buffer[index].msg, (char *)buf, len);
                    SM[sockfd].send_buffer[index].occupied = 1;
                    SM[sockfd].send_buffer[index].sent = 0;
                    return_value = min(len, MSG_SIZE);
                    break;
                }
                i++;
                i = i%SEND_BUF_SIZE;
            }
        }
        else  {
            // no space is there in sender buffer
            return_value = -1;
            errno = ENOBUFS;
        }
    }
    else {
        // not bound
        return_value = -1;
        errno = ENOTBOUND;
    }

    mtx_op.sem_op = 1;
    semop(mutex, &mtx_op, 1);
    shmdt(SM);
    return return_value;
}
int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen){
    int shm_SM = shmget(3501, sizeof(mtp_socket_info)*MAX_SOCKETS, 0777|IPC_CREAT);
    SM = (mtp_socket_info*) shmat(shm_SM, 0, 0);
    int mutex;
    mutex = semget(5000, 1, 0777|IPC_CREAT);
    struct sembuf mtx_op;
    mtx_op.sem_flg = 0;
    mtx_op.sem_num = 0;

    int return_value = -1;

    struct sockaddr_in dest = *(struct sockaddr_in *)addr;
    mtx_op.sem_op = -1;
    semop(mutex, &mtx_op, 1);
    
    int index = (SM[sockfd].recv_window.start_index + SM[sockfd].recv_window.recv_wnd_size)%RECV_BUF_SIZE;

    int i= index;
    if (SM[sockfd].alloted != 1){
        return_value = -1;
        errno = EBADF;
    }
    else {
        while(i<SM[sockfd].recv_window.start_index || i>=index){
            if (SM[sockfd].recv_buffer[i].recvd==1 && SM[sockfd].recv_buffer[i].occupied==1){
                // means a valid msg that can be received
                strncpy(buf, SM[sockfd].recv_buffer[i].msg, min(len, MSG_SIZE));
                SM[sockfd].recv_buffer[i].recvd==0;
                SM[sockfd].recv_buffer[i].occupied==0;
                return_value = min(len, MSG_SIZE);
                break;
            }
            i++;
            i = i%RECV_BUF_SIZE;
        }
        if (i == SM[sockfd].recv_window.start_index){
            // no msg came
            return_value = -1;
            errno = ENOMSG;
        }
    }
    
    mtx_op.sem_op = 1;
    semop(mutex, &mtx_op, 1);
    shmdt(SM);
    return return_value;
}
int m_close(int sockfd){
    int shm_SM = shmget(3501, sizeof(mtp_socket_info)*MAX_SOCKETS, 0777|IPC_CREAT);
    SM = (mtp_socket_info*) shmat(shm_SM, 0, 0);
    int mutex;
    mutex = semget(5000, 1, 0777|IPC_CREAT);
    struct sembuf mtx_op;
    mtx_op.sem_flg = 0;
    mtx_op.sem_num = 0;

    int return_value = -1;

    mtx_op.sem_op = -1;
    semop(mutex, &mtx_op, 1);
    
    if (sockfd >= MAX_SOCKETS || sockfd < 0) {
        // Invalid socket descriptor
        printf("m_close: Invalid socket descriptor\n");
        errno = EBADF;
        return_value = -1;
    } else if (SM[sockfd].alloted == 0) {
        // Socket not allocated
        printf("m_close: Socket not allocated\n");
        errno = EINVAL;
        return_value = -1;
    } else {
        close(SM[sockfd].udp_sock_id);

        SM[sockfd].alloted = 0;

        return_value = 0;
    }
    
    mtx_op.sem_op = 1;
    semop(mutex, &mtx_op, 1);
    shmdt(SM);
    return return_value;
}

int dropMessage(float a) {
    float random_num = ((float) rand()) / RAND_MAX; // Generate random number between 0 and 1
    return (random_num < p) ? 1 : 0; // Return 1 if random_num is less than p, else return 0
}


void set_curr_time(struct timeval* tv){
    gettimeofday(tv, NULL);
}