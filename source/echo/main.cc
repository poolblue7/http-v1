#include "echo.hpp"

int main()
{
    EchoServer server(8000);
    server.Start();
    return 0;
}