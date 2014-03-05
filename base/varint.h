/*
* steal from facebook folly
* 变长的整型值编码方式
* 参考 https://developers.google.com/protocol-buffers/docs/encoding?hl=zh-CN&csw=1
* 每个字节的高位是标志位，1表示还有后续字节，0表示无后续字节
*/
#ifndef FOLLY_VARINT_H_
#define FOLLY_VARINT_H_

namespace base
{
  static const size_t kMaxVarintLength32 = 5;
  static const size_t kMaxVarintLength64 = 10;
  size_t EncodeVarint(uint64_t val, uint8_t* buf);
  uint64_t DecodeVarint(uint8_t* data,size_t len);
  // 有符号转换为无符号，符号位放到最后一位
  inline uint64_t EncodeZigZag(int64_t val) 
  {
    return static_cast<uint64_t>((val << 1) ^ (val >> 63));
  }
  inline int64_t DecodeZigZag(uint64_t val) 
  {
    return static_cast<int64_t>((val >> 1) ^ -(val & 1));
  }
  inline size_t EncodeVarint(uint64_t val, uint8_t* buf) 
  {
    uint8_t* p = buf;
    while (val >= 128) 
    {
      *p++ = 0x80 | (val & 0x7f);
      val >>= 7;
    }
    *p++ = val;
    return p - buf;
  }

  inline uint64_t DecodeVarint(uint8_t* data,size_t len) 
  {
    const int8_t* begin = reinterpret_cast<const int8_t*>(data);
    const int8_t* end = reinterpret_cast<const int8_t*>(data+len);
    const int8_t* p = begin;
    uint64_t val = 0;

    if (LIKELY(end - begin >= kMaxVarintLength64)) 
    {
      int64_t b;
      do 
      {
        b = *p++; val  = (b & 0x7f)      ; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) <<  7; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 14; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 21; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 28; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 35; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 42; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 49; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 56; if (b >= 0) break;
        b = *p++; val |= (b & 0x7f) << 63; if (b >= 0) break;
        throw std::invalid_argument("Invalid varint value");  // too big
      } while (false);
    } 
    else 
    {
      int shift = 0;
      while (p != end && *p < 0) 
      {
        val |= static_cast<uint64_t>(*p++ & 0x7f) << shift;
        shift += 7;
      }
      if (p == end) throw std::invalid_argument("Invalid varint value");
      val |= static_cast<uint64_t>(*p++) << shift;
    }
    return val;
  }

}  // namespaces

#endif /* FOLLY_VARINT_H_ */

