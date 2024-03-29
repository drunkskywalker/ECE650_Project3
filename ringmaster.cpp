

#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "potato.hpp"

using namespace std;

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
    cout << "Potato Ringmaster" << endl
         << "Players = " << player_num << endl
         << "Hops = " << hop_num << endl;
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
    freeaddrinfo(host_info_list);
  }

  void connect_player() {
    char prev_ip[100];
    char this_ip[100];
    char first_ip[100];

    int prev_port;
    int this_port;
    int first_port;
    memset(&prev_port, 0, sizeof(prev_port));
    memset(&this_port, 0, sizeof(this_port));
    memset(&first_port, 0, sizeof(first_port));


    for (int i = 0; i < player_num; i ++) {
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
      if(send(client_connection_fd, &i, sizeof(int), 0) <= 0) {exit(1);}
      if(send(client_connection_fd, &player_num, sizeof(int), 0) <= 0) {exit(1);}

      if (i != player_num - 1) {
        if(recv(client_connection_fd, &this_port, sizeof(int), 0) <= 0) {exit(1);}
        if(recv(client_connection_fd, this_ip, 100, 0) <= 0) {exit(1);}
      }

      if (i == 0) {
        memcpy(first_ip, this_ip, 100);
        first_port = this_port;
        memcpy(prev_ip, this_ip, 100);
        prev_port = this_port;
      }

      else {
        if(send(client_connection_fd, prev_ip, 100, 0) <= 0) {exit(1);}
        if(send(client_connection_fd, &prev_port, sizeof(prev_port), 0) <= 0) {exit(1);}

        memcpy(prev_ip, this_ip, 100);
        prev_port = this_port;
        //cout << "Player " << i << " is ready to play" << endl;
      }

      if (i == player_num - 1) {
        if(send(client_connection_fd, first_ip, 100, 0) <= 0) {exit(1);}
        if(send(client_connection_fd, &first_port, sizeof(first_port), 0) <= 0) {exit(1);}
        // cout << "Player " << 0 << " is ready to play" << endl;
      }
   //   cout << i << endl;
    }
   // cout << "begin validation" << endl;
    // validify connection
    char b;
    for (int i = 0; i < player_num; i++) {
      if(recv(player_fd_no_map[i], &b, sizeof(char), 0) <= 0) {exit(1);}
      cout << b;
      if (b == 'p') {
        cout << "Player " << i << " is ready to play" << endl;
      }
      b = 'r';
      if(send(player_fd_no_map[i], &b, sizeof(char), 0) <= 0) {exit(1);}
    }
  }

  void close_connections() {
    for (int i = 0; i < player_num; i++) {
      char state = '1';
      if(send(player_fd_no_map[i], &state, sizeof(char), MSG_WAITALL) <= 0) {exit(1);}
      close(player_fd_no_map[i]);
      close(socket_fd);
    }
  }

  void play() {
  
  
    Potato p;
    memset(&p, 0, sizeof(Potato));
    if (hop_num == 0) {
      close_connections();
      return;
    }
    
    
    else {
    p.hops = hop_num;
    p.count = 0;

      srand(time(NULL));
      int r = rand() % player_num;
      cout << "Ready to start the game, sending potato to player " << r << endl;

      // state: 0: potato; 1: stop
      char state = '0';
      if(send(player_fd_no_map[r], &state, sizeof(char), 0) <= 0) {exit(1);}
      if(send(player_fd_no_map[r], &p, sizeof(Potato), 0) <= 0) {exit(1);}

      //TODO: select
      fd_set fdset;
      int last;

      FD_ZERO(&fdset);
      int curr_max = -1;
      for (int i = 0; i < player_num; i++) {
        FD_SET(player_fd_no_map[i], &fdset);
        if (player_fd_no_map[i] > curr_max) {
          curr_max = player_fd_no_map[i];
        }
      }
      

      int status = select(curr_max + 1, &fdset, NULL, NULL, NULL);
      if (status == -1) {
        cerr << "select() returned " << status << " with error code " << errno << endl;
        exit(1);
      }
      for (int i = 0; i < player_num; i++) {
        if (FD_ISSET(player_fd_no_map[i], &fdset)) {
          last = player_fd_no_map[i];
          break;
        }
      }
      //cout << last << endl;
      memset(&state, 0, sizeof(char));
      memset(&p, 0, sizeof(Potato));
      char state_end = '0';
      if(recv(last, &state_end, sizeof(char), 0) <= 0) {exit(1);}
      if(recv(last, &p, sizeof(Potato), 0) <= 0) {exit(1);}
        if (state_end != 1) {
        cerr << "bad transmission" << endl;
        exit(1);
      }
      int i = 0;
      if (p.hops == 0) {close_connections(); return;}
      cout << "Trace of potato:" << endl;
      while (p.players[i + 1] != -1) {
        cout << p.players[i] << ",";
        i++;
      }
      cout << p.players[i] << endl;
      
      close_connections();
    }
  }
};
int main(int argc, char * argv[]) {
  srand(time(NULL));

  if (argc != 4) {
    cout << "Syntax: ringmaster <port_num> <num_players> <num_hops>" << endl;
    return 1;
  }

  char * p;
  const char * port_num = argv[1];
  strtol(argv[1], &p, 10);
  if (*p != '\0') {
    cout << "Syntax error: expected a number as port_num" << endl;
    return 1;
  }

  long num_players = strtol(argv[2], &p, 10);
  if (*p != '\0') {
    cout << "Syntax error: expected a number as num_players" << endl;
    return 1;
  }

  if (num_players < 2) {
    cout << "Syntax error: num_players must be greater than 1" << endl;
    return 1;
  }
  long num_hops = strtol(argv[3], &p, 10);
  if (*p != '\0') {
    cout << "Syntax error: expected a number as num_hops" << endl;
    return 1;
  }
  if (num_hops < 0 || num_hops > 512) {
    cout << "Syntax error: num_hops must be between 0 and 512 inclusive" << endl;
    return 1;
  }
  RingMaster r(port_num, num_players, num_hops);
  r.init();
  r.connect_player();
  r.play();
}
