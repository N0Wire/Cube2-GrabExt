CXX = g++
CXXFLAGS = -Wall

OBJS = main.o \
	config.o \
	grabber.o

default: grabext

%.o: %.cpp
	$(CXX) -c -o $@ $<

grabext: $(OBJS)
	$(CXX) $(CXXFLAGS) -o grabext $(OBJS)
	mv grabext ../grabext

clean: 
	rm -f $(OBJS) ../grabext


main.o: config.h grabber.h
config.o: config.h
grabber.o: grabber.h
