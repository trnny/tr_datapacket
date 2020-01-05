#pragma once

#include<iostream>
#include<string>
#include<cstring>
#include<fstream>
#include<cstdio>
#include<map>

using namespace std;

typedef unsigned char _byte;


struct mdata
{
    uint16_t data_len;
    _byte* data_ptr;
};

/**
 * 作用类似于map<string, mdata>
 * 放到一个数据块里方便传输
 * 每次放进去数据是不能更改的
 */
class mpack
{
private:
    uint16_t cl = 0;    // 总个数
    uint16_t tl = 4;    // 总长度
    map<string, mdata> data;
    void push(const map<string, mdata>& ad) {
        for(auto iter = ad.begin(); iter != ad.end(); iter++) {
            if(data.find(iter->first) != data.end()) continue;
            _byte* p = new _byte[iter->second.data_len];
            if(p) {
                mdata t = {iter->second.data_len, p};
                if( memcpy(t.data_ptr, iter->second.data_ptr, t.data_len) ) {
                    cl ++;
                    tl += 4 + iter->first.length() + 1 + t.data_len;
                    data[iter->first] = t;
                }
                else 
                    delete[] p;
            }
        }
    }
    void load(const map<string, mdata>& cd) {clear();push(cd);}
public:
    mpack(){};
    mpack(const mpack& copy) {push(copy.data);}
    /**
     * 从一个地址加载,比如用packet后的小包
     * 这个是用于在网络上传送或保存成2进制文件
     */
    mpack(void* mpacket) {
        if(!mpacket) return;
        // memcpy(&cl, mpacket, 2);
        // memcpy(&tl, mpacket+2, 2);
        cl = *(uint16_t*)(mpacket);
        tl = *(uint16_t*)(mpacket+2);
        int saved = 4;
        char* key;
        uint16_t key_len, data_len;
        for(int i =0;i < cl; i++){
            // memcpy(&key_len, mpacket + saved, 2);
            key_len = *(uint16_t*)(mpacket + saved);
            key = (char*)mpacket + saved + 2;
            saved += 2 + key_len;
            // memcpy(&data_len, mpacket + saved, 2);
            data_len = *(uint16_t*)(mpacket + saved);
            _byte* p = new _byte[data_len];
            if(!p) {clear();return;}
            memcpy(p, mpacket + saved + 2, data_len);
            data[key] = { data_len, p };
            saved += 2 + data_len;
        }
        if(saved != tl) clear();    // 发生错误
    }
    mpack& operator=(const mpack& ap){load(ap.data);return *this;}
    /**
     * 回到初始状态
     */
    void clear() {
        cl = 0;
        tl = 4;
        for(auto iter = data.begin(); iter!=data.end() ; delete[] iter->second.data_ptr, iter++);
        data.clear();
    }
    bool push(const char* key, const void* data_ptr, uint16_t data_len){
        uint16_t key_len = strlen(key) + 1;
        if(key_len == 1 || !data_len || tl + 4 + key_len + data_len > 65507) return false;   // key长度问题 或总长度超出
        if(data.find(key) != data.end()) return false;          // key已经有值
        _byte* p = new _byte[data_len];
        if(!p || !memcpy(p, data_ptr, data_len)) return false;    // 空间申请失败 或者复制失败
        cl++;
        tl += 4 + key_len + data_len;
        data[key] = {data_len, p};
        return true;
    };
    bool push(const char* key, const char* str) {
        if(!str) return false;
        return push(key, str, strlen(str) + 1);
    }
    bool getdata(const char* key, void* data_ptr, int& data_len)const {
        if(!key || strlen(key) == 0 || !data_ptr) return false;
        if(data.find(key) == data.end()) return false;
        mdata t = data.at(key);
        if(!memcpy(data_ptr, t.data_ptr, t.data_len)) return false;
        data_len = t.data_len;
        return true;
    }
    /**
     * 将map里面数据放到save里
     * 返回对象长度(可以先用该函数获取预计需要的空间)
     */
    uint16_t packet(void* save = NULL) const{
        if(!save) return tl;
        memcpy(save, &cl, 2);
        memcpy(save+2, &tl, 2);
        void* saved = save + 4;
        uint16_t key_len, data_len;
        for(auto iter = data.begin(); iter!= data.end(); iter++){
            key_len = iter->first.length() + 1;
            data_len = iter->second.data_len;
            memcpy(saved, &key_len, 2);
            memcpy(saved+2, iter->first.c_str(), key_len);
            saved = saved+2+key_len;
            memcpy(saved, &data_len, 2);
            memcpy(saved+2, iter->second.data_ptr, data_len);
            saved = saved+2+data_len;
        }
        return tl;
    }
    ~mpack() {clear();}
};



/**
 * 单个包,由字符串和数据组成
 * 用于把多个字节组合在一起
 * 好处是能自由读指定字节数据
 */
class pack{
    uint16_t len = 0;       // 总长度
    uint16_t pt = 0;        // 有数据的尾部(已用长度)
    _byte* data = NULL;     // [0-1] 描述长度 [2-3] 数据(总)长度

public:
    pack() = delete;
    pack(const pack& cp){
        len = cp.len;
        pt = cp.pt;
        data = new _byte[len]{0};       // 虽然申请了总长度的空间
        memcpy(data, cp.data, pt);      // 但是只给有值的部分内存复制一下就行了
    };
    pack& operator=(const pack& ap){
        if(this == &ap) return *this;
        if(NULL != data) delete[] data; // 先清理掉旧数据的空间
        len = ap.len;
        pt = ap.len;
        data = new _byte[len]{0};
        memcpy(data, ap.data, pt);
        return *this;
    };
    /**
     * 从对象内存复制构造
     * 可以用于把packet后的对象再转成pack
     * pt为data的长度
     */ 
    pack(void* _pack, uint16_t pt){
        len = pt;
        this->pt = pt;
        data = new _byte[pt];
        memcpy(data, _pack, pt);
    };
    /**
     * 给定字段名和包长(数据长,非总长)
     * 包的总长度最大为65507
     */ 
    pack(const char* desc, uint16_t size = 0){                         // 申请空间没有放数据进去
        pt = 4;
        uint16_t desc_len = strlen(desc) + 1;
        pt += desc_len;
        if(size + pt > 65507)size = 65507 - pt;
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
    pack(const char* desc, const char* strdata){
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
     * 这个构造复制一个指针内存对象到pack
     * 参数一描述 参数二源地址 参数三要复制的长度
     */
    pack(const char* desc, void* dptr, uint16_t size){
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
    bool push(const void* dptr, uint16_t size) {
        if(!data || pt + size > 65507) return false;
        if(pt + size > len) {           // 重新申请空间并复制 如果不需要扩长 dlen没变
            uint16_t dlen = *(uint16_t*)(data + 2);
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
    void get(char* desc, char* dest)const{
        desc && strcpy(desc, (const char*)data + 4);
        if(!dest) return;
        uint16_t desc_len = *(uint16_t*)data;
        strcpy(dest, (const char*)data + 4 + desc_len);
    };
    /**
     * 将有数据的部分读出, 即忽略包尾空数据(如果有)
     * 参数一保存描述 参数二保存读到的数据 参数三保存读到的长度
     * 注意参数二的容量, 容量不足会引发潜在错误
     * 需要读取部分范围数据, 使用getd
     * 可以用该函数取内容长度
    */
    void get(char* desc, void *dest, int& len)const{
        desc && strcpy(desc, (const char*)data + 4);
        uint16_t desc_len = *(uint16_t*)data;
        uint16_t dlen = *(uint16_t*)(data + 2);
        len = dlen + pt - this->len;
        dest && memcpy(dest, data + 4 + desc_len, len);
    };
    /**
     * 将数据全部读出 包括包尾可能存在的空数据
     * 参数一保存读到的数据 参数二保存数据的总长度
     * 由于是全部读出,所以接收数据的参数二容量得够
    */
    void geta(void *dest, int& len)const{
        uint16_t desc_len = *(uint16_t*)data;
        uint16_t dlen = *(uint16_t*)(data+2);
        len = dlen;
        memcpy(dest, data + 4 + desc_len, len);
    };
    /**
     * 按照偏移读
     * 从start开始读len长读到dest中
     */
    void getd(void* dest, uint16_t len, uint16_t start = 0)const{
        uint16_t desc_len = *(uint16_t*)data;
        uint16_t dlen = *(uint16_t*)(data+2);
        memcpy(dest, data + 4 + desc_len + start, start + len > dlen ? dlen - start : len);
    };
    /**
     * 将包里面所有数据取出
     * 参数一用来放包的数据 参数二用来放包长度
     * 这里不会将包尾空数据(如果有)取出
     */
    bool packet(void* dest, int& size) const{
        if(len == 0 || !data) return false;                  // 包是空的(肯定是前面哪出错)
        size = pt;
        memcpy(dest, data, pt);
        if(pt != len){                              // 压缩一点,去除后面没用数据
            uint16_t dlen = *(uint16_t*)(data+2);
            dlen -= len - pt;                       // `len - pt` 是空数据长度
            memcpy(dest + 2, &dlen, 2);
        }
        return true;
    };
    /**
     * 返回包大小, 
     * true则返回总长度 false表示已用长度
     */
    uint16_t length(bool f = false) const{
        return f ? len : pt;
    };
    /**
     * 返回这个包最大的容量
     */
    uint16_t maxsize() const{
        uint16_t desc_len = *(uint16_t*)data;
        // memcpy(&desc_len, data, 2);
        return 65503 - desc_len;
    };
    ~pack(){
        if(NULL != data){
            delete[] data;
            data = NULL;
        }
    };
};

/**
 * 块信息  , 
 * 为了范围更大用64位整数
 */
struct stblock{
    stblock* nex = NULL;
    uint64_t beg;
    uint64_t end;
    stblock(uint64_t beg, uint64_t end){
        this->beg = beg;
        this->end = end;
    }
};
/**
 * 进度信息
 * 在原来基础上增加移除子段接口
 * 接口都换成64位无符号对应的
 */
class progress{
    uint64_t end;          // 初始化时指定的结尾
    stblock* head;
public:
    /**
     * 参数指定进度终止长度
     * 0表示不定长
     */
    progress(uint64_t end){this->end = end;head = new stblock(0,0);};
    progress(){end = 0;head = new stblock(0,0);}
    progress(const progress& cp){
        end = cp.end;
        stblock* p = cp.head, *q;
        head = new stblock(p->beg, p->end);
        for(q = head, p = p->nex;NULL != p;){
            q->nex = new stblock(p->beg, p->end);
            p = p->nex;
            q = q->nex;
        }
    }
    /**
     * 从保存了进度信息的地址读进度信息
     */
    progress(const void* dsrc){         // end/count/blocks...
        this->end = *(uint64_t*)dsrc;
        uint64_t cnt = *(uint64_t*)(dsrc+8), beg = *(uint64_t*)(dsrc+16), end = *(uint64_t*)(dsrc+24);  //  <-- head
        head = new stblock(beg, end);
        stblock* p = head;
        cnt *= 16;
        for(uint64_t i = 32;i < cnt;i += 16){    // 从32开始是因为head已经处理过了
            beg = *(uint64_t*)(dsrc + i);
            end = *(uint64_t*)(dsrc + i + 8);
            p->nex = new stblock(beg, end);
            p = p->nex;
        }
    }
    ~progress(){
        stblock* p;
        while(head){
            p = head->nex;
            delete head;
            head = p;
        }
    }
    progress& operator=(const progress& sp){
        if(this == &sp) return *this;
        end = sp.end;
        stblock* p = sp.head, *q;
        while(head){
            q = head->nex;
            delete head;
            head = q;
        }
        head = new stblock(p->beg, p->end);
        for(q = head, p = p->nex;NULL != p;){
            q->nex = new stblock(p->beg, p->end);
            p = p->nex;
            q = q->nex;
        }
    }
    /**
     * 取第一个完整块百分百进度
     * -1表示未知
     */
    int prog() const {
        if(!end) return -1;
        return (head->end - head->beg) * 100 / end;
    }
    /**
     * 取总百分百进度
     */
    int proga() const {
        if(!end) return -1;
        return size() * 100 / end;
    }
    /**
     * 取总大小
     */
    uint64_t size() const {
        uint64_t cnt = 0;
        stblock* p = head;
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
    bool empty_block(uint64_t& beg, uint64_t& end,uint64_t b = 0) const {
        if(b == 0 && head->beg){                    // 开头有空块
            beg = 0;
            end = head->beg;
            return true;
        }
        if(b == 0 && !head->end && this->end){      // 一整块都是空 (head{0,0})
            beg = 0;
            end = this->end;
            return true;
        }
        uint64_t i = !!head->beg;                    // 不为0表示head前面有段空
        for(stblock* p = head; NULL != p;p = p ->nex, i++){
            if(i == b){
                if(!p->nex && !this->end) return false;  // p是最后一块 但是未指定end时会返回false
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
    bool block(uint64_t& beg, uint64_t& end, uint64_t b = 0) const {
        stblock *p = head;
        for(uint64_t i = 0;i < b && p;i++,p = p->nex);
        if(!p) return false;
        beg = p->beg;
        end = p->end;
        return true;
    }
    /**
     * 取块的个数
     * 建议在save()之前用这个函数预先算得需要多大的空间来放数据
     * 加1后乘16
     */
    uint64_t count() const {
        stblock* p= head;
        uint64_t i = 0;
        for(; NULL != p; i++, p = p->nex);
        return i;
    };
    /**
     * 填充从beg到end的进度
     * 不能与已有的段有相交
     */
    bool put(uint64_t beg, uint64_t end){
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
            stblock* p = new stblock(beg, end);
            p->nex = head;
            head = p;
            return true;
        }
        stblock* p = head;
        while(NULL != p){
            if(NULL == p->nex){
                if(p->end > beg) return false;
                else if(p->end == beg) p->end = end;
                else p->nex = new stblock(beg, end);
                return true;
            }
            if(beg >= p->nex->end) {
                p = p->nex;
                continue;
            }
            if(p->end > beg || p->nex->beg < end) return false;
            if(p->end == beg && p->nex->beg == end){
                stblock* t = p->nex;
                p->nex = t->nex;
                p->end = t->end;
                delete t;
            }
            else if(p->end < beg && p->nex->beg > end){
                stblock* t = new stblock(beg, end);
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
     * 丢掉指定区间进度
     */
    void drop(uint64_t beg, uint64_t end){
        if(beg >= end || beg < 0 || end < head->beg) return;
        if(this->end && end > this->end) return;
        stblock* p = head, *t, *d;
        bool pi = false, ti;
        // 找到 t,p位置, 确定 ti,pi
        while(p){
            if(beg < p->beg) ti = false; // 左
            else if(beg <= p->end) ti = true; // 区间内
            else {  // 右移
                p = p->nex;
                continue;
            }
            t = p;
            break;
        }

        while (p)
        {
            if(end >= p->beg && end <= p->end) pi = true;
            else if(p->nex && end > p->nex->beg){
                p = p->nex;
                continue;
            }
            break;
        }
        if(ti && pi && t == p){     // t后插入d
            d = new stblock(end, p->end);
            d->nex = t->nex;
            t->nex = d;
            t->end = beg;
        }
        else {
            if(ti) t->end = beg;
            if(pi) p->beg = end;
            if(t != p) {
                if(!ti) {
                    t->beg = 0;
                    t->end = 0;
                }
                if(!pi) {
                    p->beg = 0;
                    p->end = 0;
                }
            }
            // 删除从t到p之间
            d = t->nex;
            if( t != p)
                while(d != p){
                    t->nex = d->nex;
                    delete d;
                    d = t->nex;
                }

        }

        // 删除空段
        while (head->beg == head->end)  // 后面依然可能有(最多前后分别一个)
        {
            if(head->nex) {
                p = head;
                head = head->nex;
                delete p;
            }
            else {      // 只有头块
                head->beg = 0;
                head->end = 0;
            }
        }
        p = head;
        t = p->nex; // p的下一个
        while (t)
        {
            if(t->beg == t->end){
                p->nex = t->nex;
                delete t;
                t = p->nex;
                continue;
            }
            p = p->nex;
            t = p->nex;
        }

    }
    /**
     * 将进度信息保存在指定位置
     * 数据长度 / 16 - 1 = 结点数
     * 返回结果为保存的数据的长度
     */
    uint64_t save(void* dest) const {    // end/count/blocks...
        if(!dest) return 0;
        memcpy(dest, &end, 8);
        stblock* p= head;
        uint64_t len;
        for(len = 0;p;len+=16, memcpy(dest + len, &(p->beg), 8), memcpy(dest + len + 8, &(p->end), 8), p = p->nex);
        memcpy(dest + 8, &len, 8);
        return len;
    }
};


/**
 * 建立文件或内存缓冲区
 * 修改为与64位无符号整数对应
 * 取消最大bufsize限制
 */
class buffer{
    const static bool Mbuf = true;
    const static bool Fbuf = false;
    // unsigned long int max_buf_size = 4 * 1024 * 1024;       // 已取消
    typedef bool BufType;
    _byte* mbuf = NULL;         // 指针没有设定默认值不能访问
    FILE* fbuf = NULL;
    progress bufprog;
    uint64_t bufsize;
    BufType buftype;
    /**
     * 从src将数据写入缓冲区
     * 内存或文件,
     * 如果是文件,会自动将bufsize改动
     * bufsize改动只有在一开始指定为0的情况有用
     */
    bool buf_write(const void* dsrc, uint64_t beg, uint64_t len) {
        return (buftype && mbuf && memcpy(mbuf + beg, dsrc, len)) || (!buftype && fbuf && !fseek(fbuf, beg, 0) && fwrite(dsrc, 1, len, fbuf) && ((bufsize < beg + len && (bufsize = beg + len)) || true));
    };
    bool buf_read(void* dest, uint64_t beg, uint64_t len) const {
        return (buftype && mbuf && memcpy(dest, mbuf + beg, len)) || (!buftype && fbuf && !fseek(fbuf, beg, 0) && fread(dest, 1, len, fbuf));
    };
public:
    ~buffer(){
        if(buftype && mbuf)
            delete[] mbuf;
        else if(!buftype && fbuf){
            fclose(fbuf);
        }
    };
    buffer() = delete;
    buffer(const buffer&) = delete;
    buffer(uint64_t bufsize){
        buftype = Mbuf;
        this->bufsize = bufsize;
        mbuf = new _byte[bufsize]{0};       // 若bufsize过大,可能会失败
        bufprog = progress(bufsize);
    }
    /**
     * 构造一个文件缓冲区
     * 不指定文件名会创建一个临时文件
     */
    buffer(const char* fbufname, uint64_t bufsize){
        buftype = Fbuf;
        this->bufsize = bufsize;
        fbuf = fopen(fbufname, "wb+");
        if(!fbuf) fbuf = tmpfile();
        bufprog = progress(bufsize);
    }
    /**
     * 判断缓冲区是否成功
     */
    bool good() const{
        return (buftype == Mbuf && mbuf || buftype == Fbuf && fbuf);
    }
    /**
     * 从缓冲区指定位置读
     * end 超出不负责
     */
    bool get_buf(void* dest, uint64_t beg, uint64_t end) const{
        return dest && beg >= 0 && end > beg && buf_read(dest, beg, end - beg);
    };
    /**
     * 取空块
     * 在往缓冲区写的时候用的到
     */
    bool get_empty_block(uint64_t& beg, uint64_t& end, int b = 0) const{
        return bufprog.empty_block(beg, end, b);
    }
    /**
     * 读指定有数据块的数据和起始位置信息
     */
    bool get_block(void* dest, uint64_t& beg, uint64_t& end, int b = 0) const{
        if(!bufprog.block(beg, end, b)) return false;
        if(!dest) return true;
        return buf_read(dest, beg, end - beg);
    };
    /**
     * 往指定位置写入数据
     * 超出范围会返回false
     */
    bool put(const void* dsrc, uint64_t beg, uint64_t len) {
        return bufprog.put(beg, beg + len) && buf_write(dsrc, beg, len);
    };
    /**
     * 移除指定位置数据,
     * cl为真时才会清文件
     * 移除指定段progress纪录
     */
    void remove(uint64_t beg, uint64_t len, bool cl = false) {
        bufprog.drop(beg, beg + len);
        if(cl){
            _byte* temp = new _byte[len]{0};
            buf_write(temp, beg, len);
            delete[] temp;
        }
    }
};