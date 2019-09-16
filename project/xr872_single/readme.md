  
网络测试程序:


demo主要完成：client多线程收发socket数据，server端受到数据后转发给client端
server 跑在pc 端，代码在pc 目录，server.c
linux 下编译方法: gcc server.c -o server 
运行方法：./server

client 代码 在工程 下 为 socket_test.c,编译前需修改main.c 中的 wifi ssid,psk,和 pc 端 serverip
char * sta_ssid = "ROKID.TC";
char * sta_psk = "rokidguys";
char * serverip = "10.88.133.46";
编译后烧入 xr872中，先运行pc 端 server,在启动xr872,正常情况下pc 端会持续打印收到的0 ~9 的数据帧。

异常现象：
网络收发一段时间后pc 端收不到数据，xr872侧串口输出
tcpip_thread: invalid message at line 155 in src/api/tcpip.c

