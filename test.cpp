#include "datapacket.h"

int main(){
    int a = 34643875;
    tr_buffer buff(1024);
    buff.put("123", 0, 3);
    buff.put("567", 10, 3);
    buff.put(&a, 300, 4);
    a = 0;
    unsigned int b, e;
    buff.get_block(&a, b, e, 2);
    cout << a << ' ' << b << ' ' << e << endl;
    tr_buffer buff2("./tmp.buf", 0);    // 0表示总大小未知
    char c = 'a';
    int i = 0;
    for(;i < 10;i++){
        buff2.put(&c, i, 1);
        c++;
    }
    c = 0;
    buff2.put(&c, i, 1);
    a = 0;
    for(i = 20;i < 70; i+= 5){
        buff2.put(&a, i, 4);
        a++;
    }
    char s[20];
    buff2.get_block(s, b, e);
    cout << s << endl;
    for(i = 1; buff2.get_block(&a, b, e, i);i++, cout << a << endl);
    char tmpa[50];
    int tmpl;
    progress pr1(1233);
    pr1.put(20, 46);
    pr1.put(57, 99);
    pr1.save(tmpa, tmpl);
    cout << "保存和读\n块数:" << tmpl << endl;
    progress pr2(tmpa);
    for(int i = 0; pr2.block(b, e, i); i++){
        cout << b << ' ' << e << endl;
    }
}