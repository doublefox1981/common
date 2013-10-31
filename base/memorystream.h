#ifndef _MEMORYSTREAM_H_
#define _MEMORYSTREAM_H_
#include <memory.h>
#include <assert.h>
#include <string>

namespace base
{
class ezBufferReader;
class ezBufferWriter
{
public:
    ezBufferWriter(char* buff,int buffsize);
    void ReBind(char* buff,int buffsize);
    char* GetBuff(){return mBuf;}
    int GetUsedSize(){return mPos;}
    void SetUsedSize(int size);
    bool CanIncreaseSize(int size);
    void IncreaseSize(int size);
    int GetBufSize(){return mBufSize;}
    bool Fail(){return mFail;}
    template<class T> T* Write();
    template<class T> void Write(T data);
    void WriteBuffer(const char* buf,int len);
    void WriteString(const std::string& str);
    int WriteMaxBuffer(const char* buf,int maxlen);
    void WriteData(int c,int len);
    void WriteReader(ezBufferReader& reader,int len);
    int WriteMaxReader(ezBufferReader& reader,int readLen);
private:
    char* mBuf;
    int mBufSize;
    int mPos;
    bool mFail;
};

inline ezBufferWriter::ezBufferWriter(char* buff,int buffsize)
{
    assert(buffsize>=0);
    mBuf=buff;
    mBufSize=buffsize;
    mPos=0;
    mFail=false;
}
inline void ezBufferWriter::ReBind(char* buff,int buffsize)
{
    assert(buffsize>=0);
    mBuf=buff;
    mBufSize=buffsize;
    mPos=0;
    mFail=false;
}
inline void ezBufferWriter::SetUsedSize(int size)
{
    if(size>=0&&size<=mBufSize)
        mPos=size;
    else
        mFail=true;
}
inline bool ezBufferWriter::CanIncreaseSize(int size)
{
    return mPos+size>=0&&mPos+size<=mBufSize;
}
inline void ezBufferWriter::IncreaseSize(int size)
{
    if(mPos+size>=0&&mPos+size<=mBufSize)
        mPos+=size;
    else
        mFail=true;
}
template<class T>
inline T* ezBufferWriter::Write()
{
    static T defaultData;
    if(mPos+(int)sizeof(T)<=mBufSize)
    {
        T* data=(T*)(mBuf+mPos);
        mPos+=sizeof(T);
        return data;
    }
    mFail=true;
    return &defaultData;
}
template<class T>
inline void ezBufferWriter::Write(T data)
{
    if(mPos+(int)sizeof(T)<=mBufSize)
    {
        *(T*)(mBuf+mPos)=data;
        mPos+=sizeof(T);
    }else mFail=true;
}
inline void ezBufferWriter::WriteBuffer(const char* buf,int len)
{
    if(len>=0&&mPos+len<=mBufSize)
    {
        memcpy(mBuf+mPos,buf,len);
        mPos+=len;
    }else mFail=true;
}
inline void ezBufferWriter::WriteString(const std::string& str)
{
    if(mPos+(int)str.size()<=mBufSize)
    {
        memcpy(mBuf+mPos,str.c_str(),str.size());
        mPos+=str.size();
    }else mFail=true;
}

inline int ezBufferWriter::WriteMaxBuffer(const char* buf,int maxlen)
{
    int writeLen=0;
    if(maxlen>=0){
        if(mPos+maxlen<=mBufSize){
            writeLen=maxlen;
            memcpy(mBuf+mPos,buf,maxlen);
            mPos+=maxlen;
        }else{
            writeLen=mBufSize-mPos;
            memcpy(mBuf+mPos,buf,mBufSize-mPos);
            mPos=mBufSize;
        }
    }else mFail=true;
    return writeLen;
}
inline void ezBufferWriter::WriteData(int c,int len)
{
    if(len>=0&&mPos+len<=mBufSize)
    {
        memset(mBuf+mPos,c,len);
        mPos+=len;
    }else mFail=true;
}

class ezBufferReader
{
public:
    ezBufferReader(char* buff,int buffsize);
    void ReBind(char* buff,int buffsize);
    char* GetBuff(){return mBuf;}
    int GetUsedSize(){return mPos;}
    void SetUsedSize(int size);
    bool CanIncreaseSize(int size);
    void IncreaseSize(int size);
    int GetBufSize(){return mBufSize;}
    bool Fail(){return mFail;}
    template<class T> T* Read();
    template<class T> void Read(T& outdata);
    void ReadBuffer(char* buf,int len);
    void ReadString(std::string& str,int len);
    int ReadMaxBuffer(char* buf,int maxlen);
    void ReadFixString(char* buf,int bufsize,int readLen);
private:
    char* mBuf;
    int mBufSize;
    int mPos;
    bool mFail;
};

inline ezBufferReader::ezBufferReader(char* buff,int buffsize)
{
    assert(buffsize>=0);
    mBuf=buff;
    mBufSize=buffsize;
    mPos=0;
    mFail=false;
}
inline void ezBufferReader::ReBind(char* buff,int buffsize)
{
    assert(buffsize>=0);
    mBuf=buff;
    mBufSize=buffsize;
    mPos=0;
    mFail=false;
}
inline void ezBufferReader::SetUsedSize(int size)
{
    if(size>=0&&size<=mBufSize)
        mPos=size;
    else
        mFail=true;
}
inline bool ezBufferReader::CanIncreaseSize(int size)
{
    return mPos+size>=0&&mPos+size<=mBufSize;
}
inline void ezBufferReader::IncreaseSize(int size)
{
    if(mPos+size>=0&&mPos+size<=mBufSize)
        mPos+=size;
    else
        mFail=true;
}
template<class T>
inline T* ezBufferReader::Read()
{
    static T defaultData;
    if(mPos+(int)sizeof(T)<=mBufSize)
    {
        T* data=(T*)(mBuf+mPos);
        mPos+=sizeof(T);
        return data;
    }
    mFail=true;
    return &defaultData;
}
template<class T>
inline void ezBufferReader::Read(T& outdata)
{
    if(mPos+(int)sizeof(T)<=mBufSize)
    {
        outdata=*(T*)(mBuf+mPos);
        mPos+=sizeof(T);
    }else mFail=true;
}
inline void ezBufferReader::ReadBuffer(char* buf,int len)
{
    if(len>=0&&mPos+len<=mBufSize)
    {
        memcpy(buf,mBuf+mPos,len);
        mPos+=len;
    }else mFail=true;
}
inline void ezBufferReader::ReadString(std::string& str,int len)
{
    if(len>=0&&mPos+len<=mBufSize)
    {
        str.append(mBuf+mPos,len);
        mPos+=len;
    }else mFail=true;
}
inline int ezBufferReader::ReadMaxBuffer(char* buf,int maxlen)
{
    int readLen=0;
    if(maxlen>=0){
        if(mPos+maxlen<=mBufSize){
            readLen=maxlen;
            memcpy(buf,mBuf+mPos,maxlen);
            mPos+=maxlen;
        }else{
            readLen=mBufSize-mPos;
            memcpy(buf,mBuf+mPos,mBufSize-mPos);
            mPos=mBufSize;
        }
    }else mFail=true;
    return readLen;
}
inline void ezBufferReader::ReadFixString(char* buf,int bufsize,int readLen)
{
    assert(bufsize>0);
    if(readLen>=0){
        if(readLen<bufsize){
            ReadBuffer(buf,readLen);
            buf[readLen]='\0';
        }else{
            ReadBuffer(buf,bufsize-1);
            IncreaseSize(readLen-(bufsize-1));
            if(bufsize>0)
                buf[bufsize-1]='\0';
        }
    }else mFail=true;
}

inline void ezBufferWriter::WriteReader(ezBufferReader& reader,int len)
{
    if(len>=0&&mPos+len<=mBufSize)
    {
        reader.ReadBuffer(mBuf+mPos,len);
        mPos+=len;
    }else mFail=true;
}

inline int ezBufferWriter::WriteMaxReader(ezBufferReader& reader,int maxlen)
{
    int writeLen=0;
    if(maxlen>=0){
        int readLen=maxlen<mBufSize-mPos?maxlen:mBufSize-mPos;
        writeLen=reader.ReadMaxBuffer(mBuf+mPos,readLen);
        assert(mPos+writeLen<=mBufSize);
        mPos+=writeLen;
    }else mFail=true;
    return writeLen;
}
}
#endif//_CROSSENGINE_MEMORYSTREAM_H_
