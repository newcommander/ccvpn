#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>

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
#define CONFIG_BUF_LEN 4096

using namespace std;

static char active_addr[128];
static char config_json[CONFIG_BUF_LEN * 2];

static vector<char*> direct_cmd;
static vector<char*>::iterator cmd_it;
static map<char*, char*> direct_opt;
static map<char*, char*>::iterator opt_it;
static Json::Value g_config;

/*
static int config_to_jsoncpp(char *content)
{
    Json::CharReaderBuilder builder;
    Json::CharReader *reader(builder.newCharReader());
    string errs;

    if (!content) {
        fprintf(stderr, "content is NULL\n");
        return -1;
    }

    if (strlen(content) == 0)
        return -1;

    if (!reader->parse(content, content + strlen(content), &config, &errs)) {
        printf("parse json failed: %s\n", errs.c_str());
        return -1;
    }

    return 0;
}
*/

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

static int handle_push(char *line, char *line_end, char *word_buf, int max_buf_len)
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
        g_config["push"]["route"].append(value);
    } else if (!strncmp(word_buf, "dhcp-option", 11)) {
        begin = get_next_word(begin, line_end, word_buf, max_buf_len);
        if (!strncmp(word_buf, "DNS", 3)) {
            if (!begin) {
                printf("error format: push dhcp-option DNS\n");
                return -1;
            }
            if (g_config["push"]["dhcp-option"]["DNS"].size() >= 2) {
                printf("push too many DNS server\n");
                return -1;
            }
            get_next_word(begin, line_end, word_buf, max_buf_len);
            g_config["push"]["dhcp-option"]["DNS"].append(word_buf);
        }
    }

    return 0;
}

static int write_config_file(const char *filename)
{
    Json::Value::Members members, push_members, push_dhcp_members;
    Json::Value cmd, route, push, push_dhcp, push_dhcp_dns, push_route;
    string config_str = "";
    size_t i;
    int ii, jj;

    members = g_config.getMemberNames();
    for (i = 0; i < members.size(); i++) {
        if (members[i] == "ca" ||
            members[i] == "cert" ||
            members[i] == "key" ||
            members[i] == "cipher" ||
            members[i] == "client-config-dir" ||
            members[i] == "dev" ||
            members[i] == "dh" ||
            members[i] == "ifconfig-pool-persist" ||
            members[i] == "log-append" ||
            members[i] == "port" ||
            members[i] == "proto" ||
            members[i] == "status" ||
            members[i] == "verb") {

            config_str += members[i] + " " + g_config[members[i]].asString() + "\n";
        } else if (members[i] == "cmd") {
            cmd = g_config[members[i]];
            if (!cmd.isArray()) {
                printf("error format: cmd not array.\n");
                return -1;
            }
            for (ii = 0; ii < (int)cmd.size(); ii++)
                config_str += cmd[ii].asString() + "\n";
        } else if (members[i] == "keepalive") {
            if (!g_config[members[i]].isMember("interval") || !g_config[members[i]].isMember("timeout")) {
                printf("error format: keepalive\n");
                return -1;
            }
            config_str += members[i] + " " + g_config[members[i]]["interval"].asString() + " " + g_config[members[i]]["timeout"].asString() + "\n";
        } else if (members[i] == "management") {
            if (!g_config[members[i]].isMember("address") || !g_config[members[i]].isMember("port")) {
                printf("error format: management\n");
                return -1;
            }
            config_str += members[i] + " " + g_config[members[i]]["address"].asString() + " " + g_config[members[i]]["port"].asString() + "\n";
        } else if (members[i] == "server") {
            if (!g_config[members[i]].isMember("subnet") || !g_config[members[i]].isMember("netmask")) {
                printf("error format: server\n");
                return -1;
            }
            config_str += members[i] + " " + g_config[members[i]]["subnet"].asString() + " " + g_config[members[i]]["netmask"].asString() + "\n";
        } else if (members[i] == "route") {
            route = g_config[members[i]];
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
            push = g_config[members[i]];
            push_members = push.getMemberNames();
            for (ii = 0; ii < (int)push_members.size(); ii++) {
                if (push_members[ii] == "dhcp-option") {
                    push_dhcp = push[push_members[ii]];
                    push_dhcp_members = push_dhcp.getMemberNames();
                    for (jj = 0; jj < (int)push_dhcp_members.size(); jj++) {
                        if (push_dhcp_members[jj] == "DNS") {
                            push_dhcp_dns = push_dhcp[push_dhcp_members[jj]];
                            if (!push_dhcp_dns.isArray()) {
                                printf("error format: push dhcp dns is not an array.\n");
                                return -1;
                            }
                            config_str += members[i] + " \"" + push_members[ii] + " " + push_dhcp_members[jj] + " " + push_dhcp_dns[0].asString() + "\"\n";
                            config_str += members[i] + " \"" + push_members[ii] + " " + push_dhcp_members[jj] + " " + push_dhcp_dns[1].asString() + "\"\n";
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
        }
    }
    printf("%s", config_str.c_str());

    return 0;
}

static int read_config_file(const char *filename)
{
    Json::StyledWriter styledwriter;
    Json::Value value;
    FILE *fp = NULL;
    char word[512], *line = NULL, *line_end, *begin;
    int ret;
    size_t line_len;
    ssize_t n_read;

    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "open %s failed, %s\n", filename, strerror(errno));
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
            g_config["management"]["address"] = word;
            get_next_word(begin, line_end, word, 512);
            g_config["management"]["port"] = word;
        } else if (!strncmp(word, "daemon", 6)) {
            g_config["cmd"].append(word);
        } else if (!strncmp(word, "askpass", 7)) {
            g_config["cmd"].append(word);
        } else if (!strncmp(word, "port", 4)) {
            get_next_word(begin, line_end, word, 512);
            g_config["port"] = word;
        } else if (!strncmp(word, "proto", 5)) {
            get_next_word(begin, line_end, word, 512);
            g_config["proto"] = word;
        } else if (!strncmp(word, "dev", 3)) {
            get_next_word(begin, line_end, word, 512);
            g_config["dev"] = word;
        } else if (!strncmp(word, "ca", 2)) {
            get_next_word(begin, line_end, word, 512);
            g_config["ca"] = word;
        } else if (!strncmp(word, "cert", 4)) {
            get_next_word(begin, line_end, word, 512);
            g_config["cert"] = word;
        } else if (!strncmp(word, "key", 3)) {
            get_next_word(begin, line_end, word, 512);
            g_config["key"] = word;
        } else if (!strncmp(word, "dh", 2)) {
            get_next_word(begin, line_end, word, 512);
            g_config["dh"] = word;
        } else if (!strncmp(word, "server", 6)) {
            begin = get_next_word(begin, line_end, word, 512);
            g_config["server"]["subnet"] = word;
            if (!begin) {
                printf("error format: server\n");
                ret = 1;
                goto out;
            }
            get_next_word(begin, line_end, word, 512);
            g_config["server"]["netmask"] = word;
        } else if (!strncmp(word, "ifconfig-pool-persist", 21)) {
            get_next_word(begin, line_end, word, 512);
            g_config["ifconfig-pool-persist"] = word;
        } else if (!strncmp(word, "push", 4)) {
            if (handle_push(begin, line_end, word, 512) < 0) {
                ret = 1;
                goto out;
            }
        } else if (!strncmp(word, "client-config-dir", 17)) {
            get_next_word(begin, line_end, word, 512);
            g_config["client-config-dir"] = word;
        } else if (!strncmp(word, "route", 5)) {
            value.clear();
            begin = get_next_word(begin, line_end, word, 512);
            value["subnet"] = word;
            if (!begin) {
                printf("error format: route\n");
                ret = 1;
                goto out;
            }
            get_next_word(begin, line_end, word, 512);
            value["netmask"] = word;
            g_config["route"].append(value);
            // TODO
        } else if (!strncmp(word, "client-to-client", 16)) {
            g_config["cmd"].append(word);
        } else if (!strncmp(word, "keepalive", 9)) {
            begin = get_next_word(begin, line_end, word, 512);
            g_config["keepalive"]["interval"] = word;
            if (!begin) {
                printf("error format: keepalive\n");
                ret = 1;
                goto out;
            }
            get_next_word(begin, line_end, word, 512);
            g_config["keepalive"]["timeout"] = word;
        } else if (!strncmp(word, "cipher", 6)) {
            get_next_word(begin, line_end, word, 512);
            g_config["cipher"] = word;
        } else if (!strncmp(word, "persist-key", 11)) {
            g_config["cmd"].append(word);
        } else if (!strncmp(word, "persist-tun", 11)) {
            g_config["cmd"].append(word);
        } else if (!strncmp(word, "status", 6)) {
            get_next_word(begin, line_end, word, 512);
            g_config["status"] = word;
        } else if (!strncmp(word, "log-append", 10)) {
            get_next_word(begin, line_end, word, 512);
            g_config["log-append"] = word;
        } else if (!strncmp(word, "verb", 4)) {
            get_next_word(begin, line_end, word, 512);
            g_config["verb"] = word;
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

    printf("%s", styledwriter.write(g_config).c_str());
    write_config_file("json_output");

    if (ret)
        g_config.clear();

    return ret;
}

#if 0
static void load_config_to_json(char *filename, Json::Value &config)
{
    Json::CharReaderBuilder builder;
    Json::CharReader *reader(builder.newCharReader());
    Json::Value local_config;
    Json::StyledWriter writer;
    string errs;
    struct stat st;
    char *content = NULL;
    char *p1, *p2, *p3, *p4, *current, *opt, *arg, *head, *tail;
    char tmp1[512], tmp2[512];
    size_t filesize;

    config.clear();
    local_config.clear();

    if (!filename) {
        fprintf(stderr, "filename is NULL.\n");
        return;
    }

    if (stat(filename, &st) < 0) {
        printf("stat file %s failed.\n", filename);
        return;
    }

    filesize = st.st_size;
    if (filesize == 0) {
        printf("empty config file: %s.\n", filename);
        return;
    }

    content = (char*)calloc(filesize + 1, 1);
    if (!content) {
        printf("alloc content buffer failed.\n");
        return;
    }

//    if (read_config_file(filename, content) < 0)
//        goto out;

    if (strlen(content) == 0) {
        printf("content length is 0.\n");
        goto out;
    }

    p1 = content;
    while (1) {
        p2 = index(p1, ' ');
        p3 = index(p1, '#');
        while (isblank(*p2) || *p2 == '\n')
            p2++;
        p4 = index(p1, '\n');
        if (p2 && p4) {
            if (p2 > p4) {
                *p4 = '\0';
                local_config["cmd"].append((string)p1);
            } else {
            }
        } else if (!p2 && !p4) {
            local_config["cmd"].append((string)p1);
            break;
        } else if (p2) {
        } else if (p4) {
            *p4 = '\0';
            local_config["cmd"].append((string)p1);
        }
        if (!p2 || (p2 > p4)) {
            *p4 = '\0';
            local_config["cmd"].append((string)p1);
        } else {
        }
        p1 = p4 + 1;
        while (isblank(*p1))
            p1++;
        if ((p1 - content + 1) > strlen(content))
            break;
    }
    config.copy(local_config);
    printf("%s", writer.write(g_config).c_str());
/*
    if (!reader->parse(content, content + strlen(content), &config, &errs)) {
        printf("parse json failed: %s\n", errs.c_str());
        config.clear();
        goto out;
    }

    p1 = content;
    current = directive_holder;
    while (1) {
        p2 = index(p1, ' ');
        p3 = index(p1, '\n');
        if (!p3)
            break;
        if ((!p2) || (p2 > p3)) {
            head = p1;
            tail = p3 - 1;
            while ((*head) == '\"') head++;
            while ((*tail) == '\"') tail--;
            snprintf(tmp1, tail - head + 2, "%s", head);
            strncpy(current, tmp1, p3 - p1 + 1);
            direct_cmd.push_back(current);
            current += (p3 - p1 + 1);
        } else {
            snprintf(tmp1, p2 - p1 + 1, "%s", p1);
            strncpy(current, tmp1, p2 - p1 + 1);
            opt = current;
            current += (p2 - p1 + 1);

            head = p2 + 1;
            tail = p3 - 1;
            while ((*head) == '\"') head++;
            while ((*tail) == '\"') tail--;
            snprintf(tmp2, tail - head + 2, "%s", head);
            strncpy(current, tmp2, tail - head + 2);
            arg = current;
            current += (tail - head + 2);

            direct_opt.insert(std::pair<char*, char*>(opt, arg));
        }
        p1 = p3 + 1;
    }

    memset(config_json, 0, CONFIG_BUF_LEN * 2);
    current = config_json;
    current[0] = '{';
    current++;

    if (!direct_cmd.empty()) {
        strncpy(current, "\"cmd\":[", 7);
        current += 7;
        for (cmd_it = direct_cmd.begin(); cmd_it != direct_cmd.end(); cmd_it++) {
            *(current++) = '\"';
            strncpy(current, *cmd_it, strlen(*cmd_it));
            current += strlen(*cmd_it);
            *(current++) = '\"';
            *(current++) = ',';
        }
        current[-1] = ']';
        *(current++) = ',';
    }

    if (!direct_opt.empty()) {
        for (opt_it = direct_opt.begin(); opt_it != direct_opt.end(); opt_it++) {
            *(current++) = '\"';
            strncpy(current, opt_it->first, strlen(opt_it->first));
            current += strlen(opt_it->first);
            *(current++) = '\"';
            *(current++) = ':';
            *(current++) = '\"';
            strncpy(current, opt_it->second, strlen(opt_it->second));
            current += strlen(opt_it->second);
            *(current++) = '\"';
            *(current++) = ',';
        }
    }

    if (current[-1] == ',')
        current[-1] = '}';
    else
        current[0] = '}';
*/
out:
    if (content)
        free(content);
}
#endif

static void read_cb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    const char *response = "HTTP/1.1 200 OK\nServer: nginx/1.12.2\nDate: Mon, 19 Feb 2018 06:51:54 GMT\nContent-Type: text/html\nContent-Length: 752\nLast-Modified: Tue, 02 Jan 2018 14:19:42 GMT\nConnection: keep-alive\nETag: \"5a4b94fe-264\"\nAccept-Ranges: bytes\n\n";
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
    fprintf(stdout, "%s\n", buf);

    //bufferevent_write(bev, buf, len);
    bufferevent_write(bev, response, strlen(response));
    bufferevent_write(bev, config_json, strlen(config_json));
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
    char *nic_name = NULL, *filename = NULL;
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
            filename = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-i nic_name]", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!filename) {
        printf("specify config filename use -c.\n");
        return 1;
    }

    read_config_file(filename);
    /*
    load_config_to_json(filename, g_config);
    if (g_config.empty()) {
        printf("Convert config to json format failed.\n");
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
