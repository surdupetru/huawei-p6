#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/net.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/ppp_channel.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/notifier.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <net/ip.h>
#include <linux/netfilter_ipv4.h>
#include <net/netns/generic.h>

extern int cmp_addr(struct pppoe_addr *a, __be16 sid, char *addr);
extern int cmp_2_addr(struct pppoe_addr *a, struct pppoe_addr *b);
extern int hash_item(__be16 sid, unsigned char *addr);
extern int pppoe_dzc_ioctl(unsigned int cmd, unsigned long arg);
extern int get_dzc_header_len(void);
extern int pppoe_dzc_skb_handle(unsigned char *head, int len);
extern int pppoe_dzc_pack_check(struct iphdr *iph, struct udphdr *udph);
extern void DZC_ACMM_INIT(void);
extern int pppoe_dzc_lcp(unsigned char *skb_data, int len, unsigned char *obuf);
extern bool stage_session(__be16 sid);
extern unsigned char *pppoe_skb_pull(char *data, int *skb_len, int len);
extern void pppoe_dzc_connect(__be16 sid);
extern __be16 pppoe_dzc_get_connect(void);
extern int pppoe_dzc_escape(unsigned char *data, int len);
extern bool is_ppp_fg(unsigned char *data);
extern bool is_ppp_at(unsigned char *data);
extern bool is_ppp_ui(unsigned char *data);
extern bool is_ppp_ip(unsigned char *data);
/**************************************************************************/
static int dzc_pppoe_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
static int dzc_pppoe_xmit(struct ppp_channel *chan, struct sk_buff *skb);
static int __dzc_pppoe_xmit(struct sock *sk, struct sk_buff *skb);

static const struct proto_ops pppoe_ops;
static const struct ppp_channel_ops pppoe_chan_ops;

static int pppoe_net_id __read_mostly;
struct pppoe_net {
	struct pppox_sock *hash_table[1 << 4];
	rwlock_t hash_lock;
};

struct ppp_hdr {
	__u8 flag;
	__u8 address;
	__u8 control;
} __attribute ((packed));

static inline struct pppoe_net *dzc_pppoe_pernet(struct net *net)
{
	BUG_ON(!net);

	return net_generic(net, pppoe_net_id);
}

void kernel_ip_send_check(struct iphdr *ip)
{
	ip_send_check(ip);
	return;
}

static struct pppox_sock *__get_item(struct pppoe_net *pn, __be16 sid,
				unsigned char *addr, int ifindex)
{
	int hash = hash_item(sid, addr);
	struct pppox_sock *ret;

	ret = pn->hash_table[hash];
	while (ret) {
		if (cmp_addr(&ret->pppoe_pa, sid, addr) &&
		    ret->pppoe_ifindex == ifindex)
			return ret;

		ret = ret->next;
	}

	return NULL;
}

static int __set_item(struct pppoe_net *pn, struct pppox_sock *po)
{
	int hash = hash_item(po->pppoe_pa.sid, po->pppoe_pa.remote);
	struct pppox_sock *ret;

	ret = pn->hash_table[hash];
	while (ret) {
		if (cmp_2_addr(&ret->pppoe_pa, &po->pppoe_pa) &&
		    ret->pppoe_ifindex == po->pppoe_ifindex)
			return -EALREADY;

		ret = ret->next;
	}

	po->next = pn->hash_table[hash];
	pn->hash_table[hash] = po;

	return 0;
}

static struct pppox_sock *__delete_item(struct pppoe_net *pn, __be16 sid,
					char *addr, int ifindex)
{
	int hash = hash_item(sid, addr);
	struct pppox_sock *ret, **src;

	ret = pn->hash_table[hash];
	src = &pn->hash_table[hash];

	while (ret) {
		if (cmp_addr(&ret->pppoe_pa, sid, addr) &&
		    ret->pppoe_ifindex == ifindex) {
			*src = ret->next;
			break;
		}

		src = &ret->next;
		ret = ret->next;
	}

	return ret;
}

static inline struct pppox_sock *dzc_get_item(struct pppoe_net *pn, __be16 sid,
					unsigned char *addr, int ifindex)
{
	struct pppox_sock *po;

	read_lock_bh(&pn->hash_lock);
	po = __get_item(pn, sid, addr, ifindex);
	if (po)
		sock_hold(sk_pppox(po));
	read_unlock_bh(&pn->hash_lock);

	return po;
}

static inline struct pppox_sock *dzc_get_item_by_addr(struct net *net,
						struct sockaddr_pppox *sp)
{
	struct net_device *dev;
	struct pppoe_net *pn;
	struct pppox_sock *pppox_sock = NULL;

	int ifindex;

	rcu_read_lock();
	dev = dev_get_by_name_rcu(net, sp->sa_addr.pppoe.dev);
	if (dev) {
		ifindex = dev->ifindex;
		pn = dzc_pppoe_pernet(net);
		pppox_sock = dzc_get_item(pn, sp->sa_addr.pppoe.sid,
				sp->sa_addr.pppoe.remote, ifindex);
	}
	rcu_read_unlock();
	return pppox_sock;
}

static inline struct pppox_sock *dzc_delete_item(struct pppoe_net *pn, __be16 sid,
					char *addr, int ifindex)
{
	struct pppox_sock *ret;

	write_lock_bh(&pn->hash_lock);
	ret = __delete_item(pn, sid, addr, ifindex);
	write_unlock_bh(&pn->hash_lock);

	return ret;
}

static void pppoe_dzc_flush_dev(struct net_device *dev)
{
	struct pppoe_net *pn;
	int i;

	pn = dzc_pppoe_pernet(dev_net(dev));
	write_lock_bh(&pn->hash_lock);
	for (i = 0; i < (1 << 4); i++) {
		struct pppox_sock *po = pn->hash_table[i];
		struct sock *sk;

		while (po) {
			while (po && po->pppoe_dev != dev) {
				po = po->next;
			}

			if (!po)
				break;

			sk = sk_pppox(po);
			
			sock_hold(sk);
			write_unlock_bh(&pn->hash_lock);
			lock_sock(sk);

			if (po->pppoe_dev == dev &&
			    sk->sk_state & (PPPOX_CONNECTED | PPPOX_BOUND | PPPOX_ZOMBIE)) {
				pppox_unbind_sock(sk);
				sk->sk_state = PPPOX_ZOMBIE;
				sk->sk_state_change(sk);
				po->pppoe_dev = NULL;
				dev_put(dev);
			}

			release_sock(sk);
			sock_put(sk);

			BUG_ON(dzc_pppoe_pernet(dev_net(dev)) == NULL);
			write_lock_bh(&pn->hash_lock);
			po = pn->hash_table[i];
		}
	}
	write_unlock_bh(&pn->hash_lock);
}

static int pppoe_dzc_device_event(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	struct net_device *dev = (struct net_device *)ptr;

	switch (event) {
	case NETDEV_CHANGEADDR:
	case NETDEV_CHANGEMTU:
	case NETDEV_GOING_DOWN:
	case NETDEV_DOWN:
		pppoe_dzc_flush_dev(dev);
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block pppoe_notifier = {
	.notifier_call = pppoe_dzc_device_event,
};

static int pppoe_rcv_core(struct sock *sk, struct sk_buff *skb)
{
	struct pppox_sock *po = pppox_sk(sk);
	struct pppox_sock *relay_po;

	if (sk->sk_state & PPPOX_BOUND) {
		ppp_input(&po->chan, skb);
	} else if (sk->sk_state & PPPOX_RELAY) {
		relay_po = dzc_get_item_by_addr(sock_net(sk),
					    &po->pppoe_relay);
		if (relay_po == NULL)
			goto abort_kfree;

		if ((sk_pppox(relay_po)->sk_state & PPPOX_CONNECTED) == 0)
			goto abort_put;

		if (!__dzc_pppoe_xmit(sk_pppox(relay_po), skb))
			goto abort_put;
	} else {
		if (sock_queue_rcv_skb(sk, skb))
			goto abort_kfree;
	}

	return NET_RX_SUCCESS;

abort_put:
	sock_put(sk_pppox(relay_po));

abort_kfree:
	kfree_skb(skb);
	return NET_RX_DROP;
}

static int dzc_pppoe_rcv(struct sk_buff *skb, struct net_device *dev,
		     struct packet_type *pt, struct net_device *orig_dev)
{
	struct pppoe_hdr *ph;
	struct pppox_sock *po;
	struct pppoe_net *pn;
	int len;

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		goto out;

	if (!pskb_may_pull(skb, sizeof(struct pppoe_hdr)))
		goto drop;

	ph = pppoe_hdr(skb);
	len = ntohs(ph->length);

	skb_pull_rcsum(skb, sizeof(*ph));
	if (skb->len < len)
		goto drop;

	if (pskb_trim_rcsum(skb, len))
		goto drop;

	pn = dzc_pppoe_pernet(dev_net(dev));

	po = dzc_get_item(pn, ph->sid, eth_hdr(skb)->h_source, dev->ifindex);
	if (!po)
		goto drop;

	return sk_receive_skb(sk_pppox(po), skb, 0);

drop:
	kfree_skb(skb);
out:
	return NET_RX_DROP;
}

static int dzc_pppoe_disc_rcv(struct sk_buff *skb, struct net_device *dev,
			  struct packet_type *pt, struct net_device *orig_dev)

{
	struct pppoe_hdr *ph;
	struct pppox_sock *po;
	struct pppoe_net *pn;

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		goto out;

	if (!pskb_may_pull(skb, sizeof(struct pppoe_hdr)))
		goto abort;

	ph = pppoe_hdr(skb);
	if (ph->code != PADT_CODE)
		goto abort;

	pn = dzc_pppoe_pernet(dev_net(dev));
	po = dzc_get_item(pn, ph->sid, eth_hdr(skb)->h_source, dev->ifindex);
	if (po) {
		struct sock *sk = sk_pppox(po);

		bh_lock_sock(sk);

		if (sock_owned_by_user(sk) == 0) {
			
			sk->sk_state = PPPOX_ZOMBIE;
		}

		bh_unlock_sock(sk);
		sock_put(sk);
	}

abort:
	kfree_skb(skb);
out:
	return NET_RX_SUCCESS;
}

static struct packet_type pppoes_ptype __read_mostly = {
	.type	= cpu_to_be16(ETH_P_PPP_SES),
	.func	= dzc_pppoe_rcv,
};

static struct packet_type pppoed_ptype __read_mostly = {
	.type	= cpu_to_be16(ETH_P_PPP_DISC),
	.func	= dzc_pppoe_disc_rcv,
};

static struct proto pppoe_sk_proto __read_mostly = {
	.name	  = "PPPOE",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct pppox_sock),
};

static int dzc_pppoe_create(struct net *net, struct socket *sock)
{
	struct sock *sk;

	sk = sk_alloc(net, PF_PPPOX, GFP_KERNEL, &pppoe_sk_proto);
	if (!sk)
		return -ENOMEM;

	sock_init_data(sock, sk);

	sock->state	= SS_UNCONNECTED;
	sock->ops	= &pppoe_ops;

	sk->sk_backlog_rcv	= pppoe_rcv_core;
	sk->sk_state		= PPPOX_NONE;
	sk->sk_type		= SOCK_STREAM;
	sk->sk_family		= PF_PPPOX;
	sk->sk_protocol		= PX_PROTO_OE;

	return 0;
}

static int dzc_pppoe_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po;
	struct pppoe_net *pn;
	struct net *net = NULL;

	if (!sk)
		return 0;

	lock_sock(sk);
	if (sock_flag(sk, SOCK_DEAD)) {
		release_sock(sk);
		return -EBADF;
	}

	po = pppox_sk(sk);

	if (sk->sk_state & (PPPOX_CONNECTED | PPPOX_BOUND)) {
		dev_put(po->pppoe_dev);
		po->pppoe_dev = NULL;
	}

	pppox_unbind_sock(sk);

	sk->sk_state = PPPOX_DEAD;

	net = sock_net(sk);
	pn = dzc_pppoe_pernet(net);

	dzc_delete_item(pn, po->pppoe_pa.sid, po->pppoe_pa.remote,
		    po->pppoe_ifindex);

	sock_orphan(sk);
	sock->sk = NULL;

	skb_queue_purge(&sk->sk_receive_queue);
	release_sock(sk);
	sock_put(sk);

	return 0;
}

static int dzc_pppoe_connect(struct socket *sock, struct sockaddr *uservaddr,
		  int sockaddr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct sockaddr_pppox *sp = (struct sockaddr_pppox *)uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	struct net_device *dev = NULL;
	struct pppoe_net *pn;
	struct net *net = NULL;
	int error;

	lock_sock(sk);

	error = -EINVAL;
	if (sp->sa_protocol != PX_PROTO_OE)
		goto end;

	error = -EBUSY;
	if ((sk->sk_state & PPPOX_CONNECTED) &&
	     stage_session(sp->sa_addr.pppoe.sid))
	{
		return 0;
		goto end;
	}

	error = -EALREADY;
	if ((sk->sk_state & PPPOX_DEAD) &&
	     !stage_session(sp->sa_addr.pppoe.sid))
		goto end;

	error = 0;

	if (stage_session(po->pppoe_pa.sid)) {
		pppox_unbind_sock(sk);
		pn = dzc_pppoe_pernet(sock_net(sk));
		dzc_delete_item(pn, po->pppoe_pa.sid,
			    po->pppoe_pa.remote, po->pppoe_ifindex);
		if (po->pppoe_dev) {
			dev_put(po->pppoe_dev);
			po->pppoe_dev = NULL;
		}

		memset(sk_pppox(po) + 1, 0,
		       sizeof(struct pppox_sock) - sizeof(struct sock));
		sk->sk_state = PPPOX_NONE;
	}

	if (stage_session(sp->sa_addr.pppoe.sid)) {
		error = -ENODEV;
		net = sock_net(sk);
		dev = dev_get_by_name(net, sp->sa_addr.pppoe.dev);
		if (!dev)
			goto err_put;

		pppoe_dzc_connect(sp->sa_addr.pppoe.sid);

		po->pppoe_dev = dev;
		po->pppoe_ifindex = dev->ifindex;
		pn = dzc_pppoe_pernet(net);
		if (!(dev->flags & IFF_UP)) {
			goto err_put;
		}

		memcpy(&po->pppoe_pa,
		       &sp->sa_addr.pppoe,
		       sizeof(struct pppoe_addr));

		write_lock_bh(&pn->hash_lock);
		error = __set_item(pn, po);
		write_unlock_bh(&pn->hash_lock);
		if (error < 0)
			goto err_put;

		po->chan.hdrlen = (sizeof(struct pppoe_hdr) +
				   dev->hard_header_len);

		po->chan.mtu = dev->mtu - get_dzc_header_len() - 64;
		po->chan.private = sk;
		po->chan.ops = &pppoe_chan_ops;

		error = ppp_register_net_channel(dev_net(dev), &po->chan);
		if (error) {
			dzc_delete_item(pn, po->pppoe_pa.sid,
				    po->pppoe_pa.remote, po->pppoe_ifindex);
			goto err_put;
		}

		sk->sk_state = PPPOX_CONNECTED;
	}

	po->num = sp->sa_addr.pppoe.sid;

end:
	release_sock(sk);
	return error;
err_put:
	if (po->pppoe_dev) {
		dev_put(po->pppoe_dev);
		po->pppoe_dev = NULL;
	}
	goto end;
}

static int dzc_pppoe_getname(struct socket *sock, struct sockaddr *uaddr,
		  int *usockaddr_len, int peer)
{
	int len = sizeof(struct sockaddr_pppox);
	struct sockaddr_pppox sp;

	sp.sa_family	= AF_PPPOX;
	sp.sa_protocol	= PX_PROTO_OE;
	memcpy(&sp.sa_addr.pppoe, &pppox_sk(sock->sk)->pppoe_pa,
	       sizeof(struct pppoe_addr));

	memcpy(uaddr, &sp, len);

	*usockaddr_len = len;

	return 0;
}

static int dzc_pppoe_ioctl(struct socket *sock, unsigned int cmd,
		unsigned long arg)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	int val;
	int err;

	switch (cmd) {
	case PPPIOCGMRU:
		err = -ENXIO;
		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (put_user(po->pppoe_dev->mtu -
			     sizeof(struct pppoe_hdr) -
			     PPP_HDRLEN,
			     (int __user *)arg))
			break;
		err = 0;
		break;

	case PPPIOCSMRU:
		err = -ENXIO;
		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (get_user(val, (int __user *)arg))
			break;

		if (val < (po->pppoe_dev->mtu
			   - sizeof(struct pppoe_hdr)
			   - PPP_HDRLEN))
			err = 0;
		else
			err = -EINVAL;
		break;

	case PPPIOCSFLAGS:
		err = -EFAULT;
		if (get_user(val, (int __user *)arg))
			break;
		err = 0;
		break;

	case PPPOEIOCSFWD:
	{
		struct pppox_sock *relay_po;

		err = -EBUSY;
		if (sk->sk_state & (PPPOX_BOUND | PPPOX_ZOMBIE | PPPOX_DEAD))
			break;

		err = -ENOTCONN;
		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (copy_from_user(&po->pppoe_relay,
				   (void __user *)arg,
				   sizeof(struct sockaddr_pppox)))
			break;

		err = -EINVAL;
		if (po->pppoe_relay.sa_family != AF_PPPOX ||
		    po->pppoe_relay.sa_protocol != PX_PROTO_OE)
			break;

		relay_po = dzc_get_item_by_addr(sock_net(sk), &po->pppoe_relay);
		if (!relay_po)
			break;

		sock_put(sk_pppox(relay_po));
		sk->sk_state |= PPPOX_RELAY;
		err = 0;
		break;
	}

	case PPPOEIOCDFWD:
		err = -EALREADY;
		if (!(sk->sk_state & PPPOX_RELAY))
			break;

		sk->sk_state &= ~PPPOX_RELAY;
		err = 0;
		break;

	default:
		err = -ENOTTY;
	}

	err = pppoe_dzc_ioctl(cmd, arg);

	return err;
}

static int __dzc_pppoe_xmit(struct sock *sk, struct sk_buff *skb)
{
	struct pppox_sock *po = pppox_sk(sk);
	struct net_device *dev = po->pppoe_dev;
	unsigned char *hdr_ptr;
	struct sk_buff *n;
	int i = 0;
	unsigned char *data, *obuf;

	if (sock_flag(sk, SOCK_DEAD) || !(sk->sk_state & PPPOX_CONNECTED))
		goto abort;

	if (!dev)
		goto abort;

	{
		n = skb_copy_expand(skb, get_dzc_header_len() + dev->hard_header_len + 16, skb->len + 32, GFP_ATOMIC);
		if (!n)
			goto abort;

		kfree_skb(skb);
		skb = n;
	}

	obuf = kmalloc(4096, GFP_ATOMIC);
	if (!obuf)
		goto abort;

	i = pppoe_dzc_lcp(skb->data, skb->len, obuf);

	__skb_trim(skb, 0);
	if (i > skb_tailroom(skb)) {
		kfree(obuf);
		goto abort;
	}

	data =__skb_put(skb, i);
	memcpy((void *)data, (const void *)obuf, i);
	kfree(obuf);

	if (skb_cow_head(skb, dev->hard_header_len + get_dzc_header_len()))
		goto abort;
	__skb_push(skb, get_dzc_header_len());
	skb_reset_network_header(skb);
	hdr_ptr = skb_network_header(skb);

	pppoe_dzc_skb_handle(hdr_ptr, skb->len);

	skb->dev = dev;

	dev_hard_header(skb, dev, ETH_P_IP,
			po->pppoe_pa.remote, NULL, skb->len);

	dev_queue_xmit(skb);
	return 1;

abort:
	kfree_skb(skb);
	return 1;
}

static int dzc_pppoe_xmit(struct ppp_channel *chan, struct sk_buff *skb)
{
	struct sock *sk = (struct sock *)chan->private;
	return __dzc_pppoe_xmit(sk, skb);
}

static const struct ppp_channel_ops pppoe_chan_ops = {
	.start_xmit = dzc_pppoe_xmit,
};

#ifdef CONFIG_PROC_FS
static int pppoe_seq_show(struct seq_file *seq, void *v)
{
	struct pppox_sock *po;
	char *dev_name;

	if (v == SEQ_START_TOKEN) {
		seq_puts(seq, "Id       Address              Device\n");
		goto out;
	}

	po = v;
	dev_name = po->pppoe_pa.dev;

	seq_printf(seq, "%08X %pM %8s\n",
		po->pppoe_pa.sid, po->pppoe_pa.remote, dev_name);
out:
	return 0;
}

static inline struct pppox_sock *pppoe_get_idx(struct pppoe_net *pn, loff_t pos)
{
	struct pppox_sock *po;
	int i;

	for (i = 0; i < (1 << 4); i++) {
		po = pn->hash_table[i];
		while (po) {
			if (!pos--)
				goto out;
			po = po->next;
		}
	}

out:
	return po;
}

static void *pppoe_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(pn->hash_lock)
{
	struct pppoe_net *pn = dzc_pppoe_pernet(seq_file_net(seq));
	loff_t l = *pos;

	read_lock_bh(&pn->hash_lock);
	return l ? pppoe_get_idx(pn, --l) : SEQ_START_TOKEN;
}

static void *pppoe_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct pppoe_net *pn = dzc_pppoe_pernet(seq_file_net(seq));
	struct pppox_sock *po;

	++*pos;
	if (v == SEQ_START_TOKEN) {
		po = pppoe_get_idx(pn, 0);
		goto out;
	}
	po = v;
	if (po->next)
		po = po->next;
	else {
		int hash = hash_item(po->pppoe_pa.sid, po->pppoe_pa.remote);

		po = NULL;
		while (++hash < (1 << 4)) {
			po = pn->hash_table[hash];
			if (po)
				break;
		}
	}

out:
	return po;
}

static void pppoe_seq_stop(struct seq_file *seq, void *v)
	__releases(pn->hash_lock)
{
	struct pppoe_net *pn = dzc_pppoe_pernet(seq_file_net(seq));
	read_unlock_bh(&pn->hash_lock);
}

static const struct seq_operations pppoe_seq_ops = {
	.start		= pppoe_seq_start,
	.next		= pppoe_seq_next,
	.stop		= pppoe_seq_stop,
	.show		= pppoe_seq_show,
};

static int pppoe_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &pppoe_seq_ops,
			sizeof(struct seq_net_private));
}

static const struct file_operations pppoe_seq_fops = {
	.owner		= THIS_MODULE,
	.open		= pppoe_seq_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release_net,
};

#endif /* CONFIG_PROC_FS */

static const struct proto_ops pppoe_ops = {
	.family		= AF_PPPOX,
	.owner		= THIS_MODULE,
	.release	= dzc_pppoe_release,
	.bind		= sock_no_bind,
	.connect	= dzc_pppoe_connect,
	.socketpair	= sock_no_socketpair,
	.accept		= sock_no_accept,
	.getname	= dzc_pppoe_getname,
	.poll		= sock_no_poll,
	.listen		= sock_no_listen,
	.shutdown	= sock_no_shutdown,
	.setsockopt	= sock_no_setsockopt,
	.getsockopt	= sock_no_getsockopt,
	.sendmsg	= sock_no_sendmsg,
	.recvmsg	= sock_no_recvmsg,
	.mmap		= sock_no_mmap,
	.ioctl		= pppox_ioctl,
};

static struct pppox_proto pppoe_proto = {
	.create	= dzc_pppoe_create,
	.ioctl	= dzc_pppoe_ioctl,
	.owner	= THIS_MODULE,
};

static __net_init int dzc_pppoe_init_net(struct net *net)
{
	struct pppoe_net *pn = dzc_pppoe_pernet(net);
	struct proc_dir_entry *pde;

	rwlock_init(&pn->hash_lock);

	pde = proc_net_fops_create(net, "pppoe", S_IRUGO, &pppoe_seq_fops);
#ifdef CONFIG_PROC_FS
	if (!pde)
		return -ENOMEM;
#endif

	return 0;
}

static __net_exit void dzc_pppoe_exit_net(struct net *net)
{
	proc_net_remove(net, "pppoe");
}

static struct pernet_operations pppoe_net_ops = {
	.init = dzc_pppoe_init_net,
	.exit = dzc_pppoe_exit_net,
	.id   = &pppoe_net_id,
	.size = sizeof(struct pppoe_net),
};

static unsigned int pppoxt_rcv(unsigned int hook,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct pppox_sock *po;
	struct pppoe_net *pn;
	struct iphdr *iph = (struct iphdr *)skb_network_header(skb);
	struct udphdr *udph = (struct udphdr *)((char *)iph + iph->ihl * 4);
	unsigned int pos;

	if (-1 == pppoe_dzc_pack_check(iph, udph))
		return NF_ACCEPT;
	
	if (!pskb_may_pull(skb, get_dzc_header_len()))
		return NF_DROP;

	__skb_pull(skb, get_dzc_header_len());

	pos = pppoe_dzc_escape(skb->data, skb->len);

	__skb_trim(skb, pos);

	if (is_ppp_fg(skb->data)) {
		skb->len -= 1;
		skb->data +=1;
	}
	if ((is_ppp_at(skb->data))) {
		skb->len -= 1;
		skb->data +=1;
	}
	if (is_ppp_ui(skb->data))
	{
		skb->len -= 1;
		skb->data +=1;
	}
	if (is_ppp_ip(skb->data)) {
		skb->len += 1;
		skb->data -=1;
		skb->data[0] = 0;
	}

		
	pn = dzc_pppoe_pernet(dev_net(in));

	po = dzc_get_item(pn, pppoe_dzc_get_connect(), eth_hdr(skb)->h_source, in->ifindex);
	if (!po)
		return NF_DROP;

	sk_receive_skb(sk_pppox(po), skb, 0);

	return NF_STOLEN;
}

static struct nf_hook_ops pppoxt_hook_ops = {
	.hook = pppoxt_rcv,
	.pf = PF_INET,
	.hooknum = NF_INET_LOCAL_IN,
	.priority = NF_IP_PRI_FILTER - 1,
};

int pppoe_dzc_init(void)
{
	int err;

	DZC_ACMM_INIT();

	err = register_pernet_device(&pppoe_net_ops);
	if (err)
		goto out;

	err = proto_register(&pppoe_sk_proto, 0);
	if (err)
		goto out_unregister_net_ops;

	err = register_pppox_proto(PX_PROTO_OE, &pppoe_proto);
	if (err)
		goto out_unregister_pppoe_proto;

	nf_register_hook(&pppoxt_hook_ops);

	dev_add_pack(&pppoes_ptype);
	dev_add_pack(&pppoed_ptype);
	register_netdevice_notifier(&pppoe_notifier);

	return 0;

out_unregister_pppoe_proto:
	proto_unregister(&pppoe_sk_proto);
out_unregister_net_ops:
	unregister_pernet_device(&pppoe_net_ops);
out:
	return err;
}

void pppoe_dzc_exit(void)
{
	unregister_netdevice_notifier(&pppoe_notifier);
	dev_remove_pack(&pppoed_ptype);
	dev_remove_pack(&pppoes_ptype);
	nf_unregister_hook(&pppoxt_hook_ops);
	unregister_pppox_proto(PX_PROTO_OE);
	proto_unregister(&pppoe_sk_proto);
	unregister_pernet_device(&pppoe_net_ops);
}


