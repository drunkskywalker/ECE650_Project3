TARGETS=ringMaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)


#client: client.cpp#
#	g++ -g -o $@ $<

#ringMaster: ringMaster.cpp
#	g++ -g -o $@ $<
 
ringMaster: ringMaster.cpp potato.hpp
	g++ -g -o $@ $<
  
player: player.cpp potato.hpp
	g++ -g -o $@ $<
