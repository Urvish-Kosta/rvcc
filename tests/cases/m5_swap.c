// expect: 73
int swap(int *a, int *b){ int t=*a; *a=*b; *b=t; return 0; }
int main(){ int x=3; int y=7; swap(&x,&y); return x*10+y; }
