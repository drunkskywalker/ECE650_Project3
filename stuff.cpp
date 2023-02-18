#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <set>

using namespace std;

class Potato {
  int num_hops;

 public:
  Potato(int h) : num_hops(h) {}
  int turn() {
    if (num_hops == 0) {
      // TODO: u r it
      return 1;
    }
    num_hops--;
    return 0;
  }
};

class RingMaster {
  Potato potato;
  int num_players;
  set<int, int> player_fd;

 public:
  RingMaster(int num_hops, int p) :
      potato(Potato(num_hops)), num_players(p), player_fd(set<int, int>()) {}

  /* 
  Wait for num_players connections from clients.
  Map between player_fd and player_id. 
  Establish connections between adjacent players.
  Note: when last player joins, establish a connection to the first player.
  Latter listen, former connect. 
  0 -> 1 -> 2 -> ... -> n -> 0
*/
  void init_game() {}

  /*
  After game is initialized, play the game. 
  Initial: give the potato to a random player. 
  Subsequent: receive from player, (print?), call potato.turn() to count.
  if potato.turn() returns 1, end the game.
*/
  void play() {}
};

//OVERRIDE: makesure only one socket listens.

class Player {
  int player_no;
  int left_fd;
  int right_fd;
  int ringMaster_fd;

 public:
  // Wait for me to be holding the potato or game end.
  void wait() {
    //TODO: recv()
    const char * message;
    // Hold potato
    if (message[0] == '0') {
      pass();
    }
    else if (message[0] == '1') {
      // TODO: print end message. I'm it

      cout << "I'm it" << endl;
    }
  }

  // If I am holding the potato and game has not yet ended, pass it to one of my adjacent players. Send to both chosen player and ringMaster.
  int pass() {
    //TODO: stuff

    int r = rand() % 2;
    if (r == 0) {
      //TODO: send to left and ringMaster
      return left_fd;
    }
    else {
      //TODO: sent to right and ringMaster
      return right_fd;
    }
  }
};
