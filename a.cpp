#include<iostream>
using namespace std;

struct A {
    /* data */
    int aa;
    int bb;
    int *cc;
};

int main() {
    A a;
    a.aa = 1;
    a.bb = 2;
    // int tmp = 0;
    // a.cc = &tmp;
    a.cc = nullptr;
    // cout << a.aa << " " << a.bb << endl;
    A *b = new A(a);
    // cout << b->aa << " " << b->bb << endl;
    cout << &a << " " << b << endl;
    cout << a.cc << " " << b->cc << endl;
    return 0;
}