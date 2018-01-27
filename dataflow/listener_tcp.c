#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <uv.h>

static const int DEFAULT_PORT = 5544;
static char active_addr[128];

static void read_cb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    char *buf = NULL, *p = NULL;
    int offset = 0;
    size_t i, len;

    len = evbuffer_get_length(input);
    buf = (char*)calloc(len + 1, 1);
    if (!buf) {
        fprintf(stderr, "%s: calloc failed\n", __func__);
        return;
    }

    p = buf;
    offset = 0;
    while ((offset = bufferevent_read(bev, p, buf + len - p)) > 0)
        p += offset;

    fprintf(stdout, "read[%lu]:\n", len);
    for (i = 0; i < len; i++) {
        fprintf(stdout, "%02X", (unsigned char)buf[i]);
        if ((i + 1) % 32) {
            fprintf(stdout, " ");
            if (!((i + 1) % 16))
                fprintf(stdout, " ");
        } else {
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, "\n");

    bufferevent_write(bev, buf, len);
    bufferevent_flush(bev, EV_WRITE, BEV_FLUSH);

    free(buf);
}

static void write_cb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) {
        ;
    }
}

static void event_cb(struct bufferevent *bev, short events, void *user_data)
{
    if (events & BEV_EVENT_EOF) {
        printf("%s: Connection closed.\n", __func__);
    } else if (events & BEV_EVENT_ERROR) {
        printf("%s: Got an error on the connection: %s\n", __func__, strerror(errno));
    } else {
        switch (events) {
        case BEV_EVENT_READING:
            printf("%s: Unexpected event: BEV_EVENT_READING\n", __func__);
            break;
        case BEV_EVENT_WRITING:
            printf("%s: Unexpected event: BEV_EVENT_WRITING\n", __func__);
            break;
        case BEV_EVENT_TIMEOUT:
            printf("%s: Unexpected event: BEV_EVENT_TIMEOUT\n", __func__);
            break;
        case BEV_EVENT_CONNECTED:
            printf("%s: Unexpected event: BEV_EVENT_CONNECTED\n", __func__);
            break;
        default:
            printf("%s: Unknow event: 0x%x\n", __func__, events);
        }
        return;
    }

    bufferevent_flush(bev, EV_WRITE, BEV_FLUSH);

    bufferevent_free(bev);
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = (struct event_base*)user_data;
    struct bufferevent *bev = NULL;

    if (!base) {
        printf("%s: Invalid parameter\n", __func__);
        return;
    }

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        printf("%s: Error constructing bufferevent!", __func__);
        event_base_loopbreak(base);
        return;
    }

    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_WRITE);
    bufferevent_enable(bev, EV_READ);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = (struct event_base*)user_data;
    //struct timeval delay = { 0, 500000 }; // 0.5 second
    struct timeval delay = { 0, 0 }; // 0.0 second

    if (!base) {
        printf("%s: Invalid parameter\n", __func__);
        return;
    }
    event_base_loopexit(base, &delay);
}

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

int main(int argc, char **argv)
{
    struct event_base *base = NULL;
    struct evconnlistener *listener = NULL;
    struct event *signal_event = NULL;
    struct sockaddr_in sin;
    char *nic_name = NULL;
    int opt, ret = 0;
    int port = DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
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
        default:
            fprintf(stderr, "Usage: %s [-i nic_name] [-p port]", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    memset(active_addr, 0, sizeof(active_addr));
    if (get_active_address(nic_name, active_addr, sizeof(active_addr) - 1) < 0)
        exit(EXIT_FAILURE);

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        exit(EXIT_FAILURE);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(active_addr);
    sin.sin_port = htons(port);

    listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
            LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        fprintf(stderr, "Could not create a listener!\n");
        ret = 1;
        goto listener_bind_failed;
    }

    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Could not create/add a signal event!\n");
        ret = 1;
        goto evsignal_new_failed;
    }

    event_base_dispatch(base);

    evconnlistener_free(listener);
evsignal_new_failed:
    event_free(signal_event);
listener_bind_failed:
    event_base_free(base);

    return ret;
}
