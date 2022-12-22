SRC_FILE = DataSubAPI.cpp

INC_DIR = -I ./
INC_DIR += -I ../../include
INC_DIR += -I DataSub 
INC_DIR += -I Hash
INC_DIR += -I SubTable
INC_DIR += -I ProTrans
INC_DIR += -I TaskPool


FLAGS = -lpthread

all:
		g++ -g -std=c++98 -o Test test.cpp ${SRC_FILE} ${INC_DIR} ${FLAGS}
