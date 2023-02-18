TARGETS=client ringMaster s c

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

client: client.cpp
	g++ -g -o $@ $<

ringMaster: ringMaster.cpp
	g++ -g -o $@ $<
 
s: s.cpp
	g++ -g -o $@ $<
  
c: c.cpp
	g++ -g -o $@ $<
