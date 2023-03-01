TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

 
ringmaster: ringmaster.cpp potato.hpp
	g++ -g -o $@ $<
  
player: player.cpp potato.hpp
	g++ -g -o $@ $<
