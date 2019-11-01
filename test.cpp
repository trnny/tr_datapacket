#include "datapacket.h"

int main(){
    int a = 34643875;
    tr_buffer buff(1024);
    buff.put("123", 0, 3);
    buff.put("567", 10, 3);
    buff.put(&a, 300, 4);
    a = 0;
    int b, e;
    buff.get_block(&a, b, e, 2);
    cout << a << ' ' << b << ' ' << e << endl;

    tr_buffer buff2("./tmp.buf", 10);
    buff2.put("123", 0, 3);
    buff2.put("789", 5, 4);
}