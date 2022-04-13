/**
 * Reused code:
 * Chapter5 of Beej's Code: http://www.beej.us/guide/bgnet/ 
 *  1. create/bind UDP socket
 *  2. send/receive UDP message
 *  3. create/bind/listen/accept TCP socket
 *  4. send/receive TCP message
 * Chapter7 of Beej's Code: http://www.beej.us/guide/bgnet/ 
 *  1. poll TCP socket
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <poll.h> // for Synchronous 

#include <iostream> 
#include <sstream>
#include <fstream>
#include <string> 
#include <vector>
#include <unordered_map>
#include <algorithm>
using namespace std;

#define TCP_SERVER_HOST "127.0.0.1"
#define UDP_CLIENT_HOST "127.0.0.1"
#define UDP_SERVER_HOST "127.0.0.1"
#define UDP_CLIENT_PORT 24800
#define TCP_SERVER_PORT_A 25800
#define TCP_SERVER_PORT_B 26800
#define UDP_SERVER_PORT_A 21800
#define UDP_SERVER_PORT_B 22800
#define UDP_SERVER_PORT_C 23800
#define BACKLOG 10   // how many pending connections queue will hold
#define FAIL -1
#define MAX_DATA_SIZE 1024
#define FILE_NAME "alichain.txt"

const unordered_map<int, string> port_map = 
    {{UDP_SERVER_PORT_A, "ServerA"}, {UDP_SERVER_PORT_B, "ServerB"}, {UDP_SERVER_PORT_C, "ServerC"}, {TCP_SERVER_PORT_A, "ClientA"}, {TCP_SERVER_PORT_B, "ClientB"}};

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
 * create TCP server socket
 */
int create_tcp_server_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    socket_error_checking(sockfd, "TCP socket");
    return sockfd; 
}

/**
 * store the TCP server socket address information into the struct sockaddr_in * data type(include IPv4, server IP address and server port number)
 */
struct sockaddr_in set_tcp_server_addr(int port) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_HOST);
    server_addr.sin_port = htons(port);
    return server_addr;
}

/**
 * get the port number to which the socket sockfd is bound
 */
int get_port(int sockfd) {
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    socklen_t len = sizeof(my_addr);
    int status = getsockname(sockfd, (struct sockaddr *) &my_addr, &len);
    socket_error_checking(status, "get sock name");
    return ntohs(my_addr.sin_port);
}

/**
 * TCP bind server socket
 */
void tcp_bind(int sockfd, struct sockaddr_in server_addr) {
    int res = ::bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    socket_error_checking(res, "TCP bind");
}

/**
 * TCP listen for connections on server socket
 */
void tcp_listen(int sockfd) {
    int res = listen(sockfd, BACKLOG);
    socket_error_checking(res, "TCP listen");
}

/**
 * accept tcp connection
 */ 
int tcp_accept(int parent_sockfd) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int child_sockfd = accept(parent_sockfd, (struct sockaddr *) &client_addr, &client_addr_size);
    socket_error_checking(child_sockfd, "TCP accept");
    return child_sockfd;
}

/**
 * TCP respond client
 */ 
void tcp_send(int sockfd, string msg) {
    char send_buf[MAX_DATA_SIZE];
    strcpy(send_buf, msg.c_str());
    int bytes_send = send(sockfd, send_buf, sizeof(send_buf), 0);
    socket_error_checking(bytes_send, "TCP send");
}

/** 
 * TCP receive request from client
 */ 
string tcp_recv(int tcp_child_sockfd) {
    char recv_buf[MAX_DATA_SIZE];
    int status = recv(tcp_child_sockfd, recv_buf, sizeof(recv_buf), 0);
    socket_error_checking(status, "TCP recv");
    string recv_msg = recv_buf;
    return recv_msg;
}

/**
 * TCP poll
 */ 
void tcp_poll(int sockfd) {
    int res = listen(sockfd, BACKLOG);
    socket_error_checking(res, "TCP listen");
}

/**
 * put the tcp socket into the pfds array to be monitored
 * This funciton is adapted from "Beej's Guide to Network Programming"
 */
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size) {
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it
        *pfds = (struct pollfd*) realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read
    (*fd_count)++;
}

/**
 * remove the tcp socket which index is "i" from the pfds array
 * This funciton is adapted from "Beej's Guide to Network Programming"
 */ 
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];
    (*fd_count)--;
}

/**
 * create and initialize the tcp sockfd array, including the tcp socket descriptors we want to monitor
 * This funciton is adapted from "Beej's Guide to Network Programming"
 */
struct pollfd* initial_tcp_poll_array(int parent_sockfd_A, int parent_sockfd_B, int fd_size) {
    struct pollfd *pfds = (struct pollfd*) malloc(sizeof *pfds * fd_size);
    pfds[0].fd = parent_sockfd_A;
    pfds[0].events = POLLIN; // Report ready to read on incoming connection
    pfds[1].fd = parent_sockfd_B;
    pfds[1].events = POLLIN; // Report ready to read on incoming connection
    return pfds;
}

/**
 * create UDP client socket
 */
int create_udp_client_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd == FAIL) {
        perror("[ERROR] client: udp socket");
        exit(1);
    }
    return sockfd; 
}

/**
 * store the UDP client socket address information into the struct sockaddr_in * data type(include IPv4, client IP address and client port number)
 */
struct sockaddr_in set_udp_client_addr(int port) {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr)); 
    client_addr.sin_family = AF_INET; 
    client_addr.sin_addr.s_addr = inet_addr(UDP_CLIENT_HOST); 
    client_addr.sin_port = htons(port); 
    return client_addr;
}

/**
 * store the UDP server socket address information into the struct sockaddr_in * data type(include IPv4, server IP address and server port number)
 */
struct sockaddr_in set_udp_server_addr(int port) {
    struct sockaddr_in backend_server_addr;
    memset(&backend_server_addr, 0, sizeof(backend_server_addr)); 
    backend_server_addr.sin_family = AF_INET; 
    backend_server_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_HOST); 
    backend_server_addr.sin_port = htons(port); 
    return backend_server_addr;
}


/**
 * UDP request to the backend server
 */
void udp_sendto(int udp_client_sockfd, struct sockaddr_in server_addr, string msg) {
    char send_buf[MAX_DATA_SIZE];
    strcpy(send_buf, msg.c_str());
    int bytes_send = sendto(udp_client_sockfd, send_buf, sizeof(send_buf), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    socket_error_checking(bytes_send, "UDP send");
}

/**
 * UDP receive from server
 */ 
string udp_recv(int udp_client_sockfd, struct sockaddr_in server_addr) {
    char recv_udp_buf[MAX_DATA_SIZE];
    socklen_t server_addr_size = sizeof(server_addr);
    int status = recvfrom(udp_client_sockfd, recv_udp_buf, sizeof(recv_udp_buf), 0, (struct sockaddr *) &server_addr, &server_addr_size);
    socket_error_checking(status, "UDP recv");
    string recv_str = recv_udp_buf;
    return recv_str;
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
 * UDP request to backend server, and get the response
 * msg might be
 * 1) INQUIRY,username
 * 2) INQUIRY,sender_name receiver_name
 * 3) TXLIST,
 * 4) WRITE,serial_num sender_name receiver_name amount
 */ 
vector<string> udp_communicate_with_servers(int sockfd, string msg, vector<struct sockaddr_in> server_addr_list, string operation) {
    string transactions = "";
    for (auto& server_addr : server_addr_list) {
        int port = ntohs(server_addr.sin_port);
        udp_sendto(sockfd, server_addr, msg);
        if (operation == "BALANCE" || operation == "TRANSFER") {
            cout << "The main server sent a request to " << port_map.at(port) << "." << endl;
        }
        string recv_str = udp_recv(sockfd, server_addr);
        if (operation == "BALANCE") {
            cout << "The main server received transactions from " << port_map.at(port) << " using UDP over port " << port << "." << endl;
        } else if (operation == "TRANSFER") {
            cout << "The main server received the feedback from " << port_map.at(port) << " using UDP over port " << port << "." << endl;
        }
        transactions += recv_str;
    }
    return parse_string(transactions, ',');
}

/**
 * write all transaction records into log file
 */
void write_file(vector<string> all_transactions_list) {
    string arr[all_transactions_list.size()];
    for (auto& transaction : all_transactions_list) {
        stringstream ss(transaction);
        string serial_num;
        getline(ss, serial_num, ' ');
        arr[stoi(serial_num) - 1] = transaction;
    }
    ofstream myfile;
    myfile.open(FILE_NAME, ios::out | ios::trunc); 
    for (auto& transcation : arr) {
        myfile << transcation << endl;
    }
    myfile.close();
}

/**
 * randomly choose one backend
 */ 
vector<struct sockaddr_in> randomly_choose_one_backend(vector<struct sockaddr_in> backend_addr_list) {
    srand(time(NULL));
    int backend_num = backend_addr_list.size();
    int rand_backend = rand() % backend_num;
    vector<struct sockaddr_in> backend_addr;
    backend_addr.push_back(backend_addr_list.at(rand_backend));
    return backend_addr;
}

/**
 * get the maximum serial number
 */ 
int get_max_serial_num(vector<string> max_num_transaction_list) {
    int max_num = 0;
    for (auto& item : max_num_transaction_list) {
        max_num = max(max_num, stoi(item));
    }
    return max_num;
}

/**
 * calculate user's balance
 */
int calculate_balance(string name, vector<Data> data_list) {
    int balance = 1000;
    for (auto& data : data_list) {
        if (name == data.sender) {
            balance -= data.amount;
        } else if (name == data.receiver) {
            balance += data.amount;
        }
    }
    return balance;
}

/**
 * sort function
 */
bool sort_by_val(const pair<string, int> &a, const pair<string, int> &b) { 
    return (a.second > b.second); 
} 

/**
 * generate statistics list
 */
vector<string> gen_stats_list(vector<string> transaction_list, string username) {
    unordered_map<string, int> freq_map;
    unordered_map<string, int> amount_map;
    for (auto& transaction : transaction_list) {
        vector<string> item = parse_string(transaction, ' ');
        string partner_name;
        if (item[1] == username) {
            partner_name = item[2];
            freq_map.insert(make_pair(partner_name, 0));
            amount_map.insert(make_pair(partner_name, 0));
            freq_map.at(partner_name)++;
            amount_map.at(partner_name) -= stoi(item[3]);
        } else if (item[2] == username) {
            partner_name = item[1];
            freq_map.insert(make_pair(partner_name, 0));
            amount_map.insert(make_pair(partner_name, 0));
            freq_map.at(partner_name)++;
            amount_map.at(partner_name) += stoi(item[3]);
        }
    }
    // copy key-value pairs from the map to the vector
	vector<pair<string, int> > freq_list;
    unordered_map<string, int> :: iterator it1;
    for (it1 = freq_map.begin(); it1 != freq_map.end(); it1++) {
        freq_list.push_back(make_pair(it1->first, it1->second));
    }

    sort(freq_list.begin(), freq_list.end(), sort_by_val); 
    
    vector<string> stats_list;
    int rank = 1;
    for (auto& item : freq_list) {
        string stats = to_string(rank) + " " +  item.first + " " + to_string(item.second) + " " + to_string(amount_map.at(item.first));
        stats_list.push_back(stats);
        rank++;
    }
    return stats_list;
}

/**
 * check balance
 */
void check_balance(int udp_client_sockfd, int tcp_child_sockfd, string username, vector<struct sockaddr_in> backend_addr_list) {
    string msg = "";
    cout << "The main server received input=\"" << username << "\" from the client using TCP over port " << TCP_SERVER_PORT_A << "." << endl;

    vector<string> transaction_list = udp_communicate_with_servers(udp_client_sockfd, "INQUIRY," + username , backend_addr_list, "BALANCE");
    vector<Data> data_list;
    for (auto& transaction : transaction_list) {
        Data data(transaction);
        data_list.push_back(data);
    }

    if (data_list.empty()) {
        msg = "Unable to proceed with the request as \"" + username + "\" is not part of the network.";
        cout << "Username was not found on database." << endl;
        tcp_send(tcp_child_sockfd, msg);
    } else {
        msg = "The current balance of \"" + username + "\" is : " + to_string(calculate_balance(username, data_list)) + " alicoins.";
    	tcp_send(tcp_child_sockfd, msg);
    	cout << "The main server sent the current balance to " << port_map.at(get_port(tcp_child_sockfd)) << "." << endl;
    }
}

/**
 * get all transaction records
 */
void get_records(int udp_client_sockfd, int tcp_child_sockfd, vector<struct sockaddr_in> backend_addr_list) {
    string msg = "";
    vector<string> all_transactions_list = udp_communicate_with_servers(udp_client_sockfd, "TXLIST", backend_addr_list, "TXLIST");
    write_file(all_transactions_list);
    tcp_send(tcp_child_sockfd, msg);
}

/**
 * get the statistics list of all the users that the client has made transactions with
 */ 
void get_stats(int udp_client_sockfd, int tcp_child_sockfd, string username, vector<struct sockaddr_in> backend_addr_list) {
    string msg = "";
    vector<string> username_transaction_list = udp_communicate_with_servers(udp_client_sockfd, "INQUIRY," + username , backend_addr_list, "STATS");
    vector<string> stats_list = gen_stats_list(username_transaction_list, username);
    msg = "\"" + username + "\" statistics are the following.:,Rank--Username--NumofTransactions--Total,";
    for (auto& s : stats_list) {
        msg += (s + ",");
    }
    tcp_send(tcp_child_sockfd, msg);
}

/**
 * sender transfer coins to receiver
 */
void transfer_coins(int udp_client_sockfd, int tcp_child_sockfd, string sender, string receiver, string amount, vector<struct sockaddr_in> backend_addr_list) {
        string msg = "";
        cout << "The main server received from \"" << sender << "\" to transfer " << amount << " coins to \"" << receiver << "\" using TCP over port " << TCP_SERVER_PORT_A << "." << endl;

        vector<string> transaction_list = udp_communicate_with_servers(udp_client_sockfd, "INQUIRY," + sender + " " + receiver, backend_addr_list, "TRANSFER");
        vector<Data> sender_data_list;
        vector<Data> receiver_data_list;   
        for (auto& transaction : transaction_list) {
            Data data(transaction);
            if (data.sender == sender || data.receiver == sender) {
                sender_data_list.push_back(data);
            } else if (data.sender == receiver || data.receiver == receiver) {
                receiver_data_list.push_back(data);
            }
        }

        vector<string> max_serial_list = udp_communicate_with_servers(udp_client_sockfd, "NUM,", backend_addr_list, "NUM");
        
        if (sender_data_list.empty() && receiver_data_list.empty()) {
            msg = "Unable to proceed with the transaction as \"" + sender + "\" and \"" + receiver + "\" are not part of the network.";
        } else if (sender_data_list.empty()) {
            msg = "Unable to proceed with the transaction as \"" + sender + "\" is not part of the network.";
        } else if (receiver_data_list.empty()) {
            msg = "Unable to proceed with the transaction as \"" + receiver + "\" is not part of the network.";
        } else {
            int sender_balance = calculate_balance(sender, sender_data_list);
            if (sender_balance < stoi(amount)) {
                msg = "\"" + sender + "\" was unable to transfer " + amount + " alicoins to \"" + receiver + "\" because of insufficient balance.\n";
                msg += "The current balance of \"" + sender + "\" is : " + to_string(sender_balance) + " alicoins.";
            } else {
                int new_balance = sender_balance - stoi(amount);
                msg = "\"" + sender + "\" successfully transferred " + amount + " alicoins to \"" + receiver + "\".\n";
                msg += "The current balance of \"" + sender + "\" is : " + to_string(new_balance) + " alicoins.";
                string str = "WRITE," + to_string(get_max_serial_num(max_serial_list) + 1) + " " + sender + " " + receiver + " " + amount;
                udp_communicate_with_servers(udp_client_sockfd, str, randomly_choose_one_backend(backend_addr_list), "WRITE");
            }
        }
        tcp_send(tcp_child_sockfd, msg);
        cout << "The main server sent the result of the transaction to " << port_map.at(get_port(tcp_child_sockfd)) << "." << endl; 
}


int main(int argc, char *argv[]) {
    cout << "The main server is up and running." << endl;

    // create udp client socket and bind its socket address
    int udp_client_sockfd = create_udp_client_socket(); 
    struct sockaddr_in client_addr = set_udp_client_addr(UDP_CLIENT_PORT);
    int res = ::bind(udp_client_sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr));
    socket_error_checking(res, "UDP bind");

    // create tcp server socket for client a, bind its socket address, and listen
    int parent_sockfd_A = create_tcp_server_socket(); 
    struct sockaddr_in server_addr_A = set_tcp_server_addr(TCP_SERVER_PORT_A);
    tcp_bind(parent_sockfd_A, server_addr_A);
    tcp_listen(parent_sockfd_A);

    // create tcp server socket for client b, bind its socket address, and listen
    int parent_sockfd_B = create_tcp_server_socket(); 
    struct sockaddr_in server_addr_B = set_tcp_server_addr(TCP_SERVER_PORT_B);
    tcp_bind(parent_sockfd_B, server_addr_B);
    tcp_listen(parent_sockfd_B);

    vector<struct sockaddr_in> backend_addr_list;
    backend_addr_list.push_back(set_udp_server_addr(UDP_SERVER_PORT_A));
    backend_addr_list.push_back(set_udp_server_addr(UDP_SERVER_PORT_B));
    backend_addr_list.push_back(set_udp_server_addr(UDP_SERVER_PORT_C));

    int fd_size = 2; // pfds initial for two connections
    struct pollfd *pfds = initial_tcp_poll_array(parent_sockfd_A, parent_sockfd_B, fd_size);
    int fd_count = 2; // how many connection in the pfds right now

    while(true) {
        int poll_count = poll(pfds, fd_count, -1);
        socket_error_checking(poll_count, "poll");

        for (int i = 0; i < fd_count ; i++) {
            if (pfds[i].revents & POLLIN) { // pfds[i].fd is a socket, and it is ready to read on incoming connection
                if (pfds[i].fd == parent_sockfd_A || pfds[i].fd == parent_sockfd_B) { 
                    // pfds[i].fd is a tcp parent socket, and it is ready to accept connection
                    add_to_pfds(&pfds, tcp_accept(pfds[i].fd), &fd_count, &fd_size);
                } else { 
                    // pfds[i].fd is a tcp child socket, and it is ready to receive message
                    int tcp_child_sockfd = pfds[i].fd;
                    string recv_msg = tcp_recv(tcp_child_sockfd);
                    vector<string> operation = parse_string(recv_msg, ',');
                    vector<string> argument;
                    if (operation.size() > 1) {
                        argument = parse_string(operation[1], ' ');
                    }
                    if (operation[0] == "BALANCE") {
                        check_balance(udp_client_sockfd, tcp_child_sockfd, argument[0], backend_addr_list);
                    } else if (operation[0] == "TXLIST") {
                        get_records(udp_client_sockfd, tcp_child_sockfd, backend_addr_list);
                    } else if (operation[0] == "STATS") {
                        get_stats(udp_client_sockfd, tcp_child_sockfd, argument[0], backend_addr_list);
                    } else if (operation[0] == "TRANSFER") {
                        transfer_coins(udp_client_sockfd, tcp_child_sockfd, argument[0], argument[1], argument[2], backend_addr_list);
                    }

                    close(tcp_child_sockfd);
                    del_from_pfds(pfds, i, &fd_count);
                }
            } 
        }
    }
    close(udp_client_sockfd);
    close(parent_sockfd_A);
    close(parent_sockfd_B);
    return 0;
}