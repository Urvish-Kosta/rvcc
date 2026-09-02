// expect: 159
// Sorts {5,2,8,1,9} ascending in place, then returns min*100 + median*10 + max.
int bubble(int *a, int n) {
    for (int i = 0; i < n; i = i + 1)
        for (int j = 0; j < n - 1 - i; j = j + 1)
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
    return 0;
}
int main() {
    int a[5];
    a[0] = 5; a[1] = 2; a[2] = 8; a[3] = 1; a[4] = 9;
    bubble(a, 5);                 // -> 1 2 5 8 9
    return a[0] * 100 + a[2] * 10 + a[4];   // 1*100 + 5*10 + 9 = 159
}
