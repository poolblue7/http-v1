#include "../source/server.hpp"


void OnMessage(const PtrConnection &conn,Buffer *buf){
   DBG_LOG("%s",buf->ReadPosition());
   buf->MoveReadOffset(buf->ReadAbleSize());
   std::string str="Hello World";
   conn->Send(str.c_str(),str.size());

}

void OnClosed(const PtrConnection &conn){
    DBG_LOG("CLOSE CONNECTION:%p",conn.get());
}

void OnConnected(const PtrConnection &conn){
    DBG_LOG("NEW CONNECTION:%p",conn.get());
}

int main(){

    TcpServer server(8000);
    server.SetThreadCount(2);
    server.SetConnectedCallback(OnConnected);
    server.SetMessageCallback(OnMessage);
    server.SetClosedCallback(OnClosed);
    server.EnableInactiveRelease(10);
    server.Start();
    return 0;
}