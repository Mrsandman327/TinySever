#区分系统
ifeq ($(OS),Windows_NT) #Windows stuff 
DEL = del
APP = tinyserver.exe
LIB = -lpthread -lws2_32
else #Linux stuff
APP = tinyserver
DEL = rm -rf
LIB = -lpthread
endif
#设置编译器
CC = g++
# 如果是当前目录，也可以不指定
BUILD_DIR = .\bin
SOURCE_DIR = .\TinySever
OBJ_DIR = .\obj
TARGET = ${BUILD_DIR}\${APP}
#获取当前目录下所有的.c .cpp文件
CUR_C_SOURCE = ${wildcard $(SOURCE_DIR)\*.c}
CUR_CPP_SOURCE = ${wildcard $(SOURCE_DIR)\*.cpp}
#将.c .cpp 文件的后缀换成.o。基本语法是：$(patsubst 原模式，目标模式，文件列表)# notdir去除路径只保留文件名 addprefix添加前缀
CUR_C_OBJS = $(addprefix $(OBJ_DIR)\, $(patsubst %.c, %.o, $(notdir $(CUR_C_SOURCE))))
CUR_CPP_OBJS = $(addprefix $(OBJ_DIR)\, $(patsubst %.cpp, %.o, $(notdir $(CUR_CPP_SOURCE))))
#显示
target:
	@echo $(CUR_C_SOURCE)
	@echo $(CUR_CPP_SOURCE)
	@echo $(CUR_C_OBJS)
	@echo $(CUR_CPP_OBJS)

all:${TARGET}
#生成带GBD调试选项的可执行文件
${TARGET}:$(CUR_C_OBJS) $(CUR_CPP_OBJS)
	$(CC) -g -std=c++11 $^ -o $@ $(LIB)
#编译所有.c文件
$(CUR_C_OBJS):$(OBJ_DIR)\%.o:$(SOURCE_DIR)\%.c
	$(CC) -g -c -std=c++11 $^ -o $@
#编译所有.cpp文件
$(CUR_CPP_OBJS):$(OBJ_DIR)\%.o:$(SOURCE_DIR)\%.cpp
	$(CC) -g -c -std=c++11 $^ -o $@
	
clean:
	$(DEL) $(OBJ_DIR)\*.o

clear:
	$(DEL) $(OBJ_DIR)\*.o ${TARGET}