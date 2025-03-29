// 以下請新增 include 項目
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
using namespace std;

int main() {
    // code
    int a, b;
    cin >> a >> b;
    cout << a + b;
    for (int i = 0; i < 1000000000; ++i) {
        a += i;
        b += a;
        a %= b;
    }
    cout << a + b << endl;
}