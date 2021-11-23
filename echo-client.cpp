#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 100

char msg[BUF_SIZE] = { 0, };
uint16_t port;
char ip[20];

void usage(){
    printf("syntax : echo-client <ip> <port>\n");
    printf("sample : echo-client 192.168.10.2 1234");
}


int main(int argc, char *argv[]){
    int sd;                         // server socket descripter
    struct sockaddr_in serv_addr;   // server's sockaddr_in

    struct timeval timeout;         // timeout
    fd_set reads, cpy_read;         // fd_set
    int fd_max, fd_num;             

    // argument error handel
    if(argc != 3)
    {
        usage();
        return -1;
    }
    
    sprintf(ip, "%s", argv[1]);
    port = atoi(argv[2]);

    // Connection Server info
    printf("+-------------------------+\n");
    printf("+   Server   Connection    +\n");
    printf("+  ip   : %-16s+\n",ip);
    printf("+  port : %-5d           +\n",port);
    printf("+-------------------------+\n");

    //IPv4 TCP socket descriptor
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("[-]socket error!");
        exit(-1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));   //초기화
    serv_addr.sin_family = AF_INET;             //IPv4
    serv_addr.sin_addr.s_addr = inet_addr(ip);   //IP address
    serv_addr.sin_port = htons(port);     //port number

    // connect to server
    if (connect(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1){
        perror("[-] conncet error");
        exit(-1);
    }
    else
        printf("[+] Connected to Server!\n");

    // initialize 
    FD_ZERO(&reads);
    FD_SET(0, &reads);  //detect stdin's change 
    FD_SET(sd, &reads); //detect sd's change

    fd_max = sd;        // save socket number

    while(1){
        cpy_read = reads;

        //timeout setting
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        //select function
        fd_num = select(fd_max+1, &cpy_read, 0, 0, &timeout);

        if(fd_num == -1){
            perror("select error!"); // exception
        }
        else if(fd_num == 0){
            printf("time out!\n");  //time out
            continue;
        }
        else{                       
            if(FD_ISSET(0, &cpy_read)){         // If there was a change in the input descriptor
                memset(msg, 0, BUF_SIZE);       // buffer clear
                fgets(msg, BUF_SIZE, stdin);    // set message

                write(sd, msg, strlen(msg));    // write to server socket
                if(!strcmp(msg,"q\n") || !strcmp(msg, "Q\n")){
                    exit(0);    // connection terminate
                }

                FD_CLR(0, &cpy_read);           // clear fd
            }
            if(FD_ISSET(sd, &cpy_read)){         // If there was a change in the socket descriptor
                memset(msg, 0, BUF_SIZE);       // buffer clear
                int str_len = read(sd, msg, BUF_SIZE);  // read message from server socket
                if(str_len == 0){
                    close(sd);
                    return 0;
                }
                msg[str_len] = '\0';            // add NULL
                printf("[+] Message from server : %s\n", msg);
                //printf("check\n");

                FD_CLR(sd, &cpy_read);          // clear fd
            }

        }
    }

    close(sd);
    return 0;
}