#pragma once

#include<iostream>
#include<string>
#include<string.h>

using namespace std;

typedef unsigned char _byte;

class tr_field{
    unsigned int len = 0;       // 总长度
    unsigned int pt = 0;        // 有数据的尾部
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
    }
    /**
     * 从对象内存复制构造
     * 可以用于把字段packet后的对象再转成field
     */ 
    tr_field(void* _field, int pt){
        len = pt;
        this->pt = pt;
        data = new _byte[pt];
        memcpy(data, _field, pt);
    }
    /**
     * 给定字段名和包长(数据长,非总长)
     */ 
    tr_field(const char* desc, int size = 0){                         // 申请空间没有放数据进去
        len += 4;
        uint16_t desc_len = strlen(desc) + 1;
        len += desc_len;
        len += size;
        if(len <= 65507){
            pt = len - size;                                    // 留下了大小为`size`的空间
            data = new _byte[len]{0};
            memcpy(data, &desc_len, 2);                         // 描述长
            memcpy(data + 2, &size, 2);                         // 数据长
            memcpy(data + 4, desc, desc_len);                   // 描述
        }else {
            len = 0;
        }
    }
    /**
     * 如果数据是字符串,用这种构造方式更好
     * 参数一为描述 参数二为数据字符串
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
    tr_field(const char* desc, void* dptr, int size){
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
    }
    /**
     * 往包里塞数据, 自动扩容(如果没超)
     * 参数一源地址 参数二长度
     */
    bool push(const void* dptr, int size){
        if(pt + size > 65507) return false;
        if(pt + size > len) {           // 重新申请空间并复制 如果不需要扩长 dlen没变
            uint16_t dlen;
            memcpy(&dlen, data + 2, 2);
            dlen += pt + size - len;    // `pt + size` 是新长度
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
    }
    /**
     * 将有数据的部分读出, 即忽略包尾空数据(如果有)
     * 参数一保存描述 参数二保存读到的数据 参数三保存读到的长度
     * 注意参数二的容量, 容量不足会引发潜在错误
     * 需要读取部分范围数据, 使用getd
    */
    void get(char* desc, void *dest, int& len){
        desc && strcpy(desc, (const char*)data + 4);
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);                     // 取不到?
        uint16_t dlen;                                  // 数据总长度
        memcpy(&dlen, data + 2, 2);
        len = dlen + pt - this->len;
        memcpy(dest, data + 4 + desc_len, len);
    };
    /**
     * 将数据按照字符串读出
     * 参数一保存描述 参数二保存读到的字符串
    */
    void get(char* desc, char* dest){
        desc && strcpy(desc, (const char*)data + 4);
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        strcpy(dest, (const char*)data + 4 + desc_len);
    }
    /**
     * 按照偏移读
     * 从start开始读len长读到dest中
     */
    void getd(char* desc, void* dest, int len, int start = 0){
        desc && strcpy(desc, (const char*)data + 4);
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        uint16_t dlen;
        memcpy(&dlen, data + 2, 2);
        memcpy(dest, data + 4 + desc_len + start, start + len > dlen ? dlen - start : len);
    }
    /**
     * 将数据全部读出 包括包尾可能存在的空数据
     * 参数一保存描述 参数二保存读到的数据 参数三保存数据的总长度
     * 由于是全部读出,所以接收数据的参数二容量得够
    */
    void geta(char* desc, void *dest, int& len){
        desc && strcpy(desc, (const char*)data + 4);
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        uint16_t dlen;
        memcpy(&dlen, data + 2, 2);
        len = dlen;                                 // 保险起见, 读到uint16_t里面,万一读到int里面是错的
        memcpy(dest, data + 4 + desc_len, len);
    };
    /**
     * 将包里面所有数据取出
     * 参数一用来放包的数据 参数二用来放包长度
     * 这里不会将包尾空数据(如果有)取出
     */
    bool packet(void* dest, int& size){
        if(len == 0) return false;                  // 包是空的(肯定是前面哪出错)
        size = pt;
        memcpy(dest, data, pt);
        if(pt != len){                              // 压缩一点,去除后面没用数据
            uint16_t dlen;
            memcpy(&dlen, data + 2, 2);
            dlen -= len - pt;                       // `len - pt` 是空数据长度
            memcpy(dest + 2, &dlen, 2);
        }
        return true;
    }
    /**
     * 返回包大小, 
     * true则返回总长度 false表示已用长度
     */
    int length(bool f = false){
        return f ? len : pt;
    }
    /**
     * 返回这个包最大的容量
     */
    uint16_t maxsize(){
        uint16_t desc_len;
        memcpy(&desc_len, data, 2);
        return 65503 - desc_len;
    }
    ~tr_field(){
        if(NULL != data){
            delete[] data;
            data = NULL;
        }
    }
};

/**
 * 块信息
 */
struct tr_block{
    tr_block* nex = NULL;
    int beg;
    int end;
    tr_block(int beg, int end){
        this->beg = beg;
        this->end = end;
    }
};
/**
 * 进度信息
 */
class progress{
    int end;
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
     * 取第一个完整块进度
     * -1表示未知
     */
    int prog(){
        if(!end) return -1;
        return (head->end - head->beg) * 100 / end;
    }
    /**
     * 取总进度
     */
    int proga(){
        if(!end) return -1;
        return size() * 100 / end;
    }
    /**
     * 取总大小
     */
    int size(){
        int cnt = 0;
        tr_block* p = head;
        while(NULL != p){
            cnt += p->end - p->beg;
            p = p->nex;
        }
        return cnt;
    }
    /**
     * 取空的块
     * 参数三表示第b个空块
     */
    bool block(int& beg, int& end, int b = 0){
        if(head->beg && b == 0){
            beg = 0;
            end = head->beg;
            return true;
        }
        tr_block* p = head;
        int i = !!head->beg;
        for(; NULL != p;p = p ->nex, i++){
            if(i == b){
                if(NULL == p->nex && !this->end) return false;
                beg = p->end;
                end = NULL == p->nex ? this->end : p->nex->beg;
                return true;
            }
        }
        return false;
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
    bool put(int beg, int end){
        if(beg >= end) return false;                    // end比beg小
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
};
class datapacket{                                       // 将多个数据包组合起来
    const static int max_datagram_len = 65507;          // 每个数据包最大容量
    const static int max_buf_size = 1024 * 1024 * 4;    // 用内存作缓冲区时,最大容量
    _byte* buf;                                         // 缓冲区
    progress bufprog;
    int dlen, flen, svd, svp;
    _byte tempdata[max_datagram_len];                   // 单个包
public:
    datapacket() = delete;
    datapacket(int bufsize){
        buf = new _byte[bufsize]{0};
    };
    datapacket(const char* fn);                         // 用文件来存
    void getb(void* dest, int& len);                    // 取缓冲区第一个有连续数据的部分
    void putb(void* dsrc, int& len);
};