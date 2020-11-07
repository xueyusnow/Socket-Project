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

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#define SERVERAPORT "30054"
#define SERVERBPORT "31054"
#define TCPPORT "33054"
#define UDPPORT "32054"
#define HOSTNAME "127.0.0.1"

#define MAXBUFLEN 1024

//refer to http://www.beej.us/guide/bgnet/html/#a-simple-stream-server
#define BACKLOG 10   // how many pending connections queue will hold
using namespace std;
vector<string> countryA;
vector<string> countryB;

void sigchld_handler(int s);
void getcountry(char* query, string &country, int &id);
void countrylist(unordered_map<string,int> &map,char* buf1,int server1,char* buf2,int server2);
void *get_in_addr(struct sockaddr *sa);

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}
void getcountry(char* query, string &country, int &id){//extract country and user id from the query
	int len=strlen(query);
	int i=0;
	while(i<len){
		if(query[i]==' '){
			country=query+i+1;
			query[i]='\0';
			id=stoi(query);
			return;
		}
		i++;
	}
}
//combine the two country lists and ready for filtering
void countrylist(unordered_map<string,int> &map,char* buf1,int server1,char* buf2,int server2){
	int s=0;
	int len1=strlen(buf1);
	string country;
	for(int i=0;i<len1;i++){
		if(buf1[i]==' '){
			buf1[i]='\0';
			country=buf1+s;
			s=i+1;
			map[country]=server1;
			countryA.push_back(country);
		}
        if(i==len1-1){
            country=buf1+s;
            s=i+1;
            map[country]=server1;
            countryA.push_back(country);
        }
	}
	s=0;
	int len2=strlen(buf2);
	for(int i=0;i<len2;i++){
		if(buf2[i]==' '){
			buf2[i]='\0';
			country=buf2+s;
			s=i+1;
			map[country]=server2;
			countryB.push_back(country);
		}
        if(i==len2-1){
            country=buf2+s;
            s=i+1;
            map[country]=server2;
            countryB.push_back(country);
        }
	}
    //print country lists in two backend servers
	cout<<left<<setw(21)<<"Server A"<<"| Server B"<<endl;
    int max=countryA.size();
    if(countryA.size()<countryB.size()){
        max=countryB.size();
    }
	for(int i=0;i<max;i++){
		if(i>=countryA.size()){
			cout<<left<<setw(21)<<" "<<"| "<<countryB[i]<<endl;
		}else if(i>=countryB.size()){
			cout<<left<<setw(21)<<countryA[i]<<"|"<<endl;
		}else{
			cout<<left<<setw(21)<<countryA[i]<<"|"<<countryB[i]<<endl;
		}
	}
}

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
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    char ss[INET6_ADDRSTRLEN];
    int rv;
    int receivebyte;
    unordered_map<string,int>countrymap;
    char buf1[MAXBUFLEN];
    char buf2[MAXBUFLEN];
    char buf[MAXBUFLEN];
    string error;

    //Set up UDP
    struct sockaddr_storage their_addrua;
    socklen_t addr_lenua;
    struct sockaddr_storage their_addrub;
    socklen_t addr_lenub;

    
    
    //UDP serverA
    //refer to https://beej.us/guide/bgnet/examples/talker.c
    int sockfdua;
    struct addrinfo hintsua, *servinfoua, *pua;
    int rvua;
    int numbytesua;

    memset(&hintsua, 0, sizeof hintsua);
    hintsua.ai_family = AF_UNSPEC;
    hintsua.ai_socktype = SOCK_DGRAM;

    if ((rvua = getaddrinfo(HOSTNAME, SERVERAPORT, &hintsua, &servinfoua)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rvua));
        return 1;
    }

    // loop through all the results and make a socket
    for(pua = servinfoua; pua != NULL; pua = pua->ai_next) {
        if ((sockfdua = socket(pua->ai_family, pua->ai_socktype,
                pua->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (pua == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    

    //UDP serverB
    //refer to https://beej.us/guide/bgnet/examples/talker.c
    int sockfdub;
    struct addrinfo hintsub, *servinfoub, *pub;
    int rvub;
    int numbytesub;

    memset(&hintsub, 0, sizeof hintsub);
    hintsub.ai_family = AF_UNSPEC;
    hintsub.ai_socktype = SOCK_DGRAM;

    if ((rvub = getaddrinfo(HOSTNAME, SERVERBPORT, &hintsub, &servinfoub)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rvub));
        return 1;
    }

    // loop through all the results and make a socket
    for(pub = servinfoub; pub != NULL; pub = pub->ai_next) {
        if ((sockfdub = socket(pub->ai_family, pub->ai_socktype,
                pub->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (pub == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    

    //set up TCP
    //refer to https://beej.us/guide/bgnet/examples/server.c
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, TCPPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    cout<<"The Main server is up and running"<<endl;

    //ask country list from serverA
    memset(buf1,0,MAXBUFLEN);
    string a="ask countries";
    strncpy(buf1,a.c_str(),a.length()+1);
    if ((numbytesua = sendto(sockfdua, buf1, strlen(buf1), 0,
             pua->ai_addr, pua->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    memset(buf1,0,MAXBUFLEN);
    addr_lenua=sizeof(their_addrua);
    if ((numbytesua = recvfrom(sockfdua, buf1, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addrua, &addr_lenua)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    cout<<"The Main server has received the country list from server A using UDP over port "<<UDPPORT<<endl;
    //ask country list from serverB
    memset(buf2,0,MAXBUFLEN);
    strncpy(buf2,a.c_str(),a.length()+1);
    if ((numbytesub = sendto(sockfdub, buf2, strlen(buf2), 0,
             pub->ai_addr, pub->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    memset(buf2,0,MAXBUFLEN);
    addr_lenub=sizeof(their_addrub);
    if ((numbytesub = recvfrom(sockfdub, buf2, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addrub, &addr_lenub)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    cout<<"The Main server has received the country list from server B using UDP over port "<<UDPPORT<<endl;
    
    //print country lists
    countrylist(countrymap,buf1,1,buf2,2);

    // int first=1;
    // int clientid=1;
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        // if(first==1){
        //     strncpy(ss,s,sizeof(s));
        //     first--;
        // }
        // if(strcmp(s,ss)!=0){
        //     clientid=2;
        // }else{
        //     clientid=1;
        // }
        if (!fork()) { // this is the child process
            close(sockfd);// child doesn't need the listener
            while(1){
                memset(buf,0,MAXBUFLEN);
                // if (send(new_fd, "Ready", 5, 0) == -1){
                //     perror("send");
                //     exit(1);
                // }
                if ((receivebyte = recv(new_fd, buf, MAXBUFLEN-1, 0)) == -1) {
    		        perror("recv");
    		        exit(1);
    		    }

    		    buf[receivebyte] = '\0';
                memset(buf1,0,MAXBUFLEN);
                strncpy(buf1,buf,sizeof(buf));
                memset(buf2,0,MAXBUFLEN);
                strncpy(buf2,buf,sizeof(buf));
                string country;
                int id;
    		    getcountry(buf,country,id);

    		    cout<<"The Main server has received the request on User "<<id<<" in "<<country<<" from client using TCP over port "<<TCPPORT<<endl;
    		    if(countrymap.find(country)==countrymap.end()){
    		    	cout<<country<<" does not show up in server A&B"<<endl;
    		    	memset(buf,0,MAXBUFLEN);
    		    	error="Country Name: Not found";
    		    	strncpy(buf,error.c_str(),error.length()+1);
    		    	if(send(new_fd,buf,strlen(buf),0)==-1)
    		    		perror("send");
    		    	cout<<"The Main Server has sent \"Country Name: Not found\" to client1/2 using TCP over port "<<TCPPORT<<endl;
    		    }else{
    		    	if(countrymap[country]==1){
    			    	cout<<country<<" shows up in server A"<<endl;
    			    	if ((numbytesua = sendto(sockfdua, buf1, strlen(buf1), 0,
    				             pua->ai_addr, pua->ai_addrlen)) == -1) {
    				        perror("talker: sendto");
    				        exit(1);
    				    }
    				    cout<<"The Main Server has sent request from User "<<id<<" to server A using UDP over port "<<UDPPORT<<endl;
    			    	memset(buf1,0,MAXBUFLEN);
    			    	if ((numbytesua = recvfrom(sockfdua, buf1, MAXBUFLEN-1 , 0,
    				        (struct sockaddr *)&their_addrua, &addr_lenua)) == -1) {
    				        perror("recvfrom");
    				        exit(1);
    				    }
                        buf1[numbytesua] = '\0';
    				    if(strcmp(buf1,"not found")==0){
    				    	cout<<"The Main server has received \"User ID: Not found\" from server A"<<endl;
    				    	memset(buf1,0,MAXBUFLEN);
                            error="User ID: Not found";
                            strncpy(buf1,error.c_str(),error.length()+1);
                            if(send(new_fd,buf1,strlen(buf1),0)==-1)
    		    				perror("send");
    	    				cout<<"The Main Server has sent error to client using TCP over port "<<TCPPORT<<endl;
    				    }else{
    				    	cout<<"The Main server has received searching result(s) of User "<<id<<" from server A"<<endl;
    				    	if(send(new_fd,buf1,strlen(buf1),0)==-1)
    		    				perror("send");
    	    				cout<<"The Main Server has sent searching result(s) to client using TCP over port "<<TCPPORT<<endl;
    			    	}
    				}else{
    					cout<<country<<" shows up in server B"<<endl;
    			    	if ((numbytesub = sendto(sockfdub, buf2, strlen(buf2), 0,
    				             pub->ai_addr, pub->ai_addrlen)) == -1) {
    				        perror("talker: sendto");
    				        exit(1);
    				    }
    				    cout<<"The Main Server has sent request from User "<<id<<" to server B using UDP over port "<<UDPPORT<<endl;
    			    	memset(buf2,0,MAXBUFLEN);
    			    	if ((numbytesub = recvfrom(sockfdub, buf2, MAXBUFLEN-1 , 0,
    				        (struct sockaddr *)&their_addrub, &addr_lenub)) == -1) {
    				        perror("recvfrom");
    				        exit(1);
    				    }
                        buf2[numbytesub] = '\0';
    				    if(strcmp(buf2,"not found")==0){
    				    	cout<<"The Main server has received \"User ID: Not found\" from server B"<<endl;
    				    	memset(buf2,0,MAXBUFLEN);
                            error="User ID: Not found";
                            strncpy(buf2,error.c_str(),error.length()+1);
                            if(send(new_fd,buf2,strlen(buf2),0)==-1)
    		    				perror("send");
    	    				cout<<"The Main Server has sent error to client using TCP over port "<<TCPPORT<<endl;
    				    }else{
    				    	cout<<"The Main server has received searching result(s) of User "<<id<<" from server B"<<endl;
    				    	if(send(new_fd,buf2,strlen(buf2),0)==-1)
    		    				perror("send");
    	    				cout<<"The Main Server has sent searching result(s) to client using TCP over port "<<TCPPORT<<endl;
    			    	}
    				}
    		    }
                
                //exit(0);
            }
            close(new_fd);
            exit(0);
        }
        
        close(new_fd);  // parent doesn't need this
    }
    freeaddrinfo(servinfoua);
    freeaddrinfo(servinfoub);
    freeaddrinfo(servinfo); // all done with this structure
    close(sockfd);
    close(sockfdua);
    close(sockfdub);
    return 0;
}