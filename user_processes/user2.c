#include "msocket.h"

int main(){
    int mtp_sock = m_socket(AF_INET, SOCK_MTP, 0);
    
    printf("Hello world from user2\n");
    m_close(mtp_sock);
    return 0;
}