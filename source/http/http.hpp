#include "../server.hpp"
#include <fstream>
#include <regex>
#include <sys/stat.h>

std::unordered_map<int, std::string> _statu_msg = {
    {100,  "Continue"},
    {101,  "Switching Protocol"},
    {102,  "Processing"},
    {103,  "Early Hints"},
    {200,  "OK"},
    {201,  "Created"},
    {202,  "Accepted"},
    {203,  "Non-Authoritative Information"},
    {204,  "No Content"},
    {205,  "Reset Content"},
    {206,  "Partial Content"},
    {207,  "Multi-Status"},
    {208,  "Already Reported"},
    {226,  "IM Used"},
    {300,  "Multiple Choice"},
    {301,  "Moved Permanently"},
    {302,  "Found"},
    {303,  "See Other"},
    {304,  "Not Modified"},
    {305,  "Use Proxy"},
    {306,  "unused"},
    {307,  "Temporary Redirect"},
    {308,  "Permanent Redirect"},
    {400,  "Bad Request"},
    {401,  "Unauthorized"},
    {402,  "Payment Required"},
    {403,  "Forbidden"},
    {404,  "Not Found"},
    {405,  "Method Not Allowed"},
    {406,  "Not Acceptable"},
    {407,  "Proxy Authentication Required"},
    {408,  "Request Timeout"},
    {409,  "Conflict"},
    {410,  "Gone"},
    {411,  "Length Required"},
    {412,  "Precondition Failed"},
    {413,  "Payload Too Large"},
    {414,  "URI Too Long"},
    {415,  "Unsupported Media Type"},
    {416,  "Range Not Satisfiable"},
    {417,  "Expectation Failed"},
    {418,  "I'm a teapot"},
    {421,  "Misdirected Request"},
    {422,  "Unprocessable Entity"},
    {423,  "Locked"},
    {424,  "Failed Dependency"},
    {425,  "Too Early"},
    {426,  "Upgrade Required"},
    {428,  "Precondition Required"},
    {429,  "Too Many Requests"},
    {431,  "Request Header Fields Too Large"},
    {451,  "Unavailable For Legal Reasons"},
    {501,  "Not Implemented"},
    {502,  "Bad Gateway"},
    {503,  "Service Unavailable"},
    {504,  "Gateway Timeout"},
    {505,  "HTTP Version Not Supported"},
    {506,  "Variant Also Negotiates"},
    {507,  "Insufficient Storage"},
    {508,  "Loop Detected"},
    {510,  "Not Extended"},
    {511,  "Network Authentication Required"}
};

std::unordered_map<std::string, std::string> _mime_msg = {
    {".aac",        "audio/aac"},
    {".abw",        "application/x-abiword"},
    {".arc",        "application/x-freearc"},
    {".avi",        "video/x-msvideo"},
    {".azw",        "application/vnd.amazon.ebook"},
    {".bin",        "application/octet-stream"},
    {".bmp",        "image/bmp"},
    {".bz",         "application/x-bzip"},
    {".bz2",        "application/x-bzip2"},
    {".csh",        "application/x-csh"},
    {".css",        "text/css"},
    {".csv",        "text/csv"},
    {".doc",        "application/msword"},
    {".docx",       "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot",        "application/vnd.ms-fontobject"},
    {".epub",       "application/epub+zip"},
    {".gif",        "image/gif"},
    {".htm",        "text/html"},
    {".html",       "text/html"},
    {".ico",        "image/vnd.microsoft.icon"},
    {".ics",        "text/calendar"},
    {".jar",        "application/java-archive"},
    {".jpeg",       "image/jpeg"},
    {".jpg",        "image/jpeg"},
    {".js",         "text/javascript"},
    {".json",       "application/json"},
    {".jsonld",     "application/ld+json"},
    {".mid",        "audio/midi"},
    {".midi",       "audio/x-midi"},
    {".mjs",        "text/javascript"},
    {".mp3",        "audio/mpeg"},
    {".mpeg",       "video/mpeg"},
    {".mpkg",       "application/vnd.apple.installer+xml"},
    {".odp",        "application/vnd.oasis.opendocument.presentation"},
    {".ods",        "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt",        "application/vnd.oasis.opendocument.text"},
    {".oga",        "audio/ogg"},
    {".ogv",        "video/ogg"},
    {".ogx",        "application/ogg"},
    {".otf",        "font/otf"},
    {".png",        "image/png"},
    {".pdf",        "application/pdf"},
    {".ppt",        "application/vnd.ms-powerpoint"},
    {".pptx",       "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar",        "application/x-rar-compressed"},
    {".rtf",        "application/rtf"},
    {".sh",         "application/x-sh"},
    {".svg",        "image/svg+xml"},
    {".swf",        "application/x-shockwave-flash"},
    {".tar",        "application/x-tar"},
    {".tif",        "image/tiff"},
    {".tiff",       "image/tiff"},
    {".ttf",        "font/ttf"},
    {".txt",        "text/plain"},
    {".vsd",        "application/vnd.visio"},
    {".wav",        "audio/wav"},
    {".weba",       "audio/webm"},
    {".webm",       "video/webm"},
    {".webp",       "image/webp"},
    {".woff",       "font/woff"},
    {".woff2",      "font/woff2"},
    {".xhtml",      "application/xhtml+xml"},
    {".xls",        "application/vnd.ms-excel"},
    {".xlsx",       "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml",        "application/xml"},
    {".xul",        "application/vnd.mozilla.xul+xml"},
    {".zip",        "application/zip"},
    {".3gp",        "video/3gpp"},
    {".3g2",        "video/3gpp2"},
    {".7z",         "application/x-7z-compressed"}
};
class Util
{

public:
        //字符串分割函数,将src字符串按照sep字符进行分割，得到的各个字串放到arry中，最终返回字串的数量
        static size_t Split(const std::string &src, const std::string &sep, std::vector<std::string> *arry){
               size_t offset=0;
               while(offset < src.size()){
               //在src字符串偏移量offset处，开始向后查找sep字符/字串，返回查找到的位置
                size_t pos=src.find(sep,offset);
                if(pos==std::string::npos){//没有找到特定的字符
                    //将剩余的部分当作一个字串，放入arry中
                    if(pos==src.size()) break;
                    arry->push_back(src.substr(offset));
                    return arry->size();
                }
                if(pos==offset){
                    offset+=pos + sep.size();
                    continue;//当前字串是一个空的，没有内容
                }
                 arry->push_back(src.substr(offset,pos-offset));
                 offset+=pos + sep.size();
            }               
          return arry->size();
        }
         //读取文件的所有内容，将读取的内容放到buf中
        static bool ReadFile(const std::string &filename, std::string *buf){
           std::ifstream ifs(filename,std::ios::binary);
           if(ifs.is_open()==false){
              ERR_LOG("OPEN %s FILE FAILED!!",filename);
              return false;
           }
          size_t fsize=0;
          ifs.seekg(0,ifs.end);
          fsize = ifs.tellg();
          ifs.seekg(0,ifs.beg);
          buf->resize(fsize);
          ifs.read(&(*buf)[0],fsize);
          if(ifs.good()==false){
            ERR_LOG("READ %s FILE FAILED!!",filename);
            ifs.close();
            return false;
          }
           ifs.close();
          return true;
        }
        //向文件写入数据
        static bool WriteFile(const std::string &filename, const std::string &buf){
             std::ofstream ofs(filename,std::ios::binary | std::ios::trunc);
             if(ofs.is_open()==false){
              ERR_LOG("OPEN %s FILE FAILED!!",filename);
              return false;
           }
           ofs.write(&(buf)[0],buf.size());
           if(ofs.good()==false){
            ERR_LOG("WRITE %s FILE FAILED!!",filename);
            ofs.close();
            return false;
          }
           ofs.close();
            return true;
        }
        //URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产生歧义
        //编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀%   C++ -> C%2B%2B
        //  不编码的特殊字符： RFC3986文档规定 . - _ ~ 字母，数字属于绝对不编码字符
        //RFC3986文档规定，编码格式 %HH 
        //W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
        static std::string UrlEncode(const std::string& url, bool convert_space_to_plus){
              std::string res;
              for(auto &c : url ){
                 if(c == '.' || c == '- ' || c=='_' || c=='~' ){
                   res+=c;
                   continue;
                 }
                 if (c == ' ' && convert_space_to_plus == true) {
                    res += '+';
                    continue;
                }
                char tmp[4]={0};
                snprintf(tmp,4,"%%%02X",c);
                res+=tmp;
              }
              return res;
        }
        static char HEXTOI(char c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }else if (c >= 'a' && c <= 'z') {
                return c - 'a' + 10;
            }else if (c >= 'A' && c <= 'Z') {
                return c - 'A' + 10;
            }
            return -1; 
        }
        static std::string UrlDecode(const std::string& url, bool convert_plus_to_space){
                std::string res;
            for(int i=0;i<url.size();i++){
              if(url[i]=='+' && convert_plus_to_space ==true){
                res+=' ';
                continue;
              }
              if(url[i]=='%' &&( i+2)<url.size()){
                 char v1=HEXTOI(url[i+1]);
                 char v2=HEXTOI(url[i+2]);
                 char v= v1+v2;
                  res+=v;
                  continue;
              }
              res +=url[i];
            }
            return res;
        }
         //响应状态码的描述信息获取
        static std::string StatuDesc(int statu){
           auto it =_statu_msg.find(statu);
           if(it!=_statu_msg.end()){
              return it->second;
           }
           return "Unknow";
        }       
         //根据文件后缀名获取文件mime
        static std::string ExtMime(const std::string &filename){
          size_t pos=filename.find_first_of('.');
          std::string ext=filename.substr(pos);
           auto it =_mime_msg.find(ext);
           if(it==_mime_msg.end()){
              return "application/octet-stream";
           }
           return it->second;
        }
        //判断一个文件是否是一个目录
        static bool IsDirectory(const std::string &filename){
           struct stat st;
           int ret =stat(filename.c_str(),&st);
           if(ret<0){
            return false;
           }
           return S_ISDIR(st.st_mode);
        }       
        //判断一个文件是否是一个普通文件
        static bool IsRegular(const std::string &filename){
           struct stat st;
           int ret =stat(filename.c_str(),&st);
           if(ret<0){
            return false;
           }
           return S_ISREG(st.st_mode);
        }
         //http请求的资源路径有效性判断
        // /index.html  --- 前边的/叫做相对根目录  映射的是某个服务器上的子目录
        // 想表达的意思就是，客户端只能请求相对根目录中的资源，其他地方的资源都不予理会
        // /../login, 这个路径中的..会让路径的查找跑到相对根目录之外，这是不合理的，不安全的
        static bool ValidPath(const std::string &path){
           int level=0;
           std::vector<std::string> subdir;
           Split(path,"/",&subdir);
           for(auto & dir :subdir){
             if(dir == ".."){
              level--;
              if(level < 0)  return false;
              continue;
             }
             level++;
           }
           return true;
        }
};
class HttpRequest {
   
  public:
        std::string _method;      //请求方法
        std::string _path;        //资源路径
        std::string _version;     //协议版本
        std::string _body;        //请求正文
        std::smatch _matches;     //资源路径的正则提取数据
        std::unordered_map<std::string, std::string> _headers;  //头部字段
        std::unordered_map<std::string, std::string> _params;   //查询字符串
  public:
   HttpRequest():_version("HTTP/1.1"){}
   void Reset(){
      _method.clear();
      _path.clear();
      _version.clear();
      _body.clear();
      std::smatch tmp;
      _matches.swap(tmp);
      _headers.clear();
      _params.clear();
   }
   //插入头部字段
   void SetHeader(const std::string &key,const std::string & val){
         _headers.insert(std::make_pair(key,val));
   }
   //判断是否存在指定头部字段
   bool HasHeader(const std::string &key)const{
     auto it =_headers.find(key);
     if(it==_headers.end()){
       return false;
      }
     return true;
   }
    //获取指定头部字段的值
   std::string GetHeader(const std::string &key)const{
      auto it =_headers.find(key);
      if(it==_headers.end()){
       return false;
      }
      return it->second;
   }
   //插入查询字符串
   void SetParam(const std::string &key,const std::string & val){
       _params.insert(std::make_pair(key,val));
   }
    //判断是否有某个指定的查询字符串
   bool HasParam(const std::string &key)const{
      auto it =_params.find(key);
     if(it==_params.end()){
       return false;
      }
     return true;
   }
    //获取指定的查询字符串
   std::string GetParam(const std::string &key)const{
     auto it =_params.find(key);
      if(it==_params.end()){
       return false;
      }
      return it->second;
   }
   //获取正文长度
   size_t ContentLenth()const{
            // Content-Length: 1234\r\n
            bool ret = HasHeader("Content-Length");
            if (ret == false) {
                return 0;
            }
            std::string clen = GetHeader("Content-Length");
            return std::stol(clen);
   }
   //判断是否是短链接
    bool Close() const {
      // 没有Connection字段，或者有Connection但是值是close，则都是短链接，否则就是长连接
       if(HasHeader("Connection")== true && GetHeader("Connection")=="keep-alive"){
         return false;
       }
       return true;
    }
};
class HttpResponse {
  public:
  public:
};
