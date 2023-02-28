#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include <string>
using namespace std;
class Potato {
 public:
  int hops;
  int players[514];
  Potato(int h) : hops(h) {}
};

class RingMaster {
  const char * port;
  int player_num;
  int hop_num;
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * hostname;
  map<int, int> player_fd_no_map;

 public:
  RingMaster(const char * port, int player_num, int hop_num) :
      port(port), player_num(player_num), hop_num(hop_num), hostname(NULL) {}

  // setup ringmaster socket
  void init() {
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags = AI_PASSIVE;
    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }

    socket_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot bind socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }
    

    status = listen(socket_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen on socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
      
    }
  }

  void connect_player() {
    string prev_ip;
    int prev_port;
    string first_ip;
    int first_port;
    
      int i = 0;
      while (i < player_num) {
        cout << "Waiting for connection on port " << port << endl;
        struct sockaddr_storage socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        int client_connection_fd;
        client_connection_fd =
            accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (client_connection_fd == -1) {
          cerr << "Error: cannot accept connection on socket" << endl;  
          exit(1);
        }
        player_fd_no_map[i] = client_connection_fd;
        cout << i << " player with client connection fd " << player_fd_no_map[i] << endl;
        cout << "Server sent: " << i << endl;
        send(client_connection_fd, &i, sizeof(int), 0);
        cout << "Server sent: " << player_num << endl;
        send(client_connection_fd, &player_num, sizeof(int), 0);
        
        
        //TODO:receive ip and port
        
        
        recv(client_connection_fd, &prev_port, sizeof(int), 0);
        struct sockaddr_in* in = (struct sockaddr_in*) &socket_addr;
        prev_ip = *(inet_ntoa(in->sin_addr));
        cout << "player " << i << " is listening from "<< prev_ip <<" on port " << prev_port << endl;
        if (i == 0) {
          first_ip = prev_ip;
          first_port = prev_port;
        }
        else {
          size_t s = sizeof(prev_ip);
          send(client_connection_fd, &s, sizeof(size_t), 0);
          send(client_connection_fd, &prev_ip, s, 0);
          send(client_connection_fd, &prev_port, sizeof(prev_port), 0);
        
        }
        
        i++;

      }
    
    cout << "all is ready" << endl;
    close(socket_fd);
  }
};






