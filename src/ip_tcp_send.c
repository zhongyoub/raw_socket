/*
 * 构造发送IP-TCP的报文，tcp报文封装在IP中，此时套接字：
 * sockfd=socket(PF_INET,SOCK_RAW,IPPROTO_TCP)
 * 此时只能构造tcp报文，如果想进一步构造ip首部，需要开启sockfd的IP_HDRINCL
 * int on=1;
 * setsockopt(sockfd,IPPROTO_IP,IP_HDRINCL,&on, sizeof(on))
 *
 * u_int16_t * trans_hdr = (u_int16_t *)((char*)ip_hdr + ip_hdr->ihl*4); 从IP首部进入到tcp或udp首部
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>

/* ip header length */
#define IP_HEADER_LEN sizeof(struct ip)

/* tcp header length */
#define TCP_HEADER_LEN sizeof(struct tcphdr)

/* ip_tcp header length */
#define IP_TCP_HEADER_LEN IP_HEADER_LEN+TCP_HEADER_LEN

void err_exit(const char* err_msg)
{
	perror(err_msg);
	exit(1);
}
/*
 * check sum
 */
unsigned short check_sum(unsigned short* addr ,int len)
{
	int len_left=len;
	int sum=0;
	unsigned short* w=addr;
	while(len_left >1)
	{
		sum+=*w++;
		len_left-=2;
	}
	if(len_left==1)
	{
		sum+=*(unsigned char*)w;
	}
	sum=(sum>>16)+(sum&0xffff);   //高16位和低16相加
	sum+=(sum>>16);               //将进位相加
	return (unsigned short)(~sum);
}

/*
 * fill ip header
 */
void fill_ip_header(const char* src_ip,const char* dst_ip, int ip_packet_len, struct ip* ip_header)
{
//	struct ip* ip_header;
//	ip_header=(struct ip*)malloc(IP_HEADER_LEN);
	ip_header->ip_v=IPVERSION;             // ip version 4
	ip_header->ip_hl=sizeof(struct ip)/4;  //ip首部长度的单位是4个字节
	ip_header->ip_tos=0;                     //ip的服务类型
	ip_header->ip_len=htons(ip_packet_len);  //ip数据包总长度
	ip_header->ip_id=0;                      //让内核自己填充标识
	ip_header->ip_off=0;
	ip_header->ip_ttl=MAXTTL;               // 生存期 TTL 255
	ip_header->ip_p=IPPROTO_TCP;           // ip包封装的协议类型
	ip_header->ip_sum=0;
	ip_header->ip_src.s_addr=inet_addr(src_ip);
	ip_header->ip_dst.s_addr=inet_addr(dst_ip);
	ip_header->ip_sum=check_sum((unsigned short*)ip_header, IP_HEADER_LEN);  //计算校验和
//	return ip_header;
}

/*
 * fill tcp header
 */
void fill_tcp_header(int src_port ,int dst_port,struct tcphdr* tcp_header)
{
//	struct tcphdr* tcp_header;
//	tcp_header=(struct tcp*)malloc(TCP_HEADER_LEN);
	tcp_header->th_sport=htons(src_port);
	tcp_header->th_sport=htons(dst_port);
	tcp_header->th_off=sizeof(struct tcphdr)/4;
	tcp_header->th_win=4096;
	tcp_header->th_sum=0;
	tcp_header->th_flags=TH_SYN;
	tcp_header->th_urp=0;
	tcp_header->th_sum=check_sum((unsigned short*)tcp_header, TCP_HEADER_LEN);
//	return tcp_header;
}

void ip_tcp_send(const char* src_ip, int src_port, const char* dst_ip ,int dst_port, const char* data)
{
	struct ip* ip_header;
	struct tcphdr* tcp_header;
	struct sockaddr_in dst_addr;
	socklen_t sock_addr_len=sizeof(struct sockaddr_in);   // socklen_t == int
	int data_len=sizeof(data);
	int pack_len=IP_TCP_HEADER_LEN+data_len;
	char buf[pack_len];
	int sockfd, ret_len, on=1;
	bzero(&dst_addr,sock_addr_len);   // 清零
	dst_addr.sin_family=PF_INET;
	dst_addr.sin_addr.s_addr=inet_addr(dst_ip);
	dst_addr.sin_port=htons(dst_port);

	/* 创建TCP套接字 */
	if((sockfd=socket(PF_INET,SOCK_RAW,IPPROTO_TCP))==1)
	{
		err_exit("socket() failed");
	}
	/* 开启IP_HRINCL，自定义IP首部 */
	if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on))==-1)
	{
		err_exit("setsocketopt() failed");
	}

	ip_header=(struct ip*)malloc(IP_HEADER_LEN);
	tcp_header=(struct tcphdr*)malloc(TCP_HEADER_LEN);

	fill_ip_header(src_ip, dst_ip, pack_len,ip_header);
	fill_tcp_header(src_port, dst_port,tcp_header);
	bzero(buf,pack_len);
	memcpy(buf,ip_header,IP_HEADER_LEN);
	memcpy(buf+IP_HEADER_LEN,tcp_header,TCP_HEADER_LEN);
	memcpy(buf+IP_TCP_HEADER_LEN,data,data_len);

	/* send packet */
	ret_len=sendto(sockfd, buf ,pack_len, 0, (struct sockaddr*)&dst_addr, sock_addr_len);
	if(ret_len>0)
	{
		printf("sendto() ok\n");
	}
	else
		printf("send() failed\n");
	close(sockfd);
	if(ip_header!=NULL)
	{
		free(ip_header);
	}
	if(tcp_header!=NULL)
	{
		free(tcp_header);
	}
}
int main(int argc, char* argv[])
{
	if(argc!=0)
	{
		printf("usage:%s src_ip, src_port, dst_ip, dst_port, data\n",argv[0]);
		exit(1);
	}
	ip_tcp_send(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), argv[5]);
	return 0;
}
