#include "../source/server.hpp"
std::unordered_map<uint64_t,PtrConnection> _conns;
uint64_t conn_id =0;
void ConnectionDestory(const PtrConnection &conn){
    _conns.erase(conn->Id());
}
void OnMessage(const PtrConnection &conn,Buffer *buf){
   DBG_LOG("%s",buf->ReadPosition());
   buf->MoveReadOffset(buf->ReadAbleSize());
   std::string str="Hello World";
   conn->Send(str.c_str(),str.size());
   
}



void OnConnected(const PtrConnection &conn){
    DBG_LOG("NEW CONNECTION:%p",conn.get());
}
void Accepter(EventLoop *loop,Channel *lst_channel){
    int fd =lst_channel->Fd();
    int newfd=accept(fd,NULL,NULL);
    if(newfd <0 ) {return ;}
    conn_id++;
    PtrConnection conn(new Connection(loop,conn_id,newfd));
    conn->SetMessageCallback(std::bind(OnMessage,std::placeholders::_1,std::placeholders::_2));
    conn->SetConnectedCallback(std::bind(OnConnected,std::placeholders::_1));
    conn->SetSrvClosedCallback(std::bind(ConnectionDestory,std::placeholders::_1));
    conn->EnableInactiveRelease(10);//启动非活跃超时销毁
    conn->Established();//就绪初始化
    _conns.insert(std::make_pair(conn_id,conn));
    
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