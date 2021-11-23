#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

#include <iostream>
#include <set>
using namespace std;

#define BUF_SIZE 100
#define BACK_LOG 3

char msg[BUF_SIZE] = { 0, };

void usage(){
    printf("syntax : echo-server <port> [-e[-b]]\n");
    printf("sample : echo-server 1234 -e -b");
}

struct Param {
	bool echo{false};
    bool broadcast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
        port = atoi(argv[1]);
		for (int i = 2; i < argc; i++) {
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
		}
            if(echo){
            for (int i = 2; i < argc; i++) {
                if (strcmp(argv[i], "-b") == 0) {
                    broadcast = true;
                    continue;
                }
            }
        }
		return port != 0;
	}
} param;


int main(int argc, char *argv[]){
    int s_sd;                       // server socket
    int client;                     // client socket

    struct sockaddr_in serv_addr;   // server's sockaddr_in
    struct sockaddr_in client_addr; // client's sockaddr_in

    struct timeval timeout;         // timeout
    fd_set reads, cpy_read;         
    int fd_max, fd_num;             
    
    // argument error handle
    if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

    // IPv4 Tcp socket descriptor
    if((s_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket error!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));   //초기화
    serv_addr.sin_family = AF_INET;             //IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);      //IP address
    serv_addr.sin_port = htons(param.port);             //port number

    //binding
    if(bind(s_sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        perror("[-] bind error");
        exit(-1);
    }
    else
        printf("[+] bind success\n");

    //listening
    if(listen(s_sd, BACK_LOG) == -1){
        perror("[-] listen error");
        exit(-1);
    }
    else
        printf("[+] listening now...\n");

    // initialize
    FD_ZERO(&reads);        
    FD_SET(s_sd, &reads);   //detect socket change 
    fd_max = s_sd;          // save socket number

    set<int> fd_s;

    while(1)
    {
        cpy_read = reads;

        //timeout setting
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        //select function
        fd_num = select(fd_max+1, &cpy_read, 0, 0, &timeout);

        if(fd_num == -1){
            perror("select error!");    // exception
        }
        else if(fd_num == 0){
            //printf("time out!\n");    //time out
            continue;
        }
        else
        {
            for(int i = 0 ; i < fd_max +1 ; i++)        // loop all client socket
            {
                if(FD_ISSET(i, &cpy_read))              // If there was a change in the socket descriptor
                {
                    if(i==s_sd)                         // connection request
                    {
                        socklen_t client_addr_size = sizeof(client_addr);
                        client = accept(s_sd, (struct sockaddr*)&client_addr, &client_addr_size); //accept
                        fd_s.insert(client);

                        FD_SET(client, &reads);         //SET client socket
                        if(fd_max < client);        
                            fd_max = client;            //update fd_max
                        //info
                        printf("[+] Connection accepted from %s:%d : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client);  
                        //write(client, "Hello!\n", strlen("Hello!\n"));  // just for check
                    }
                    else                                // read and echo
                    {
                        memset(msg, 0, BUF_SIZE);       // buffer clear
                        int str_len = read(i, msg, BUF_SIZE);   //read client socket
                        if(!strcmp(msg,"q\n") || !strcmp(msg, "Q\n"))  
                        {
                            FD_CLR(i, &reads);          //clear client socket
                            fd_s.erase(i);
                            close(i);
                            printf("[-] Closed client %d\n", i);//detect terminate
                        } 
                        else
                        {
                            if(param.echo){
                                printf("%d : %s\n",i, msg); // socket descriptor : message 
                                msg[str_len] = 0;           // add NULL
                                //write(i, "test!\n", strlen("test!\n"));

                                if(param.broadcast){                    
                                    for(set<int>::iterator it = fd_s.begin(); it != fd_s.end(); it++){
                                        //cout << *it << endl;
                                        write(*it, msg, str_len); // broadcast
                                    }
                                }
                                else{
                                    write(i, msg, str_len);     // echo
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    close(s_sd);

    return 0;
}  