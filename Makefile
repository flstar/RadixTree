.PHONY: default test

TARGET = TestRadixTree
OBJS = RadixTree.o TestRadixTree.o
LIBS = -L/usr/local/lib -lgtest

CXXFLAGS = -g

default: $(TARGET)

	
$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

RadixTree.o: RadixTree.cxx RadixTree.h
	$(CXX) -o $@ $(CXXFLAGS) -c $<

TestRadixTree.o: TestRadixTree.cxx RadixTree.h
	$(CXX) -o $@ $(CXXFLAGS) -c $<

test: default
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
