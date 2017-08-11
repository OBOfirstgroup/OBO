
CC=gcc
CPPFLAGS= -I./include 
CFLAGS=-Wall 
LIBS=-lpthread -levent -lm -lssl -lcrypto -levent_openssl

#找到当前目录下所有的.c文件
src = $(wildcard src/*.c)

#将当前目录下所有的.c  转换成.o给obj
obj = $(patsubst src/%.c, obj/%.o, $(src))


server=server

target=$(server)


ALL:$(target)


#生成所有的.o文件
$(obj):obj/%.o:src/%.c
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS) 


#test_main程序
#$(server): https-server.o https-common.o cJSON.o persistent.o cache.o
#	$(CC) $^ -o $@ $(LIBS)

$(server): $(obj)
	$(CC) $^ -o $@ $(LIBS)




#clean指令

clean:
	-rm -f obj/$(obj) $(target) 

distclean:
	-rm -f obj/$(obj) $(target) 

#将clean目标 改成一个虚拟符号
.PHONY: clean ALL distclean
