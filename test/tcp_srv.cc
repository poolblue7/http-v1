#include "../source/server.hpp"


int main()
{
    Socket lst_sock;
    lst_sock.CreateServer(8000);
    while(1){
        int newfd=lst_sock.Accept();
        if(newfd<0){
            continue;
        }
        Socket cli_sock(newfd);
        char buf[1024]={0};
        int ret=cli_sock.Recv(buf,1023);
        if(ret<0){
            cli_sock.Close();
            continue;
        }
        cli_sock.Send(buf,ret);
        cli_sock.Close();
    }
    lst_sock.Close();
    return 0;
}