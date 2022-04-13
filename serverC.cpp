/**
 * Reused code:
 * Chapter5 of Beej's Code: http://www.beej.us/guide/bgnet/ 
 * 1. create/bind UDP socket
 * 2. send/receive UDP message
 */
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

#include <iostream> 
#include <sstream>
#include <fstream>
#include <vector>
#include <string> 
#include <unordered_map>
#include <unordered_set>
using namespace std;

#define FAIL -1
#define MAX_DATA_SIZE 1024
#define SERVER_HOST "127.0.0.1"
#define CLIENT_HOST "127.0.0.1"
#define SERVER_PORT 23800
#define CLIENT_PORT 24800
#define SERVER_NAME "ServerC"
#define FILE_NAME "block3.txt"

vector<string> parse_string(string str, char c);

/**
 * Data object for storing one transaction record
 */ 
class Data {
    public:
        int serial_num;
        string sender;
        string receiver;
        int amount;
        string my_str;

        Data() {} 
        Data(string str) {
            my_str = str;
            vector<string> v;
            v = parse_string(str, ' ');
            serial_num = stoi(v[0]);
            sender = v[1];
            receiver = v[2];
            amount = stoi(v[3]);
        }
};

/**
 * print error message and exit when the socket related function is failed
 */
void socket_error_checking(int status, string str) {
    const char *error_msg = str.c_str();
    if (status == FAIL) {
        perror(error_msg);
        exit(1);
    }
}

/**
 * create UDP server socket
 */
int create_udp_server_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    socket_error_checking(sockfd, "UDP socket");
    return sockfd; 
}

/**
 * store the UDP server socket address information into the struct sockaddr_in * data type(include IPv4, server IP address and server port number)
 */
struct sockaddr_in set_server_addr() {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_HOST); 
    server_addr.sin_port = htons(SERVER_PORT);
    return server_addr;
}

/**
 * store the UDP client socket address information into the struct sockaddr_in * data type(include IPv4, client IP address and client port number)
 */
struct sockaddr_in set_client_addr() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr)); 
    client_addr.sin_family = AF_INET; 
    client_addr.sin_addr.s_addr = inet_addr(CLIENT_HOST); 
    client_addr.sin_port = htons(CLIENT_PORT); 
    return client_addr;
}

/**
 * UDP bind server socket
 */
void udp_bind(int sockfd, struct sockaddr_in server_addr) {
    int res = ::bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    socket_error_checking(res, "UDP bind");
}

/**
 * UDP respond to the client
 */
void sendto_client(int client_sockfd, struct sockaddr_in client_addr, string msg) {
    char send_buf[MAX_DATA_SIZE];
    strcpy(send_buf, msg.c_str());
    int res = sendto(client_sockfd, send_buf, sizeof(send_buf), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
    socket_error_checking(res, "UDP sendto");
    cout << "The " << SERVER_NAME << " finished sending the response to the Main Server." << endl;
}

/**
 * UDP receive message from client and return the received message
 */
string receive_msg(int server_sockfd, struct sockaddr_in client_addr) {
    char recv_buf[MAX_DATA_SIZE];
    socklen_t client_addr_size = sizeof(client_addr);
    int res = recvfrom(server_sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *) &client_addr, &client_addr_size);
    socket_error_checking(res, "UDP recvfrom");
    cout << "The " << SERVER_NAME << " received a request from the Main Server." << endl;
    string recv_msg = recv_buf;
    return recv_msg;
}

/**
 * split a string with character c
 */ 
vector<string> parse_string(string str, char c) {
    vector<string> v;
    stringstream ss(str);
    while (ss.good()) {
        string substr;
        getline(ss, substr, c);
        if (substr != "") {
            v.push_back(substr);
        }
    }
    return v;
}   

/**
 * read the transaction record from log file and stored in the Data object
 */
void read_file(vector<Data> *data_list) {
    string line;
    fstream myfile(FILE_NAME);
    if(myfile.is_open()) {
        while (getline(myfile, line)) {
            if (line == "") {
                continue;
            }
            Data data(line);
            data_list->push_back(data);
        }
        myfile.close();
    } else {
        cout << "Can not open log file" << endl;
        system("pause");
    }
}

/**
 * write the transaction record into log file
 */
void write_file(string new_transaction) {
    ofstream myfile;
    myfile.open(FILE_NAME, ios::app);
    myfile << new_transaction << endl;
    myfile.close();
}

/**
 * generate an unordered_map to map username to all corresponding transaction data
 */ 
void gen_username_map(vector<Data> *data_list, unordered_map<string, vector<Data> > *map) {
    for (auto& data : *data_list) {
        map->insert(make_pair(data.sender, vector<Data>()));
        map->insert(make_pair(data.receiver, vector<Data>()));
        map->at(data.sender).push_back(data);
        map->at(data.receiver).push_back(data);
    }
}

/**
 * update the username map and transaction list by adding new transaction
 */ 
void insert_new_data(vector<Data> *data_list, string new_transaction, unordered_map<string, vector<Data> > *map) {
    data_list->push_back(Data(new_transaction));
    Data data(new_transaction);
    map->insert(make_pair(data.sender, vector<Data>()));
    map->insert(make_pair(data.receiver, vector<Data>()));
    map->at(data.sender).push_back(data);
    map->at(data.receiver).push_back(data);
}

/**
 * check balance
 */
string check_balance(string names, unordered_map<string, vector<Data> > *username_map) {
    vector<string> name_list = parse_string(names, ' ');
    string msg = "";
    unordered_set<string> user_data;
    for (auto& name : name_list) {
        if (username_map->find(name) != username_map->end()) {
            for (auto& data : username_map->at(name)) {
                user_data.insert(data.my_str);
            }
        }
    }
    for (auto& data : user_data) {
        msg += (data + ",");
    }
    return msg;
}

/**
 * get all transaction records
 */
string get_records(vector<Data> *data_list) {
    string msg = "";
    for (auto& t : *data_list) {
        msg += (t.my_str + ",");
    }
    return msg;
}

/**
 * get maximum serial number
 */
string get_max_serial_num(vector<Data> *data_list) {
    int max_serial_num = 0;
    for (auto& data : *data_list) {
        max_serial_num = max(max_serial_num, data.serial_num);
    } 
    return to_string(max_serial_num);
}      

int main() {
    cout << "The " << SERVER_NAME << " is up and running using UDP on port " << SERVER_PORT << "." << endl;

    // create udp server socket and bind it's socket address
    int server_sockfd = create_udp_server_socket();
    struct sockaddr_in server_addr = set_server_addr();
    udp_bind(server_sockfd, server_addr);
    
    // read transactions data from file and store them in the vector and unordered_map
    vector<Data> data_list;
    unordered_map<string, vector<Data> > username_map;
    read_file(&data_list);
    gen_username_map(&data_list, &username_map);

    while (true) {
        struct sockaddr_in client_addr = set_client_addr();
        string recv_msg = receive_msg(server_sockfd, client_addr);
        vector<string> operation = parse_string(recv_msg, ',');

        if (operation[0] == "INQUIRY") {
            sendto_client(server_sockfd, client_addr, check_balance(operation[1], &username_map));
        } else if (operation[0] == "TXLIST") {
            sendto_client(server_sockfd, client_addr, get_records(&data_list));   
        } else if (operation[0] == "WRITE") {
            write_file(operation[1]);
            insert_new_data(&data_list, operation[1], &username_map);
            sendto_client(server_sockfd, client_addr, "Already write in the log file!");
        } else if (operation[0] == "NUM") {
            sendto_client(server_sockfd, client_addr, get_max_serial_num(&data_list) + ",");
        }
    }
    close(server_sockfd);
    return 0;
}