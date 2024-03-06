#include <iostream>
#include <vector>

#define BUFFER_DEFAULT_SPACE 1024
class Buffer
{
private:
    std::vector<char> _buffer;//使用vector进行内存空间管理
    uint64_t _reader_idx;//读偏移
    uint64_t _writer_idx;//写偏移
public:
    Buffer(): _reader_idx(0),_writer_idx(0),_buffer(BUFFER_DEFAULT_SPACE) {}
    ~Buffer();
    //获取当前写入起始位置
    //获取当前读取起始位置
    //获取缓冲器末尾空闲位置
    //获取缓冲器起始空闲位置
    //获取可读取数据大小
    //将读偏移向后移动
    //将写偏移向后移动
    //确保整体缓冲区有空闲位置，有则将可读数据向前移动，无则扩容
    //清空数据
};

