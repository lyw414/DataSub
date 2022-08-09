SRC_FILE = DataSub.cpp

INC_DIR = -I ./
INC_DIR += -I DataSubFunc
INC_DIR += -I SimpleMemoryPool
INC_DIR += -I RWBlockArray
INC_DIR += -I Protocol
INC_DIR += -I Hash
INC_DIR += -I ProClientTable
INC_DIR += -I TaskPool 
INC_DIR += -I ProSubTable


FLAGS = -lpthread

all:
		g++ -g -std=c++98 -o Test test.cpp ${SRC_FILE} ${INC_DIR} ${FLAGS}
