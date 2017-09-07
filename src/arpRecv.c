#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/if_ether.h>
#include<net/if_arp.h>
#include<net/ethernet.h>
/*
 *
 struct    ether_arp {
    struct    arphdr ea_hdr;        // fixed-size header
    u_int8_t arp_sha[ETH_ALEN];    // sender hardware address
    u_int8_t arp_spa[4];        // sender protocol address
    u_int8_t arp_tha[ETH_ALEN];    // target hardware address
    u_int8_t arp_tpa[4];        // target protocol address
};
#define    arp_hrd    ea_hdr.ar_hrd
#define    arp_pro    ea_hdr.ar_pro
#define    arp_hln    ea_hdr.ar_hln
#define    arp_pln    ea_hdr.ar_pln
#define    arp_op    ea_hdr.ar_op
 */

/* ether header */
#define ETHER_HEADER_LEN sizeof(struct ether_header)
/* arp struct length */
#define ETHER_ARP_LEN sizeof(struct ether_arp)

/* ether + arp length*/
#define ETHER_ARP_PACKET_LEN ETHER_HEADER_LEN+ETHER_ARP_LEN

/* Ip length */
#define IP_ADDR_ELN 4

void err_exit(const char* err_msg)
{
	perror(err_msg);
	exit(1);
}
int main()
{
	struct ether_arp* arp_packet;
	char buf[ETHER_ARP_PACKET_LEN];
	int sock_raw_fd,ret_len,i;
	if((sock_raw_fd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ARP)))==-1)
	{
		err_exit("socket()");
	}
	while(1)
	{
		bzero(buf,ETHER_ARP_PACKET_LEN);
		ret_len=recv(sock_raw_fd,buf,ETHER_ARP_PACKET_LEN,0);
		if(ret_len>0)
		{
			/* 剥去以太网首部 */
			arp_packet=(struct ether_arp*)(buf+ETHER_HEADER_LEN);
			/* arp 操作码为2 表示应答,1位请求*/
			if(ntohs(arp_packet->arp_op)==2)
			{
				printf("-------------arp reply----------\n");
				printf("from ip:");
				for(i=0;i<IP_ADDR_ELN-1;i++)
				{
					printf("%u.",arp_packet->arp_spa[i]);
				}
				printf("%u",arp_packet->arp_spa[i]);
				printf("\nfrom mac");
				for(i=0;i<ETH_ALEN-1;i++)
				{
					printf("%02x：",arp_packet->arp_sha[i]);
				}
				printf("%02x\n",arp_packet->arp_sha[i]);
			}
		}
	}
	close(sock_raw_fd);
	return 0;
}

