

#include "support.cpp"
int main() {
  RingMaster r("4444", 25535, 4);
  r.init();
  r.connect_player();
}