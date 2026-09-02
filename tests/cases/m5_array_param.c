// expect: 30
int sum(int *p, int n){ int s=0; for(int i=0;i<n;i=i+1) s=s+p[i]; return s; }
int main(){ int a[3]; a[0]=5; a[1]=10; a[2]=15; return sum(a,3); }
