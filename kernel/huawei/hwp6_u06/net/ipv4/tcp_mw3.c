#include <linux/mm.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/inet_diag.h>
#include <net/tcp.h>
#include <net/inet_connection_sock.h>

#define TCP_INFINITE_SSTHRESH 0x7fffffff
#define NETLINK_MW3_PROL 25
#define NUM 0x19870627
#define LENGTH (20*sizeof(u32))

struct ump
{
    int num;
    int len;
    u32 data[20];
};

struct ump_mw3
{
    u8     reset_rtt_min:1;
    u8     first_ack:1;
    u8     phase:1;
    u8     zero:1;
    u8     reset_rtt_max:1;
    u8     rtt_phase:1;
    u8     filter_phase:1;
    u8     est_phase:1;
    u8     ssthresh_phase:1;
    u8     reset_rcved:1;
    u8     mss_phase:1;
    u8     reset_mw3:1;
    u8     rcved_una:1;
    u8     is_reno:1;
    u8     reset_bw_est:1;
    u8     ssthresh_choose:1;
    u8     reserved[2];
};

struct mw3{
    u32    bw_ns_est;
    u32    bw_est;
    u32    rtt;
    u32    rtt_min;
    u32    last_ack_time;
    u32    rcved;
    u32    accounted;
    u32    snd_una;
    u32    last_rc_time;
    u32    m_cwnd;
    u32    m_snd_cwnd;
    u32    ack_count;
    u32    loss_count;
    u32    loss_rate;
    u8     reset_rtt_min:1;
    u8     first_ack:1;
    u8     phase:1;
    u8     zero:1;
    u8     reset_rtt_max:1;
    u8     rtt_phase:1;
    u8     filter_phase:1;
    u8     est_phase:1;
    u8     ssthresh_phase:1;
    u8     reset_rcved:1;
    u8     mss_phase:1;
    u8     reset_mw3:1;
    u8     rcved_una:1;
    u8     is_reno:1;
    u8     reset_bw_est:1;
    u8     ssthresh_choose:1;
};

static struct sock *g_nl_sk = NULL;

static u32 ALPHA = HZ;
static u32 RTT_THRESH = HZ;
static u32 TCP_mw3_RTT_MIN = HZ;
static u32 TCP_WESTWOOD_INIT_RTT = HZ;
static u32 g_rtt_para = HZ;
static u32 g_filter_para = 2;
static u32 BETA = 1000;
static u32 MSS_ALPHA = 1460;
static u32 MSS_BETA = 1000;
static u32 ssthresh_para = 10;
static u32 g_acked_para = HZ/10;
static u32 INC = 10;
static u32 PHASE = 10;
static u32 inc_para = 5;
static u32 g_init_bw = 10;
static u32 g_init_rcved = 20;
static u32 g_init_count = 30;
static u32 g_init_rate = 10;
static u32 g_init_cwnd = 0;
static struct ump_mw3 g_m = {
            0,1,0,1,\
            0,0,1,0,\
            1,1,1,0,\
            0,0,0,1,\
            {0}\
            };

static void tcp_mw3_init(struct sock *sk)
{
    struct tcp_sock *tp=tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    m->bw_ns_est = g_init_bw;
    m->bw_est = g_init_bw;
    m->rtt_min = m->rtt = TCP_WESTWOOD_INIT_RTT;
    m->rcved = g_init_rcved;
    m->accounted = g_init_rcved;
    m->last_ack_time = tcp_time_stamp;
    m->last_rc_time = tcp_time_stamp;
    m->snd_una = tp->snd_una;
    m->m_snd_cwnd = tp->snd_cwnd;
    m->ack_count = g_init_count;
    m->loss_count = g_init_count;
    m->loss_rate = g_init_rate;
    m->m_cwnd = g_init_cwnd;
    m->reset_rtt_min = g_m.reset_rtt_min;
    m->phase = g_m.phase;
    m->first_ack = g_m.first_ack;
    m->reset_rtt_max = g_m.reset_rtt_max;
    m->rtt_phase = g_m.rtt_phase;
    m->filter_phase = g_m.filter_phase;
    m->est_phase = g_m.est_phase;
    m->ssthresh_phase = g_m.ssthresh_phase;
    m->reset_rcved = g_m.reset_rcved;
    m->mss_phase = g_m.mss_phase;
    m->reset_mw3 = g_m.reset_mw3;
    m->rcved_una = g_m.rcved_una;
    m->is_reno = g_m.is_reno;
    m->reset_bw_est = g_m.reset_bw_est;
    m->ssthresh_choose = g_m.ssthresh_choose;
}

static inline void update_rtt_max(struct mw3 *m)
{
    if (!m->rtt_phase) {
        m->rtt_min = m->rtt + g_rtt_para;
        m->rtt_phase = 1;
    }
    else
    {
        m->rtt_min = max_t(u32, m->rtt_min, m->rtt);
    }
}

static inline void update_rtt_min(struct mw3 *m)
{
    if (m->reset_rtt_min) {
        m->rtt_min = m->rtt + g_rtt_para;
        m->reset_rtt_min = 0;
    }
    else if(!m->rtt_phase)
    {
        m->rtt_min = min_t(u32, m->rtt_min, m->rtt);
    }
    else
    {
        m->rtt_min = (m->rtt + m->rtt_min) >> 1;
    }
}

static void tcp_mw3_pkts_acked(struct sock *sk, u32 cnt, s32 rtt)
{
    struct mw3 *m = inet_csk_ca(sk);
    struct tcp_sock *tp = tcp_sk(sk);
    if(m->reset_rtt_max)
    {
        m->rtt = usecs_to_jiffies(tp->srtt);
        update_rtt_max(m);
    }
    else if(rtt > 0)
    {
        m->rtt = usecs_to_jiffies(rtt);
        update_rtt_min(m);
    }
}

static inline u32 mw3_do_filter(struct sock *sk,u32 a, u32 b)
{
    struct mw3 *m = inet_csk_ca(sk);
    if(m->filter_phase)
    {
        return g_filter_para * ( (a) + (7 * b)) >> 3;
    }
    else
        return g_filter_para *(((3 * a) + (b)) >> 2);
}

static void mw3_filter(struct sock *sk, u32 delta)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    u32 new_ssthresh;
    if (m->bw_ns_est == 0 && m->bw_est == 0 && m->est_phase) {
        m->bw_ns_est = m->rcved / delta;
        m->bw_est = ssthresh_para * m->bw_ns_est;
    }
    else if(!m->est_phase)
    {
        m->bw_ns_est = m->rcved * m->ack_count / delta;
        m->bw_est = TCP_mw3_RTT_MIN * m->bw_ns_est;
        m->est_phase = 1;
    }
    else{
        m->bw_ns_est = mw3_do_filter(sk,m->bw_ns_est, m->rcved / delta);
        m->bw_est = mw3_do_filter(sk,m->bw_est, m->bw_ns_est);
    }
    if(m->phase)
    {
        if(m->rtt_min){
            new_ssthresh = (m->bw_est + BETA) * (m->rtt_min + ALPHA) / (tp->mss_cache + MSS_ALPHA);
            if(m->ssthresh_phase && m->m_snd_cwnd < new_ssthresh)
            {
                m->m_snd_cwnd = new_ssthresh / ssthresh_para;
            }
            else if(m->m_cwnd < new_ssthresh)
            {
                m->m_cwnd = new_ssthresh / ssthresh_para;
            }
        }
    }
}

static void bw_estimation(struct sock *sk,u32 delta){
    struct mw3 *m = inet_csk_ca(sk);
    if(m->reset_rcved && m->rcved)
    {
        mw3_filter(sk, delta);
        m->rcved = g_init_rcved;
        m->reset_rcved = 0;
    }
    else if (!m->reset_rcved && m->rcved && delta > 0)
    {
        mw3_filter(sk,delta);
        m->rcved = g_init_rcved;
        m->last_ack_time = tcp_time_stamp;
    }
}

static inline void mw3_fast_bw(struct sock *sk){
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    u32 delta;
    m->ack_count++;
    if (m->first_ack && m->mss_phase) {
        m->snd_una = tp->snd_una + MSS_ALPHA;
        m->last_ack_time = tcp_time_stamp;
        m->first_ack = 0;
        return;
    }
    else if(!m->mss_phase)
    {
        m->snd_una = tp->snd_una + MSS_BETA;
        m->last_ack_time = tcp_time_stamp;
        m->mss_phase = 1;
        return ;
    }
    if(m->mss_phase)
    {
        m->rcved += tp->snd_una - m->snd_una + MSS_ALPHA;
    }
    else
    {
        m->rcved += tp->snd_una - m->snd_una + MSS_BETA;
    }

    delta = tcp_time_stamp - m->last_ack_time;
    if(delta > max_t(u32, m->rtt, TCP_mw3_RTT_MIN)){
        bw_estimation(sk,delta);
    }
    if(m->mss_phase)
    {
        m->snd_una = tp->snd_una + MSS_ALPHA;
    }
    else
    {
        m->snd_una = tp->snd_una + MSS_BETA;
    }
}

static inline void mw3_other_ack(struct sock *sk){
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    u32 delta;
    u32 cumul_ack;
    m->ack_count++;
    if (m->first_ack && m->mss_phase) {
        m->snd_una = tp->snd_una + MSS_ALPHA;
        m->last_ack_time = tcp_time_stamp;
        m->first_ack = 0;
        return;
    }
    else if(!m->mss_phase)
    {
        m->snd_una = tp->snd_una + MSS_BETA;
        m->last_ack_time = tcp_time_stamp;
        m->mss_phase = 1;
        return ;
    }

    delta = tcp_time_stamp - m->last_ack_time;

    cumul_ack = tp->snd_una - m->snd_una;
    if(!cumul_ack){
        m->accounted += tp->mss_cache;
        m->rcved += tp->mss_cache;
    }
    else if(cumul_ack > 0){
        if(m->accounted >= cumul_ack){
            m->accounted -= cumul_ack;
            m->rcved += tp->mss_cache;
        }
        else{
            cumul_ack -= m->accounted;
            m->rcved += cumul_ack;
            m->accounted = 0;
        }
    }
    if(delta > max_t(u32, m->rtt, TCP_mw3_RTT_MIN)){
        bw_estimation(sk,delta);
    }
    if(m->mss_phase)
    {
        m->snd_una = tp->snd_una + MSS_ALPHA;
    }
    else
    {
        m->snd_una = tp->snd_una + MSS_BETA;
    }
}

static inline u32 update_m_cwnd(struct sock *sk){
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    if(!m->est_phase)
    {
        return (m->bw_est + BETA) *  ( m->rtt_min + ALPHA)/ (tp->mss_cache + MSS_ALPHA) ;
    }
    else if(m->loss_rate <=1 )
    {
        return (m->bw_est + BETA) * m->rtt_min / (tp->mss_cache + MSS_ALPHA);
    }
    else
    {
        return (m->bw_est + BETA) * ( m->rtt_min + m->rtt_min * 10 / m->loss_rate )/ (tp->mss_cache + MSS_ALPHA);
    }
}

static void update_beta(struct sock *sk,u32 acked, u32 loss){
    struct mw3 *m = inet_csk_ca(sk);
    u32 loss_rate;
    loss_rate = g_acked_para * acked / loss;
    if(!m->loss_rate)
        m->loss_rate = loss_rate;
    else
        m->loss_rate = ( m->loss_rate + loss_rate ) >> 1;

    m->loss_rate = m->loss_rate / 10;
    m->loss_rate = m->loss_rate * 10;
}

static void mw3_end_BA_phase(struct sock *sk){
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);

    m->phase = 0;

    if(m->bw_est > 0)
        tp->snd_cwnd = update_m_cwnd(sk);
    else
        tp->snd_cwnd = (tp->snd_cwnd + BETA) * m->rtt_min / (m->rtt_min + ALPHA);
}

static void tcp_mw3_onloss(struct sock *sk){
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    u32    w_cwnd;
    m->loss_count++;
    if(tcp_time_stamp - m->last_rc_time > ALPHA){
        update_beta(sk, m->ack_count, m->loss_count);
        m->loss_count = g_init_count;
        m->ack_count = g_init_count;
        m->last_rc_time = tcp_time_stamp;
    }

    tp->snd_cwnd = m->m_snd_cwnd + BETA;
    if ( m->reset_mw3 && m->phase)
    {
        mw3_end_BA_phase(sk);
        m->m_snd_cwnd = tp->snd_cwnd + MSS_ALPHA;
        return ;
    }
    if(m->phase)
    {
        if(tp->snd_cwnd < m->m_cwnd){
            return;
        }else{
            mw3_end_BA_phase(sk);
            m->m_snd_cwnd = tp->snd_cwnd + MSS_ALPHA;
        }
        return;
    }
    if(m->loss_rate)
        w_cwnd = (m->bw_est + BETA) * (m->rtt_min + m->rtt_min * 2 / m->loss_rate) / (tp->mss_cache + MSS_ALPHA);
    else
        w_cwnd = (m->bw_est + BETA) * (m->rtt_min) / (tp->mss_cache + MSS_ALPHA);

    if(tp->snd_cwnd <= w_cwnd)
        return;
    if (m->rcved_una)
    {
        tp->snd_cwnd = max(w_cwnd, tp->snd_cwnd/2);
        m->m_snd_cwnd = tp->snd_cwnd + MSS_ALPHA;
    }
    else
    {
        tp->snd_cwnd = g_init_cwnd;
        m->m_snd_cwnd = tp->snd_cwnd + MSS_ALPHA;
    }
}

static void tcp_mw3_ontimeout(struct sock *sk){
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    m->loss_count++;
    if(tcp_time_stamp - m->last_rc_time > ALPHA){
        update_beta(sk, m->ack_count, m->loss_count);
        m->loss_count = g_init_count;
        m->ack_count = g_init_count;
        m->last_rc_time = tcp_time_stamp;
    }

    tp->snd_cwnd = m->m_snd_cwnd + BETA;
    if ( m->reset_mw3 && m->phase)
    {
        mw3_end_BA_phase(sk);
        m->m_snd_cwnd = tp->snd_cwnd + MSS_ALPHA;
        return ;
    }
    if(m->phase)
    {
        if(tp->snd_cwnd < m->m_cwnd){
            return;
        }
        else
            mw3_end_BA_phase(sk);
        tp->snd_cwnd = g_init_cwnd;

    }
    else
    {
        m->m_cwnd = update_m_cwnd(sk);
        if(tp->snd_cwnd < m->m_cwnd){
            return;
        }
        tp->snd_cwnd = g_init_cwnd;
    }
    m->m_snd_cwnd = tp->snd_cwnd + MSS_ALPHA;
    m->reset_rtt_min = 1;
    m->rtt_phase = 1;
}

void tcp_cong_avoid_mw3(struct sock *sk, u32 w, u32 in_flight)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    if(m->loss_rate)
        w = w >> (200 / m->loss_rate);
    if(m->rtt_min/RTT_THRESH){
        w = w /(m->rtt_min / RTT_THRESH);
    }
    if (sysctl_tcp_abc) {
        if (tp->bytes_acked >= tp->snd_cwnd * tp->mss_cache)
        {
            tp->bytes_acked -= tp->snd_cwnd * tp->mss_cache;
            if (tp->snd_cwnd  <  tp->snd_cwnd_clamp)
                tp->snd_cwnd++;
        }
    }
    else if (m->rcved_una)
    {
        tcp_cong_avoid_ai(tp, w);
    }
    else
    {
        tcp_reno_cong_avoid(sk, 0, in_flight);
    }
}

static void tcp_mw3_cong_avoid(struct sock *sk, u32 ack, u32 in_flight)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    u32 w_cwnd;
    u32 B=0;
    u32 inc;
    tp->snd_cwnd = m->m_snd_cwnd + MSS_ALPHA;
    if (!m->is_reno)
    {
        tcp_reno_cong_avoid(sk, ack, in_flight);
        m->m_snd_cwnd = tp->snd_cwnd + BETA;
        return;
    }
    if(m->phase){
        tcp_slow_start(tp);
        m->m_snd_cwnd = tp->snd_cwnd + BETA;
        return;
    }
    if(m->bw_est)
        m->m_cwnd = update_m_cwnd(sk);

    if(m->loss_rate)
        w_cwnd = (m->bw_est + BETA) * (m->rtt_min + m->rtt_min * 2 / m->loss_rate) / (tp->mss_cache + MSS_ALPHA);
    else
        w_cwnd = (m->bw_est + BETA) * (m->rtt_min) / (tp->mss_cache + MSS_ALPHA);

    if (m->reset_bw_est && m->bw_est )
    {
        tp->snd_cwnd = g_init_cwnd;
        m->reset_bw_est = 0;
    }
    else if(tp->snd_cwnd < w_cwnd)
        tp->snd_cwnd = inc_para * (tp->snd_cwnd + INC);
    else if(w_cwnd <= tp->snd_cwnd &&  tp->snd_cwnd < m->m_cwnd){
        B = m->m_cwnd - tp->snd_cwnd;
        inc = B / PHASE;
        if(0 == inc)
            inc = INC;
        tp->snd_cwnd = inc_para * (tp->snd_cwnd + inc);
    }
    else
        tcp_cong_avoid_mw3(sk, tp->snd_cwnd, in_flight);

    m->m_snd_cwnd = tp->snd_cwnd + BETA;

}

u32 tcp_mw3_ssthresh(struct sock *sk)
{
    struct tcp_sock *tp = tcp_sk(sk);
    struct mw3 *m = inet_csk_ca(sk);
    u32 a = 0;
    tp->snd_cwnd = m->m_snd_cwnd + BETA;
    if (m->ssthresh_choose)
    {
        m->m_cwnd = update_m_cwnd(sk);
        a = m->m_cwnd;
    }
    else
    {
        m->m_snd_cwnd = update_m_cwnd(sk);
        a =  m->m_snd_cwnd ;
    }

    if(tp->snd_cwnd < a)
        return TCP_INFINITE_SSTHRESH;
    else
        return tcp_reno_ssthresh(sk);
}

static u32 tcp_mw3_min_cwnd(const struct sock *sk)
{
    const struct tcp_sock *tp = tcp_sk(sk);
    const struct mw3 *m = inet_csk_ca(sk);
    return max_t(u32, (m->bw_est + BETA) * m->rtt_min / (tp->mss_cache + MSS_ALPHA), max_t(u32,  2,  tp->snd_ssthresh/2));
}

void tcp_mw3_cwnd_event(struct sock *sk, enum tcp_ca_event event)
{
    switch(event)
    {
        case CA_EVENT_FAST_ACK:
            mw3_fast_bw(sk);
            break;
        case CA_EVENT_SLOW_ACK:
            mw3_other_ack(sk);
            break;
        case CA_EVENT_COMPLETE_CWR:
            tcp_mw3_onloss(sk);
            break;
        case CA_EVENT_FRTO:
            tcp_mw3_ontimeout(sk);
            break;
        default:
            break;
    }
}

static void tcp_mw3_info(struct sock *sk, u32 ext,struct sk_buff *skb)
{
    const struct mw3 *ca = inet_csk_ca(sk);
    if (ext & (1 << (INET_DIAG_VEGASINFO - 1))) {
        struct tcpvegas_info info = {
            .tcpv_enabled = 1,
            .tcpv_rtt = jiffies_to_usecs(ca->rtt),
        };
        nla_put(skb, INET_DIAG_VEGASINFO, sizeof(info), &info);
    }
}

static struct tcp_congestion_ops tcp_mw3={
    .flags                  = TCP_CONG_RTT_STAMP,
    .init                    = tcp_mw3_init,
    .ssthresh             = tcp_mw3_ssthresh,
    .cong_avoid         = tcp_mw3_cong_avoid,
    .get_info             = tcp_mw3_info,
    .pkts_acked         = tcp_mw3_pkts_acked,
    .cwnd_event         = tcp_mw3_cwnd_event,
    .min_cwnd            = tcp_mw3_min_cwnd,
    .owner                 = THIS_MODULE,
    .name                  = "mw3",

};

static int g_revd = 0;
static int ump_kernel_receive(struct sk_buff *skb);

static int ump_kernel_netlink_init(void)
{
    if(!g_nl_sk)
    {
        g_nl_sk = netlink_kernel_create(&init_net,NETLINK_MW3_PROL,0,
            (void *)ump_kernel_receive,NULL,THIS_MODULE);
        if (NULL == g_nl_sk)
        {
            printk(KERN_EMERG "netlink_kernel_create failure !\n");
            return -1;
        }
    }
    return 0;

}

static void ump_kernel_netlink_fini(void)
{
    if(g_nl_sk)
    {
        netlink_kernel_release(g_nl_sk);
        g_nl_sk = NULL;
        printk(KERN_EMERG "-------%s-------\n","release netlink sock ok");
    }
}

static void ump_handle_data(u32 *data)
{
    g_revd = 1;
    if(data[0])
        ALPHA /= data[0];
    if(data[1])
        RTT_THRESH /= data[1];
    if(data[2])
        TCP_mw3_RTT_MIN /= data[2];
    if(data[3])
        TCP_WESTWOOD_INIT_RTT *=  data[3];
    g_rtt_para = data[4];
    g_filter_para = data[5];
    BETA = data[6];
    MSS_ALPHA = data[7];
    MSS_BETA = data[8];
    ssthresh_para = data[9];
    g_acked_para = data[10];
    INC = data[11];
    PHASE = data[12];
    inc_para = data[13];
    g_init_bw = data[14];
    g_init_rcved = data[15];
    g_init_count = data[16];
    g_init_rate = data[17];
    g_init_cwnd = data[18];
    memcpy(&g_m, &data[19], sizeof(u32));
}

static int ump_kernel_receive(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    struct ump *data;
    int actual_len = 0;

    if(NULL == skb)
    {
        goto out;
    }

    nlh = nlmsg_hdr(skb);
    if(NULL == nlh)
    {
        goto out;
    }

    if((nlh->nlmsg_len < NLMSG_HDRLEN) || (skb->len < nlh->nlmsg_len))
    {
        goto out;
    }

    data = (struct ump *)NLMSG_DATA(nlh);
    actual_len = nlh->nlmsg_len - NLMSG_HDRLEN;

    if (!data)
    {
        return -1;
    }
    if (data->num != NUM)
    {
        goto out;
    }
    if(actual_len == data->len)
    {
        int len = actual_len - 2 * sizeof(int);
        if( len != LENGTH)
        {
            printk(KERN_EMERG "line:%d ump length is wrong.\n",__LINE__);
            goto out;
        }
        else if(!g_revd)
        {
            ump_handle_data(data->data);
            printk(KERN_EMERG"line:%d ump data set success!\n",__LINE__);
        }
    }
out:
    return 0;
}

static int __init tcp_mw3_register(void){
    BUILD_BUG_ON(sizeof(struct mw3) > ICSK_CA_PRIV_SIZE);
    if(ump_kernel_netlink_init() < 0)
         return -1;
    return tcp_register_congestion_control(&tcp_mw3);
}

static void __exit tcp_mw3_unregister(void){
    ump_kernel_netlink_fini();
    tcp_unregister_congestion_control(&tcp_mw3);
}

module_init(tcp_mw3_register);
module_exit(tcp_mw3_unregister);

MODULE_AUTHOR("ZX.Zou @Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TCP MW-3");