#include "msocket.h"

int main(){
    sleep(1);
    int mtp_sock = m_socket(AF_INET, SOCK_MTP, 0);
	printf("socket created\n");    
    printf("mtp Socket id: %d\n", mtp_sock);
   struct sockaddr_in serv_addr, cli_addr;
   memset(&serv_addr, 0, sizeof(serv_addr));
   memset(&cli_addr, 0, sizeof(cli_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(9501);
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   cli_addr.sin_family = AF_INET;
   cli_addr.sin_port = htons(9500);
   cli_addr.sin_addr.s_addr = INADDR_ANY;
   char buf[FRAME_SIZE];
    m_bind(mtp_sock, (const struct sockaddr*)&serv_addr, sizeof(serv_addr), (struct sockaddr*)&cli_addr, sizeof(cli_addr));
    printf("binded\n");
    sleep(7);
    memset(buf, 0, FRAME_SIZE);
    m_recvfrom(mtp_sock, buf, FRAME_SIZE, 0, (struct sockaddr*)&cli_addr, NULL);
    printf("[+] %s\n", buf);
    
    memset(buf, 0, FRAME_SIZE);
    sprintf(buf, "Hellohifromuser2_alliswell");
    m_sendto(mtp_sock, buf, FRAME_SIZE, 0, (const struct sockaddr*)&cli_addr, sizeof(cli_addr));
    printf("[-] %s\n", buf);

    sleep(7);
    memset(buf, 0, FRAME_SIZE);
    m_recvfrom(mtp_sock, buf, FRAME_SIZE, 0, (struct sockaddr*)&cli_addr, NULL);
    printf("[+] %s\n", buf);


    sleep(30);
    m_close(mtp_sock);
   printf("socket closed\n");
   
    return 0;
}
