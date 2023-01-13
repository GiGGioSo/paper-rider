
CC = g++
CFLAGS = -g -lglfw -ldl -lpthread -lm -Wall

SRC = src
OBJ = obj
BIN = bin
EXE = $(BIN)/paper

SRCS_CPP = $(wildcard $(SRC)/*.cpp)
OBJS_CPP = $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SRCS_CPP))

SRCS_C = $(wildcard $(SRC)/*.c)
OBJS_C = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS_C))

SRCS = $(SRCS_C) $(SRCS_CPP)
OBJS = $(OBJS_C) $(OBJS_CPP)


$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@


$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	$(RM) $(OBJ)/* $(BIN)/*
