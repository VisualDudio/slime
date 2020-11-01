CXX = /opt/rh/devtoolset-8/root/usr/bin/g++
CXXFLAGS = -std=c++17 -pedantic-errors -Wall -Wextra -Werror -pthread -fPIC
LDFLAGS  := -ldl -shared -Wl,--no-undefined
BUILD    := ./build
OBJ_DIR  := $(BUILD)/obj
BIN_DIR  := $(BUILD)/bin
TARGET   := SlimeSocket
INCLUDE  := -Iinclude/ -Ilib/ -I../Common/include/
SRC      := $(wildcard src/*.cpp) \
            $(wildcard ../Common/src/*.cpp)

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(BIN_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

$(BIN_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$(TARGET).so $^ $(LDFLAGS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(BIN_DIR)/*
