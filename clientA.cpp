/**
 * Reused code:
 * Chapter5 of Beej's Code: http://www.beej.us/guide/bgnet/ 
 *  1. create/connet TCP socket
 *  2. send/receive TCP message
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
#include <iomanip>
#include <string> 
#include <vector>
using namespace std;

#define FAIL -1 
#define MAX_DATA_SIZE 1024
#define SERVER_PORT 25800
#define SERVER_HOST "127.0.0.1"
#define CLIENT_NAME "ClientA"

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
 * create TCP client socket and assign a dynamic port number and localhost for it
 */
int create_tcp_client_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    socket_error_checking(sockfd, "create TCP client socket");
    struct sockaddr_in client_addr;
    getsockname(sockfd,(struct sockaddr *) &client_addr, (socklen_t *) &client_addr);
    return sockfd;
}

/**
 * store the TCP server socket address information into the struct sockaddr_in * data type(include IPv4, server IP address and server port number)
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
 * TCP connect to server
 */
void tcp_connect(int sockfd, struct sockaddr_in server_addr) {
    int res = connect(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr));
    socket_error_checking(res, "TCP connect");
}

/**
 * TCP send message to server
 */
void send_msg(int sockfd, string msg) {
    char send_buf[MAX_DATA_SIZE];
    strcpy(send_buf, msg.c_str());
    int bytes_sent = send(sockfd, send_buf, sizeof(send_buf), 0);
    socket_error_checking(bytes_sent, "TCP send");
}

/**
 * TCP receive message from server
 */
string receive_msg(int sockfd) {
    char recv_buffer[MAX_DATA_SIZE];
    int bytes_received = recv(sockfd, recv_buffer, sizeof(recv_buffer), 0);
    socket_error_checking(bytes_received, "TCP recv");
    if (bytes_received == 0) {
        cout << "The server had closed the connection on the client." << endl;
    }
    string s = recv_buffer;
    return s;
}

/**
 * check balance
 */
void balance_enquiry_request(string request_name) {
    int sockfd = create_tcp_client_socket();
    struct sockaddr_in server_addr = set_server_addr();
    tcp_connect(sockfd, server_addr);
    string msg = "BALANCE," + request_name;
    send_msg(sockfd, msg);
    cout << "\"" + request_name + "\" sent a balance enquiry request to the main server." << endl;
    string s = receive_msg(sockfd);
    cout << s << endl;
    close(sockfd);
}

/**
 * get all transaction records
 */
void all_transactions_request() {
    int sockfd = create_tcp_client_socket();
    struct sockaddr_in server_addr = set_server_addr();
    tcp_connect(sockfd, server_addr);
    string msg = "TXLIST";
    send_msg(sockfd, msg);
    cout << CLIENT_NAME << " sent a sorted list request to the main server." << endl;
    close(sockfd);
}

/**
 * sender transfer coins to receiver
 */
void transfer_coins_request(string sender_name, string receiver_name, string transfer_amount) {
    int sockfd = create_tcp_client_socket();
    struct sockaddr_in server_addr = set_server_addr();
    tcp_connect(sockfd, server_addr);
    string msg = "TRANSFER," + sender_name + " " + receiver_name + " " + transfer_amount;
    send_msg(sockfd, msg);
    cout << "\"" << sender_name << "\" has requested to transfer " << transfer_amount << " coins to \"" << receiver_name << "\"." << endl;
    string s = receive_msg(sockfd);
    cout << s << endl;
    close(sockfd);
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
 * print the statistics table
 */
void print_stats_table(string str) {
    vector<string> row = parse_string(str, ',');
    cout << row[0] << endl << row[1] << endl;
    int rank = 1;
    for (int i = 2; i < row.size(); i++) {
        vector<string> v = parse_string(row[i], ' ');
        cout << setiosflags(ios::left) << setw(6) << v[0] << setw(15) << v[1] << resetiosflags(ios::left)
        << setiosflags(ios::right) << setw(5) << v[2] << setw(14) << v[3]
        << resetiosflags(ios::right) << endl;
        rank++;
    }
}

/**
 * get the statistics list of all the users that the client has made transactions with
 */
void stats_request(string request_name) {
    int sockfd = create_tcp_client_socket();
    struct sockaddr_in server_addr = set_server_addr();
    tcp_connect(sockfd, server_addr);
    string msg = "STATS," + request_name;
    send_msg(sockfd, msg);
    cout << "\"" << request_name << "\" sent a statistics enquiry request to the main server." << endl;
    string s = receive_msg(sockfd);
    print_stats_table(s);
    close(sockfd);
}

int main(int argc, char *argv[]) {
    cout << "The " << CLIENT_NAME << " is up and running." << endl; 
	if (argc == 2) { 
        if (string(argv[1]) == "TXLIST") {
            all_transactions_request();   
        } else {
            balance_enquiry_request(argv[1]);      
        }
	} else if (argc == 3 && string(argv[2]) == "stats") {
        stats_request(argv[1]);
    } else if (argc == 4) { 
        transfer_coins_request(argv[1], argv[2], argv[3]);
    } else { 
        cout << "The input format is not correct!" << endl;
    }
    return 0;
}