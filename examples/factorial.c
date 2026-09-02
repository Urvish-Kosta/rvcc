// expect: 120
int main() {
    int f = 1;
    for (int i = 1; i <= 5; i = i + 1)
        f = f * i;
    return f;      // 5! = 120
}
