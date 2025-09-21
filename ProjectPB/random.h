#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <windows.h>
#include <time.h>

char *sign="zxcvbnm";
char *row="WRGBYES";
int te[7]={0,1,2,3,4,5,6};
#define random(x) (rand()%x)//random function
extern int iro;

void swap(int *x, int *y) {
	int p;
	p=*x;
	*x=*y;
	*y=p;
}

void ran()
{
	int i;
	char order[4];
	FILE *fp;
	fp=fopen("./1.txt","at+");
	srand(time(NULL));
	for (i=3;i>=1;--i) swap(&te[i],&te[random(i)]);
	for (i=0;i<7;++i) order[te[i]]=i;
	//printf("%d:%d-%d-%d-%d-%d %c-%c-%c-%c-%c\n",pic,test[0],test[1],test[2],test[3],test[4],sign[order[0]],sign[order[1]],sign[order[2]],sign[order[3]],sign[order[4]]);
	//fprintf(fp,"%c\t%c\t%c\t%c\n%c\t%c\t%c\t%c\n\n",row[0],row[1],row[2],row[3],sign[order[0]],sign[order[1]],sign[order[2]],sign[order[3]]);
	//fclose(fp);
	iro=te[0];
}