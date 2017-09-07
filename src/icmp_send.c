#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netinet/ip_icmp.h>   //icmp packet
#include<sys/time.h>
/*
 * icmp封装在ip数据包里面，所以icmp请求可以直接使用网络层的原始套接字，socket()的第一个参数是PF_INET
 */
#define ICMP_PACKET_LEN sizeof(struct icmp)
void err_exit(const char* err_msg)
{
	perror(err_msg);
	exit(1);
}

/* 计算校验和
 * 当发送IP包时，需要计算IP报头的校验和：
1、  把校验和字段置为0；
2、  对IP头部中的每16bit进行二进制求和；
3、  如果和的高16bit不为0，则将和的高16bit和低16bit反复相加，直到和的高16bit为0，从而获得一个16bit的值；
4、  将该16bit的值取反，存入校验和字段。
当接收到IP包时，需要对报头进行确认，检查IP头是否有误，同上面2,3步，然后判断取反的结果是否为0
 */
unsigned short check_sum(unsigned short* addr, int len)
{
	int nleft=len;
	int sum=0;
	unsigned short* w=addr;
	while(nleft>1)
	{
		sum+=*w++;
		nleft-=2;
	}
	if(nleft==1)
	{
		sum+=*(unsigned char*)w;
	}
	sum=(sum>>16)+(sum & 0xfffff);  //将高16位与低16位相加
	sum+=(sum>>16);                 //将进位到高位的16位与低16位相加
	return (unsigned short)(~sum);  //取反
}

/*  fill the icmp packet */
struct icmp* fill_icmp_packet(int icmp_type, int icmp_sequ)
{
	struct icmp* icmp_packet;
	icmp_packet=(struct icmp*)malloc(ICMP_PACKET_LEN);
	icmp_packet->icmp_type=icmp_type;
	icmp_packet->icmp_code=0;
	icmp_packet->icmp_cksum=0;
	icmp_packet->icmp_id=htons(getpid());    //标识符，一般为进程ID号
	icmp_packet->icmp_seq=htons(icmp_sequ);  //序号
	/* 发送时间 */
	gettimeofday((struct timeval*)icmp_packet->icmp_data,NULL);   //icmp_data填充为时间
	/* 校验和*/
	icmp_packet->icmp_cksum=check_sum((unsigned short*)icmp_packet,ICMP_PACKET_LEN);
	return icmp_packet;
}
/* 发送请求 */
void icmp_request(const char* dst_ip, int icmp_type, int seq)
{
	struct sockaddr_in* dst_addr;
	struct icmp* icmp_packet;
	int sockfd,ret_len;
	char buf[ICMP_PACKET_LEN];
	/* 请求地址 */
	bzero(&dst_addr, sizeof(struct sockaddr_in));
	dst_addr->sin_family=AF_INET;
	dst_addr->sin_addr.s_addr=inet_addr(dst_ip);

	if((sockfd=socket(PF_INET,SOCK_RAW,IPPROTO_ICMP))==-1)
	{
		err_exit("sock() faild");
	}
	icmp_packet=fill_icmp_packet(icmp_type, seq);
	memcpy(buf,icmp_packet,ICMP_PACKET_LEN);
	/* send request */
	ret_len=sendto(sockfd,buf,ICMP_PACKET_LEN,0,(struct sockaddr*)&dst_addr,sizeof(struct sockaddr_in));
	if(ret_len>0)
	{
		printf("sendto() success\n");
	}
	close(sockfd);
}
int main(int argc, char* argv[])
{
	if(argc!=2)
	{
		printf("usage:%s dst_ip\n",argv[0]);
		exit(1);
	}
	icmp_request(argv[1],8,1);      //icmp_type为8表示发送，序号为1
	return 0;
}


