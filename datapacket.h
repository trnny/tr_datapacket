#pragma once

#include<iostream>
#include<string>
#include<cstring>
#include<fstream>
#include<cstdio>

using namespace std;

typedef unsigned char _byte;

/**
 * 用于把多个字节组合在一起
 */
class tr_field{
    uint16_t len = 0;       // 总长度
    uint16_t pt = 0;        // 有数据的尾部
    _byte* data = NULL;         // [0-1] 描述长度 [2-3] 数据(总)长度

public:
    tr_field() = delete;
    tr_field(const tr_field& cp){
        len = cp.len;
        pt = cp.pt;
        data = new _byte[len]{0};       // 虽然申请了总长度的空间
        memcpy(data, cp.data, pt);      // 但是只给有值的部分内存复制一下就行了
    };
    tr_field& operator=(const tr_field& ap){
        if(this == &ap) return *this;
        if(NULL != data) delete[] data; // 先清理掉旧数据的空间
        len = ap.len;
        pt = ap.len;
        data = new _byte[len]{0};
        memcpy(data, ap.data, pt);
    };
    /**
     * 从对象内存复制构造
     * 可以用于把字段packet后的对象再转成field
     * pt为data的长度
     */ 
    tr_field(void* _field, uint16_t pt){
        len = pt;
        this->pt = pt;
        data = new _byte[pt];
        memcpy(data, _field, pt);
    };
    /**
     * 给定字段名和包长(数据长,非总长)
     * 包的总长度最大为65507
     */ 
    tr_field(const char* desc, uint16_t size = 0){                         // 申请空间没有放数据进去
        pt = 4;
        uint16_t desc_len = strlen(desc) + 1;
        pt += desc_len;
        if((int)size + pt > 65507)size = 65507 - pt;
        len = pt + size;
        data = new _byte[len]{0};
        memcpy(data, &desc_len, 2);                         // 描述长
        memcpy(data + 2, &size, 2);                         // 数据长
        memcpy(data + 4, desc, desc_len);                   // 描述
    };
    /**
     * 如果数据是字符串,用这种构造方式更好
     * 参数一为描述 参数二为数据字符串
     * 注意报长度不能超,超过会有潜在错误
     */
    tr_field(const char* desc, const char* strdata){
        len += 4;
        uint16_t desc_len = strlen(desc) + 1;
        len += desc_len;
        uint16_t strdata_len = strlen(strdata) + 1;
        len += strdata_len;
        if(len <= 65507){
            pt = len;
            data = new _byte[len]{0};
            memcpy(data, &desc_len, 2);                         // 描述长
            memcpy(data + 2, &strdata_len, 2);                  // 数据长
            memcpy(data + 4, desc, desc_len);                   // 描述
            memcpy(data + 4 + desc_len, strdata, strdata_len);  // 数据
        }else{
            len = 0;
        }
    };
    /**
     * 这个构造是复制一个指针所指向的内存空间
     * 参数一描述 参数二源地址 参数三要复制的长度
     */
    tr_field(const char* desc, void* dptr, uint16_t size){
        len += 4;
        uint16_t desc_len = strlen(desc) + 1;
        len += desc_len;
        len += size;
        if(len < 65507){
            pt = size;
            data = new _byte[len]{0};
            memcpy(data, &desc_len, 2);                         // 描述长
            memcpy(data + 2, &size, 2);                         // 数据长
            memcpy(data + 4, desc, desc_len);                   // 描述
            memcpy(data + 4 + desc_len, dptr, size);            // 数据
        }else{
            len = 0;
        }
    };
    /**
     * 往包里塞数据, 自动扩容(如果没超)
     * 参数一源地址 参数二长度
     */
    bool push(const void* dptr, uint16_t size){
        if(!data || pt + size > 65507) return false;
        if((int)pt + size > len) {           // 重新申请空间并复制 如果不需要扩长 dlen没变
            uint16_t dlen;
            memcpy(&dlen, data + 2, 2); // 原数据部分总长度
            dlen += pt + size - len;    // `pt + size` 是新长度  `pt + size -len`是增长长度
            len = pt + size;
            _byte* temp = new _byte[len]{0};
            memcpy(temp, data, pt);
            delete[] data;
            data = temp;
            memcpy(data + 2, &dlen, 2);
        }
        memcpy(data + pt, dptr, size);  // 现在空间足够, 直接在后面写入
        pt += size;
        return true;
    };
    /**
     * 将数据按照字符串读出
     * 参数一保存描述 参数二保存读到的字符串
     * 可以用该函数取得描述名
    */
    void get(char* desc, char* dest){
        desc && strcpy(desc, (const char*)data + 4);
        if(!dest) return;
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        strcpy(dest, (const char*)data + 4 + desc_len);
    };
    /**
     * 将有数据的部分读出, 即忽略包尾空数据(如果有)
     * 参数一保存描述 参数二保存读到的数据 参数三保存读到的长度
     * 注意参数二的容量, 容量不足会引发潜在错误
     * 需要读取部分范围数据, 使用getd
     * 可以用该函数取内容长度
    */
    void get(char* desc, void *dest, int& len){
        desc && strcpy(desc, (const char*)data + 4);
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);                     // 取不到?
        uint16_t dlen;                                  // 数据总长度
        memcpy(&dlen, data + 2, 2);
        len = dlen + pt - this->len;
        dest && memcpy(dest, data + 4 + desc_len, len);
    };
    /**
     * 将数据全部读出 包括包尾可能存在的空数据
     * 参数一保存读到的数据 参数二保存数据的总长度
     * 由于是全部读出,所以接收数据的参数二容量得够
    */
    void geta(void *dest, int& len){
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        uint16_t dlen;
        memcpy(&dlen, data + 2, 2);
        len = dlen;
        memcpy(dest, data + 4 + desc_len, len);
    };
    /**
     * 按照偏移读
     * 从start开始读len长读到dest中
     */
    void getd(void* dest, uint16_t len, uint16_t start = 0){
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        uint16_t dlen;
        memcpy(&dlen, data + 2, 2);
        memcpy(dest, data + 4 + desc_len + start, start + len > dlen ? dlen - start : len);
    };
    /**
     * 将包里面所有数据取出
     * 参数一用来放包的数据 参数二用来放包长度
     * 这里不会将包尾空数据(如果有)取出
     */
    bool packet(void* dest, int& size){
        if(len == 0 || !data) return false;                  // 包是空的(肯定是前面哪出错)
        size = pt;
        memcpy(dest, data, pt);
        if(pt != len){                              // 压缩一点,去除后面没用数据
            uint16_t dlen;
            memcpy(&dlen, data + 2, 2);
            dlen -= len - pt;                       // `len - pt` 是空数据长度
            memcpy(dest + 2, &dlen, 2);
        }
        return true;
    };
    /**
     * 返回包大小, 
     * true则返回总长度 false表示已用长度
     */
    unsigned int length(bool f = false){
        return f ? len : pt;
    };
    /**
     * 返回这个包最大的容量
     */
    uint16_t maxsize(){
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        return 65503 - desc_len;
    };
    ~tr_field(){
        if(NULL != data){
            delete[] data;
            data = NULL;
        }
    };
};

/**
 * 块信息
 */
struct tr_block{
    tr_block* nex = NULL;
    unsigned int beg;
    unsigned int end;
    tr_block(unsigned int beg, unsigned int end){
        this->beg = beg;
        this->end = end;
    }
};
/**
 * 进度信息
 */
class progress{
    unsigned int end;
    tr_block* head;
public:
    /**
     * 参数指定进度终止长度
     */
    progress(int end){
        this->end = end > 0 ? end : 0;
        head = new tr_block(0,0);
    };
    progress(){
        end = 0;
        head = new tr_block(0,0);
    }
    progress(const progress& cp){
        end = cp.end;
        tr_block* p = cp.head, *q;
        head = new tr_block(p->beg, p->end);
        for(q = head, p = p->nex;NULL != p;){
            q->nex = new tr_block(p->beg, p->end);
            p = p->nex;
            q = q->nex;
        }
    }
    /**
     * 从保存了进度信息的地址读进度信息
     */
    progress(const void* dsrc){
        int cnt, beg, end;
        memcpy(&this->end, dsrc, 4);
        memcpy(&cnt, dsrc + 4, 4);
        memcpy(&beg, dsrc + 8, 4);
        memcpy(&end, dsrc + 12, 4);
        head = new tr_block(beg, end);
        tr_block* p = head;
        cnt *= 8;
        for(int i = 8;i < cnt;){
            i += 8;
            memcpy(&beg, dsrc + i, 4);
            memcpy(&end, dsrc + i + 4, 4);
            p->nex = new tr_block(beg, end);
            p = p->nex;
        }
    }
    ~progress(){
        tr_block* p;
        while(head){
            p = head->nex;
            delete head;
            head = p;
        }
    }
    progress& operator=(const progress& sp){
        if(this == &sp) return *this;
        end = sp.end;
        tr_block* p = sp.head, *q;
        while(head){
            q = head->nex;
            delete head;
            head = q;
        }
        head = new tr_block(p->beg, p->end);
        for(q = head, p = p->nex;NULL != p;){
            q->nex = new tr_block(p->beg, p->end);
            p = p->nex;
            q = q->nex;
        }
    }
    /**
     * 取第一个完整块百分百进度
     * -1表示未知
     */
    int prog(){
        if(!end) return -1;
        return (head->end - head->beg) * 100 / end;
    }
    /**
     * 取总百分百进度
     */
    int proga(){
        if(!end) return -1;
        return size() * 100 / end;
    }
    /**
     * 取总大小
     */
    unsigned int size(){
        int cnt = 0;
        tr_block* p = head;
        while(NULL != p){
            cnt += p->end - p->beg;
            p = p->nex;
        }
        return cnt;
    }
    /**
     * 取第b个空块的起始
     * 返回false 要么没有第b块 要么是最后一块尾未知
     * 所以如果返回了true, beg和end里的数据是有效的
     */
    bool empty_block(unsigned int& beg, unsigned int& end, int b = 0){
        if(head->beg && b == 0){                // 开头有空块
            beg = 0;
            end = head->beg;
            return true;
        }
        if(b == 0 && !head->end && this->end){ // 一整块都是空
            beg = 0;
            end = this->end;
            return true;
        }
        tr_block* p = head;
        int i = !!head->beg;                    // 不为0表示前面有个空块
        for(; NULL != p;p = p ->nex, i++){
            if(i == b){
                if(NULL == p->nex && !this->end) return false;  // 最后一块终点未知
                beg = p->end;
                end = NULL == p->nex ? this->end : p->nex->beg;
                return true;
            }
        }
        return false;
    }
    /**
     * 获取第b个块的起始
     * 返回false表示没有第b块
     */
    bool block(unsigned int& beg, unsigned int& end, int b = 0){
        tr_block *p = head;
        for(int i = 0;i < b && p;i++,p = p->nex);
        if(!p) return false;
        beg = p->beg;
        end = p->end;
        return true;
    }
    /**
     * 取块的个数
     * 空块个数 = 块数 - 1
     */
    int count(){
        tr_block* p= head;
        int i = 0;
        for(; NULL != p; i++, p = p->nex);
        return i;
    };
    /**
     * 填充从beg到end的进度
     */
    bool put(unsigned int beg, unsigned int end){
        if(beg >= end || beg < 0) return false;         // beg,end不合规
        if(this->end && end > this->end) return false;  // 有上限且end超上限
        if(head->beg == 0 && head->end == 0){
            head->beg = beg;
            head->end = end;
            return true;
        }
        if(head->beg == end){
            head->beg = beg;
            return true;
        }
        if(head->beg > end){
            tr_block* p = new tr_block(beg, end);
            p->nex = head;
            head = p;
            return true;
        }
        tr_block* p = head;
        while(NULL != p){
            if(NULL == p->nex){
                if(p->end > beg) return false;
                else if(p->end == beg) p->end = end;
                else p->nex = new tr_block(beg, end);
                return true;
            }
            if(beg >= p->nex->end) {
                p = p->nex;
                continue;
            }
            if(p->end > beg || p->nex->beg < end) return false;
            if(p->end == beg && p->nex->beg == end){
                tr_block* t = p->nex;
                p->nex = t->nex;
                p->end = t->end;
                delete t;
            }
            else if(p->end < beg && p->nex->beg > end){
                tr_block* t = new tr_block(beg, end);
                t->nex = p->nex;
                p->nex = t;
            }
            else if(p->end == beg)
                p->end = end;
            else if(p->nex->beg == end)
                p->nex->beg = beg;
            return true;
        }
        return false;
    }
    /**
     * 将进度信息保存在指定位置
     * len保存节点数
     */
    void save(void* dest, int& len){    // end/count/blocks...
        memcpy(dest, &end, 4);
        tr_block* p= head;
        for(len = 0;p;len+=8, memcpy(dest + len, &(p->beg), 4), memcpy(dest + len + 4, &(p->end), 4), p = p->nex);
        len /= 8;
        memcpy(dest + 4, &len, 4);
    }
};

class tr_buffer{
    const static bool Mbuf = true;
    const static bool Fbuf = false;
    const static unsigned int max_buf_size = 4 * 1024 * 1024;
    typedef bool BufType;
    _byte* mbuf = NULL;         // 指针没有设定默认值不能访问
    FILE* fbuf = NULL;
    progress bufprog;
    unsigned int bufsize;
    BufType buftype;
    /**
     * 从src将数据写入缓冲区
     * 内存或文件,
     * 如果是文件,会自动将bufsize改动
     * bufsize改动只有在一开始指定为0的情况有用
     */
    bool buf_write(const void* dsrc, unsigned int beg, unsigned int len){
        return (buftype && mbuf && memcpy(mbuf + beg, dsrc, len)) || (!buftype && fbuf && !fseek(fbuf, beg, 0) && fwrite(dsrc, len, 1, fbuf) && ((bufsize < beg + len && (bufsize = beg + len)) || true));
    };
    bool buf_read(void* dest, unsigned int beg, unsigned int len){
        return (buftype && mbuf && memcpy(dest, mbuf + beg, len)) || (!buftype && fbuf && !fseek(fbuf, beg, 0) && fread(dest, len, 1, fbuf));
    };
public:
    ~tr_buffer(){
        if(buftype && mbuf)
            delete[] mbuf;
        else if(!buftype && fbuf){
            fclose(fbuf);
        }
    };
    tr_buffer() = delete;
    tr_buffer(const tr_buffer&) = delete;
    tr_buffer(unsigned int bufsize){
        buftype = Mbuf;
        bufsize = (bufsize != 0 && bufsize < max_buf_size ? bufsize : max_buf_size);
        this->bufsize = bufsize;
        mbuf = new _byte[bufsize]{0};
        bufprog = progress(bufsize);
    }
    /**
     * 构造一个文件缓冲区
     * 不指定文件名会创建一个临时文件
     */
    tr_buffer(const char* fbufname, unsigned int bufsize){
        buftype = Fbuf;
        this->bufsize = bufsize;
        fbuf = fopen(fbufname, "wb+");
        if(!fbuf) fbuf = tmpfile();
        bufprog = progress(bufsize);
    }
    /**
     * 从缓冲区指定位置读
     * end 超出不负责
     */
    bool get_buf(void* dest, unsigned int beg, unsigned int end){
        return dest && beg >= 0 && end > beg && buf_read(dest, beg, end - beg);
    };
    /**
     * 取空块
     * 在往缓冲区写的时候用的到
     */
    bool get_empty_block(unsigned int& beg, unsigned int& end, int b = 0){
        return bufprog.empty_block(beg, end, b);
    }
    /**
     * 读指定有数据块的数据和起始位置信息
     */
    bool get_block(void* dest, unsigned int& beg, unsigned int& end, int b = 0){
        if(!bufprog.block(beg, end, b)) return false;
        if(!dest) return true;
        return buf_read(dest, beg, end - beg);
    };
    /**
     * 往指定位置写入数据
     * 超出范围会返回false
     */
    bool put(const void* dsrc, unsigned int beg, unsigned int len){
        return bufprog.put(beg, beg + len) && buf_write(dsrc, beg, len);
    };
};