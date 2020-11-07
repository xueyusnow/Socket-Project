#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#define PORT "33054"   // the port users will be connecting to
#define HOSTNAME "127.0.0.1"

#define MAXDATASIZE 1024 // max number of bytes we can get at once 
using namespace std;

void *get_in_addr(struct sockaddr *sa);

//refer to https://beej.us/guide/bgnet/examples/client.c
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    
    struct sockaddr_storage my_addr;
    socklen_t addrlen;
    
    int getsock_check;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((rv = getaddrinfo(HOSTNAME, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        /*Retrieve the locally-bound name of the specified socket and store it in the sockaddr structure*/
        addrlen = sizeof my_addr;
        getsock_check=getsockname(sockfd,(struct sockaddr *)&my_addr,(socklen_t *)&addrlen);
        //Error checking
        if (getsock_check== -1) { 
            perror("getsockname"); 
            exit(1);
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);

    cout<<"Client is up and running"<<endl;

    while(1){
        int id;
        string country;
        cout<<"-----Start a new request-----"<<endl;
        cout<<"Please enter an User ID: ";
        cin>>id;
        cout<<"Please enter the Country Name: ";
        cin>>country;
        string tmp=to_string(id)+" "+country;
        // memset(buf,0,MAXDATASIZE);
        // if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        //     perror("recv");
        //     exit(1);
        // }
        memset(buf,0,MAXDATASIZE);
        strncpy(buf, tmp.c_str(), tmp.length()+1);
        if ((numbytes = send(sockfd, buf, strlen(buf), 0)) == -1) {
            perror("send");
            exit(1);
        }
        cout<<"Client has sent User "<<id<<" and "<<country<<" to Main Server using TCP"<<endl;
        memset(buf,0,MAXDATASIZE);

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        buf[numbytes] = '\0';
        if(strcmp(buf,"Country Name: Not found")==0){
            cout<<country<<" not found"<<endl;
        }
        else {
            if(strcmp(buf,"User ID: Not found")==0){
                cout<<"User "<<id<<" not found"<<endl;
            }else{
                cout<<"Client has received results from Main Server: User "<<buf<<" is possible friend of User "<<id<<" in "<<country<<endl;
            }
        }
    }
    close(sockfd);
    freeaddrinfo(servinfo); // all done with this structure
    return 0;
}