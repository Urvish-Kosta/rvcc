// expect: 60
int main(){ int a[4]; for(int i=0;i<4;i=i+1) a[i]=i*10; int *p=a; int s=0; for(int i=0;i<4;i=i+1) s=s+*(p+i); return s; }
