CXX = clang
CXXFLAGS = --debug
LDLIBS = -lcurl -lcjson

SRCS = main.c

OBJS = $(SRCS:.c=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

%.o : %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clear:
	rm -f $(OBJS) $(TARGET)
