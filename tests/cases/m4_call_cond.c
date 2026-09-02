// expect: 100
int pos(int x){ return x>0; }
int main(){ int r=0; if(pos(5)) r=100; return r; }
