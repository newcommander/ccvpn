#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <ifaddrs.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <pcap/pcap.h>
#include <assert.h>

typedef __u8  u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

#define EMU
#ifdef EMU
static char errbuf[PCAP_ERRBUF_SIZE];
static pcap_t *pcap_handle = NULL;
static struct pcap_pkthdr *pkt_header = NULL;
static const unsigned char *pkt_data = NULL;
static u8 my_mac[6];
static char *pcap_filename = NULL;
#endif

static const int DEFAULT_PORT = 5544;
static char active_addr[128];
static struct sockaddr_in local_udp_addr;

static uv_udp_t udp_handle;
static uv_loop_t udp_loop;
static uv_timer_t loop_alarm;

#define RECIVE_CHUNK_COUNT 32
#define RECIVE_CHUNK_SIZE 65536
static char *chunks[RECIVE_CHUNK_COUNT];
int chunk_index = 0;
static pthread_mutex_t chunk_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef EMU
int reopen_pcap_file()
{
    int ret = 0;

    assert(pcap_filename != NULL);

    fprintf(stdout, "load file: %s.\n", pcap_filename);

    if (pcap_handle)
        pcap_close(pcap_handle);

    pcap_handle = pcap_open_offline(pcap_filename, errbuf);
    if (!pcap_handle) {
        fprintf(stderr, "open failed: %s.\n", errbuf);
        pcap_handle = NULL;
        return -1;
    }

    if (pcap_datalink(pcap_handle) != 1) {
        fprintf(stderr, "not ETHERNET packet\n");
        pcap_close(pcap_handle);
        pcap_handle = NULL;
        return -1;
    }

    ret = pcap_next_ex(pcap_handle, &pkt_header, &pkt_data); // skip first packet
    if (ret == -1) {
        fprintf(stderr, "skip first packet failed: %s.\n", pcap_geterr(pcap_handle));
        pcap_close(pcap_handle);
        pcap_handle = NULL;
        return -1;
    }

    return 0;
}
#endif

void udp_handle_close_cb(uv_handle_t *handle)
{
    fprintf(stdout, "%s():\n", __func__);
}

static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    pthread_mutex_lock(&chunk_mutex);
    buf->base = chunks[chunk_index];
    chunk_index = (chunk_index + 1) % RECIVE_CHUNK_COUNT;
    pthread_mutex_unlock(&chunk_mutex);

    buf->len = RECIVE_CHUNK_SIZE;
}

static void print_buf(unsigned char *buf, ssize_t size)
{
    ssize_t i;

    return;
    if (!buf)
        return;

    fprintf(stdout, "recive[%lu]:\n", size);
    for (i = 0; i < size; i++) {
        fprintf(stdout, "%02X", buf[i]);
        if ((i + 1) % 32) {
            fprintf(stdout, " ");
            if (!((i + 1) % 16))
                fprintf(stdout, " ");
        } else {
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, "\n");
}

static int send_buf(int fd, unsigned char *buf, ssize_t len, const struct sockaddr *addr)
{
    struct msghdr msg;
    struct iovec msg_iov[64];
    ssize_t remain, max_pkg_len = 1400, pkg_count;
    int i;

    pkg_count = len / max_pkg_len;
    pkg_count += (len % max_pkg_len) ? 1 : 0;
    if (pkg_count >= 64) {
        errno = EMSGSIZE;
        return -1;
    }

    remain = len;
    for (i = 0; i < pkg_count; i++) {
        msg_iov[i].iov_base = &buf[i * max_pkg_len];
        msg_iov[i].iov_len = (remain > max_pkg_len) ? max_pkg_len : remain;
        remain -= msg_iov[i].iov_len;
    }

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = msg_iov;
    msg.msg_iovlen = pkg_count;
    msg.msg_name = (void*)addr;
    msg.msg_namelen = sizeof(struct sockaddr_in);

    return sendmsg(fd, &msg, 0);
}

static void recv_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
#ifdef EMU
    unsigned char *p;
    ssize_t len;
    int ret;
#endif

    if (nread < 0) {
        fprintf(stderr, "%s: UDP recive error: %s\n", __func__, uv_strerror(nread));
        return;
    }
    if (nread == 0) {
        if (addr)
            ; // recive packet with 0 byte.
        else
            ; // there is no more data to read, do release buf.
        return;
    }

    print_buf((unsigned char*)(buf->base), nread);
#ifdef EMU
    if (!pcap_handle)
        return;

    ret = pcap_next_ex(pcap_handle, &pkt_header, &pkt_data);
    if (ret == -2) {
        if (reopen_pcap_file() < 0)
            return;
    } else if (ret == -1) {
        fprintf(stderr, "%s\n", pcap_geterr(pcap_handle));
        return;
    }

    while (!memcmp(my_mac, &pkt_data[6], 6)) {
        len = ntohs(*(u16*)(&pkt_data[38])) - 8; // minus length of UDP header
        p = (unsigned char*)&pkt_data[42];

        if (send_buf(handle->io_watcher.fd, p, len, addr) < 0)
            fprintf(stderr, "send message failed: %s\n", strerror(errno));

        ret = pcap_next_ex(pcap_handle, &pkt_header, &pkt_data);
        if (ret == -2) {
            if (reopen_pcap_file() < 0)
                return;
            break;
        } else if (ret == -1) {
            fprintf(stderr, "%s\n", pcap_geterr(pcap_handle));
            pcap_close(pcap_handle);
            pcap_handle = NULL;
            return;
        }
    }
#else
    if (send_buf(handle->io_watcher.fd, (unsigned char*)(buf->base), nread, addr) < 0)
        fprintf(stderr, "send message failed: %s\n", strerror(errno));
#endif
}

static void timeout_cb(uv_timer_t *timer) { }

static int get_active_address(char *nic_name, char *buf, int len)
{
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *iflist = NULL;
    char *tmp = NULL;

    if (len <= 0) {
        fprintf(stderr, "%s: Invalid parameter\n", __func__);
        return -1;
    }

    if (getifaddrs(&iflist) < 0) {
        fprintf(stderr, "%s: getting NIC list failed: %s\n", __func__, strerror(errno));
        return -1;
    }

    for (ifa = iflist; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (nic_name) {
                if (strcmp(nic_name, ifa->ifa_name))
                    continue;
            } else {
                if (!strncmp(ifa->ifa_name, "lo", 2))
                    continue;
                if (!strncmp(ifa->ifa_name, "virbr", 5))
                    continue;
            }
            sin = (struct sockaddr_in*)ifa->ifa_addr;
            tmp = inet_ntoa(sin->sin_addr);
            strncpy(buf, tmp, len < strlen(tmp) ? len : strlen(tmp));
            fprintf(stdout, "Use active NIC: %s[%s]\n", ifa->ifa_name, buf);
            break;
        }
    }

    if (strlen(buf) == 0) {
        fprintf(stderr, "%s: Could not find address for NIC[%s]\n", __func__, nic_name ? nic_name : "default");
        freeifaddrs(iflist);
        return -1;
    }

    freeifaddrs(iflist);
    return 0;
}

int alloc_recv_buf(int count, int chunk_size)
{
    char *buf = NULL;
    int i;

    buf = (char*)calloc(count, chunk_size);
    if (!buf) {
        fprintf(stderr, "alloc recive buffer failed: %s", strerror(errno));
        return -1;
    }

    pthread_mutex_lock(&chunk_mutex);
    for (i = 0; i < count; i++)
        chunks[i] = &(buf[i * chunk_size]);
    chunk_index = 0;
    pthread_mutex_unlock(&chunk_mutex);

    return 0;
}

void free_recv_buf()
{
    free(chunks[0]);
}

void sigint_handler(int signal)
{
    uv_stop(&udp_loop);
}

int main(int argc, char **argv)
{
    struct sigaction sigac;
    char *nic_name = NULL;
    int opt, ret = 0, status;
    int port = DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "i:p:f:")) != -1) {
        switch (opt) {
        case 'i':
            nic_name = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            if ((port < 1) || (port > 65535)) {
                fprintf(stderr, "Invalid port number: %d\n", port);
                exit(EXIT_FAILURE);
            }
            break;
#ifdef EMU
        case 'f':
            pcap_filename = optarg;
            break;
#endif
        default:
            fprintf(stderr, "Usage: %s [-i nic_name] [-p port] [-f pcap_filename]", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    memset(&sigac, 0, sizeof(sigac));
    sigac.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sigac, NULL) < 0) {
        fprintf(stderr, "set signal action failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(active_addr, 0, sizeof(active_addr));
    if (get_active_address(nic_name, active_addr, sizeof(active_addr) - 1) < 0)
        exit(EXIT_FAILURE);

    memset(&udp_loop, 0, sizeof(udp_loop));
    ret = uv_loop_init(&udp_loop);
    if (ret != 0) {
        fprintf(stderr, "%s: Init UDP loop failed: %s\n", __func__, uv_strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret = uv_udp_init(&udp_loop, &udp_handle);
    if (ret) {
        fprintf(stderr, "%s: Init UDP handle failed: %s\n", __func__, uv_strerror(ret));
        exit(EXIT_FAILURE);
    }

    uv_ip4_addr(active_addr, port, &local_udp_addr);
    ret = uv_udp_bind(&udp_handle, (struct sockaddr*)&local_udp_addr, 0);
    if (ret) {
        fprintf(stderr, "%s: bind UDP addr failed: %s\n", __func__, uv_strerror(ret));
        uv_loop_close(&udp_loop);
        exit(EXIT_FAILURE);
    }

    if (alloc_recv_buf(RECIVE_CHUNK_COUNT, RECIVE_CHUNK_SIZE) < 0) {
        uv_close((uv_handle_t*)&udp_handle, udp_handle_close_cb);
        uv_loop_close(&udp_loop);
        exit(EXIT_FAILURE);
    }

    ret = uv_udp_recv_start(&udp_handle, alloc_cb, recv_cb);
    if (ret) {
        fprintf(stderr, "%s: Starting UDP recive failed: %s\n", __func__, uv_strerror(ret));
        free_recv_buf();
        uv_close((uv_handle_t*)&udp_handle, udp_handle_close_cb);
        uv_loop_close(&udp_loop);
        exit(EXIT_FAILURE);
    }

    ret = uv_timer_init(&udp_loop, &loop_alarm);
    assert(ret == 0);
    ret = uv_timer_start(&loop_alarm, timeout_cb, 100, 100);
    assert(ret == 0);

#ifdef EMU
    if (!pcap_filename) {
        printf("no pcap file setup, use -f.\n");
        status = EXIT_FAILURE;
        goto out;
    }
    memset(errbuf, 0, PCAP_ERRBUF_SIZE);
    if (reopen_pcap_file() < 0) {
        status = EXIT_FAILURE;
        goto out;
    }
    memcpy(my_mac, &pkt_data[0], 6);
#endif

    uv_run(&udp_loop, UV_RUN_DEFAULT);

    status = EXIT_SUCCESS;

out:
    uv_udp_recv_stop(&udp_handle);
    uv_close((uv_handle_t*)&udp_handle, udp_handle_close_cb);

    ret = uv_timer_stop(&loop_alarm);
    assert(ret == 0);

    uv_loop_close(&udp_loop);

    free_recv_buf();

    exit(status);
}
