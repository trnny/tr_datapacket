#include "datapacket.h"

int main(){
    buffer buf1("./tmp.buf", 0);
    buf1.put("udfgjhkdfhgkjerudbkfld", 0, 23);
    buf1.put("我爱中国", 50, 13);
    int a = 1024;
    uint64_t b, e;
    buf1.put(&a, 100, 4);
    char t[30];
    buf1.get_block(t, b, e);
    cout << t << ", " << b << ", " << e << endl;
    buf1.get_block(t, b, e, 1);
    cout << t << ", " << b << ", " << e << endl;
    buf1.get_block(&a, b, e, 2);
    cout << a << ", " << b << ", " << e << endl;
}