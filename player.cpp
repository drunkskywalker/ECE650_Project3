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

  struct sockaddr_in * addr_in;
  int status;

 public:
  Player(const char * rm_host, const char * rm_port) :
      rm_host(rm_host), rm_port(rm_port) {}

  void init_sock() {
    struct addrinfo host_info;
  struct addrinfo * host_info_list;
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
    addr_in = (struct sockaddr_in *)(host_info_list->ai_addr);
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

    socklen_t len = sizeof(*addr_in);
    status = getsockname(my_fd, (struct sockaddr *)addr_in, &len);
    if (status == -1) {
      cerr << "Error: cannot listen socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }
    int p = htons(addr_in->sin_port);
    send(rm_fd, &p, sizeof(int), 0);
    char host[100];
    gethostname(host, sizeof(host));
    send(rm_fd, host, 100, 0);

    status = listen(my_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      exit(1);
    }
    freeaddrinfo(host_info_list);
  }

  void connect_server(const char * host, int port, int * fd) {
    char port_c[5];
    sprintf(port_c, "%d", port);
    struct addrinfo host_info;
    struct addrinfo * host_info_list;
    int status;

    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(host, port_c, &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << host << "," << port << ")" << endl;
      exit(1);
    }

    *fd = socket(host_info_list->ai_family,
                 host_info_list->ai_socktype,
                 host_info_list->ai_protocol);
    if (*fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << host << "," << port << ")" << endl;
      exit(1);
    }

    status = connect(*fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << host << "," << port << ")" << endl;
      exit(1);
    }
    freeaddrinfo(host_info_list);
  }

  void accept_request(int socket_fd, int * client_fd) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd =
        accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      exit(1);
    }
    *client_fd = client_connection_fd;
  }

  void init() {
    connect_server(rm_host, atoi(rm_port), &rm_fd);
    recv(rm_fd, &player_no, sizeof(int), 0);
    recv(rm_fd, &player_tot, sizeof(int), 0);
    cout << "Connected as player " << player_no << " out of " << player_tot
         << " total players" << endl;
    if (player_no == player_tot - 1) {
      int prev_port;
      int next_port;

      char prev_host[100];
      recv(rm_fd, &prev_host, 100, 0);
      recv(rm_fd, &prev_port, sizeof(int), 0);

      char next_host[100];
      recv(rm_fd, &next_host, 100, 0);
      recv(rm_fd, &next_port, sizeof(int), 0);

      connect_server(prev_host, prev_port, &prev_fd);
      connect_server(next_host, next_port, &next_fd);
    }

    else if (player_no == 0) {
      init_sock();

      accept_request(my_fd, &next_fd);
      accept_request(my_fd, &prev_fd);
    }

    else {
      init_sock();

      int prev_port;
      char prev_host[100];
      recv(rm_fd, &prev_host, 100, 0);
      recv(rm_fd, &prev_port, sizeof(int), 0);

      connect_server(prev_host, prev_port, &prev_fd);
      accept_request(my_fd, &next_fd);
    }
    char b = 'p';
    send(rm_fd, &b, sizeof(char), 0);
    recv(rm_fd, &b, sizeof(char), MSG_WAITALL);
    //cout << b << endl;
    if (b != 'r') {
      cerr << "server is not ready; quitting" << endl;
      exit(1);
    }
    //    else {
    //cout << "server is ready" << endl;
    //}

    char n = '1';
    if (player_no == 0) {
      n = '0';
      send(next_fd, &n, sizeof(char), 0);
      n = 'x';
      recv(prev_fd, &n, sizeof(char), 0);

      //cout << "prev sent " << n << endl;
      /*
      if (n != '0') {
        cerr << "bad connection" << endl;
        exit(1);
      }
      */
    }
    else {
      recv(prev_fd, &n, sizeof(char), 0);
      //cout << "prev sent " << n << endl;
      /*if (n != '0') {
        cerr << "bad connection" << endl;
        exit(1);
      }
      */
      n++;
      send(next_fd, &n, sizeof(char), 0);
    }
    //cout << "player connections ready" << endl;
  }

  void close_connections() {
    //cout << "closing" << endl;
    close(prev_fd);
    close(next_fd);
    close(my_fd);
    close(rm_fd);
  }

  void play() {
    Potato p;
    char b = '0';
    int x;
    fd_set fdset;
    //TODO: select
    while (b != '1') {
      while (true) {
        FD_ZERO(&fdset);
        FD_SET(rm_fd, &fdset);
        FD_SET(prev_fd, &fdset);
        FD_SET(next_fd, &fdset);
        int status = select(65536, &fdset, NULL, NULL, NULL);
        if (status == -1) {
          cerr << "select() returned " << status << " with error code " << errno << endl;
          exit(1);
        }
        if (FD_ISSET(rm_fd, &fdset)) {
          x = rm_fd;
          break;
        }
        else if (FD_ISSET(prev_fd, &fdset)) {
          x = prev_fd;
          break;
        }
        else if (FD_ISSET(next_fd, &fdset)) {
          x = next_fd;
          break;
        }
      }

      recv(x, &b, sizeof(char), MSG_WAITALL);
      if (b == '1') {
        close_connections();
        return;
      }
      recv(x, &p, sizeof(Potato), MSG_WAITALL);
      p.count++;
      if (p.count == p.hops) {
        b = 1;
        cout << "I'm it" << endl;
        p.players[p.count - 1] = player_no;
        p.players[p.count] = -1;
        send(rm_fd, &b, sizeof(char), 0);
        send(rm_fd, &p, sizeof(Potato), 0);
        recv(rm_fd, &b, sizeof(char), 0);
        close_connections();
        return;
      }
      else {
        int r = rand() % 2;
        int next = player_no;
        if (r == 0) {
          r = -1;
          x = prev_fd;
          next += r;
          if (next == -1) {
            next = player_tot - 1;
          }
        }
        else {
          x = next_fd;
          next += r;
          if (next == player_tot) {
            next = 0;
          }
        }
        cout << "Sending potato to " << next << endl;

        p.players[p.count - 1] = player_no;
      }
      status = send(x, &b, sizeof(char), 0);
      send(x, &p, sizeof(Potato), 0);
    }
  }
};

int main(int argc, char * argv[]) {
  if (argc != 3) {
    cout << "Syntax: player <machine_name> <port_num>" << endl;
    return 1;
  }

  char * p;
  const char * port_num = argv[2];
  strtol(argv[2], &p, 10);
  if (*p != '\0') {
    cout << "Syntax error: expected a number as port_num" << endl;
    return 1;
  }


  Player player(argv[1], port_num);
  player.init();
  player.play();
}
