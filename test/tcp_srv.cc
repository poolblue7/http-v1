#include "../source/server.hpp"
void HandleClose(Channel *channel){
     std::cout<<"close: "<<channel->Fd()<<std::endl;
     channel->Remove();
     delete channel;
}
void HandleRead(Channel *channel){
    int fd=channel->Fd();
    char buf[1024]={0};
    int ret =recv(fd,buf,1023,0);
    if(ret <= 0){
        // 如果 recv 返回 0，表示对端关闭了连接
        // 如果返回值小于 0，则表示发生了错误
        std::cerr << "recv failed with error: " << strerror(errno) << std::endl;
        return HandleClose(channel);
    }
    std::cout<<buf<<std::endl;
    channel->EnableWrite();//启动可写事件
}
void HandleWrite(Channel *channel){
    int fd = channel->Fd();
    const char *data = "测试成功！";
    int ret = send(fd, data, strlen(data), 0);
    if(ret < 0){
        std::cerr << "send failed with error: " << strerror(errno) << std::endl;
        return HandleClose(channel); // 关闭释放
    } else {
        std::cout << "发送成功" << std::endl;
        std::cout.flush(); // 确保立即输出
    }
    channel->DisableWrite(); // 关闭写监控
}

void HandleError(Channel *channel){
    return HandleClose(channel);//关闭释放
}
void HandleEvent(Channel *channel){
    std::cout<<"有一个事件到来"<<std::endl;
}
void Accepter(Poller *poller,Channel *lst_channel){
    int fd =lst_channel->Fd();
    int newfd=accept(fd,NULL,NULL);
    if(newfd <0 )return ;
    Channel *channel=new Channel(poller,newfd);
    channel->SetReadCallback(std::bind(HandleRead,channel));//为通信套接字设置可读事件的回调函数
    channel->SetWriteCallback(std::bind(HandleWrite,channel));//可写事件的回调函数
    channel->SetCloseCallback(std::bind(HandleClose,channel));//关闭事件回调函数
    channel->SetErrorCallback(std::bind(HandleError,channel));
    channel->SetEventCallback(std::bind(HandleEvent,channel));
    channel->EnableRead();
}
int main()
{   
    Poller poller;
    Socket lst_sock;
    lst_sock.CreateServer(8000);
    //为监控套接字，创建应该channel进行事件的管理，以及事件的处理
    Channel channel(&poller,lst_sock.Fd());
    //回调中，获取新连接，为新连接创建Channel并添加监控
    channel.SetReadCallback(std::bind(Accepter,&poller,&channel));
    channel.EnableRead();//启动可读事件监控
    while(1){
      std::vector<Channel*> actives;
      poller.Poll(&actives);
      for(auto &a :actives){
        a->HandleEvent();//开始监控
      }
    }
    lst_sock.Close();
    return 0;
}