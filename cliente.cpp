// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include<iostream>
#include<thread>

using namespace std;
#define PORT    8080
#define MAXLINE 1000

struct sockaddr_in       servaddr;
string tablero="000000000";
void sendmsg(int sockfd,  sockaddr_in servaddr ,string& message){
        sendto(sockfd, message.c_str(), message.length(),MSG_CONFIRM, (const struct sockaddr *) &servaddr,sizeof(servaddr));
}
string padding(string message, char pad){
        if (message.length()<MAXLINE){
                return message+string(MAXLINE-message.length(),pad);
        }
        return message;
}
void procesar(int sockfd,sockaddr_in servaddr,string message){
    string msg;
    if((strcmp(message.c_str(),"entrena"))==0){
        string protocol="e";
        msg=padding(protocol,'#');
        sendmsg(sockfd,servaddr,msg);
    }
    if((strcmp(message.c_str(),"tableros"))==0){
        string protocol="t";
        msg=padding(protocol,'#');
        sendmsg(sockfd,servaddr,msg);
    }
    if((strcmp(message.c_str(),"jugar"))==0){
        tablero="000000000";
        string protocol="j"+tablero;
        msg=padding(protocol,'#');
        sendmsg(sockfd,servaddr,msg);
    }
    if((strcmp(message.c_str(),"pos"))==0){
        string posicion;
        getline(cin,posicion);
        if(stoi(posicion)<9){
            tablero[stoi(posicion)]='2';
            string protocol="g"+tablero;
            msg=padding(protocol,'#');
            sendmsg(sockfd,servaddr,msg);
        }
    }
}

void imprimirTablero(string tablero) {
    for (int i = 0; i < 9; i += 3) {
        // Imprimir cada fila del tablero
        for (int j = 0; j < 3; ++j) {
            char c = tablero[i + j];
            if (c == '0') {
                cout << "| ";
            } else if (c == '1') {
                cout << "|X";
            } else if (c == '2') {
                cout << "|O";
            }
        }
        cout << "|" << endl;
    }
}

void lectura(int sockfd, sockaddr_in servaddr){
    int len=sizeof(servaddr);
    char buffer[MAXLINE];
    while(1){
        int n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &servaddr,(socklen_t*)&len);
        int pos=0;
        if(n>0){
            string msg=buffer;
            char tipo=msg[0];
            pos+=1;
            if(tipo=='E'){
                int error_length=stoi(msg.substr(pos,2));
                pos+=2;
                string error=msg.substr(pos,error_length);
                pos+=error_length;
                cout<<error<<endl;
            }
            if(tipo=='J'){
                cout<< "iniciaste partida pon posicion  (pos y el numero)"<<endl;
                cout<<"|0|1|2|"<<endl<<"|3|4|5|"<<endl<<"|6|7|8|"<<endl;
            }
            if(tipo=='a'){
                tablero=msg.substr(pos,9);
                imprimirTablero(tablero);  
            }
            if(tipo=='R'){
                int tam=stoi(msg.substr(pos,1));
                pos+=1;
                string palabra= msg.substr(pos,tam);
                cout<<palabra<<endl; 
            }
        }
    bzero(buffer,MAXLINE);
    }
}

// Driver code
int main() {
        int sockfd;
        char buffer[MAXLINE];
        struct hostent *host;

        

        host = (struct hostent *)gethostbyname((char *)"127.0.0.1");


        // Creating socket file descriptor
        if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                perror("socket creation failed");
                exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        // Filling server information
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(PORT);
        servaddr.sin_addr = *((struct in_addr *)host->h_addr);

        int n, len;
        thread(lectura,sockfd,servaddr).detach();
        string protocolU="U";
        string msgU=padding(protocolU,'#');
        sendmsg(sockfd,servaddr,msgU);
while (1)
   {

        cin.getline(buffer,MAXLINE);
        procesar(sockfd,servaddr,buffer);


        
        bzero(buffer,MAXLINE);


}
        close(sockfd);
        return 0;
}

