#include "socket_server.h"
//gcc test.c socket_server.c -l pthread -g -o test
#include <sys/socket.h>
#include <pthread.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
 
int c;
static void * client_send(struct socket_server * ss) {
    char *data = (char *) malloc(sizeof(char) * 20);
    // //char *data[20];
    memcpy(data, "hello world", 20);
    struct socket_sendbuffer buf;
    buf.id = c;
    buf.type = SOCKET_BUFFER_RAWPOINTER;
    buf.buffer = data;
    buf.sz = strlen(data);
    // //socket_server_send(ss, buf, data, strlen(data));              // 发送数据
    printf("test send\n");
    socket_server_send(ss, &buf);
    printf("test after_send\n");
}

static void * server_send(struct socket_server * ss, int id) {
    char *data = (char *) malloc(sizeof(char) * 20);
    // //char *data[20];
    memcpy(data, "hello world client", 20);
    struct socket_sendbuffer buf;
    buf.id = id;
    buf.type = SOCKET_BUFFER_RAWPOINTER;
    buf.buffer = data;
    buf.sz = strlen(data);
    // //socket_server_send(ss, buf, data, strlen(data));              // 发送数据
    printf("test server_send\n");
    socket_server_send(ss, &buf);
    printf("test after_server_send\n");
}

static void * continue_send(void * ud){
    struct socket_server * ss = ud;
    for(;;){
        client_send(ss);
        sleep(3);
    }
}

static void *
_poll(void * ud) {
    printf("test _poll\n");
    struct socket_server *ss = ud;
    struct socket_message result;
    for (;;) {
        int type = socket_server_poll(ss, &result, NULL);
        // DO NOT use any ctrl command (socket_server_close , etc. ) in this thread.
        switch (type) {
            case SOCKET_EXIT:
                return NULL;
            case SOCKET_DATA:
                printf("test message(%lu) [id=%d] size=%d data= %s\n",result.opaque,result.id, result.ud, result.data);
                free(result.data);
                break;
            case SOCKET_CLOSE:
                printf("test close(%lu) [id=%d]\n",result.opaque,result.id);
                break;
            case SOCKET_OPEN:
                printf("test open(%lu) [id=%d] %s\n",result.opaque,result.id,result.data);
                if (result.id==c) {
                    //pthread_t pid;
                    //pthread_create(&pid, NULL, continue_send, ss);
                    client_send(ss);
                }
                break;
            case SOCKET_ERR:
                printf("test error(%lu) [id=%d]\n",result.opaque,result.id);
                break;
            case SOCKET_ACCEPT:
                printf("test accept(%lu) [id=%d %s] from [%d]\n",result.opaque, result.ud, result.data, result.id);
                socket_server_start(ss,200,result.ud);
                server_send(ss, result.ud);
                break;
        }
        printf("ggg\n");
    }
    printf("poll_end\n");
}
 
static void
test(struct socket_server *ss) {
    //pthread_t pid;
    //pthread_create(&pid, NULL, _poll, ss);
 
    /*
    int c = socket_server_connect(ss,100,"127.0.0.1",80);
    printf("connecting %d\n",c);
    */
    int l = socket_server_listen(ss,200,"127.0.0.1",8888,32);       // 使用 127.0.0.1:8888 开启TCP监听
    printf("test listening %d\n",l);
    socket_server_start(ss,200,l);                      // 让epoll监听该TCP
    //int b = socket_server_bind(ss,300,1);                   // 让epoll监听标准输出
    //printf("binding stdin %d\n",b);
    //int i;
 
    printf("test client_socket\n");
    c = socket_server_connect(ss, 400, "127.0.0.1", 8888);          // 异步连接 127.0.0.1:8888
    //sleep(10);//马上发
    printf("test client_connected %d\n", c);
    

    _poll(ss);
    /*
    for (i=0;i<100;i++) {
        socket_server_connect(ss, 400+i, "127.0.0.1", 8888);
    }
    
    socket_server_exit(ss);
    */  
    //pthread_join(pid, NULL); 
}
 
int
main() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
 
    struct socket_server * ss = socket_server_create(0);
    test(ss);
    socket_server_release(ss);
 
    return 0;
}