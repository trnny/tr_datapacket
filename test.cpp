#include "datapacket.h"
#include <iomanip>
#include <map>

_byte a[1024];          // 模拟双方收到的数据包
int pl;                 // 模拟双方收到的数据包长度
/**
 * 模拟获得消息
 */
bool _f = true;

bool ___p(){
    // 模拟产生请求
    
}
void ___v(){
    // 模拟处理请求
    char desc[40];
    tr_field tr(a, pl); // 收到的请求
    tr.get(desc, 0);
    string str(desc);
    if(str == "请求文件准备"){              // name
        char fn[40];
        tr.get(0, fn);
        int fnl = strlen(fn) + 1;
        int fsize, fid;
        tr_field trb("应答文件准备", 300);  // id/size/name
        // ... 假设这里找到了文件
        if(_f){
            fsize = 100, fid = 123;
            cout << "请求文件(找到): " << fn << ",size: " << fsize << ", id: " << fid << endl;
        }
        // ... 假设这里没找到文件
        else{
            fsize = 0, fid = 0;
            cout << "请求文件(未找到): " << fn << endl;
        }
        trb.push(&fid, 4);
        trb.push(&fsize, 4);
        trb.push(fn, fnl);
        trb.packet(a, pl);
    }
    else if(str == "请求文件中"){           // id/beg
        int fid, beg;
        tr.getd(&fid, 4, 0);
        tr.getd(&beg, 4, 4);
        tr_field trb("应答文件中", 300);    // id/beg/len/data
        trb.push(&fid, 4);
        trb.push(&beg, 4);
        // ... 假设这是传输的数据
        char* rdata = "文件内容第一部分";
        int len = strlen(rdata);
        trb.push(&len, 4);
        trb.push(rdata, len);
        trb.packet(a, pl);
    }
    else if(str == "请求文件确认"){         // id/status
        // ... 请求完成
    }
}

int main(){
    _byte b[1024];
    datapacket(1024);
    while (___p())
    {
        ___v();
    }

    return 0;
}