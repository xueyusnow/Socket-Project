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
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <unordered_map>
#define MYPORT "30054"   // the port users will be connecting to
#define SERVERPORT "32054"
#define MAXBUFLEN 1024

using namespace std;
typedef set<int> neighbors;
typedef unordered_map<int,neighbors> graph;
unordered_map<string,graph> countrymap;

void constructgraph();
void getcountry(char* query, string &country, string &id);
int getcommonneighbors(string country, int idu, int idn);
void getallcountries(char* buf);
string recommendation(string country, int id);
void *get_in_addr(struct sockaddr *sa);


void constructgraph(){//read the data1.txt file and build the connection map
	string line;
	ifstream infile;
	string currentcountry;
	string currword;
	infile.open("data1.txt");
	if(!infile.is_open()){
		cout<<"The file data1.txt cannot be opened successfully"<<endl;
		exit(-1);
	}
	
	while(getline(infile,line)){
		int currentnum=-1;
		stringstream ss(line);
		while(ss>>currword){
			// if(currword.length()==0)
			// 	break;
			if(!std::isdigit(currword[0])){//the line shows country
				countrymap[currword];
				currentcountry=currword;
			}else{//the line shows user and his networking
				
				if(currentnum==-1){//user
					currentnum=stoi(currword);
					countrymap[currentcountry][currentnum];
				}else{//networking
					int currnei=stoi(currword);
					countrymap[currentcountry][currentnum].insert(currnei);
					countrymap[currentcountry][currnei].insert(currentnum);
				}
			}
		}
	}
}
void getcountry(char* query, string &country, string &id){//extract country and user id from the query
	int len=strlen(query);
	int i=0;
	while(i<len){
		if(query[i]==' '){
			country=query+i+1;
			query[i]='\0';
			id=query;
			return;
		}
		i++;
	}
}

int getcommonneighbors(string country, int idu, int idn){//calculate the commonneighbors between two users
	set<int> ids1=countrymap[country][idu];
	set<int> ids2=countrymap[country][idn];
	int count=0;
	for(int i:ids2){
		if(ids1.find(i)!=ids1.end()){
			count++;
		}
	}
	return count;
}
void getallcountries(char* buf){//ready for sending country list to main server
	int s=0;
	int len=0;
	char const* country;
	for(auto i:countrymap){
		country=i.first.c_str();
		len=strlen(country);
		strncpy(buf+s,country,len);
		s+=len;
		buf[s]=' ';
		s++;
	}
	buf[s]='\0';
}
string recommendation(string country, int id){//calculate the recommendation according to descriptions in pdf
	graph currgraph=countrymap[country];
	set<int> allconnects;// all users in the country
	set<int> connects=countrymap[country][id];// user id's networking
	set<int> notconnects;// the users who are not in the user id's networking
	for(auto key:currgraph){
		allconnects.insert(key.first);
	}
	connects.insert(id);
	for(int i:allconnects){
		if(connects.find(i)==connects.end()){
			notconnects.insert(i);
		}
	}
	//case1: id connects to all users
	if(notconnects.size()==0){
		return "None";
	}

	int maxcommon=-1;
	int returnid=-1;
	int maxdegree=-1;
	//case2.1:
	if(connects.size()==1){
		for(int i:notconnects){
			neighbors tmp=countrymap[country][i];
			if(maxdegree<(int)tmp.size()){
				maxdegree=(int)tmp.size();
				returnid=i;
			}
		}
		return to_string(returnid);
	}
	//case2.2:
	//case 1)
	int sum=0;
	for(int i:notconnects){
		sum+=getcommonneighbors(country,id,i);
		neighbors tmp=countrymap[country][i];
		if(maxdegree<(int)tmp.size()){
			maxdegree=(int)tmp.size();
			returnid=i;
		}
	}
	if(sum==0){
		return to_string(returnid);
	}
	maxcommon=-1;
	returnid=-1;
	maxdegree=-1;
	//case 2)
	for(int i:notconnects){
		int m=getcommonneighbors(country,id,i);
		if(maxcommon<m){
			maxcommon=m;
			returnid=i;
		}else if(maxcommon==m){
			if(i<returnid){
				returnid=i;
			}
		}
	}
	return to_string(returnid);
}
//refer to https://beej.us/guide/bgnet/examples/listener.c
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
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    string country;
    string id;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
		int option = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1) {
		    perror("setsockopt");
		    exit(1);
		}
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    

	cout<<"The server A is up and running using UDP on port "<<MYPORT<<endl;
	addr_len = sizeof their_addr;
	memset(buf,0,MAXBUFLEN);
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    constructgraph();
    memset(buf,0,MAXBUFLEN);
    getallcountries(buf);

    if ((numbytes = sendto(sockfd, buf, strlen(buf)-1, 0,
             (struct sockaddr *)&their_addr, addr_len)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    cout<<"The server A has sent a country list to Main Server"<<endl;
	while(1){
	    addr_len = sizeof their_addr;
	    memset(buf,0,MAXBUFLEN);
	    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
	        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
	        perror("recvfrom");
	        exit(1);
	    }
	    buf[numbytes]='\0';
	    getcountry(buf,country,id);
	    cout<<"The server A has received request for finding possible friends of User "<<id<<" in "<<country<<endl;
		if(countrymap[country].find(stoi(id))==countrymap[country].end()){
			cout<<"User "<<id<<" does not show up in "<<country<<endl;
			memset(buf,0,MAXBUFLEN);
			string reply="not found";
			strncpy(buf, reply.c_str(), reply.length());
			if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,
		             (struct sockaddr *)&their_addr, addr_len)) == -1) {
		        perror("talker: sendto");
		        exit(1);
		    }
			cout<<"The server has sent \"User "<<id<<" not found\" to Main Server"<<endl;
		}else{
			memset(buf,0,MAXBUFLEN);
			string resrec=recommendation(country,stoi(id));
			strncpy(buf, resrec.c_str(), resrec.length()+1);
			cout<<"The server A is searching possible friends for User "<<id<<" ..."<<endl;
			cout<<"Here are the results: User "<<buf<<endl;
			if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,
		             (struct sockaddr *)&their_addr, addr_len)) == -1) {
		        perror("talker: sendto");
		        exit(1);
		    }
			cout<<"The server A has sent the result(s) to Main Server"<<endl;
		}
	}
	freeaddrinfo(servinfo);
	close(sockfd);
    return 0;
}