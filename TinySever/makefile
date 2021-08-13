#设置输出文件
APP = tinyserver
#设置编译器
CC = g++
# 如果是当前目录，也可以不指定
SOURCE_DIR = ./									
#获取当前目录下所有的.c .cpp文件
CUR_C_SOURCE = ${wildcard *.c}
CUR_CPP_SOURCE = ${wildcard *.cpp}
#将.c .cpp 文件的后缀换成.o。基本语法是：$(patsubst 原模式，目标模式，文件列表)#
CUR_C_OBJS = ${patsubst %.c, %.o, $(CUR_C_SOURCE)}
CUR_CPP_OBJS = ${patsubst %.cpp, %.o, $(CUR_CPP_SOURCE)}
#显示
target:											
	@echo $(CUR_C_SOURCE)
	@echo $(CUR_CPP_SOURCE)
	@echo $(CUR_C_OBJS)
	@echo $(CUR_CPP_OBJS)

all:${APP}
#生成带GBD调试选项的可执行文件
${APP}:$(CUR_C_OBJS) $(CUR_CPP_OBJS)
	$(CC) -g -std=c++11 $^ -o $@ -lpthread
#编译所有.c文件
$(CUR_C_OBJS):%.o:%.c
	$(CC) -c -std=c++11 $^ -o $@
#编译所有.cpp文件
$(CUR_CPP_OBJS):%.o:%.cpp
	$(CC) -c -std=c++11 $^ -o $@

clean:
	rm -rf *.o 
#rm -rf *.o ${APP}