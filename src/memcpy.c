#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main()
{
	char* p="thisistest";
	int len=strlen(p);
	char* str=(char*)malloc(sizeof(char)*(len+1));
	memcpy(str,p,len);
	str[len]=0;        //必须加上对字符串设置结尾符
	printf("%s\n",str);
	return 0;
}
