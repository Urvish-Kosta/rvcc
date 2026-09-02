// expect: 3
int main(){ int a[3]; a[0]=1; a[1]=2; a[2]=3; int *p=a; return *(p+2); }
