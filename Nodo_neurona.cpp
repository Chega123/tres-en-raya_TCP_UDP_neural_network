
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
#include<mutex>
#include <random>
#include <chrono>

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
int num_cliente=0;
mutex mtx; 
bool server_existente=false;
void leer_configuracion(const string &config) {
    int pos = 0;
	configuraciones.clear();
    while (pos < config.size()) {
        Configuracion nodo_actual;
        nodo_actual.puerto = stoi(config.substr(pos, 5));
        pos += 5;
        int maestro = stoi(config.substr(pos, 1));
        if (maestro == 1){
            nodo_actual.es_maestro =true;
        }
        else{nodo_actual.es_maestro =false;}
        pos += 1;
        configuraciones.push_back(nodo_actual);
    }
    
    int puerto_maestro = -1;
    for (const auto& nodo : configuraciones) {
        if (nodo.es_maestro) {
            puerto_maestro = nodo.puerto;
            break;
        }
    }
    cout<<"puerto maestro: "<<puerto_maestro<<endl;
    for (auto& nodo : configuraciones) {
        nodo.puerto_maestro = puerto_maestro;
    }
}

  
int total_Cli = configuraciones.size();
vector<int>mapa_clientes;
bool terminar=false;


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
    vector<double>pesosfun;
    int contador=0;
    string peso="";  
    for (int i=0;i<=len;i++){
        if (weight[i]==','){
           // cout<<"contador: "<<contador<<"  version string"<<peso<<"version double:"<<stod(peso)<<endl;
            pesosfun.push_back(stod(peso));
            peso="";
            contador++;
        }
        else{
            peso+=weight[i];
        }
    }
    //cout<<"tamp de peso funcion: "<<pesosfun.size()<<endl;
    return pesosfun;
}



void funcion_RW_SCli(int S_Cliente){  
    //cout<<"solte thread"<<endl;
	char bufferw[MAXLINE];
	mapa_clientes.push_back(S_Cliente);
    bool termino=false;
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
            //cout<<"la info que le llega"<<bufferw<<endl;
            string weigth_cadena=bufferw;
            weigth_cadena+=",";
            //cout<<"pesos que me mandaron:  "<<weigth_cadena<<endl<<endl<<endl;
            total_Cli = configuraciones.size();

            if (num_cliente < total_Cli) {
                all_weights.push_back(string_to_vector_weights(weigth_cadena,weigth_len+1));
                mtx.lock();
                num_cliente++;
                termino=true;
                mtx.unlock();
            }
            if(num_cliente==total_Cli){
                cout<<"entro a esta cosa"<<endl;
                for(auto vector:all_weights){
                    for(int i=0;i<40;i++){
                        avg_weights[i] +=vector[i];
                    //  cout<<"numero "<<i<<"  "<<avg_weights[i]<<endl;
                    }
                    
                }
                cout<<total_Cli<<endl;
                mtx.lock();
                for (int j=0;j<40;j++){
                    avg_weights[j]=avg_weights[j]/total_Cli;
                }
                mtx.unlock();
                cout<<"saque promedio"<<endl;
                string pesos_string = vector_to_string(avg_weights);
                int length_pe_string=pesos_string.length();
                string protocol="W";
                if(length_pe_string<100){protocol+="0";}
                if(length_pe_string<10){protocol+="0";}
                protocol+=to_string(length_pe_string)+pesos_string;
                cout<<"Estoy enviando sus promedios: "<<protocol<<endl;
                bzero(bufferw,MAXLINE);
                for (auto cli:mapa_clientes){
                    //cout<<"pesos promedio: "<<pesos_string<<endl;
                    char* char_msg=new char[protocol.length()];
                    strcpy(char_msg,protocol.c_str());
                    write(cli,char_msg, protocol.length());
                }
                terminar=true;
                break;
            }
        } 
        cout<<termino<<endl;
        if(termino==true){
            cout<<"no entro D:"<<endl;

            break;
        }

    } 
    cout<<"saliste thread"<<endl; 
}



void server_TCP_thread(){
 struct sockaddr_in stSockAddr;
  int S_Server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	cout<<"me toco ser el maestro"<<endl;
  int n;
  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(Port_codigo);
  cout<<"Soy el server y este es mi puerto: "<<Port_codigo<<endl;
  stSockAddr.sin_addr.s_addr = INADDR_ANY;
  bind(S_Server,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
  listen(S_Server, 10);
  int num=0;
  for(;;)
  {
    if (terminar==true){ 
        mapa_clientes.clear();
        all_weights.clear();
        num_cliente=0;
        total_Cli=configuraciones.size();
        num=0;
        mtx.lock();
        terminar=false;
        mtx.unlock();
    }
    if(num<total_Cli){
        //cout<<"conecte algo"<<endl;
        int S_Cliente = accept(S_Server, NULL, NULL);

        cout<<"se conecto alguien"<<endl;
        num++;
        thread (funcion_RW_SCli,S_Cliente).detach();
        //cout<<terminar<<endl;
        }
  }
	
}

void Client_TCP(int Puerto,string mensaje){
    struct sockaddr_in stSockAddr;
    cout<<"dentro de cliente TCP"<<endl;
    int Res;
    int SocketCliente = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));  
    stSockAddr.sin_family = AF_INET;
    cout<<"soy el  cliente y me estoy conectando a: "<<Puerto<<endl;
    stSockAddr.sin_port = htons(Puerto);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);
    connect(SocketCliente, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
    char buffer[MAXLINE];
    cout<<"mi protocolo de pesos"<<mensaje<<endl;
    cout<<"puerto al que me conecto" <<Puerto<<endl;
    char* char_protocol=new char[mensaje.length()+1];
    strcpy(char_protocol,mensaje.c_str());
    write(SocketCliente,char_protocol,mensaje.length()+1);
    bzero(buffer,MAXLINE);
    for (;;) {  
        int n = read(SocketCliente, buffer, 1);
        if (n > 0) {
            if  (buffer[0]=='W'){
                read(SocketCliente, buffer, 3);
                int len_msg=atoi(buffer);
                //cout<<"tamaño de la data de pesos_"<<len_msg<<endl;
                read(SocketCliente, buffer, len_msg);
                //cout<<"pesos que me llegaron: "<<buffer<<endl<<endl;
                string pesos_nuevos=buffer;
                //cout<<"pesos nuevos: "<<pesos_nuevos<<endl;
                p.set_weights_and_biases(string_to_vector_weights(pesos_nuevos,len_msg));
                cout<<"puse los pesos nuevos"<<endl;
                cout<<"desconecte cliente"<<endl;
                //shutdown(SocketCliente,SHUT_RDWR);
                //close(SocketCliente);
                break;
            }
        }  
        //cout<<"sigo vivo"<<endl; 
    }
}



void agregarJugadasPosibles( string tablero, vector<vector<int>>& tableros_opciones) {
    for (int i = 0; i < tablero.size(); ++i) {
        if (tablero[i] == '0') {
            vector<int> jugada(tablero.size());
            for (int j = 0; j < tablero.size(); ++j) {
                jugada[j] = tablero[j] - '0'; 
            }
            jugada[i] = 1;
            tableros_opciones.push_back(jugada);
        }
    }
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    shuffle(tableros_opciones.begin(), tableros_opciones.end(), default_random_engine(seed));
}
int max_ind(vector<int>vector){
    int max_index=0;
    for(size_t i=1;i<vector.size();++i){
        if(vector[i]>vector[max_index]){
            max_index=i;
        }
    }
    return max_index;
}

void lectura(int sockfd, sockaddr_in servaddr){
  int len=sizeof(servaddr);
  int last_seq = 0; // Número de secuencia esperado
  map<int, string> missing_packets;

  while(1){
    char buffer[MAXLINE];
    int n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &servaddr,(socklen_t*)&len);
    int pos=0;
    if(n>0){
      string msg=buffer;
      //cout<<msg<<endl;
      //cout<<"Existe un server"<<server_existente<<endl;
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
                bzero(buffer,MAXLINE);
            } else {
                missing_packets[secuencianumber] = datos;
                string protocolo = "s" + to_string(last_seq + 1);
                string solicitud=padding(protocolo,'#');
                sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                bzero(buffer,MAXLINE);
            }
        } else {
            string protocolo = "s" + to_string(secuencianumber);
            string solicitud=padding(protocolo,'#');
            sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            bzero(buffer,MAXLINE);
        }
      }
        if (tipo == 'E'){

            double learning_rate = 0.1;
            for (int epoch = 0; epoch < 100; ++epoch) {
                //cout<<"entrenando epoca: "<<epoch<<endl;
                for (const auto& data : training_data) {
                p.train(data.first, data.second, learning_rate);
                }
            }  
            vector<double>pesos_actuales=p.get_weights_and_biases();
            string pesos_protocolo=vector_to_string(pesos_actuales);
            int length_string=pesos_protocolo.length();
            string protocol="w";
            if(length_string<100){protocol+="0";}
            if(length_string<10){protocol+="0";}
            protocol+=to_string(length_string)+pesos_protocolo;
            thread(Client_TCP,configuraciones.begin()->puerto_maestro, protocol).detach();
            bzero(buffer,MAXLINE);
        }
        if(tipo == 'F'){
            int tam_file = stoi(msg.substr(pos, 3));
            pos+=3;
            string file_conf = msg.substr(pos, tam_file);
            leer_configuracion(file_conf);
            //cout<<"lista de puertos "<<endl
            cout<<"puerto maestrito :D "<<configuraciones.begin()->puerto_maestro<<endl;

            //cout<<"recibi file config"<<endl;
            total_Cli = configuraciones.size();
            //cout<<"clientes totales: "<<total_Cli<<endl;
            //cout<<Port_codigo<<endl;
            bzero(buffer,MAXLINE);
             if(configuraciones.begin()->puerto_maestro==Port_codigo){
                cout<<"cree server  "<<endl;
                thread(server_TCP_thread).detach();
            }
            

        }
        if(tipo == 'K'){
            string protocol_keep="k"+to_string(Port_codigo);
            string solicitud=padding(protocol_keep,'#');
            //cout<<"mande keep alive confirmacion"<<endl;
            bzero(buffer,MAXLINE);
            sendto(sockfd, solicitud.c_str(), solicitud.length(), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
			}
        if(tipo=='G'){
            vector<vector<int>> tableros_opciones;
            string tablero_actual=msg.substr(pos, 9);
            pos+=9;
            string id=msg.substr(pos, 1);
            pos+=1;
            cout<<tablero_actual<<endl;
            agregarJugadasPosibles(tablero_actual, tableros_opciones);
            vector<int>resultados(tableros_opciones.size());
            int contador=0;
            for(auto it_tableros:tableros_opciones){
                vector<double> result = p.predict(it_tableros);
                cout<<"Hay algo en lo de results? "<<result[0]<<endl;
                int predicted_class = distance(result.begin(), max_element(result.begin(), result.end()));
                cout<<"prediccion "<<predicted_class<<endl;
                resultados[contador]=predicted_class;
                contador++;
            }
            cout<<"calcule jugada"<<endl;
            int eleccion=max_ind(resultados);
            cout<<resultados[1]<<endl;
            string tablero_mandar;
            for(int i=0;i<9;i++ ){
                cout<<"tablero ganador partes"<<tableros_opciones[eleccion][i]<<endl;
                tablero_mandar+=to_string(tableros_opciones[eleccion][i]);
            }

            cout<<"mandare este tablero: "<<tablero_mandar<<endl;
            string proto="A"+tablero_mandar+id;
            string solicitud=padding(proto,'#');
            cout<<solicitud<<endl;
            bzero(buffer,MAXLINE);
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
