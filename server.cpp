// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <map>
#include <thread>
#include <vector>
#include <utility>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <cstring>
#include <sstream>

#define PORT     8080
#define MAXLINE 1000

using namespace std;

struct sockaddr_in servaddr, cliaddr;
vector<pair<int,sockaddr_in>>nodos_neurona;
vector<pair<int,bool>>keep_alive;
vector<pair<int,sockaddr_in>>Clientes;
string numeric_string;
map<int, vector<string>> paquetes_enviados;
vector<pair<sockaddr_in,string>>tableros;

string padding(string message, char pad){
        if (message.length()<MAXLINE){
                return message+string(MAXLINE-message.length(),pad);
        }
        return message;
}

int verificarEstadoJuego(const string& tablero) {
    for (int i = 0; i < 3; ++i) {
        if (tablero[i*3] == tablero[i*3 + 1] && tablero[i*3 + 1] == tablero[i*3 + 2]) {
            if (tablero[i*3] == '1') return 1; 
            if (tablero[i*3] == '2') return 2; 
        }
    }

    for (int i = 0; i < 3; ++i) {
        if (tablero[i] == tablero[i + 3] && tablero[i + 3] == tablero[i + 6]) {
            if (tablero[i] == '1') return 1; 
            if (tablero[i] == '2') return 2; 
        }
    }
    if (tablero[0] == tablero[4] && tablero[4] == tablero[8]) {
        if (tablero[0] == '1') return 1; 
        if (tablero[0] == '2') return 2; 
    }
    if (tablero[2] == tablero[4] && tablero[4] == tablero[6]) {
        if (tablero[2] == '1') return 1; 
        if (tablero[2] == '2') return 2; 
    }

    bool empate = true;
    for (char c : tablero) {
        if (c == '0') {
            empate = false;
            break;
        }
    }
    if (empate) return 3; // Es un empate

    // El juego está en curso
    return 0;
}


string convert_json_to_numeric_string(const string& json_content) {
    stringstream ss(json_content);
    string numeric_string;

    char c;
    while (ss.get(c)) {
        if (isdigit(c)) {
            numeric_string += c;
        }
    }

    return numeric_string;
}

int sum_digits(const string &numero_string) {
    int suma = 0;

    for (char c : numero_string) {
        if (c >= '0' && c <= '9') { 
            suma += c - '0'; 
        }
    }

    return suma;
}

int contador=0;
void guardar_nodos_neurona(const vector<pair<int, sockaddr_in>>& nodos_neurona, const string& filename) {
    srand(time(0));
    int num_nodos = nodos_neurona.size();
    int indice_aleatorio = rand() % num_nodos;  // Selecciona un índice aleatorio para el 1

    ofstream archivo(filename, ios::trunc);  // Abrir en modo truncar para eliminar contenido previo
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo para escribir." << endl;
        return;
    }
    for (int i = 0; i < num_nodos; ++i) {
        int puerto = nodos_neurona[i].first;
        int booleano = (i == indice_aleatorio) ? 1 : 0;
        archivo << puerto << booleano << "\n";
    }
    archivo.close();
}

string leer_nodos_neurona(const string& filename) {
    ifstream archivo(filename);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo para leer." << endl;
        return "";
    }
    string resultado;
    string linea;
    while (getline(archivo, linea)) {
        resultado += linea;
    }
    archivo.close();
    return resultado;
}


void sendmsg(int sockfd,  sockaddr_in cliaddr ,string& message){
        sendto(sockfd, message.c_str(), message.length(),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,sizeof(cliaddr));
}

void procesar(int sockfd, sockaddr_in Cli_addr, string message ){
  int pos=0;
  string answer;
  char tipo=message[0];
  pos+=1;
  if(tipo=='P'){
    int port=stoi(message.substr(pos,5));
    pos+5;
    nodos_neurona.push_back(make_pair(port,Cli_addr));
    paquetes_enviados[port] = vector<string>();
	keep_alive.push_back(make_pair(port,true));
	guardar_nodos_neurona(nodos_neurona, "fileconfig.txt");
	cout<<"Neurona entro"<<endl;
  }
  if(tipo=='U'){
	cout<<"cliente entro"<<endl;
    Clientes.push_back(make_pair(contador,Cli_addr));
    contador++;
  }
  if (tipo=='e'){
	string protocolo="E";
	answer=padding(protocolo,'#');
    cout<<answer<<endl;
	for(auto it:nodos_neurona){
		sendmsg(sockfd,it.second,answer);
	}
	cout<<"estan entrenando"<<endl;
  }
    if (tipo=='j'){
        string tablero=message.substr(pos,9);
        tableros.push_back(make_pair(Cli_addr,tablero));
        string protocoloA="A"+tablero;
        string answerA=padding(protocoloA,'#');
        string protocolo="J";
        answer=padding(protocolo,'#');
        cout<<answer<<endl;
        sendmsg(sockfd,Cli_addr,answer);
        sendmsg(sockfd,Cli_addr,answerA);
    }
    if (tipo=='g'){
        string partida=message.substr(pos,9);
        int id_cliente;
        for(auto it = Clientes.begin(); it != Clientes.end(); ++it) {
            if (memcmp(&it->second, &Cli_addr, sizeof(Cli_addr)) == 0) {
                    id_cliente=it->first;                
            }                
        }
        string id_cliente_str=to_string(id_cliente);
        string protocolo="G"+partida+id_cliente_str;
        answer=padding(protocolo,'#');
        cout<<answer<<endl;   
        sendmsg(sockfd,nodos_neurona[0].second,answer);
    }

    if(tipo=='A'){
        string partida=message.substr(pos,9);
        pos+=9;
        int id=stoi(message.substr(pos,1));
        sockaddr_in jugador;
        for(auto it_jugador:Clientes){
            if(id==it_jugador.first){
                jugador=it_jugador.second;
            }
        } 
        for(auto it = tableros.begin(); it != tableros.end(); ++it) {
            if (memcmp(&it->first, &jugador, sizeof(jugador)) == 0) {
                it->second=partida;
            }                
        }

        int resultado=verificarEstadoJuego(partida);
        if(resultado==3){
            string res="R6Empate";
            string answerR=padding(res,'#');
            cout<<answerR<<endl; 
            sendmsg(sockfd,jugador,answerR);
        }
        else if(resultado==2){
            string res="R7Ganaste";
            string answerR=padding(res,'#');
            cout<<answerR<<endl; 
            sendmsg(sockfd,jugador,answerR);
        }
        else if(resultado==1){
            string res="R7Derrota";
            string answerR=padding(res,'#');
            cout<<answerR<<endl; 
            sendmsg(sockfd,jugador,answerR);
        }
        else{

        }
        string protocolo="a"+partida;
        answer=padding(protocolo,'#');
        cout<<answer<<endl; 
        sendmsg(sockfd,jugador,answer);
    }

    if(tipo=='t'){
        int posdata=0;
        for(auto it:nodos_neurona){
            int port = it.first;
            for (int i=0;i<10;i++){
                string protocol="T";
                if (i < 10) protocol += "0";
                protocol += to_string(i);
                string data=numeric_string.substr(posdata,900);
                posdata+=900;
                protocol+=data;
                int checksum=sum_digits(data)%256;
                if(checksum<100){protocol+="0";}
                if(checksum<10){protocol+="0";}
                protocol+=to_string(checksum);
                cout<<protocol<<endl<<endl;
                answer=padding(protocol,'#');
                
                paquetes_enviados[port].push_back(answer);
                sendmsg(sockfd,it.second,answer);
            }
            cout<<"ya le envie a uno"<<endl;
        }
        cout<<"envie todos"<<endl;

        string protoFile="F";
        string File_content= leer_nodos_neurona("fileconfig.txt");
        int tamFile=File_content.length();
        if (tamFile < 100) protoFile += "0";
        if (tamFile < 10) protoFile += "0";
        protoFile+=to_string(tamFile);
        protoFile+=File_content;
        string ansfile=padding(protoFile,'#');
        cout<<"protocoloFile: "<<ansfile<<endl;
        for(auto it:nodos_neurona){
            sendmsg(sockfd,it.second,ansfile);
            cout<<"envie file config"<<endl;
        }

  }
    if (tipo == 's') {
        int requested_seq = stoi(message.substr(pos, 2));
        int port = -1;
        for (const auto& nodo : nodos_neurona) {
            if (nodo.second.sin_port == Cli_addr.sin_port) {
                port = nodo.first;
                break;
            }
        }
        if (port != -1 && paquetes_enviados.count(port) > 0 && paquetes_enviados[port].size() > requested_seq) {
            string packet = paquetes_enviados[port][requested_seq];
            sendmsg(sockfd, Cli_addr, packet);
        }
    }
	if (tipo == 'k') {
        int port = stoi(message.substr(pos, 5));
        for (auto& ka : keep_alive) {
            if (ka.first == port) {
                ka.second = true;
                break;
            }
        }
    }
}

void keep_alive_check(int sockfd) {
    while (true) {
        this_thread::sleep_for(chrono::seconds(20));
        for (auto& ka : keep_alive) {
            ka.second = false;
        }

        for (const auto& nodo : nodos_neurona) {
            string keep_msg = padding("K", '#');

           // cout<<keep_msg<<endl;
            sendmsg(sockfd, nodo.second, keep_msg);
        }
        this_thread::sleep_for(chrono::seconds(10));

        auto it_ka = keep_alive.begin();
        auto it_nn = nodos_neurona.begin();
        while (it_ka != keep_alive.end() && it_nn != nodos_neurona.end()) {
            if (!it_ka->second) {
                it_nn = nodos_neurona.erase(it_nn);
                it_ka = keep_alive.erase(it_ka);
                guardar_nodos_neurona(nodos_neurona, "fileconfig.txt");
                string protoFile="F";
                string File_content= leer_nodos_neurona("fileconfig.txt");
                int tamFile=File_content.length();
                if (tamFile < 100) protoFile += "0";
                if (tamFile < 10) protoFile += "0";
                protoFile+=to_string(tamFile);
                protoFile+=File_content;
                string ansfile=padding(protoFile,'#');
                //cout<<"protocoloFile: "<<ansfile<<endl;
                for(auto it:nodos_neurona){
                sendmsg(sockfd,it.second,ansfile);
                cout<<"envie file config"<<endl;
                }
            } else {
                ++it_ka;
                ++it_nn;
            }
        }

    }
}




// Driver code
int main() {
	int sockfd;
	char buffer[MAXLINE];

	ifstream input_file("tic_tac_toe_dataset.json");
	if (!input_file.is_open()) {
		cerr << "No se pudo abrir el archivo JSON." << endl;
		return 1;
	}

	stringstream bufferjson;
	bufferjson << input_file.rdbuf();
	string json_content = bufferjson.str();


	numeric_string = convert_json_to_numeric_string(json_content);

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
			perror("socket creation failed");
			exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
					sizeof(servaddr)) < 0 )
	{
			perror("bind failed");
			exit(EXIT_FAILURE);
	}

	int len, n;
	thread(keep_alive_check, sockfd).detach();
	len = sizeof(cliaddr); //len is value/resuslt

	while(1){
		n = recvfrom(sockfd, (char *)buffer, MAXLINE,
								MSG_WAITALL, ( struct sockaddr *) &cliaddr,
								(socklen_t*)&len);
		string mensaje=buffer;                    
		if(n>0){thread(procesar,sockfd,cliaddr,mensaje).detach();}
		bzero(buffer,MAXLINE);
	}

	return 0;
}

