#include "msocket.h"

void* R(void* arg);
void* S(void * arg);
void* G(void* arg);
sock_info* SOCK_INFO;
mtp_socket_info* SM;
struct sembuf mtx_op;
int mutex;
int main(){
    // creating the shared memory segments
    int shm_SOCK_INFO = shmget(3500, sizeof(sock_info), IPC_CREAT|0777);
    SOCK_INFO = (sock_info*)shmat(shm_SOCK_INFO, 0, 0 );
    SOCK_INFO->err=0;
    SOCK_INFO->port = 0;
    SOCK_INFO->sock_id = 0;
    memset(SOCK_INFO->ip, 0, INET_ADDRSTRLEN);

    int shm_SM = shmget(3501, sizeof(mtp_socket_info)*MAX_SOCKETS, 0777|IPC_CREAT);
    SM = (mtp_socket_info*) shmat(shm_SM, 0, 0);

    // creating the semaphores
    int sem1, sem2;
    struct sembuf sem1_op;
    struct sembuf sem2_op;
    sem1_op.sem_flg = 0;
    sem1_op.sem_num = 0;
    sem2_op.sem_flg = 0;
    sem2_op.sem_num = 0;
    // mutex for sendto and recvfrom
    mutex = semget(5000, 1, 0777|IPC_CREAT);
    
    mtx_op.sem_flg = 0;
    mtx_op.sem_num = 0;
    semctl(mutex, 0, SETVAL, 1);

    sem1 = semget(4100, 1, 0777|IPC_CREAT);
    sem2 = semget(4101, 1, 0777|IPC_CREAT);

    semctl(sem1, 0, SETVAL, 0);
    semctl(sem1, 0, SETVAL, 0);

    // creating the threads R and S
    pthread_t R_thread, S_thread, G_thread;
    pthread_create(&R_thread, NULL, R, NULL);
    pthread_create(&S_thread, NULL, S, NULL);
    pthread_create(&G_thread, NULL, G, NULL);

    while(1){
        // wait on sem1
        sem1_op.sem_op = -1;
        semop(sem1, &sem1_op, 1);

        if (SOCK_INFO->err==0 && SOCK_INFO->port == 0 && SOCK_INFO->sock_id==0){
            // m_socket call
            int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
            if (udp_socket == -1){
                SOCK_INFO->sock_id = -1;
                SOCK_INFO->err = errno;
            }
            else{
                SOCK_INFO->sock_id = udp_socket;
            }
        }
        else if (SOCK_INFO->sock_id != 0 && SOCK_INFO->port != 0){
            // m_bind call
            struct sockaddr_in servaddr;
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = inet_addr(SOCK_INFO->ip);
            servaddr.sin_port = htons(SOCK_INFO->port);
            int bind_return = bind(SOCK_INFO->sock_id, (const struct sockaddr*)&servaddr, sizeof(servaddr));
            if (bind_return == -1){
                SOCK_INFO->sock_id = -1;
                SOCK_INFO->err = errno;
            }
        }
        // signal on sem2
        sem2_op.sem_op = 1;
        semop(sem2, &sem2_op, 1);
    }
    

    // waiting for the threads to end
    pthread_join(R_thread, NULL);
    pthread_join(S_thread, NULL);
    pthread_join(G_thread, NULL);
    // cleaning the shared memory segments by the end of init process
    shmdt(SOCK_INFO);
    shmdt(SM);
    shmctl(shm_SOCK_INFO, IPC_RMID, 0);
    shmctl(shm_SM, IPC_RMID, 0);
    // cleaning the semaphores created
    semctl(sem1, 0, IPC_RMID, 0);
    semctl(sem2, 0, IPC_RMID, 0);
    semctl(mutex, 0, IPC_RMID, 0);
    return 0;
}

void* R(void* arg){
    fd_set readfds;
    int nospace = 0;
    while(1){
        FD_ZERO(&readfds);
        int maxfd=0;
        struct timeval tv;
        tv.tv_sec = T;
        tv.tv_usec = 0;
        mtx_op.sem_op = -1;
        semop(mutex, &mtx_op, 1);   // wait
        for(int i=0; i<MAX_SOCKETS; i++){
            if (SM[i].alloted == 1){
                if (!kill(SM[i].pid, 0)){
                    // killed process
                    
                }
                else {
                    FD_SET(SM[i].udp_sock_id, &readfds);
                    maxfd = max(maxfd, SM[i].udp_sock_id);
                }
            }
        }

        int activity = select(maxfd+1, &readfds, NULL, NULL,&tv );
        if (activity == -1){
            perror("Select: ");
        }
        else if (activity == 0){
            // time  out happend
            
        }

        mtx_op.sem_op = 1;
        semop(mutex, &mtx_op, 1);   // signal
    }
    return NULL;
}
void* S(void* arg){
    
    return NULL;    
}
void* G(void* arg){
    while(1){
        usleep(1000);
        mtx_op.sem_op = -1;
        semop(mutex, &mtx_op, 1);  // wait
        
        for(int i=0; i<MAX_SOCKETS; i++){
            // close those udp sockets for killed processes
            if (SM[i].alloted==1 && !kill(SM[i].pid, 0)) { 
                close(SM[i].udp_sock_id);
                SM[i].alloted = 0;
            }
        }

        mtx_op.sem_op = 1;
        semop(mutex, &mtx_op, 1);   // signal
    }
    return NULL;
}