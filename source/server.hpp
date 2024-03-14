#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <mutex>
#include <thread>

#define INF 0
#define DBG 1
#define ERR 2
#define LOG_LEVEL INF

#define LOG(level, format, ...)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        if (level < LOG_LEVEL)                                                                                         \
            break;                                                                                                     \
        time_t t = time(NULL);                                                                                         \
        struct tm *ltm = localtime(&t);                                                                                \
        char tmp[32] = {0};                                                                                            \
        strftime(tmp, 31, "%H:%M:%S", ltm);                                                                            \
        fprintf(stdout, "[%p %s %s:%d] " format "\n", (void *)pthread_self(), tmp, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__)
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__)
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__)

#define BUFFER_DEFAULT_SPACE 1024
class Buffer
{
private:
    std::vector<char> _buffer; // 使用vector进行内存空间管理
    uint64_t _reader_idx;      // 读偏移
    uint64_t _writer_idx;      // 写偏移
public:
    Buffer() : _reader_idx(0), _writer_idx(0), _buffer(BUFFER_DEFAULT_SPACE) {}
    char *Begin() { return &*_buffer.begin(); }
    // 获取当前写入起始位置=buffer的起始位置+写偏移量
    char *WritePosition() { return Begin() + _writer_idx; }
    // 获取当前读取起始位置=buffer的起始位置+读偏移量
    char *ReadPosition() { return Begin() + _reader_idx; }
    // 获取缓冲器末尾空闲空间大小--写偏移后的空闲空间
    uint64_t TailIdleSize() { return _buffer.size() - _writer_idx; }
    // 获取缓冲器起始空闲空间大小--读偏移前的空闲空间
    uint64_t HeadIdleSize() { return _reader_idx; }
    // 获取可读取数据大小
    uint64_t ReadAbleSize() { return _writer_idx - _reader_idx; }
    // 将读偏移向后移动
    void MoveReadOffSet(uint64_t len)
    {
        // 向后移动的大小必须小于可读数据大小
        assert(len <= ReadAbleSize());
        _reader_idx += len;
    }
    // 将写偏移向后移动
    void MoveWriteOffSet(uint64_t len)
    {
        // 向后移动的大小必须小于缓存器末尾的空闲空间大小
        assert(len <= TailIdleSize());
        _writer_idx += len;
    }
    // 确保可写空闲空间足够，有则将可读数据向前移动，无则扩容
    void EnsureWriteSpace(uint64_t len)
    {
        // 如果缓冲器末尾位置足够，直接返回
        if (len <= TailIdleSize())
            return;
        // 末尾位置不够，则判断整体空闲空间是否足够，不够则扩容
        if (len <= HeadIdleSize() + TailIdleSize())
        {
            // 获取可读数据大小
            uint64_t rsz = ReadAbleSize();
            // 将可读数据拷贝到起始位置
            std::copy(ReadPosition(), ReadPosition() + rsz, Begin());
            _reader_idx = 0;   // 读偏移置为0
            _writer_idx = rsz; // 写偏移为可读数据的大小
        }
        else
        {
            // 末尾空间不够，扩容
            _buffer.resize(_writer_idx + len);
        }
    }
    // 写入数据
    void Write(const void *data, uint64_t len)
    {
        // 1.保证有足够空间
        if (len == 0)
            return;
        EnsureWriteSpace(len);
        // 2. 拷贝数据
        const char *d = (const char *)data;
        std::copy(d, d + len, WritePosition());
    }
    // 写入数据并推入数据
    void WriteAndPush(const void *data, uint64_t len)
    {
        Write(data, len);
        MoveWriteOffSet(len);
    }
    void WriteString(const std::string &data)
    {
        Write(data.c_str(), data.size());
    }
    void WriteStringAndPush(const std::string &data)
    {
        WriteString(data);
        MoveWriteOffSet(data.size());
    }
    void WriteBuffer(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
    }
    void WriteBufferAndPush(Buffer &data)
    {
        Write(data.ReadPosition(), data.ReadAbleSize());
        MoveWriteOffSet(data.ReadAbleSize());
    }
    // 读取数据
    void Read(void *buf, uint64_t len)
    {
        // 保证读取数据小于等于可读数据大小
        assert(len <= ReadAbleSize());
        std::copy(ReadPosition(), ReadPosition() + len, (char *)buf);
    }
    // 读取数据并弹出数据
    void ReadAndPop(void *buf, uint64_t len)
    {
        Read(buf, len);
        MoveReadOffSet(len);
    }
    std::string ReadAsString(uint64_t len)
    {
        assert(len <= ReadAbleSize());
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }
    std::string ReadAsStringAndPop(uint64_t len)
    {
        assert(len <= ReadAbleSize());
        std::string str = ReadAsString(len);
        MoveReadOffSet(len);
        return str;
    }
    // 获取换行符位置
    char *FindCRLF()
    {
        char *s = (char *)memchr(ReadPosition(), '\n', ReadAbleSize());
        return s;
    }
    // 获取一行字符串
    std::string GetLine()
    {
        char *pos = FindCRLF();
        if (pos == NULL)
        {
            return "";
        }
        // 位置+1为了把换行字符也取出来
        return ReadAsString(pos - ReadPosition() + 1);
    }
    std::string GetLineAndPop()
    {
        std::string str = GetLine();
        MoveReadOffSet(str.size());
        return str;
    }
    // 清空缓冲区
    void clear()
    {
        _reader_idx = 0;
        _writer_idx = 0;
    }
};
#define MAX_LISTEN 1024
class Socket
{
private:
    int _sockfd;

public:
    Socket() : _sockfd(-1) {}
    Socket(int fd) : _sockfd(fd) {}
    ~Socket() { Close(); }
    int Fd() { return _sockfd; }
    // 创建套接字
    bool Create()
    {
        // int socket(int domain, int type, int protocol)
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd < 0)
        {
            ERR_LOG("CREATE SOCKET FAILED!!");
            return false;
        }
        return true;
    }
    // 绑定地址信息
    bool Bind(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        socklen_t len = sizeof(struct sockaddr_in);
        // int bind(int sockfd, struct sockaddr*addr, socklen_t len);
        int ret = bind(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("BIND ADDRESS FAILED!");
            return false;
        }
        return true;
    }
    // 开始监听
    bool Listen(int backlog = MAX_LISTEN)
    {
        // int listen(int backlog)
        int ret = listen(_sockfd, backlog);
        if (ret < 0)
        {
            ERR_LOG("SOCKET LISTEN FAILED!");
            return false;
        }
        return true;
    }
    // 向服务器发起连接
    bool Connect(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        socklen_t len = sizeof(struct sockaddr_in);
        // int connect(int sockfd, struct sockaddr*addr, socklen_t len);
        int ret = connect(_sockfd, (struct sockaddr *)&addr, len);
        if (ret < 0)
        {
            ERR_LOG("CONNECT SERVER FAILED!");
            return false;
        }
        return true;
    }
    // 获取新连接
    int Accept()
    {
        // int accept(int sockfd, struct sockaddr *addr, socklen_t *len);
        int newfd = accept(_sockfd, NULL, NULL);
        if (newfd < 0)
        {
            ERR_LOG("SOCKET ACCEPT FAILED!");
            return -1;
        }
        return newfd;
    }
    // 接收数据
    ssize_t Recv(void *buf, size_t len, int flag = 0)
    {
        // ssize_t recv(int sockfd, void *buf, size_t len, int flag);
        ssize_t ret = recv(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            // EAGAIN 当前socket的接收缓冲区中没有数据了，在非阻塞的情况下才会有这个错误
            // EINTR  表示当前socket的阻塞等待，被信号打断了，
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0; // 表示这次接收没有接收到数据
            }
            ERR_LOG("SOCKET RECV FAILED!!");
            return -1;
        }
        return ret; // 实际接收的数据长度
    }
    ssize_t NonBlockRecv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT 表示当前接收为非阻塞。
    }
    // 发送数据
    ssize_t Send(const void *buf, size_t len, int flag = 0)
    {
        // ssize_t send(int sockfd, void *data, size_t len, int flag);
        ssize_t ret = send(_sockfd, buf, len, flag);
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            ERR_LOG("SOCKET SEND FAILED!!");
            return -1;
        }
        return ret; // 实际发送的数据长度
    }
    ssize_t NonBlockSend(void *buf, size_t len)
    {
        if (len == 0)
            return 0;
        return Send(buf, len, MSG_DONTWAIT); // MSG_DONTWAIT 表示当前发送为非阻塞。
    }
    // 关闭套接字
    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }
    // 创建一个服务端连接
    bool CreateServer(uint16_t port, const std::string &ip = "0.0.0.0", bool block_flag = false)
    {
        // 1. 创建套接字，2. 绑定地址，3. 开始监听，4. 设置非阻塞， 5. 启动地址重用
        if (Create() == false)
            return false;
        if (block_flag)
            NonBlock();
        if (Bind(ip, port) == false)
            return false;
        if (Listen() == false)
            return false;
        ReuseAddress();
        return true;
    }
    // 创建一个客户端连接
    bool CreateClient(uint16_t port, const std::string &ip)
    {
        // 1. 创建套接字，2.指向连接服务器
        if (Create() == false)
            return false;
        if (Connect(ip, port) == false)
            return false;
        return true;
    }
    // 设置套接字选项---开启地址端口重用
    void ReuseAddress()
    {
        // int setsockopt(int fd, int leve, int optname, void *val, int vallen)
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(int));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(int));
    }
    // 设置套接字阻塞属性-- 设置为非阻塞
    void NonBlock()
    {
        // int fcntl(int fd, int cmd, ... /* arg */ );
        int flag = fcntl(_sockfd, F_GETFL, 0);
        fcntl(_sockfd, F_SETFL, flag | O_NONBLOCK);
    }
};

class Poller;
class EventLoop;
class Channel
{
private:
    int _fd;
    EventLoop *_loop;
    uint32_t _events;  // 当前需要监控的事件
    uint32_t _revents; // 当前连接触发的事件
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;  // 可读事件被触发的回调函数
    EventCallback _write_callback; // 可写事件被触发的回调函数
    EventCallback _error_callback; // 错误事件被触发的回调函数
    EventCallback _close_callback; // 连接断开事件被触发的回调函数
    EventCallback _event_callback; // 任意事件被触发的回调函数
public:
    Channel(EventLoop *loop, int fd) : _fd(fd), _events(0), _revents(0), _loop(loop) {}
    int Fd() { return _fd; }
    uint32_t Events() { return _events; }                   // 获取需要监控的事件
    void SetREvents(uint32_t events) { _revents = events; } // 设置当前描述符就绪的事件
    void SetReadCallback(const EventCallback &cb) { _read_callback = cb; }
    void SetWriteCallback(const EventCallback &cb) { _write_callback = cb; }
    void SetErrorCallback(const EventCallback &cb) { _error_callback = cb; }
    void SetCloseCallback(const EventCallback &cb) { _close_callback = cb; }
    void SetEventCallback(const EventCallback &cb) { _event_callback = cb; }
    // 当前是否监控了可读
    bool ReadAble() { return (_events & EPOLLIN); }
    // 当前是否监控了可写
    bool WriteAble() { return (_events & EPOLLOUT); }
    // 启动读事件监控
    void EnableRead()
    {
        _events |= EPOLLIN;
        Update();
    }
    // 启动写事件监控
    void EnableWrite()
    {
        _events |= EPOLLOUT;
        Update();
    }
    // 关闭读事件监控
    void DisableRead()
    {
        _events &= ~EPOLLIN;
        Update();
    }
    // 关闭写事件监控
    void DisableWrite()
    {
        _events &= ~EPOLLOUT;
        Update();
    }
    // 关闭所有事件监控
    void DisableAll()
    {
        _events = 0;
        Update();
    }
    // 移除监控
    void Remove();
    void Update();
    // 事件处理，一旦连接触发了事件就调用函数，触发什么事件由自己决定
    void HandleEvent()
    {
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI))
        {
            /*不管任何事件，都调用的回调函数，事件处理完调用，刷新活跃度*/

            if (_event_callback)
                _event_callback();
            if (_read_callback)
                _read_callback();
        }
        // 有可能会释放连接的操作时间，一次只处理一个
        if (_revents & EPOLLOUT)
        {

            /*不管任何事件，都调用的回调函数，事件处理完调用，刷新活跃度*/
            if (_event_callback)
                _event_callback(); // 第二个有事件到来，是这个被执行了，证明_revents上的EPOLLOUT事件响应了，阻塞在了_write_callback回调上面
            if (_write_callback)
                _write_callback();
        }
        else if (_revents & EPOLLERR)
        {
            // 一旦出错，没必要调用任意回调，需要在前面调用任意回调
            if (_event_callback)
                _event_callback();
            if (_error_callback)
                _error_callback();
        }
        else if (_revents & EPOLLHUP)
        {
            if (_event_callback)
                _event_callback();
            if (_close_callback)
                _close_callback();
        }
    }
};

#define MAX_EPOLLEVENTS 1024
class Poller
{
private:
    int _epfd;
    struct epoll_event _evs[MAX_EPOLLEVENTS];
    std::unordered_map<int, Channel *> _channels;

private:
    // 对epoll的直接操作
    void Update(Channel *channel, int op)
    {
        // int epoll_ctl(int epfd, int op,  int fd,  struct epoll_event *ev);
        int fd = channel->Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = channel->Events();
        int ret = epoll_ctl(_epfd, op, fd, &ev);
        if (ret < 0)
        {
            ERR_LOG("EPOLLCTL FAILED!");
        }
        return;
    }
    // 判断一个Channel是否已经添加了事件监控
    bool HasChannel(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it == _channels.end())
        {
            return false;
        }
        return true;
    }

public:
    Poller()
    {
        _epfd = epoll_create(256);
        if (_epfd < 0)
        {
            ERR_LOG("EPOLL CREATE FAILED!!");
            abort(); // 退出程序
        }
    }
    // 添加或修改监控事件
    void UpdateEvent(Channel *channel)
    {
        bool ret = HasChannel(channel);
        if (ret == false)
        {
            // 不存在则添加
            _channels.insert(std::make_pair(channel->Fd(), channel));
            return Update(channel, EPOLL_CTL_ADD);
        }

        return Update(channel, EPOLL_CTL_MOD);
    }
    // 移除监控
    void RemoveEvent(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it != _channels.end())
        {
            _channels.erase(it);
        }
        Update(channel, EPOLL_CTL_DEL);
    }
    // 开始监控，返回活跃连接
    void Poll(std::vector<Channel *> *active)
    {
        // int epoll_wait(int epfd, struct epoll_event *evs, int maxevents, int timeout)
        int nfds = epoll_wait(_epfd, _evs, 128, -1);
        if (nfds < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            ERR_LOG("EPOLL WAIT ERROR:%s\n", strerror(errno));
            abort(); // 退出程序
        }
        for (int i = 0; i < nfds; i++)
        {
            auto it = _channels.find(_evs[i].data.fd);
            assert(it != _channels.end());
            it->second->SetREvents(_evs[i].events); // 设置实际就绪的事件
            active->push_back(it->second);
        }
        return;
    }
};

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;
class TimerTask
{
private:
    uint64_t _id;         // 定时器任务对象ID
    uint32_t _timeout;    // 定时任务的超时时间
    bool _canceled;       // false-表示没有被取消， true-表示被取消
    TaskFunc _task_cb;    // 定时器对象要执行的定时任务
    ReleaseFunc _release; // 用于删除TimerWheel中保存的定时器对象信息
public:
    TimerTask(uint64_t id, uint32_t delay, const TaskFunc &cb) : _id(id), _timeout(delay), _task_cb(cb), _canceled(false) {}
    ~TimerTask()
    {
        if (_canceled == false)
            _task_cb();
        _release();
    }
    void Cancel() { _canceled = true; }
    void SetRelease(const ReleaseFunc &cb) { _release = cb; }
    uint32_t DelayTime() { return _timeout; }
};

class TimerWheel
{
private:
    using WeakTask = std::weak_ptr<TimerTask>;
    using PtrTask = std::shared_ptr<TimerTask>;
    int _tick;     // 当前的秒针，走到哪里释放哪里，释放哪里，就相当于执行哪里的任务
    int _capacity; // 表盘最大数量---其实就是最大延迟时间
    std::vector<std::vector<PtrTask>> _wheel;
    std::unordered_map<uint64_t, WeakTask> _timers;

    EventLoop *_loop;
    int _timerfd;                            // 定时器描述符--可读事件回调就是读取计时器，执行定时任务
    std::unique_ptr<Channel> _timer_channel; // 管理定时器的事件

private:
    void RemoveTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it != _timers.end())
        {
            _timers.erase(it);
        }
    }
    static int CreateTimerfd()
    {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd < 0)
        {
             ERR_LOG("TIMERFD CREATE FAILED!");
            abort();
        }
        // int timerfd_settime(int fd, int flags, struct itimerspec *new, struct itimerspec *old);
        struct itimerspec itime;
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0; // 第一次超时时间为1s后
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0; // 第一次超时后，每次超时的间隔时
        timerfd_settime(timerfd, 0, &itime, NULL);
        return timerfd;
    }
    int ReadTimefd()
    {
        uint64_t times;
        int ret = read(_timerfd, &times, 8);
        if (ret < 0)
        {
            ERR_LOG("READ TIMERFD FAILED!!");
            abort();
        }
        return times;
    }
    void OnTime()
    {
        int times = ReadTimefd();
        for (int i = 0; i < times; i++)
        {
            RunTimerTask();
        }
    }
    void TimerAddInLoop(uint64_t id, uint32_t delay, const TaskFunc &cb)
    {
        PtrTask pt(new TimerTask(id, delay, cb));
        pt->SetRelease(std::bind(&TimerWheel::RemoveTimer, this, id));
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id] = WeakTask(pt);
    }
    // 刷新/延迟定时任务
    void TimerRefreshInLoop(uint64_t id)
    {
        // 通过保存的定时器对象的weak_ptr构造一个shared_ptr出来，添加到轮子中
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return; // 没找着定时任务，没法刷新，没法延迟
        }
        PtrTask pt = it->second.lock(); // lock获取weak_ptr管理的对象对应的shared_ptr
        int delay = pt->DelayTime();
        int pos = (_tick + delay) % _capacity;
        _wheel[pos].push_back(pt);
    }
    void TimerCancelInLoop(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return; // 没找着定时任务，没法刷新，没法延迟
        }
        PtrTask pt = it->second.lock();
        if (pt)
            pt->Cancel();
    }

public:
    TimerWheel(EventLoop *loop) : _capacity(60), _tick(0), _wheel(_capacity), _loop(loop), _timerfd(CreateTimerfd()), _timer_channel(new Channel(_loop, _timerfd))
    {
        _timer_channel->SetReadCallback(std::bind(&TimerWheel::OnTime, this));
        _timer_channel->EnableRead(); // 启动可读事件监控
    }
    // 这个函数应该每秒钟被执行一次，相当于秒针向后走了一步
    void RunTimerTask()
    {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear(); // 清空指定位置的数组，就会把数组中保存的所有管理定时器对象的shared_ptr释放掉
    }
    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb);

    void TimerRefresh(uint64_t id);

    void TimerCancel(uint64_t id);

    bool HasTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return false;
        }
        return true;
    }
};
class EventLoop
{
private:
    using Functor = std::function<void()>;
    std::thread::id _thread_id;              // 线程 id
    int _event_fd;                           // eventfd唤醒IO事件监控可能导致的阻塞
    std::unique_ptr<Channel> _event_channel; // 管理event的事件
    Poller _poller;                          // 进行所有描述符的事件监控
    std::vector<Functor> _tasks;             // 任务池
    std::mutex _mutex;                       // 实现任务池操作的线程安全
    TimerWheel _timer_wheel;                 // 时间定时器
public:
    // 执行任务池的所有任务
    void RunAllTask()
    {
        std::vector<Functor> functor;
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _tasks.swap(functor);
        }
        for (auto &f : functor)
        {
            f();
        }
        return;
    }
    static int CreateEventFd()
    {
        int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (efd < 0)
        {
            ERR_LOG("CREATE EVENTFD FAILED!!");
            abort(); // 退出程序
        }
        return efd;
    }
    void ReadEventFd()
    {
        uint64_t res = 0;
        int ret = read(_event_fd, &res, sizeof(res));
        if (ret < 0)
        {
            // EINTR -- 被信号打断；   EAGAIN -- 表示无数据可读
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            ERR_LOG("READ EVENTFD FAILED!");
            abort();
        }
        return;
    }
    void WeakUpEventFd()
    {
        uint64_t val = 1;
        int ret = write(_event_fd, &val, sizeof(val));
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            ERR_LOG("READ EVENTFD FAILED!");
            abort();
        }
        return;
    }

public:
    EventLoop() : _thread_id(std::this_thread::get_id()),
                  _event_fd(CreateEventFd()),
                  _event_channel(new Channel(this, _event_fd)),
                  _timer_wheel(this)
    {
        // 设置eventfd可读事件的回调函数，读取eventfd事件通知次数
        _event_channel->SetReadCallback(std::bind(&EventLoop::ReadEventFd, this));
        // 启动eventfd的读事件监控
        _event_channel->EnableRead();
    }
    // 启动，三步走: 1.启动事件监控，2.事件就绪处理 3.执行任务
    void Start()
    {
        // 1.启动事件监控，
        Poller poller(_poller);
        std::vector<Channel *> actives;
        poller.Poll(&actives);

        // 2.事件就绪处理
        for (auto &channel : actives)
        {
            channel->HandleEvent();
        }
        // 3.执行任务
        RunAllTask();
    }
    // 判断当前线程是否为EventLoop对应的线程
    bool IsInLoop()
    {
        return (_thread_id == std::this_thread::get_id());
    }
    // 判断任务是否为该线程，是则执行，不是则放入任务池
    void RunInLoop(const Functor &cb)
    {
        if (IsInLoop())
        {
            return cb();
        }
        return QueueInLoop(cb);
    }
    // 将任务压入任务池
    void QueueInLoop(const Functor &cb)
    {
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _tasks.push_back(cb);
        }
        // 唤醒有可能因为没有事件就绪，而导致的epoll阻塞；
        // 其实就是给eventfd写入一个数据，eventfd就会触发可读事件
        WeakUpEventFd();
    }
    // 添加/修改事件监控
    void UpdateEvent(Channel *channel) { return _poller.UpdateEvent(channel); }
    // 移除事件监控
    void RemoveEvent(Channel *channel) { return _poller.RemoveEvent(channel); }

    void TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb) { return _timer_wheel.TimerAdd(id, delay, cb); }
    void TimerRefresh(uint64_t id) { return _timer_wheel.TimerRefresh(id); }
    void TimerCancel(uint64_t id) { return _timer_wheel.TimerCancel(id); }
    bool HasTimer(uint64_t id) { return _timer_wheel.HasTimer(id); }
};
void Channel::Remove() { return _loop->RemoveEvent(this); }
void Channel::Update() { return _loop->UpdateEvent(this); }
void TimerWheel::TimerAdd(uint64_t id, uint32_t delay, const TaskFunc &cb)
{
    _loop->RunInLoop(std::bind(&TimerWheel::TimerAddInLoop, this, id, delay, cb));
}
void TimerWheel::TimerRefresh(uint64_t id)
{
   _loop->RunInLoop(std::bind(&TimerWheel::TimerRefreshInLoop, this, id));
}
void TimerWheel::TimerCancel(uint64_t id)
{
     _loop->RunInLoop(std::bind(&TimerWheel::TimerCancelInLoop, this, id));
}
