#include "server.hpp"

int main(){

    Buffer buf;
    for(int i=0;i<30;i++){
       std::string str="hello! "+std::to_string(i)+'\n';
       buf.WriteStringAndPush(str);
       //std::cout<<buf.ReadAsStringAndPop(buf.ReadAbleSize())<<std::endl;
    }
    while(buf.ReadAbleSize()>0){
      
        std::string tmp=buf.GetLineAndPop();
        //std::cout<<tmp<<std::endl;
       INF_LOG("hello");
    }
   /*
      std::string str("hello!");

    buf.WriteStringAndPush(str);
   
   Buffer buf1;
   buf1.WriteBufferAndPush(buf);
   std::string tmp =buf1.ReadAsStringAndPop(buf1.ReadAbleSize());
   std::cout<<tmp<<std::endl;

    auto s=buf.ReadAsStringAndPop(buf.ReadAbleSize());

    std::cout<<s<<std::endl;
    */

}