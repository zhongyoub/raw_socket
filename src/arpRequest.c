/*
 * arp query code
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netinet/if_ether.h>  //ether_arp
#include<net/ethernet.h>      // ether_header
#include<net/if_arp.h>        // arp
#include<net/if.h>
#include<netpacket/packet.h>
/*
struct    ether_arp {
    struct    arphdr ea_hdr;         fixed-size header
    u_int8_t arp_sha[ETH_ALEN];     sender hardware address
    u_int8_t arp_spa[4];         	sender protocol address
    u_int8_t arp_tha[ETH_ALEN];     target hardware address
    u_int8_t arp_tpa[4];            target protocol address
};
#define    arp_hrd    ea_hdr.ar_hrd
#define    arp_pro    ea_hdr.ar_pro
#define    arp_hln    ea_hdr.ar_hln
#define    arp_pln    ea_hdr.ar_pln
#define    arp_op    ea_hdr.ar_op
*/
/* ether header length*/
#define ETHER_HEADER_LEN  sizeof(struct ether_header)

/* arp struct length */
#define ETHER_ARP_LEN sizeof(struct ether_arp)

/* ether header+ arp struct  length */
#define ETHER_ARP_PACKET_LEN ETHER_HEADER_LEN+ETHER_ARP_LEN

/* IP address length  */
#define IP_ADDR_LEN 4
/* broadcasr addr */
#define BROADCAST_ADDR {0xff,0xff,0xff ,0xff,0xff,0xff}

void err_exit(const char* err_msg)
{
	perror(err_msg);
	exit(1);
}

/* fill arp packet */
struct ether_arp* fill_arp_packet(const unsigned char* src_mac_addr,const char* src_ip,const char* dst_ip)
{
	struct ether_arp* arp_packet;
	struct in_addr src_in_addr,dst_in_addr;
	unsigned char dst_mac_addr[ETH_ALEN]=BROADCAST_ADDR;
	/*IP address change 点分十进制IP地址转换为网络地址*/
	inet_pton(AF_INET,src_ip,&src_in_addr);
	inet_pton(AF_INET,dst_ip,&dst_in_addr);

	/* total arp packet */
	arp_packet=(struct ether_arp*)malloc(ETHER_ARP_LEN);
	arp_packet->arp_hrd=htons(ARPHRD_ETHER);
	arp_packet->arp_pro=htons(ETHERTYPE_IP);
	arp_packet->arp_hln=ETH_ALEN;
	arp_packet->arp_pln=IP_ADDR_LEN;
	arp_packet->arp_op=htons(ARPOP_REQUEST);
	memcpy(arp_packet->arp_sha,src_mac_addr,ETH_ALEN);
	memcpy(arp_packet->arp_tha,dst_mac_addr,ETH_ALEN);
	memcpy(arp_packet->arp_spa,&src_in_addr,IP_ADDP_LEN);
	memcpy(arp_packet->arp_tpa,&dst_in_addr,IP_ADDR_LEN);
	return arp_packet;
}
/* arp request */
void arp_request(const char* if_name,const char* dst_ip)
{
	struct sockaddr_ll saddr_ll;
	struct ether_header* eth_header;
	struct ether_arp* arp_packet;
	struct ifreq ifr;
	char buf[ETHER_ARP_PACKET_LEN];
	unsigned char src_mac_addr[ETH_ALEN];
	unsigned char dst_mac_addr[ETH_ALEN]=BROADCAST_ADDR;
	char* src_ip;
	int sock_raw_fd,ret_len,i;
	/*
	 * 1.socket(AF_INET, SOCK_RAW, IPPROTO_TCP|IPPROTO_UDP|IPPROTO_ICMP)发送接收ip数据包，
	        不能用IPPROTO_IP，因为如果是用了IPPROTO_IP，系统根本就不知道该用什么协议。
	   2.socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP|ETH_P_ARP|ETH_P_ALL))发送接收以太网数据帧
	 */
	if((sock_raw_fd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ARP)))==-1)
	{
		err_exit("socket()");
	}
	bzero(&saddr_ll,sizeof(struct sockaddr_ll));
	bzero(&ifr,sizeof(struct ifreq));
	/* 网卡名字 */
	memcpy(ifr.ifr_name,if_name,strlen(if_name));
	/* 获取网卡索引 */
	if(ioctl(sock_raw_fd,SIOCGIFINDEX,&ifr)==-1)
	{
		err_exit("ioctl() get ifindex");
	}
	saddr_ll.sll_ifindex=ifr.ifr_ifindex;
	saddr_ll.sll_family=PF_PACKET;

	/* 获取网卡IP*/
	if(ioctl(sock_raw_fd,SIOCGIFADDR,&ifr)==-1)
	{
		err_exit("ioctl() get ip");
	}
	/*inet_ntoa(struct addr_in in) 将网络二进制地址转换为主机地址 */
	src_ip=inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
	printf("local ip:%s\n",src_ip);

	/* get MAC address of ifreq */
	if(ioctl(sock_raw_fd,SIOCGIFHWADDR,&ifr))
	{
		err_exit("ioctl() get mac");
	}
	memcpy(src_mac_addr,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
	printf("local mac");
	for(i=0;i<ETH_ALEN-1;i++)
	{
		printf("%02x:",src_mac_addr[i]);
	}
	printf("%0x2",src_mac_addr[i]);
	printf("\n");
	bzero(buf,ETHER_ARP_PACKET_LEN);
	/* fill ether header */
	eth_header=(struct ether_header*)buf;
	memcpy(eth_header->ether_shost,src_mac_addr,ETH_ALEN);
	memcpy(eth_header->ether_dhost,dst_mac_addr,ETH_ALEN);
	eth_header->ether_type=htons(ETHERTYPE_ARP);
//  memcpy(buf,eth_header,ETHER_HEADER_LEN);
	/* arp packet */
	arp_packet=fill_arp_packet(src_mac_addr,src_ip,dst_ip);
	memcpy(buf+ETHER_HEADER_LEN,arp_packet,ETHER_ARP_LEN);

	/* send request */

	ret_len=sendto(sock_raw_fd,buf,ETHER_ARP_PACKET_LEN,0,(struct sockaddr*)&saddr_ll,sizeof(struct sockaddr_ll));
	if(ret_len>0)
	{
		printf("sendto() success\n");
	}
	close(sock_raw_fd);

}
int main(int argc, char* argv[])
{
	if(argc!=3)
	{
		printf("usage:%s device_name dst_ip\n",argv[0]);
		exit(1);
	}
	arp_requst(argv[1],argv[2]);
	return 0;
}

