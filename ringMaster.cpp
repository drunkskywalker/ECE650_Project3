#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

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

  if (num_players < 1) {
    cout << "Syntax error: num_players must be greater than 0" << endl;
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
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;
  const char * hostname = NULL;
  const char * port = port_num;
  memset(&host_info, 0, sizeof(host_info));
  vector<int> adds;
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if

  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }  //if
  while (true) {
    cout << "Waiting for connection on port " << port << endl;
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd =
        accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
      cerr << "Error: cannot accept connection on socket" << endl;
      return -1;
    }  //if
    adds.push_back(client_connection_fd);
    char buffer[512];
    recv(client_connection_fd, buffer, 9, 0);
    buffer[9] = 0;

    cout << "Server received: " << buffer << " at " << client_connection_fd << endl;

    const char * message = "Guten Tag";
    cout << "Server sent: " << message << endl;
    send(client_connection_fd, message, strlen(message), 0);
    const char * message2 = "Guten Tag first player";
    cout << "Server sent to " <<adds[0] << ": "  << message2 << endl;
    send(adds[0], message2, strlen(message2), 0);

    // freeaddrinfo(host_info_list);
  }
  close(socket_fd);
  return 0;
}
