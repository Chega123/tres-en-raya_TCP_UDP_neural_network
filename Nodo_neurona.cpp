
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include<iostream>
#include<thread>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>
#include <iterator>

#define MAXLINE 1000
using namespace std;

int Port_codigo;
struct sockaddr_in       servaddr;

struct Configuracion {
    int puerto;
    bool es_maestro;
    int puerto_maestro; 
};
//cosas globales
vector<Configuracion> configuraciones;
vector<pair<vector<int>, int>> training_data;
int seq_number=0;
vector<vector<double>> all_weights;
vector<double>avg_weights(40);


void leer_configuracion(const string &config) {
    cout<<config<<endl;
    cout<<"entre a funcion leer_config"<<endl;
    int pos = 0;
	configuraciones.clear();
    while (pos < config.size()) {
        Configuracion nodo_actual;
        nodo_actual.puerto = stoi(config.substr(pos, 5));
        cout<<nodo_actual.puerto<<endl;
        pos += 5;
        nodo_actual.es_maestro = (config[pos] == '1');
        pos += 1;
        configuraciones.push_back(nodo_actual);
        cout<<"nodo asignado en config"<<endl;
    }
    
    int puerto_maestro = -1;
    for (const auto& nodo : configuraciones) {
        if (nodo.es_maestro) {
            puerto_maestro = nodo.puerto;
            break;
        }
    }

    for (auto& nodo : configuraciones) {
        if (!nodo.es_maestro) {
            nodo.puerto_maestro = puerto_maestro;
        }
    }
}

int num_clientes = 0;    
int total_Cli = configuraciones.size();
vector<int>mapa_clientes;



void sendmsg(int sockfd,  sockaddr_in servaddr ,string& message){
        sendto(sockfd, message.c_str(), message.length(),MSG_CONFIRM, (const struct sockaddr *) &servaddr,sizeof(servaddr));
}
string padding(string message, char pad){
        if (message.length()<MAXLINE){
                return message+string(MAXLINE-message.length(),pad);
        }
        return message;
}



class Perceptron {
public:
    Perceptron(int input_size, int output_size);
    vector<double> predict(const vector<int>& inputs);
    void train(const vector<int>& inputs, int target, double learning_rate);
    vector<double> get_weights_and_biases() const;
    void set_weights_and_biases(const vector<double>& wb);
    void print_weights_and_biases() const;

private:
    vector<vector<double>> weights;
    vector<double> bias;
    vector<double> softmax(const vector<double>& x);
};

Perceptron::Perceptron(int input_size, int output_size) {
    weights.resize(output_size, vector<double>(input_size));
    for (auto& weight_vector : weights) {
        for (double& weight : weight_vector) {
            weight = ((double)rand() / (RAND_MAX)) * 2 - 1;
        }
    }
    bias.resize(output_size);
    for (double& b : bias) {
        b = ((double)rand() / (RAND_MAX)) * 2 - 1;
    }
}

vector<double> Perceptron::softmax(const vector<double>& x) {
    vector<double> exp_values(x.size());
    double max = *max_element(x.begin(), x.end());
    double sum = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        exp_values[i] = exp(x[i] - max);
        sum += exp_values[i];
    }
    for (double& value : exp_values) {
        value /= sum;
    }
    return exp_values;
}

vector<double> Perceptron::predict(const vector<int>& inputs) {
    vector<double> logits(bias.size());
    for (size_t j = 0; j < logits.size(); ++j) {
        logits[j] = bias[j];
        for (size_t i = 0; i < inputs.size(); ++i) {
            logits[j] += inputs[i] * weights[j][i];
        }
    }
    return softmax(logits);
}

void Perceptron::train(const vector<int>& inputs, int target, double learning_rate) {
    vector<double> prediction = predict(inputs);
    vector<double> target_vector(prediction.size(), 0.0);
    target_vector[target] = 1.0;

    vector<double> errors(prediction.size());
    for (size_t i = 0; i < prediction.size(); ++i) {
        errors[i] = target_vector[i] - prediction[i];
    }

    for (size_t j = 0; j < weights.size(); ++j) {
        for (size_t i = 0; i < weights[j].size(); ++i) {
            weights[j][i] += learning_rate * errors[j] * inputs[i];
        }
        bias[j] += learning_rate * errors[j];
    }
}

vector<double> Perceptron::get_weights_and_biases() const {
    vector<double> wb;
    for (const auto& weight_vector : weights) {
        wb.insert(wb.end(), weight_vector.begin(), weight_vector.end());
    }
    wb.insert(wb.end(), bias.begin(), bias.end());
    return wb;
}

void Perceptron::set_weights_and_biases(const vector<double>& wb) {
    size_t index = 0;
    for (auto& weight_vector : weights) {
        for (double& weight : weight_vector) {
            weight = wb[index++];
        }
    }
    for (double& b : bias) {
        b = wb[index++];
    }
}

void Perceptron::print_weights_and_biases() const {
    vector<double> wb = get_weights_and_biases();
    for (const auto& w : wb) {
        cout << w << " ";
    }
    cout << endl;
}

void convert_string_to_vector(const string& numeric_string) {

    size_t num_boards = 90; 

    for (size_t i = 0; i < num_boards; ++i) {
        vector<int> board;
        int result;
        for (size_t j = 0; j < 9; ++j) {
            board.push_back(numeric_string[i * 10 + j] - '0');
        }
        result = numeric_string[i * 10 + 9] - '0';
        training_data.push_back({board, result});
    }
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

string vector_to_string(const vector<double>& vec) {
    ostringstream oss;
    if (!vec.empty()) {
        copy(vec.begin(), vec.end() - 1, ostream_iterator<double>(oss, ","));
        oss << vec.back();
    }
    return oss.str();
}

Perceptron p(9, 4);

vector<double> string_to_vector_weights(string weight,int len){
    //cout<<"funcion de string a vector"<<weight<<endl;
    vector<double>pesosfun(40);
    string peso="";  
    for (int i=0;i<=len;i++){
        if (weight[i]==','){
            cout<<"version string"<<peso<<"version double:"<<stod(peso)<<endl;
            pesosfun.push_back(stod(peso));
            peso="";
            
        }
        else{
            peso+=weight[i];
        }
    }
    //cout<<"tamp de peso funcion: "<<pesosfun.size()<<endl;
    return pesosfun;
}


void funcion_RW_SCli(int S_Cliente){
    cout<<"solte thread"<<endl;
	char bufferw[MAXLINE];
	mapa_clientes.push_back(S_Cliente);
    for(;;){
      int length;
      bzero(bufferw,MAXLINE);
      read(S_Cliente, bufferw, 1); 
      if (bufferw[0] == 'w') { 
        cout<<"me llego un peso"<<endl;
        //cout<<"clientes que llegaron:"<<num_clientes<<" Clientes totales: "<<total_Cli<<endl;
        read(S_Cliente, bufferw, 3); 
        int weigth_len=atoi(bufferw);
        cout<<weigth_len<<endl;
        read(S_Cliente,bufferw,weigth_len);
        cout<<"la info que le llega"<<bufferw<<endl;
        string weights_string=bufferw+',';
        cout<<"pesos que me mandaron:  "<<weights_string<<endl<<endl<<endl;
        if (num_clientes < total_Cli) {
            all_weights.push_back(string_to_vector_weights(weights_string,weigth_len));
            cout<<"agregando"<<all_weights[1][1]<<endl;

            num_clientes++;
        }
        if(num_clientes==total_Cli){
            for(auto vector:all_weights){
                for(int i=0;i<40;i++){
                    avg_weights[i] +=vector[i];
                    //cout<<"numero "<<i<<"  "<<avg_weights[i]<<endl;
                }
                
            }
            for(auto pes:avg_weights){
                //cout<<"suma de peso"<<pes<<endl;
                pes/=total_Cli;
               // cout<<"promedio de un solo peso"<<pes<<endl;
            }
            cout<<"saque promedio"<<endl;
            for (auto cli:mapa_clientes){
                string pesos_string = vector_to_string(avg_weights);
                cout<<"pesos promedio: "<<pesos_string<<endl;
                int length_pe_string=pesos_string.length();
                string protocol="W";
                if(length_pe_string<100){protocol+="0";}
                if(length_pe_string<10){protocol+="0";}
                protocol+=to_string(length_pe_string)+pesos_string;
                cout<<"Estoy enviando sus promedios: "<<protocol<<endl;
                char* char_msg=new char[protocol.length()];
                strcpy(char_msg,protocol.c_str());
                write(cli,char_msg, protocol.length());
            }
      	}
      } 
    }  
}



void server_TCP_thread(){
 struct sockaddr_in stSockAddr;
  int S_Server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	cout<<"me toco ser el maestro"<<endl;
  int n;

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(Port_codigo);
  stSockAddr.sin_addr.s_addr = INADDR_ANY;
  bind(S_Server,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
  listen(S_Server, 10);

  for(;;)
  {
    int S_Cliente = accept(S_Server, NULL, NULL);
		thread (funcion_RW_SCli,S_Cliente).detach();
  }
	close(S_Server);
}

void Client_TCP(int Puerto,string mensaje){
    struct sockaddr_in stSockAddr;
    cout<<"dentro de cliente TCP"<<endl;
    int Res;
    int SocketCliente = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));  
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(Puerto);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);
    connect(SocketCliente, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
    char buffer[MAXLINE];
    char* char_protocol=new char[mensaje.length()+1];
    strcpy(char_protocol,mensaje.c_str());
    write(SocketCliente,char_protocol,mensaje.length()+1);
    cout<<"mande mis pesos: "<<mensaje<<endl;
		char buffer2[255] = {};
		for (;;) {  
			int n = read(SocketCliente, buffer, 1);
			if (n > 0) {
					if  (buffer[0]=='W'){
						read(SocketCliente, buffer2, 3);
						int len_msg=atoi(buffer);
						read(SocketCliente, buffer2, len_msg);
						string pesos_nuevos=buffer2;
                        cout<<"pesos nuevos: "<<pesos_nuevos<<endl;
						p.set_weights_and_biases(string_to_vector_weights(pesos_nuevos,len_msg));
    				    shutdown(SocketCliente,SHUT_RDWR);
					}
			}
		}
}

void lectura(int sockfd, sockaddr_in servaddr){
  int len=sizeof(servaddr);
  char buffer[MAXLINE];

  int last_seq = 0; // Número de secuencia esperado
  map<int, string> missing_packets;
  while(1){
    int n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &servaddr,(socklen_t*)&len);
    int pos=0;
    if(n>0){
      string msg=buffer;
      char tipo=msg[0];
      pos+=1;
      if (tipo == 'T') {
        int pos = 1;
        int secuencianumber = stoi(msg.substr(pos, 2));
        pos += 2;
        string datos = msg.substr(pos, 900);
        pos += 900;
        int check = sum_digits(datos) % 256;
        int checksum = stoi(msg.substr(pos, 3));
        pos += 3;
        if (check == checksum) {
            if (secuencianumber == last_seq + 1) {

                convert_string_to_vector(datos);
                cout<<"me llego uno de los 90 tableros"<<endl;
                last_seq = secuencianumber;
                if (!missing_packets.empty()) {
                    auto it = missing_packets.begin();
                    while (it != missing_packets.end()) {
                        if (it->first <= last_seq) {
                            missing_packets.erase(it++);
                        } else {
                            break;
                        }
                    }
                }
            } else {
                missing_packets[secuencianumber] = datos;
                string protocolo = "s" + to_string(last_seq + 1);
                string solicitud=padding(protocolo,'#');
                sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            }
        } else {
            string protocolo = "s" + to_string(secuencianumber);
            string solicitud=padding(protocolo,'#');
            sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        }
      }
        if (tipo == 'E'){
            double learning_rate = 0.1;
            cout<<"comenzamos a entrenar"<<endl;
            for (int epoch = 0; epoch < 100; ++epoch) {
                for (const auto& data : training_data) {
                p.train(data.first, data.second, learning_rate);
                }
            }  
            cout<<"terminamos a entrenar"<<endl;
            vector<double>pesos_actuales=p.get_weights_and_biases();
            string pesos_protocolo=vector_to_string(pesos_actuales);
            int length_string=pesos_protocolo.length();
            string protocol="w";
            if(length_string<100){protocol+="0";}
            if(length_string<10){protocol+="0";}
            protocol+=to_string(length_string)+pesos_protocolo;
            
            
            //cout<<"Puerto maestro: "<<configuraciones.begin()->puerto_maestro<<endl;
            if (Port_codigo!=configuraciones.begin()->puerto_maestro){
                cout<<"llego a lanzar cliente?"<<endl;
                Client_TCP(configuraciones.begin()->puerto_maestro, protocol);}
            else{
                all_weights.push_back(p.get_weights_and_biases());
            }
        }
        if(tipo == 'F'){
            int tam_file = stoi(msg.substr(pos, 3));
            pos+=3;
            string file_conf = msg.substr(pos, tam_file);
            leer_configuracion(file_conf);

            cout<<"puerto maestrito :D "<<configuraciones.begin()->puerto_maestro<<endl;

            if(configuraciones.begin()->puerto_maestro==Port_codigo){
                thread(server_TCP_thread).detach();
            }
            //cout<<"recibi file config"<<endl;
            total_Cli = configuraciones.size();
            //cout<<"clientes totales: "<<total_Cli<<endl;
            cout<<Port_codigo<<endl;
        }
        if(tipo == 'K'){
            string protocol_keep="k"+to_string(Port_codigo);
            string solicitud=padding(protocol_keep,'#');
            sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
			}
    }
  }
}






int main(int argc, char *argv[]) {

	Port_codigo=std::atoi(argv[1]);
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
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8080);
	servaddr.sin_addr = *((struct in_addr *)host->h_addr);
	string mensaje_puerto="P"+to_string(Port_codigo);
    
	string solicitud=padding(mensaje_puerto,'#');
    sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	int n, len;
	thread(lectura,sockfd,servaddr).detach();
	while (1)
	{


	}
	close(sockfd);
	return 0;
}
