// expect: 7
int deref(int *p){ return *p; }
int main(){ int x=7; return deref(&x); }
