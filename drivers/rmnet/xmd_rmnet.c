/*
 * xmd_rmnet.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * Copyright (C) 2011 Intel Mobile Communications GmbH. All rights reserved.
 * Author: Srinivas M <srinivasaraox.mandadapu@intel.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * RMNET Driver modified by Intel from Google msm_rmnet.c
 */

#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/tty.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");

/********************CONFIGURABLE OPTIONS********************/
/* Should be defined only if streaming is enabled on the modem side for
 * the used IPC */
#define MODEM_STREAMING


/**************************DEFINES***************************/
#define RMNET_ETH_HDR_SIZE   14
#define RMNET_MTU_SIZE 1500
#define RMNET_IPV4_VER 0x4
#define RMNET_IPV6_VER 0x6
#define IPV6_HEADER_SZ 40

//#define IPV6_PROTO_TYPE 0x08DD
#define IPV6_PROTO_TYPE 0x86DD

#undef  N_RMNET
#define N_RMNET  25 
#define MAX_NET_IF      3
#define MAX_TX_TIME     3000   /*ms*/
#ifndef MODEM_STREAMING
/* For framing mode, the length of packet should be added to the head of data */
#define LEN_UP_POS  (RMNET_ETH_HDR_SIZE - 2) /* where the upper byte of length should be put in the skb buffer */
#define LEN_LO_POS  (RMNET_ETH_HDR_SIZE - 1) /* where the lower byte of length should be put in the skb buffer */
#endif

/************************************************************/

/************************STRUCTURES**************************/
struct rmnet_private
{	
	atomic_t drv_status;
	struct net_device_stats stats;
	struct tty_struct *tty;
	struct net_device *dev;
	struct sk_buff *skb;
	struct work_struct tx_wk;
	struct mutex tty_lock;

	struct sk_buff *skb_in;
	int ip_data_in_skb;
	int pkt_len;
	void *pkt;
	int hdr_len;
	unsigned char hdr[6];
};
/************************************************************/

/**********************GLOBAL VARIABLES**********************/
struct rmnet_private* g_rmnet_private[MAX_NET_IF] = {NULL,};
struct workqueue_struct *rmnet_wq;

/************************************************************/

/*******************FUNCTION DECLARATION*********************/
static int rmnet_init(void);
static void ifx_netif_rx_cb(struct net_device *ptr_dev, const unsigned char *rx_addr, int sz);
/************************************************************/

/*******************FUNCTION DEFINITIONS********************/

/*********************** Line disc driver operations starts *****************************/
static int rmnet_asynctty_open(struct tty_struct *tty)
{
	int i;
	pr_info("%s++\n", __FUNCTION__);
	
	if (tty->ops->write == NULL)
	{
		pr_info("tty->ops->write == NULL\n");
		return -EOPNOTSUPP;
	}

	for (i=0; i<MAX_NET_IF; i++) {
		if((NULL != g_rmnet_private[i])
		&& (0 == atomic_read(&(g_rmnet_private[i]->drv_status)))) {
			atomic_set(&(g_rmnet_private[i]->drv_status), 1);
			break;
		}
	}

	if (i == MAX_NET_IF){
		pr_err("%s: Failed to get a free rmnet device\n", __FUNCTION__);
		return -EOPNOTSUPP;
	}

	g_rmnet_private[i]->tty = tty;
	tty->disc_data = g_rmnet_private[i];
    tty->receive_room = 4095;
	pr_info("g_rmnet_data[%d]->tty is initialized with %p\n", i, tty);

	return 0;
}

static void rmnet_asynctty_close(struct tty_struct *tty)
{
	struct rmnet_private *p;
	pr_info("%s++\n", __FUNCTION__);

	p = tty->disc_data;
	if(!p){
		pr_err("%s:Error tty ldisc not opened\n", __FUNCTION__);
		return;
	}
	atomic_set(&(p->drv_status), 0);

	mutex_lock(&p->tty_lock);
	p->tty = NULL;
	tty->disc_data = NULL;
	mutex_unlock(&p->tty_lock);
}


/* May sleep, don't call from interrupt level or with interrupts disabled */
static void rmnet_asynctty_receive(struct tty_struct *tty, const unsigned char *buf,
		char *cflags, int count)
{	
	struct rmnet_private *p = tty->disc_data;

#if 0
	pr_info("kz rmnet_asynctty_receive data");
#endif 	

	//pr_debug("%s++\n", __FUNCTION__);	

	if(p->dev)
	{
		ifx_netif_rx_cb(p->dev, buf, count);
	}
        	
	//pr_debug("%s-- \n", __FUNCTION__);
}


/*Line disc info*/
static struct tty_ldisc_ops rmnet_ldisc = {
	.owner  = THIS_MODULE,
	.magic	= TTY_LDISC_MAGIC,
	.name	= "ip_ldisc",
	.open	= rmnet_asynctty_open,
	.close  = rmnet_asynctty_close,
	.receive_buf = rmnet_asynctty_receive,
};
/************************ Line disc driver operations ends ******************************/


/**************************** Net driver operations starts ******************************/

/* init_input_ip_pkt initialize a skb for netif_rx
*/
static inline int init_input_ip_pkt(struct rmnet_private *p, int ver, const unsigned char* hdr)
{
	if (ver == RMNET_IPV4_VER) {
		p->pkt_len = (hdr[2]<<8)| hdr[3];
		if (p->pkt_len < 4) {
			pr_warn("!!!!!!!!!!!!Bad packet, length = %d\n\n", p->pkt_len);
			p->pkt_len = 0;
			return -1;
		}
	} else if (ver == RMNET_IPV6_VER) {
		p->pkt_len = IPV6_HEADER_SZ + ((hdr[4]<<8)| hdr[5]);
		if (p->pkt_len - IPV6_HEADER_SZ < 6) {
			pr_warn("!!!!!!!!!!!!Bad packet, length = %d\n\n", p->pkt_len);
			p->pkt_len = 0;
			return -1;
		}
	} else {
		pr_warn("!!!!!!!!!!!!Wrong Version: ver = %d\n\n", ver);
		return -1;
	}
	if (p->pkt_len > RMNET_MTU_SIZE) {
		pr_warn("!!!!!!!!!!!!Large than MTU: pkt_len = %d\n\n", p->pkt_len);
		p->pkt_len = 0;
		return -1;
	} else {
		p->pkt_len += RMNET_ETH_HDR_SIZE;
		p->skb_in = dev_alloc_skb(p->pkt_len + NET_IP_ALIGN);
		if (p->skb_in == NULL) {
			/* TODO: We need to handle this case later */
			pr_warn("!!!!!!!!!!!!!!!!!!!!!!skbuf alloc failed!!!!!!!!!!!!!!!!!!\n\n");
			p->pkt_len = 0;
			return -1;
		}
		p->skb_in->dev = p->dev;
		skb_reserve(p->skb_in, NET_IP_ALIGN);
		p->pkt = skb_put(p->skb_in, p->pkt_len);
		/* adding ethernet header */
		{
			char temp[] = {0xB6,0x91,0x24,0xa8,0x14,0x72,0xb6,0x91,0x24,
						   0xa8,0x14,0x72,0x08,0x0};
			struct ethhdr *eth_hdr = (struct ethhdr *) temp;

			if (ver == RMNET_IPV6_VER) {
				unsigned short ipv6_proto_type = 0xDD86;
				memcpy((void *)(&(eth_hdr->h_proto)),
					(void *)(&ipv6_proto_type),
					sizeof(eth_hdr->h_proto));
			}

			memcpy((void *)eth_hdr->h_dest,
				   (void*)p->dev->dev_addr,
				   sizeof(eth_hdr->h_dest));
			memcpy((void *)p->pkt,
				   (void *)eth_hdr,
				   sizeof(struct ethhdr));
		}
	}

	return 0;
}

/* Receive data from TTY and forward it to TCP/IP stack 
    The implementation also handles partial packets.
    Ethernet header is appended to the IP datagram before passing it to TCP/IP stack.
*/
void ifx_netif_rx_cb(struct net_device *dev, const unsigned char *buf, int sz)
{
	struct rmnet_private *p = netdev_priv(dev);
	int ver = 0;
	int ip_hdr_len;
	unsigned short tempval;
	
	while(sz) {
		if (p->pkt_len == 0 && p->hdr_len != 0) {
#if defined(__BIG_ENDIAN_BITFIELD)
			ver = p->hdr[0] & 0x0F;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
			ver = (p->hdr[0] & 0xF0) >> 4;
#endif
			ip_hdr_len = (ver == RMNET_IPV4_VER) ? 4 : 6;
			if (p->hdr_len + sz >= ip_hdr_len) {
				memcpy(&p->hdr[p->hdr_len], buf, ip_hdr_len - p->hdr_len);
			} else {
				memcpy(&p->hdr[p->hdr_len], buf, sz);
				p->hdr_len += sz;
				return;
			}

			if (init_input_ip_pkt(p, ver, p->hdr) == 0) {
				memcpy(p->pkt + RMNET_ETH_HDR_SIZE, p->hdr, p->hdr_len);
				p->ip_data_in_skb += p->hdr_len;
				p->hdr_len = 0;
			} else {
				/* Drop the saved header and data received */
				p->hdr_len = 0;
				return;
			}
		}

		if (p->pkt_len == 0) {
#if defined(__BIG_ENDIAN_BITFIELD)
			ver = buf[0] & 0x0F;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
			ver = (buf[0] & 0xF0) >> 4;
#endif
			if (ver == RMNET_IPV4_VER) {
				ip_hdr_len = 4;
			} else if (ver == RMNET_IPV6_VER) {
				ip_hdr_len = 6;
			} else {
				pr_warn("!!!!!!!!!!!!Wrong Version: ver = %d\n\n", ver);
				return;
			}

			if (sz < ip_hdr_len) {
				memcpy(&p->hdr[0], buf, sz);
				p->hdr_len = sz;
				return;
			}

			if (init_input_ip_pkt(p, ver, buf) != 0)
				return;
		}

		tempval = min(sz, (p->pkt_len - RMNET_ETH_HDR_SIZE - p->ip_data_in_skb));
		memcpy(p->pkt + RMNET_ETH_HDR_SIZE + p->ip_data_in_skb, buf, tempval);
		p->ip_data_in_skb += tempval;
		sz -= tempval;
		buf += tempval;
		if (p->ip_data_in_skb < (p->pkt_len - RMNET_ETH_HDR_SIZE)) {
			break;
		}
		p->skb_in->protocol = eth_type_trans(p->skb_in, dev);
		p->stats.rx_packets++;
		p->stats.rx_bytes += p->skb_in->len;
		netif_rx(p->skb_in);
		p->pkt_len = 0;
		p->ip_data_in_skb = 0;
	}
}

static int rmnet_open(struct net_device *dev)
{
	pr_info("%s++\n", __FUNCTION__);
	
	netif_start_queue(dev);
	return 0;
}

static int rmnet_stop(struct net_device *dev)
{
	pr_info("%s++\n", __FUNCTION__);
	
	netif_stop_queue(dev);
	return 0;
}

/* Write data error when forward data from TCP/IP stack to TTY,
   retry adn wake up net device.
*/
static void rmnet_xmit_tx_retry(struct work_struct *work)
{
	int ret = 0;
	unsigned long tio = msecs_to_jiffies(MAX_TX_TIME); /* retry timeout */
	struct rmnet_private *p = container_of(work, struct rmnet_private, tx_wk);

	while (0 >= ret && time_before_eq(jiffies, p->dev->trans_start + tio)) {
		msleep(100);

		mutex_lock(&p->tty_lock);
		if (p->tty && p->tty->ops && p->tty->ops->write && p->skb){
			set_bit(TTY_DO_WRITE_WAKEUP, &(p->tty->flags));
#ifdef MODEM_STREAMING
			ret = p->tty->ops->write(p->tty,
				                 (&(p->skb->data[RMNET_ETH_HDR_SIZE])),
				                 ((p->skb->len) - RMNET_ETH_HDR_SIZE));

#else
			ret = p->tty->ops->write(p->tty,
				                 (&(p->skb->data[LEN_UP_POS])),
				                 ((p->skb->len) - LEN_UP_POS));
#endif
			mutex_unlock(&p->tty_lock);
		} else {
			pr_info("%s failed because tty is cleared\n", __FUNCTION__);
			p->stats.tx_dropped++;
			if (p->skb) {
				dev_kfree_skb_irq(p->skb);
				p->skb = NULL;
			}
			netif_wake_queue(p->dev);
			mutex_unlock(&p->tty_lock);
			return;
		}
	}

	if (0 >= ret) {
		pr_info("%s failed with reason: %d\n", __FUNCTION__, ret);
		p->stats.tx_dropped++;
	} else {
#if 0
		pr_info("%s write %d bytes after retry for %d ms\n",
			__FUNCTION__, ret, jiffies_to_msecs(jiffies - p->dev->trans_start));
#endif
		p->stats.tx_packets++;
		p->stats.tx_bytes += ret;
	}

	mutex_lock(&p->tty_lock);
	if (p->skb) {
		dev_kfree_skb_irq(p->skb);
		p->skb = NULL;
	}
	mutex_unlock(&p->tty_lock);

	netif_wake_queue(p->dev);
}

/* Receive data from TCP/IP stack and forward it to TTY 
    Ethernet header is removed before sending IP datagram to the modem.
    IP Datagram length is appended before the IP packet as per modem requirement.
*/
static int rmnet_xmit(struct sk_buff *skb, struct net_device *dev)
{	
	struct rmnet_private *p;
	int ret;
#if 0 
	pr_info("kz rmnet_xmit");	
	pr_debug("%s++\n", __FUNCTION__);
#endif
	
	if((skb==NULL)||(dev==NULL)){
		pr_info("rmnet_xmit:(skb==NULL)||(dev==NULL)\n");	
		return 0;
	}
	
	p = netdev_priv(dev);

	dev->trans_start = jiffies;
	if(p && p->tty && p->tty->ops && p->tty->ops->write){
#if 0 
		pr_info("p->tty== %p\n",p->tty);
		pr_info("Dest MAC Addr = %x %x %x %x %x %x\n", skb->data[0], skb->data[1], skb->data[2], skb->data[3], skb->data[4], skb->data[5]);		    
		pr_info("Src MAC Addr = %x %x %x %x %x %x\n", skb->data[6], skb->data[7], skb->data[8], skb->data[9], skb->data[10], skb->data[11]);
		pr_info("Eth Protocol type %x %x \nIP datagram start %x %x %x %x %x %x\n", skb->data[12],skb->data[13],skb->data[14],skb->data[15],skb->data[16],skb->data[17],skb->data[18],skb->data[19]);
#endif
		set_bit(TTY_DO_WRITE_WAKEUP, &(p->tty->flags));

#ifdef MODEM_STREAMING
		ret = p->tty->ops->write(p->tty,
					 (&skb->data[RMNET_ETH_HDR_SIZE]),
					 ((skb->len) - RMNET_ETH_HDR_SIZE));
#else
		skb->data[LEN_UP_POS] = ((skb->len - RMNET_ETH_HDR_SIZE) >> 8) & 0xff;
		skb->data[LEN_LO_POS] = (skb->len - RMNET_ETH_HDR_SIZE) & 0xff;

		ret = p->tty->ops->write(p->tty,
					 (&skb->data[LEN_UP_POS]),
					 ((skb->len) - LEN_UP_POS));
#endif
		if(0 >= ret){
			pr_info("rmnet_xmit wrtie fail on %s with reason: %d\n",dev->name,ret);
			netif_stop_queue(dev);
			if(p->skb != NULL)
				pr_warn("p->skb != NULL\n");
			p->skb = skb;
			queue_work(rmnet_wq, &(p->tx_wk));
			return NETDEV_TX_OK;
		}else{
			p->stats.tx_packets++;
			p->stats.tx_bytes += skb->len;
		}
	}	

	dev_kfree_skb_irq(skb);
	return NETDEV_TX_OK;
}

static struct net_device_stats *rmnet_get_stats(struct net_device *dev)
{	
	struct rmnet_private *p = netdev_priv(dev);
	return &p->stats;
}

static void rmnet_tx_timeout(struct net_device *dev)
{
	pr_info("%s++\n", __FUNCTION__);
}

/* Net device info */
static struct net_device_ops rmnet_ops = {
	.ndo_open = rmnet_open,
	.ndo_stop = rmnet_stop,
	.ndo_start_xmit = rmnet_xmit,
	.ndo_get_stats = rmnet_get_stats,
	.ndo_tx_timeout = rmnet_tx_timeout,
};

static void __init rmnet_setup(struct net_device *dev)
{
	pr_info("%s++\n", __FUNCTION__);

	dev->netdev_ops = &rmnet_ops;
	ether_setup(dev);
	dev->watchdog_timeo = msecs_to_jiffies(MAX_TX_TIME + 1000);
	dev->mtu            = RMNET_MTU_SIZE;
	dev->flags         &= ~IFF_MULTICAST;
	dev->flags         &= ~IFF_BROADCAST;
	random_ether_addr(dev->dev_addr);
}

static int __init rmnet_init(void)
{		
	int ret;
	int i;
	char dev_name[8];
	struct net_device *dev;
	struct rmnet_private *p;
	
	pr_info(" %s++\n", __FUNCTION__);

	for (i=0; i<MAX_NET_IF; i++) {
		memset(dev_name, 0, sizeof(dev_name));
		snprintf(dev_name, sizeof(dev_name), "rmnet%d", i);
		dev = alloc_netdev(sizeof(struct rmnet_private),
				   dev_name, rmnet_setup);

		if (!dev)
			return -ENOMEM;

		p = netdev_priv(dev);
		g_rmnet_private[i] = p;
		p->dev = dev;
		p->skb = NULL;
		atomic_set(&(p->drv_status), 0);

		INIT_WORK(&(p->tx_wk), rmnet_xmit_tx_retry);

		mutex_init(&p->tty_lock);

		ret = register_netdev(dev);
		if (ret < 0) {
			pr_err("%s: failed to register netdev\n", __FUNCTION__);
			free_netdev(dev);
			return ret;
		}
	}
	/* Initialize works */
	rmnet_wq = create_singlethread_workqueue("rmnet_wq");
	if (rmnet_wq == NULL) {
		pr_info("RMNET: create workqueue fail\n");
		return -ENOMEM;
	}

	return 0;
}
/***************************** Net driver operations ends *******************************/

/***************************** rmnet driver entry/exit **********************************/
static int __init rmnet_async_init(void)
{	
	int err = -EFAULT;
	
	pr_info("%s++\n", __FUNCTION__);

/*
	err = get_modem_type( MODEM_XMM6260 );
	pr_info("MODEM[%s] %s: get_modem_type \"%s\". %d\n",MODEM_DEVICE_BOOT(MODEM_XMM6260),__func__,MODEM_XMM6260,err);
	if( err<=0 ) {
            pr_err("MODEM[%s] %s: modem \"%s\" not support on this board. %d\n",MODEM_DEVICE_BOOT(MODEM_XMM6260),__func__,MODEM_XMM6260,err);
            return( -1 );
	}
*/

	/* 
	    The N_RMNET value should not be used by any other line disc driver in the system.
	    Refer to the list of line disciplines listed in tty.h file.
	    The same value (as N_RMNET) should also be used by application when setting up the line disc association:
		int fdrmnet = open("/dev/ttyACM1",  O_RDWR | O_NOCTTY | O_NONBLOCK);
		int ldisc = N_RMNET;
		ioctl (fdrmnet, TIOCSETD, &ldisc);
	    In some older Linux kernels, dynamically adding new line disc driver is not allowed.
	    In such cases,
	       either an existing line disc number (but not used in the system) can be used, or
	       a new one needs to be added in tty.h and the kernel needs to be rebuild.
	*/
	err = tty_register_ldisc(N_RMNET, &rmnet_ldisc);
	if (err != 0)
	{
		pr_info(KERN_ERR "RMNET: error %d registering line disc.\n",err);
	}
	rmnet_init();

	pr_info(KERN_ERR "RMNET: registering line disc successful.\n");
	
	return err;
}

static void __exit rmnet_async_cleanup(void)
{
	int i;
	pr_info("%s++\n", __FUNCTION__);
	
	if (tty_unregister_ldisc(N_RMNET) != 0) {
		pr_info(KERN_ERR "failed to unregister RMNET line discipline\n");
	}

	for (i = 0; i < MAX_NET_IF; i++) {
		unregister_netdev(g_rmnet_private[i]->dev);
		free_netdev(g_rmnet_private[i]->dev);
		g_rmnet_private[i] = NULL;
	}
	destroy_workqueue(rmnet_wq);
}

module_init(rmnet_async_init);
module_exit(rmnet_async_cleanup);
/****************************************************************************************/

