#include "datapacket.h"

int main(){
    tr_field tr1("123", "34535");
    tr_field tr2("345", 20);
    char a[20], b[20];
    _byte c[20];
    int l;
    tr1.packet(c, l);       // 存入_byte数组中
    cout << (uint16_t)c[0] << ", " << (uint16_t)c[2] << endl;
    tr_field tr3(c, l);
    tr3.packet(c, l);
    cout << l << endl;
    cout << (uint16_t)c[0] << ", " << (uint16_t)c[2] << endl;
    tr1.get(a, b, l);
    cout << a << ", " << b << ", " << l << endl;
    tr2.push("你好啊", 10);
    tr2.get(a, b, l);
    cout << a << ", " << b << ", " << l << endl;
    tr3.get(a, b, l);
    cout << a << ", " << b << ", " << l << endl;
}