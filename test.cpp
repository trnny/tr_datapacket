#include "datapacket.h"
#include <iomanip>

int main(){
    tr_field d1("ceshi1", 200);
    tr_field d2 = d1;
    char c[40], t[40];
    d2.push("\0\0\1\0", 4);
    d2.push("hello world", 12);
    _byte p[100];
    int l;
    d2.packet(p, l);
    tr_field d3(p, l);
    d2.getd(0, &l, 4);
    d3.getd(c, t, 12, 4);
    cout << l << '\n' << c << " " << t << endl;
    char *cz = "中文会咋样";
    int czl = strlen(cz) + 1;
    d3.push(cz, czl);
    d3.get(0, p, l);
    tr_field d4("测试2", p, l);
    d4.getd(c, t, czl, 16);
    cout << c << " " << t << endl;
    progress pr;
    pr.put(0, 23);
    pr.put(45, 70);
    pr.put(24, 30);
    pr.put(23, 24);
    cout << pr.prog() << " " << pr.size() << endl;
    progress pr2(350);
    pr2.put(100, 150);
    progress pr3(pr2);
    pr3.put(20, 80);
    cout << pr2.size() << " " << pr3.size() << endl;
    progress pr4 = pr3;
    pr4.put(234, 333);
    cout << pr2.size() << " " << pr3.size() << " " << pr4.size() << endl;
    int beg, end;
    if(pr4.block(beg, end))
        cout << beg << " " << end << endl;
    if(pr4.block(beg, end, 1))
        cout << beg << " " << end << endl;
    if(pr4.block(beg, end, 2))
        cout << beg << " " << end << endl;
    pr4.put(0, 20);
    if(pr4.block(beg, end, 0))
        cout << beg << " " << end << endl;
    return 0;
}