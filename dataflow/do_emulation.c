#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <linux/types.h>
#include <arpa/inet.h>
#include <pcap/pcap.h>

typedef __u8  u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

#define RECV_BUF_SIZE 500

char errbuf[PCAP_ERRBUF_SIZE];

static u8 my_mac[6];

int alloc_and_read_data(const char *filename, char **buf, ssize_t *len)
{
    struct stat st;
    size_t remain;
    int fd, ret;
    char *p;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open file failed: %s, %s\n", filename, strerror(errno));
        return -1;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "stat file failed: %s, %s\n", filename, strerror(errno));
        close(fd);
        return -1;
    }

    *buf = (char*)calloc(st.st_size, 1);
    if (!(*buf)) {
        fprintf(stderr, "alloc buf failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    remain = st.st_size;
    p = *buf;
    while (remain) {
        ret = read(fd, p, remain);
        if (ret < 0) {
            fprintf(stderr, "read data failed: %s\n", strerror(errno));
            free(*buf);
            close(fd);
            return -1;
        }
        remain -= ret;
        p += ret;
    }
    *len = st.st_size;

    close(fd);

    return 0;
}

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    struct pcap_pkthdr *pkt_header = NULL;
    const unsigned char *pkt_data = NULL, *p;
    pcap_t *handle = NULL;
    ssize_t len;
    int sfd, ret;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s host port datafile...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /*
    if (alloc_and_read_data(argv[3], &data, &len) < 0) {
        fprintf(stderr, "read data failed.\n");
        exit(EXIT_FAILURE);
    }
    free(data);
    */

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    ret = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
     * Try each address until we successfully connect(2).
     * If socket(2) (or connect(2)) fails, we (close the socket
     * and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */

    memset(errbuf, 0, PCAP_ERRBUF_SIZE);
    handle = pcap_open_offline(argv[3], errbuf);
    if (!handle) {
        fprintf(stderr, "%s\n", errbuf);
        exit(EXIT_FAILURE);
    }
    if (pcap_datalink(handle) != 1) {
        fprintf(stderr, "not ETHERNET packet\n");
        pcap_close(handle);
        exit(EXIT_FAILURE);
    }

    ret = pcap_next_ex(handle, &pkt_header, &pkt_data);
    if (ret == -2) {
        pcap_close(handle);
        exit(EXIT_SUCCESS);
    }
    memcpy(my_mac, &pkt_data[6], 6);

    do {
        if (memcmp(my_mac, &pkt_data[6], 6)) {
            unsigned char recv_buf[RECV_BUF_SIZE];
            memset(recv_buf, 0, RECV_BUF_SIZE);
            len = read(sfd, recv_buf, RECV_BUF_SIZE);
            if (len == -1) {
                perror("read");
                pcap_close(handle);
                exit(EXIT_FAILURE);
            }
            //fprintf(stdout, "Received %ld bytes: %s\n", (long)len, recv_buf);
        } else {
            len = ntohs(*(u16*)(&pkt_data[38])) - 8; // minus length of UDP header
            p = &pkt_data[42];
            if (write(sfd, p, len) != len) {
                perror("write");
                pcap_close(handle);
                exit(EXIT_FAILURE);
            }
        }
        ret = pcap_next_ex(handle, &pkt_header, &pkt_data);
        if (ret == -2)
            break;
        else if (ret == -1) {
            fprintf(stderr, "%s\n", pcap_geterr(handle));
            pcap_close(handle);
            exit(EXIT_FAILURE);
        }
    } while (1);

    pcap_close(handle);

    exit(EXIT_SUCCESS);
}
