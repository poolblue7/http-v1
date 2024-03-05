#include <iostream>
#include <assert.h>

class  Any
{
private:
    class holder
    {  
    public:
        virtual ~holder();
        virtual std::type_info type()=0;
        virtual  holder *clone()=0;
    };
    template <class T>
    class placeholder : public holder
    {
    public:
        placeholder(const T  &val):_val()
        //获取子类对象保存的数据类型
        virtual std::type_info type(){
            return typeid(_val);
        };
        //根据当前的子类对象，克隆新的子类对象
        virtual  holder * clone(){
           return new placeholder<T>(_val);
        };   
    private:
        T _val;
    };
    holder *_content;
public:
  Any():_content(NULL){}
  Any& swap(Any &other){
     std::swap(_content,other._content);
     return *this;
  }
  template<class T>
  Any(const T & val):new placeholder<T>(val){ }
  Any(const Any &other):_content(other._content ? other._content->clone() :NULL){}
  ~Any(){ delete _content ;}
 
 //返回子类对象所保存的数据的指针
  template<class T>
  T *get(){
    assert(typeid(T)==_content->type());
    return & (placeholder<T>*)_content->val;
  }
  //赋值运算符重载
  template<class T>
  Any& operator=(const T &val){
    //通过为val构造临时的通用容器，然后对当前容器的指针进行交换，
    //临时容器对象释放，原先的数据也被释放
     Any(val).swap(*this)

     return *this;
  }

  Any& operator=(const Any &other){
    //同上
    Any(other).swap(*this); 
    return *this;
  }

};

