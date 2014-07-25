#ifndef _MEMORYSTREAM_H_
#define _MEMORYSTREAM_H_
#include <memory.h>
#include <assert.h>
#include <string>

namespace base
{
class BufferReader;
class BufferWriter
{
public:
    BufferWriter(char* buff,int buffsize);
    void  rebind(char* buff,int buffsize);
    char* get_buffer(){return buffer_;}
    int   get_used_size(){return pos_;}
    void  set_used_size(int size);
    bool  can_increase_size(int size);
    void  increase_size(int size);
    int   get_buffer_size(){return buffer_size_;}
    bool  fail(){return fail_;}
    void  write_buffer(const char* buf,int len);
    void  write_string(const std::string& str);
    int   write_max_buffer(const char* buf,int maxlen);
    void  write_data(int c,int len);
    void  write_reader(BufferReader& reader,int len);
    int   write_max_reader(BufferReader& reader,int readLen);

    template<class T> T*   write();
    template<class T> void write(T data);
private:
    char* buffer_;
    int   buffer_size_;
    int   pos_;
    bool  fail_;
};

inline BufferWriter::BufferWriter(char* buff,int buffsize)
{
    assert(buffsize>=0);
    buffer_=buff;
    buffer_size_=buffsize;
    pos_=0;
    fail_=false;
}
inline void BufferWriter::rebind(char* buff,int buffsize)
{
    assert(buffsize>=0);
    buffer_=buff;
    buffer_size_=buffsize;
    pos_=0;
    fail_=false;
}
inline void BufferWriter::set_used_size(int size)
{
    if(size>=0&&size<=buffer_size_)
        pos_=size;
    else
        fail_=true;
}
inline bool BufferWriter::can_increase_size(int size)
{
    return pos_+size>=0&&pos_+size<=buffer_size_;
}
inline void BufferWriter::increase_size(int size)
{
    if(pos_+size>=0&&pos_+size<=buffer_size_)
        pos_+=size;
    else
        fail_=true;
}
template<class T>
inline T* BufferWriter::write()
{
    static T defaultData;
    if(pos_+(int)sizeof(T)<=buffer_size_)
    {
        T* data=(T*)(buffer_+pos_);
        pos_+=sizeof(T);
        return data;
    }
    fail_=true;
    return &defaultData;
}
template<class T>
inline void BufferWriter::write(T data)
{
    if(pos_+(int)sizeof(T)<=buffer_size_)
    {
        *(T*)(buffer_+pos_)=data;
        pos_+=sizeof(T);
    }else fail_=true;
}
inline void BufferWriter::write_buffer(const char* buf,int len)
{
    if(len>=0&&pos_+len<=buffer_size_)
    {
        memcpy(buffer_+pos_,buf,len);
        pos_+=len;
    }else fail_=true;
}
inline void BufferWriter::write_string(const std::string& str)
{
    if(pos_+(int)str.size()<=buffer_size_)
    {
        memcpy(buffer_+pos_,str.c_str(),str.size());
        pos_+=str.size();
    }else fail_=true;
}

inline int BufferWriter::write_max_buffer(const char* buf,int maxlen)
{
    int writeLen=0;
    if(maxlen>=0){
        if(pos_+maxlen<=buffer_size_){
            writeLen=maxlen;
            memcpy(buffer_+pos_,buf,maxlen);
            pos_+=maxlen;
        }else{
            writeLen=buffer_size_-pos_;
            memcpy(buffer_+pos_,buf,buffer_size_-pos_);
            pos_=buffer_size_;
        }
    }else fail_=true;
    return writeLen;
}
inline void BufferWriter::write_data(int c,int len)
{
    if(len>=0&&pos_+len<=buffer_size_)
    {
        memset(buffer_+pos_,c,len);
        pos_+=len;
    }else fail_=true;
}

class BufferReader
{
public:
    BufferReader(char* buff,int buffsize);
    void  rebind(char* buff,int buffsize);
    char* get_buffer(){return buffer_;}
    int   get_used_size(){return pos_;}
    void  set_used_size(int size);
    bool  can_increase_size(int size);
    void  increase_size(int size);
    int   get_buffer_size(){return buffer_size_;}
    bool  fail(){return fail_;}
    void  read_buffer(char* buf,int len);
    void  read_string(std::string& str,int len);
    int   read_max_buffer(char* buf,int maxlen);
    void  read_fix_string(char* buf,int bufsize,int readLen);

    template<class T> T*   read();
    template<class T> void read(T& outdata);
private:
    char* buffer_;
    int   buffer_size_;
    int   pos_;
    bool  fail_;
};

inline BufferReader::BufferReader(char* buff,int buffsize)
{
    assert(buffsize>=0);
    buffer_=buff;
    buffer_size_=buffsize;
    pos_=0;
    fail_=false;
}
inline void BufferReader::rebind(char* buff,int buffsize)
{
    assert(buffsize>=0);
    buffer_=buff;
    buffer_size_=buffsize;
    pos_=0;
    fail_=false;
}
inline void BufferReader::set_used_size(int size)
{
    if(size>=0&&size<=buffer_size_)
        pos_=size;
    else
        fail_=true;
}
inline bool BufferReader::can_increase_size(int size)
{
    return pos_+size>=0&&pos_+size<=buffer_size_;
}
inline void BufferReader::increase_size(int size)
{
    if(pos_+size>=0&&pos_+size<=buffer_size_)
        pos_+=size;
    else
        fail_=true;
}
template<class T>
inline T* BufferReader::read()
{
    static T defaultData;
    if(pos_+(int)sizeof(T)<=buffer_size_)
    {
        T* data=(T*)(buffer_+pos_);
        pos_+=sizeof(T);
        return data;
    }
    fail_=true;
    return &defaultData;
}
template<class T>
inline void BufferReader::read(T& outdata)
{
    if(pos_+(int)sizeof(T)<=buffer_size_)
    {
        outdata=*(T*)(buffer_+pos_);
        pos_+=sizeof(T);
    }else fail_=true;
}
inline void BufferReader::read_buffer(char* buf,int len)
{
    if(len>=0&&pos_+len<=buffer_size_)
    {
        memcpy(buf,buffer_+pos_,len);
        pos_+=len;
    }else fail_=true;
}
inline void BufferReader::read_string(std::string& str,int len)
{
    if(len>=0&&pos_+len<=buffer_size_)
    {
        str.append(buffer_+pos_,len);
        pos_+=len;
    }else fail_=true;
}
inline int BufferReader::read_max_buffer(char* buf,int maxlen)
{
    int readLen=0;
    if(maxlen>=0){
        if(pos_+maxlen<=buffer_size_){
            readLen=maxlen;
            memcpy(buf,buffer_+pos_,maxlen);
            pos_+=maxlen;
        }else{
            readLen=buffer_size_-pos_;
            memcpy(buf,buffer_+pos_,buffer_size_-pos_);
            pos_=buffer_size_;
        }
    }else fail_=true;
    return readLen;
}
inline void BufferReader::read_fix_string(char* buf,int bufsize,int readLen)
{
    assert(bufsize>0);
    if(readLen>=0){
        if(readLen<bufsize){
            read_buffer(buf,readLen);
            buf[readLen]='\0';
        }else{
            read_buffer(buf,bufsize-1);
            increase_size(readLen-(bufsize-1));
            if(bufsize>0)
                buf[bufsize-1]='\0';
        }
    }else fail_=true;
}

inline void BufferWriter::write_reader(BufferReader& reader,int len)
{
    if(len>=0&&pos_+len<=buffer_size_)
    {
        reader.read_buffer(buffer_+pos_,len);
        pos_+=len;
    }else fail_=true;
}

inline int BufferWriter::write_max_reader(BufferReader& reader,int maxlen)
{
    int writeLen=0;
    if(maxlen>=0){
        int readLen=maxlen<buffer_size_-pos_?maxlen:buffer_size_-pos_;
        writeLen=reader.read_max_buffer(buffer_+pos_,readLen);
        assert(pos_+writeLen<=buffer_size_);
        pos_+=writeLen;
    }else fail_=true;
    return writeLen;
}
}
#endif//_CROSSENGINE_MEMORYSTREAM_H_
