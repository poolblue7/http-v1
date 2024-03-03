#include  <iostream>
#include  <vector>
#include  <unordered_map>
#include  <functional>
#include  <memory>
#include  <cstdint>
#include  <unistd.h>

using TaskFunc =std::function<void()>;
using ReleaseFunc=std::function<void()>;
class TimerTask
{
private:
     uint64_t _id;      //定时器任务对象id
     uint32_t  _timeout;//定时任务的超时时间
     bool _cancel;     //定时器任务取消标志 --false 任务不取消，继续执行 --true 任务取消，不执行
     TaskFunc _task_cb;//定时器要执行的任务
     ReleaseFunc _release;//用于删除TimerWheel中保存的定时器对象信息
   
public:
    TimerTask(uint64_t id,uint32_t delay, const TaskFunc &cb)
    :_id(id),_timeout(delay),_task_cb(cb){};
    ~TimerTask()
    {
        _task_cb();
        _release();
    };
    void SetRelease(const ReleaseFunc &cb)
    {
        _release=cb;
    }
    uint32_t DelayTime()
    {
        return _timeout;
    }
};
class Timewheel
{
private:
    using WeakTask=std::weak_ptr<TimerTask>;
    using PtrTask=std::shared_ptr<TimerTask>;
    std::vector<std::vector<PtrTask>> _wheel;//时间轮
    int _tick;                               //当前秒针，走到哪里，释放哪里，相当于执行哪里的任务
    int _capacity;                           //最大容量
    std::unordered_map<uint64_t,WeakTask> _timers;//跟踪并管理所有添加到时间轮中的定时器
private:
    void RemoveTimer(uint64_t id)
    {
        auto it =_timers.find(id);
        if(it!=_timers.end()){
            _timers.erase(it);
        }
    }
public: 
     Timewheel():_tick(0),_capacity(60),_wheel(_capacity){};
     ~Timewheel();
     //添加定时任务
    void TimeAdd(uint64_t id,uint32_t delay, const TaskFunc &cb)
    {
       PtrTask pt(new TimerTask(id,delay,cb));
       pt->SetRelease(std::bind(RemoveTimer,this,id));
       int pos=(_tick+delay) %_capacity;
       _wheel[pos].push_back(pt);
       _timers[id]=WeakTask(pt);

    }
    // 刷新/延迟定时任务
    void TimeRefresh(u_int64_t id)
    {   //通过保存定时器对象的weak_ptr构造一个share_ptr，放入时间轮钟
        auto it =_timers.find(id);
        if(it==_timers.end()){
            return;//没找到定时任务，无法刷新/延迟
        }
        PtrTask pt=it->second.lock();//lock获取weak_ptr管理的对象中的share_ptr
        int delay=pt->DelayTime();
        int pos=(_tick+delay) % _capacity;
        _wheel[pos].push_back(pt);
        _timers[id]=pt;
    }
    //时间秒针走动，每秒钟执行一次，相当于秒针走了一步
    void RunTimerTask()
    {
        _tick=(_tick+1)%_capacity;
        _wheel[_tick].clear();//清空指定位置的数组，就会把数组中保存的share——ptr释放掉，即执行任务
    }
};




