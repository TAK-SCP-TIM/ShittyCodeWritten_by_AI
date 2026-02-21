#include <iostream>
#include <algorithm>
#include <cmath>
using namespace std;
int c,n;
int a[1002002];//所有幸运数
int b[1002002];//该数的下一个幸运数
int mn;
int main(){
	cin>>c>>n;
	mn=sqrt(c);
	for(int i=mn;i<=1001;i++){//给出所有幸运树
		if(i*i<c)continue;
		for(int j=1;j*i*i<=1002001;j++){
			a[j*i*i]=1;
		}
	}
	int next=1002002;
	for(int i=1002001;i>=1;i--){//预测这个数下一个幸运数
		if (a[i])next=i;
		b[i]=next;
	}
	for(int i=1;i<=n;i++){//对于每个数，判断是否为幸运数，不是给出下一个幸运数
		int x;
		cin>>x;
		if(a[x]){
			cout<<"lucky"<<endl;
		} else {
			cout<<b[x]<<endl;
		}
	}
	return 0;
}
