#这个 Makefile 是一个用于多目录项目编译的构建脚本，主要用于指定编译工具、编译选项、构建流程和清理规则。

#?= 是 Makefile 的条件赋值运算符：如果CROSS_COMPILE变量未被定义过，则赋值为空（默认不使用交叉编译）。
#作用：用于指定交叉编译工具链的前缀（例如 ARM 架构可能设为arm-linux-gnueabihf-），为空时使用当前系统的原生工具链。

#作用：定义编译过程中需要用到的各种工具
#细节：CROSS_COMPILE ?= 使用条件赋值，允许用户通过命令行指定交叉编译工具链前缀（如 arm-linux-gnueabihf-）
#当不需要交叉编译时，这些变量会直接使用系统默认的工具（as、ld、gcc 等）
#包含了从汇编、编译、链接到后期处理（strip、objcopy 等）的全套工具

CROSS_COMPILE ?=
AS     =$(CROSS_COMPILE)as 	#汇编器（as），用于汇编代码编译。
LD     =$(CROSS_COMPILE)ld 	#链接器（ld），用于将目标文件链接为可执行文件。
CC     =$(CROSS_COMPILE)gcc #C 编译器（gcc），用于编译 C 代码。
CPP    =$(CC) -E 			#C 预处理器（gcc -E），用于预处理代码（展开宏、头文件等）。
AR     =$(CROSS_COMPILE)ar 	#归档工具（ar），用于创建静态库。
NM     =$(CROSS_COMPILE)nm 	#符号查看工具（nm），用于查看目标文件中的符号表。

STRIP  =$(CROSS_COMPILE)strip 		#符号剥离工具（strip），用于移除可执行文件中的调试符号，减小文件体积。
OBJCOPY  =$(CROSS_COMPILE)objcopy 	#目标文件转换工具（objcopy），用于转换目标文件格式（如 elf 转 bin）。
OBJDUMP  =$(CROSS_COMPILE)objdump 	#目标文件反汇编工具（objdump），用于查看目标文件的汇编代码。

#将这些工具变量导出为环境变量，使得在子目录的 Makefile 中也能访问到这些变量（支持递归编译）。
export AS LD CC CPP AR NM 
export STRIP OBJCOPY OBJDUMP




#开启所有警告（提醒代码中的潜在问题）。表示开启二级优化
CFLAGS  := -Wall -O2 -gcc
#shell：是 Makefile 的内置函数，用于执行 shell 命令
#pwd：是 Linux 系统自带的命令，用于获取当前工作目录的绝对路径
#补充编译选项，-I $(shell pwd)/include表示指定头文件搜索路径为当前目录下的include文件夹（$(shell pwd)获取当前目录绝对路径）。
CFLAGS  += -I $(shell pwd)/include

#指定需要链接的库
LDFLAGS := -lts -lpthread -lfreetype -lm

#将CFLAGS（编译选项）和LDFLAGS（链接选项）导出为环境变量，供子目录的 Makefile 使用。
export CFLAGS LDFLAGS




TOPDIR :=$(shell pwd)	#定义项目顶层目录为当前目录的绝对路
export TOPDIR			#用于将顶层目录路径导出，供子 Makefile 定位顶层资源。

TARGET :=test	#定义最终生成的可执行文件名为test。




#指定需要编译的子目录
obj-y += display/
obj-y += input/
obj-y += font/
obj-y += ui/
obj-y += page/	
obj-y += config/
obj-y += business/

#先执行 start_recursive_build 递归编译子目录，再生成目标文件，最后输出构建完成信息
@ 符号表示不显示命令本身，只显示命令执行结果
all : start_recursive_build $(TARGET)
	@echo $(TARGET) has been built! 

#启动递归编译过程
#make -C ./：在当前目录执行 make
#-f $(TOPDIR)/Makefile.build：指定使用顶层目录的 Makefile.build 作为构建规则文件
#这通常是一个辅助 Makefile，用于遍历 obj-y 中定义的子目录并编译其中的代码
start_recursive_build:
	make -C ./ -f $(TOPDIR)/Makefile.build

#链接生成最终可执行文件
#依赖 built-in.o（由子目录编译生成的汇总目标文件）
#使用之前定义的编译器和链接选项将目标文件链接为可执行文件
$(TARGET) :built-in.o
	$(CC) -o $(TARGET) built-in.o $(LDFLAGS)

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
	