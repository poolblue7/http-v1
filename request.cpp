#include <iostream>
#include <regex>
#include <string>

int main()
{
    //HTTP请求行格式
    std::string str ="GET/https://login?user=yjl&pass=123123 HTTP/1.1\r\n";
    std::smatch matchs;

    //请求方法的匹配
    std::regex e("(GET|HEAD|POST|PUT|DELETE).*");
    bool ret=std::regex_match(str,matchs,e);
    if(ret==false){
        return -1;
    }
    for(auto &s:matchs){
        std::cout<< s<<std::endl;
    }
    return 0;
}