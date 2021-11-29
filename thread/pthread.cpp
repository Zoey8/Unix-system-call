#include <cstdio>
#include <pthread.h>

void* myThread1(void* args){
    printf("This is %s\n", (char*) args);
    /**
     * pthread_exit函数将退出当前线程
     * 第1个参数为线程退出之后的返回值
     */
    pthread_exit(nullptr);
}

void* myThread2(void* args){
    printf("This is %s\n", (char*) args);
    pthread_exit(nullptr);
}

int start(){
    pthread_t id1, id2;
    char c1[] = "thread1", c2[] = "thread2";
    /**
     * pthread_create函数将创建一个线程，创建成功将返回0
     * 第1个参数返回一个绑定特定函数的线程ID
     * 第2个参数为线程属性对象指针，用来改变线程的属性
     * 第3个参数为线程运行的函数指针，被调用的函数必须返回空指针，且只能有一个空指针参数
     * 第4个参数为传递给被调用函数的参数
     */
    if(pthread_create(&id1, nullptr, myThread1, c1) != 0){
        printf("Create pthread error!\n");
        return 1;
    }
    if(pthread_create(&id2, nullptr, myThread2, c2) != 0){
        printf("Create pthread error!\n");
        return 1;
    }
    /**
     * pthread_join函数为让当前线程以阻塞方式等待指定线程结束
     * 第1个参数等待的线程id
     * 第2个参数为线程退出之后的返回值
     */
    pthread_join(id1, nullptr);
    pthread_join(id2, nullptr);
    printf("all thread done!\n");
    return 0;
}

int main(){
    start();
    return 0;
}