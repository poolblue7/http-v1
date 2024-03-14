#include "../source/server.hpp"
void HandleClose(Channel *channel){
     DBG_LOG("close fd :%d",channel->Fd());
     channel->Remove();
     delete channel;
}
void HandleRead(Channel *channel){
    int fd=channel->Fd();
    char buf[1024]={0};
    printf("sockfd:%d\n",fd);
    int ret =recv(fd,buf,1023,0);
    if(ret <= 0){
        // 如果 recv 返回 0，表示对端关闭了连接
        // 如果返回值小于 0，则表示发生了错误
        return HandleClose(channel);
    }
    DBG_LOG("%s",buf);
    channel->EnableWrite();//启动可写事件
}
void HandleWrite(Channel *channel){
    int fd = channel->Fd();
    const char *data = "测试成功！";
    int ret = send(fd, data, strlen(data), 0);
    if(ret < 0){
        std::cerr << "send failed with error: " << strerror(errno) << std::endl;
        return HandleClose(channel); // 关闭释放
    }
    channel->DisableWrite(); // 关闭写监控
}

void HandleError(Channel *channel){
    return HandleClose(channel);//关闭释放
}
void HandleEvent(EventLoop *loop,Channel *channel,uint64_t timerid){
     loop->TimerRefresh(timerid);
}
void Accepter(EventLoop *loop,Channel *lst_channel){
    int fd =lst_channel->Fd();
    int newfd=accept(fd,NULL,NULL);
    if(newfd <0 ) {return ;}
    uint64_t timerid =rand()%10000;
    Channel *channel=new Channel(loop,newfd);
    channel->SetReadCallback(std::bind(HandleRead,channel));//为通信套接字设置可读事件的回调函数
    channel->SetWriteCallback(std::bind(HandleWrite,channel));//可写事件的回调函数
    channel->SetCloseCallback(std::bind(HandleClose,channel));//关闭事件回调函数
    channel->SetErrorCallback(std::bind(HandleError,channel));
    channel->SetEventCallback(std::bind(HandleEvent,loop,channel,timerid));
    //非活跃连接的超时释放操作,10s后关闭连接
    //注意：定时销毁任务，必须在启动读事件之前，因为有可能启动事件监控后，立即就有事件，但是这时候还没有任务
    loop->TimerAdd(timerid,10,std::bind(HandleClose,channel));
    channel->EnableRead();
}
int main()
{   
    srand(time(NULL));
    EventLoop loop;
    Socket lst_sock;
    lst_sock.CreateServer(8000);
    //为监控套接字，创建应该channel进行事件的管理，以及事件的处理
    Channel channel(&loop,lst_sock.Fd());
    //回调中，获取新连接，为新连接创建Channel并添加监控
    channel.SetReadCallback(std::bind(Accepter,&loop,&channel));
    channel.EnableRead();//启   动可读事件监控
    while(1){
     loop.Start();
    }
    lst_sock.Close();
    return 0;
}