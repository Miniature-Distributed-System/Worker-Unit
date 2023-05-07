TARGET = compute

CC = g++
CFLAGS = -lsqlite3 -pthread -std=gnu++17 -g

OUTDIR = ./bin
DATADIR = ./data
SUBDIR = algorithm data_processor instance receiver_proc scheduler sender_proc services socket lib lib/nlohmann
DIR_OBJ = ./obj
include_dir = -I /usr/include/boost -lsqlite3 -pthread

INCS = $(wildcard *.hpp $(foreach fd, $(SUBDIR), $(fd)/*.hpp))
SRCS = $(wildcard *.cpp $(foreach fd, $(SUBDIR), $(fd)/*.cpp))
NODIR_SRC = $(notdir $(SRCS))
OBJS = $(addprefix $(DIR_OBJ)/, $(SRCS:cpp=o)) # obj/xxx.o obj/folder/xxx .o
INC_DIRS = $(addprefix -I, $(SUBDIR))

.PHONY: clean echoes

$(TARGET): $(OBJS)
	$(CC) -o $(OUTDIR)/$@ $(OBJS) $(INCS) $(include_dir)

$(DIR_OBJ)/%.o: %.cpp $(INCS)
	mkdir -p $(@D)
	$(CC) -o $@ -c $< $(CFLAGS) $(INC_DIRS)

clean:
	rm -rf $(OUTDIR)/* $(DATADIR)/* $(DIR_OBJ)/*

echoes:
	@echo "INC files: $(INCS)"  
	@echo "SRC files: $(SRCS)"
	@echo "OBJ files: $(OBJS)"
