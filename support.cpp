#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <vector>

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
    char * prev_ip;
    char * prev_port;
    if (player_num == 1) {
      //
    }
    else {
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
        i++;
        cout << i << endl;
      }
    }
    cout << "ready" << endl;
    close(socket_fd);
  }
};

class Player {
  int player_no;
  int player_tot;
  int prev_fd;
  int next_fd;
  int rm_fd;
  int my_fd;
  const char * rm_host;
  const char * rm_port;
  const char * hostname = NULL;
  const char * port = "";
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  int status;
  
 public:
  Player(char * rm_host, char * rm_port) : rm_host(rm_host), rm_port(rm_port) {}
  
  void init_sock() {

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
    struct sockaddr_in * addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
    addr_in->sin_port = 0;
        my_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (my_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }

    int yes = 1;
    status = setsockopt(my_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(my_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot bind socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }

    status = listen(my_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }    
    socklen_t len = sizeof(*addr_in);
    status = getsockname(my_fd, (struct sockaddr *)addr_in, &len);
    if (status == -1) {
      cerr << "Error: cannot listen socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    } 
    cout << "Waiting for connection on port " << (addr_in->sin_port) << endl;
  }
  
  
  void connect_to_rm() {
    struct addrinfo host_info;
    struct addrinfo * host_info_list;
    int status;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(rm_host, rm_port, &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << rm_host << "," << rm_port << ")" << endl;
      exit(1);
    }
    rm_fd = socket(host_info_list->ai_family,
                   host_info_list->ai_socktype,
                   host_info_list->ai_protocol);
    if (rm_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << rm_host << "," << rm_port << ")" << endl;
      exit(1);
    }
    status = connect(rm_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << rm_host << "," << rm_port << ")" << endl;
      exit(1);
    }
    cout << "Connected to host." << endl;
    
    recv(rm_fd, &player_no, sizeof(int), 0);
    recv(rm_fd, &player_tot, sizeof(int), 0);
    cout << player_no << "th player among " << player_tot << " players." << endl;
    
  }

  void passPotato() {
    int r = rand() % 2;
    if (r == 0) {
      // TODO: send to prev player.
    }
    else {
      // TODO: send to next player.
    }
  }
};




