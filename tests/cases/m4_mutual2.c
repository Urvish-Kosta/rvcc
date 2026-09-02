// expect: 0
int iseven(int n){ if(n==0) return 1; return isodd(n-1); }
int isodd(int n){ if(n==0) return 0; return iseven(n-1); }
int main(){ return iseven(7); }
