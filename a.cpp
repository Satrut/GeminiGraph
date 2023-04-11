#include<iostream>
#include<list>
using namespace std;

struct A {
    int aaa;
    double bbbb;
    list<int> l;
};


int main() {
    A a;
    for (int i = 0;i < 10;i++) {
        a.l.push_back(i);
    }
    A b = a;
    auto it1 = b.l.begin(), it2 = a.l.begin();
    cout << "a:" << &a << endl;
    cout << "b:" << &b << endl;
    cout << "a.l:" << &a.l << endl;
    cout << "b.l:" << &b.l << endl;
    for (;it1 != b.l.end(); it1++, it2++) {
        cout << (*it1 == *it2) << endl;
    }
    return 0;
}