#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <json/json.h>
#include <uv.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#define DEFAULT_PORT 5544

using namespace std;

static const char *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char active_addr[128];

static string config_file = "";
static time_t last_mod_time;
static string config_string = "";
static Json::Value g_config;

// return next begin
static char* get_next_word(char *line_begin, char *line_end, char *word_buf, int max_buf_len)
{
    char *begin, *end, *next_begin;
    int word_len;

    if (!word_buf || max_buf_len <= 0)
        return NULL;
    memset(word_buf, 0, max_buf_len);

    if (!line_begin || !line_end || (line_begin >= line_end))
        return NULL;

    begin = line_begin;
    while (!isgraph(*begin) || *begin == '"') {
        begin++;
        if (begin >= line_end)
            return NULL;
    }
    if (*begin == '#')
        return NULL;

    end = begin;
    while (isgraph(*end) && *end != '"')
        end++;

    word_len = (end - begin) > (max_buf_len - 1) ? (max_buf_len - 1) : (end - begin);
    memcpy(word_buf, begin, word_len);
    word_buf[word_len] = '\0';

    next_begin = end;
    while (!isgraph(*next_begin)) {
        next_begin++;
        if (next_begin >= line_end)
            return NULL;
    }
    if (*next_begin == '#')
        return NULL;

    return next_begin;
}

static int handle_push(Json::Value &config, char *line, char *line_end, char *word_buf, int max_buf_len)
{
    Json::Value value;
    char *begin = line;

    begin = get_next_word(begin, line_end, word_buf, max_buf_len);
    if (!strncmp(word_buf, "route", 5)) {
        value.clear();
        if (!begin) {
            printf("error format: push route\n");
            return -1;
        }
        begin = get_next_word(begin, line_end, word_buf, max_buf_len);
        value["subnet"] = word_buf;
        if (!begin) {
            printf("error format: push route\n");
            return -1;
        }
        get_next_word(begin, line_end, word_buf, max_buf_len);
        value["netmask"] = word_buf;
        config["push"]["route"].append(value);
    } else if (!strncmp(word_buf, "dhcp-option", 11)) {
        begin = get_next_word(begin, line_end, word_buf, max_buf_len);
        if (!strncmp(word_buf, "DNS", 3)) {
            if (!begin) {
                printf("error format: push dhcp-option DNS\n");
                return -1;
            }
            if (config["push"]["dhcp_option"]["DNS"].size() >= 2) {
                printf("push too many DNS server\n");
                return -1;
            }
            get_next_word(begin, line_end, word_buf, max_buf_len);
            config["push"]["dhcp_option"]["DNS"].append(word_buf);
        }
    }

    return 0;
}

static int write_json_to_file(Json::Value &config, const string filename)
{
    Json::Value::Members members, push_members, push_dhcp_members;
    Json::Value cmd, route, push, push_dhcp, push_dhcp_dns, push_route, additional;
    ofstream ofs;
    string config_str = "";
    size_t i;
    int ii, jj;

    members = config.getMemberNames();
    for (i = 0; i < members.size(); i++) {
        if (members[i] == "ca" ||
            members[i] == "cert" ||
            members[i] == "key" ||
            members[i] == "cipher" ||
            members[i] == "dev" ||
            members[i] == "dh" ||
            members[i] == "port" ||
            members[i] == "proto" ||
            members[i] == "status" ||
            members[i] == "verb") {

            config_str += members[i] + " " + config[members[i]].asString() + "\n";
        } else if (members[i] == "client_config_dir") {
            config_str += "client-config-dir " + config[members[i]].asString() + "\n";
        } else if (members[i] == "ifconfig_pool_persist") {
            config_str += "ifconfig-pool-persist " + config[members[i]].asString() + "\n";
        } else if (members[i] == "log_append") {
            config_str += "log-append " + config[members[i]].asString() + "\n";
        } else if (members[i] == "cmd") {
            cmd = config[members[i]];
            if (!cmd.isArray()) {
                printf("error format: cmd not array.\n");
                return -1;
            }
            for (ii = 0; ii < (int)cmd.size(); ii++) {
                if (cmd[ii] == "persist_tun")
                    config_str += "persist-tun\n";
                else if (cmd[ii] == "persist_key")
                    config_str += "persist-key\n";
                else if (cmd[ii] == "client_to_client")
                    config_str += "client-to-client\n";
                else
                    config_str += cmd[ii].asString() + "\n";
            }
        } else if (members[i] == "keepalive") {
            if (!config[members[i]].isMember("interval") || !config[members[i]].isMember("timeout")) {
                printf("error format: keepalive\n");
                return -1;
            }
            config_str += members[i] + " " + config[members[i]]["interval"].asString() + " " + config[members[i]]["timeout"].asString() + "\n";
        } else if (members[i] == "management") {
            if (!config[members[i]].isMember("address") || !config[members[i]].isMember("port")) {
                printf("error format: management\n");
                return -1;
            }
            config_str += members[i] + " " + config[members[i]]["address"].asString() + " " + config[members[i]]["port"].asString() + "\n";
        } else if (members[i] == "server") {
            if (!config[members[i]].isMember("subnet") || !config[members[i]].isMember("netmask")) {
                printf("error format: server\n");
                return -1;
            }
            config_str += members[i] + " " + config[members[i]]["subnet"].asString() + " " + config[members[i]]["netmask"].asString() + "\n";
        } else if (members[i] == "route") {
            route = config[members[i]];
            if (!route.isArray()) {
                printf("error format: route not array.\n");
                return -1;
            }
            for (ii = 0; ii < (int)route.size(); ii++) {
                if (!route[ii].isMember("subnet") || !route[ii].isMember("netmask")) {
                    printf("error format: route item.\n");
                    return -1;
                }
                config_str += members[i] + " " + route[ii]["subnet"].asString() + " " + route[ii]["netmask"].asString() + "\n";
            }
        } else if (members[i] == "push") {
            push = config[members[i]];
            push_members = push.getMemberNames();
            for (ii = 0; ii < (int)push_members.size(); ii++) {
                if (push_members[ii] == "dhcp_option") {
                    push_dhcp = push[push_members[ii]];
                    push_dhcp_members = push_dhcp.getMemberNames();
                    for (jj = 0; jj < (int)push_dhcp_members.size(); jj++) {
                        if (push_dhcp_members[jj] == "DNS") {
                            push_dhcp_dns = push_dhcp[push_dhcp_members[jj]];
                            if (!push_dhcp_dns.isArray()) {
                                printf("error format: push dhcp dns is not an array.\n");
                                return -1;
                            }
                            config_str += members[i] + " \"" + "dhcp-option " + push_dhcp_members[jj] + " " + push_dhcp_dns[0].asString() + "\"\n";
                            config_str += members[i] + " \"" + "dhcp-option " + push_dhcp_members[jj] + " " + push_dhcp_dns[1].asString() + "\"\n";
                        }
                    }
                } else if (push_members[ii] == "route") {
                    push_route = push[push_members[ii]];
                    if (!push_route.isArray()) {
                        printf("error format: push route is not an array.\n");
                        return -1;
                    }
                    for (jj = 0; jj < (int)push_route.size(); jj++) {
                        if (!push_route[jj].isMember("subnet") || !push_route[jj].isMember("netmask")) {
                            printf("error format: push route item.\n");
                            return -1;
                        }
                        config_str += members[i] + " \"" + push_members[ii] + " " + push_route[jj]["subnet"].asString() + " " + push_route[jj]["netmask"].asString() + "\"\n";
                    }
                }
            }
        } else if (members[i] == "additional") {
            additional = config[members[i]];
            if (!additional.isArray()) {
                printf("error format: additional is not an array.\n");
                return -1;
            }
            for (ii = 0; ii < (int)additional.size(); ii++)
                config_str += additional[ii].asString() + "\n";
        }
    }

    ofs.open(filename.c_str(), ofstream::out);
    ofs << config_str;
    ofs.close();

    return 0;
}

static int reload_config_to_json(const string filename, Json::Value &config)
{
    Json::FastWriter fastwriter;
    Json::StyledWriter styledwriter;
    Json::Value value;
    struct stat st;
    FILE *fp = NULL;
    char word[512], *line = NULL, *line_end, *begin;
    int ret;
    size_t line_len;
    ssize_t n_read;

    if (stat(filename.c_str(), &st) < 0) {
        printf("stat file: %s failed.\n", filename.c_str());
        return -1;
    }
    last_mod_time = st.st_mtim.tv_sec;

    fp = fopen(filename.c_str(), "r");
    if (!fp) {
        fprintf(stderr, "open %s failed, %s\n", filename.c_str(), strerror(errno));
        return -1;
    }

    while ((n_read = getline(&line, &line_len, fp)) != -1) {
        if ((line[0] == '#') || (line[0] == '\n') || line[0] == '\r' || (line[0] == ';'))
            continue;
        line_end = line + n_read - 1;
        begin = get_next_word(line, line_end, word, 512);
        if (!strncmp(word, "management", 10)) {
            begin = get_next_word(begin, line_end, word, 512);
            if (!begin) {
                printf("get management address failed.\n");
                ret = -1;
                goto out;
            }
            config["management"]["address"] = word;
            get_next_word(begin, line_end, word, 512);
            config["management"]["port"] = word;
        } else if (!strncmp(word, "daemon", 6)) {
            config["cmd"].append(word);
        } else if (!strncmp(word, "askpass", 7)) {
            config["cmd"].append(word);
        } else if (!strncmp(word, "port", 4)) {
            get_next_word(begin, line_end, word, 512);
            config["port"] = word;
        } else if (!strncmp(word, "proto", 5)) {
            get_next_word(begin, line_end, word, 512);
            config["proto"] = word;
        } else if (!strncmp(word, "dev", 3)) {
            get_next_word(begin, line_end, word, 512);
            config["dev"] = word;
        } else if (!strncmp(word, "ca", 2)) {
            get_next_word(begin, line_end, word, 512);
            config["ca"] = word;
        } else if (!strncmp(word, "cert", 4)) {
            get_next_word(begin, line_end, word, 512);
            config["cert"] = word;
        } else if (!strncmp(word, "key", 3)) {
            get_next_word(begin, line_end, word, 512);
            config["key"] = word;
        } else if (!strncmp(word, "dh", 2)) {
            get_next_word(begin, line_end, word, 512);
            config["dh"] = word;
        } else if (!strncmp(word, "server", 6)) {
            begin = get_next_word(begin, line_end, word, 512);
            config["server"]["subnet"] = word;
            if (!begin) {
                printf("error format: server\n");
                ret = -1;
                goto out;
            }
            get_next_word(begin, line_end, word, 512);
            config["server"]["netmask"] = word;
        } else if (!strncmp(word, "ifconfig-pool-persist", 21)) {
            get_next_word(begin, line_end, word, 512);
            config["ifconfig_pool_persist"] = word;
        } else if (!strncmp(word, "push", 4)) {
            if (handle_push(config, begin, line_end, word, 512) < 0) {
                ret = -1;
                goto out;
            }
        } else if (!strncmp(word, "client-config-dir", 17)) {
            get_next_word(begin, line_end, word, 512);
            config["client_config_dir"] = word;
        } else if (!strncmp(word, "route", 5)) {
            value.clear();
            begin = get_next_word(begin, line_end, word, 512);
            value["subnet"] = word;
            if (!begin) {
                printf("error format: route\n");
                ret = -1;
                goto out;
            }
            get_next_word(begin, line_end, word, 512);
            value["netmask"] = word;
            config["route"].append(value);
        } else if (!strncmp(word, "client-to-client", 16)) {
            config["cmd"].append("client_to_client");
        } else if (!strncmp(word, "keepalive", 9)) {
            begin = get_next_word(begin, line_end, word, 512);
            config["keepalive"]["interval"] = word;
            if (!begin) {
                printf("error format: keepalive\n");
                ret = 1;
                goto out;
            }
            get_next_word(begin, line_end, word, 512);
            config["keepalive"]["timeout"] = word;
        } else if (!strncmp(word, "cipher", 6)) {
            get_next_word(begin, line_end, word, 512);
            config["cipher"] = word;
        } else if (!strncmp(word, "persist-key", 11)) {
            config["cmd"].append("persist_key");
        } else if (!strncmp(word, "persist-tun", 11)) {
            config["cmd"].append("persist_tun");
        } else if (!strncmp(word, "status", 6)) {
            get_next_word(begin, line_end, word, 512);
            config["status"] = word;
        } else if (!strncmp(word, "log-append", 10)) {
            get_next_word(begin, line_end, word, 512);
            config["log_append"] = word;
        } else if (!strncmp(word, "verb", 4)) {
            get_next_word(begin, line_end, word, 512);
            config["verb"] = word;
        } else {
            printf("unknow directive: %s\n", word);
            ret = -1;
            goto out;
        }
    }

    ret = 0;
out:
    if (line)
        free(line);

    fclose(fp);

    config_string = fastwriter.write(config);
    //printf("%s", styledwriter.write(config).c_str());

    if (ret < 0)
        config.clear();

    return ret;
}

static int make_http_response_header(string &response_header, size_t response_len)
{
    string status, server, date, content_type, content_length, connection;
    string last_modified, etag, accept_ranges;
    struct tm t;
    time_t now;
    char time_buf[40];

    status = "HTTP/1.1 200 OK\n";
    server = "Server: blue/1.0.0\n";

    now = time(NULL);
    if (!gmtime_r(&now, &t))
        return -1;

    memset(time_buf, 0, 40);
    sprintf(time_buf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
            week[t.tm_wday], t.tm_mday, months[t.tm_mon],
            t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec);
    date = "Date: " + string(time_buf) + "\n";

    content_type = "Content-Type: application/json\n";
    content_length = "Content-Length: " + to_string(response_len) + "\n";

    if (!gmtime_r(&last_mod_time, &t))
        return -1;

    memset(time_buf, 0, 40);
    sprintf(time_buf, "%s, %02d %s %4d %02d:%02d:%02d GMT",
            week[t.tm_wday], t.tm_mday, months[t.tm_mon],
            t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec);
    last_modified = "Last-Modified: " + string(time_buf) + "\n";

    connection = "Connection: keep-alive\n";

    response_header = status + server + date + content_type + content_length + last_modified + connection + "\n";

    return 0;
}

static void read_cb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    string response_header = "";
    char *buf = NULL, *p = NULL;
    int offset = 0;
    size_t len;

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

#if 0
    unsigned int i;
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
#endif
    //fprintf(stdout, "%s\n", buf);

    free(buf);

    if (make_http_response_header(response_header, config_string.size()) < 0) {
        printf("make http response header failed.\n");
        // TODO: write failed http response.
        return;
    }

    bufferevent_write(bev, response_header.c_str(), response_header.size());
    bufferevent_write(bev, config_string.c_str(), config_string.size());
    bufferevent_flush(bev, EV_WRITE, BEV_FLUSH);
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

static int get_active_address(char *nic_name, char *buf, size_t len)
{
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *iflist = NULL;
    char *tmp = NULL;

    if (len <= 0) {
        fprintf(stderr, "%s: Invalid parameter\n", __func__);
        return -1;
    }

    strncpy(buf, "127.0.0.1", 9);
    return 0;
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

    while ((opt = getopt(argc, argv, "i:p:c:")) != -1) {
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
        case 'c':
            config_file = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-i nic_name] [-p port] [-c config_file]", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (config_file == "") {
        printf("specify config file use -c.\n");
        return 1;
    }

    if (reload_config_to_json(config_file, g_config) < 0) {
        printf("reload config to json faile.\n");
        return 1;
    }

    /*
    if (write_json_to_file(g_config, config_file + "_out") < 0) {
        printf("write json failed.\n");
        return 1;
    }
    */

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
