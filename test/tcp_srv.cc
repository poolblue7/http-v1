#include "../source/server.hpp"
std::unordered_map<uint64_t,PtrConnection> _conns;
EventLoop base_loop;
uint64_t conn_id =0;
LoopThreadPool * loop_pool;
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
void NewConnection(int fd){
    conn_id++;
    PtrConnection conn(new Connection(loop_pool->NextLoop(),conn_id,fd));
    conn->SetMessageCallback(std::bind(OnMessage,std::placeholders::_1,std::placeholders::_2));
    conn->SetConnectedCallback(std::bind(OnConnected,std::placeholders::_1));
    conn->SetSrvClosedCallback(std::bind(ConnectionDestory,std::placeholders::_1));
    conn->EnableInactiveRelease(10);//启动非活跃超时销毁
    conn->Established();//就绪初始化
    _conns.insert(std::make_pair(conn_id,conn));
    DBG_LOG("主线程--------------");
    
}
int main()
{   
    srand(time(NULL));
    loop_pool=new LoopThreadPool(&base_loop);
    loop_pool->SetThreadCount(2);
    loop_pool->Create();
    Socket lst_sock;
    Acceptor acceptor(&base_loop,8000);
    
    //回调中，获取新连接，为新连接创建Channel并添加监控
    acceptor.SetAcceptCallback(std::bind(NewConnection,std::placeholders::_1));
    acceptor.Listen();
     base_loop.Start();
 
    return 0;
}