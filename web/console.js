$(document).ready(function()
{
var header = new Vue({
    el: '#header',
    methods: {
        showMessage: function () {
            this.$Message('message from header');
        }
    }
});
var main = new Vue({
    el: '#main',
    data: {
        config: "NULL",
        num: 1
    },
    methods: {
        showMessage: function () {
            this.$Message('message from main');
        },
        change_ca_path: function (e) {
            this.$Message('Change CA path: ' + this.num);
            this.num += 1;
        },
        update_config: function (data, status) {
            config = data;
            for (i in config.cmd) {
                if (config.cmd[i] == "daemon")
                    this.$refs.deamon_mode.checkStatus = true;
                else if (config.cmd[i] == "askpass")
                    this.$refs.ask_passwd.checkStatus = true;
                else if (config.cmd[i] == "client_to_client")
                    this.$refs.client_to_client.checkStatus = true;
                else if (config.cmd[i] == "persist_key")
                    this.$refs.persist_key.checkStatus = true;
                else if (config.cmd[i] == "persist_tun")
                    this.$refs.persist_tun.checkStatus = true;
            }
            this.$refs.port.currentValue = config.port;
            this.$refs.ca.currentValue = config.ca;
            this.$refs.server_cert.currentValue = config.cert;
            this.$refs.cipher.currentValue = config.cipher;
            this.$refs.client_config_dir.currentValue = config.client_config_dir;
            this.$refs.dev.currentValue = config.dev;
            this.$refs.dh.currentValue = config.dh;
            this.$refs.ifconfig_pool_persist.currentValue = config.ifconfig_pool_persist;
            this.$refs.keepalive_interval.currentValue = config.keepalive.interval;
            this.$refs.keepalive_timeout.currentValue = config.keepalive.timeout;
            this.$refs.key.currentValue = config.key;
            this.$refs.log_append.currentValue = config.log_append;
            this.$refs.management_address.currentValue = config.management.address;
            this.$refs.management_port.currentValue = config.management.port;
            this.$refs.proto.currentValue = config.proto;
            this.$refs.server_subnet.currentValue = config.server.subnet;
            this.$refs.server_netmask.currentValue = config.server.netmask;
            this.$refs.status.currentValue = config.status;
            this.$refs.verb.currentValue = config.verb;
        },
        reload_click: function () {
            var lala = '{"op":"reload"}';
            $.post('d', lala, this.update_config);
        },
        reset_click: function () {
            this.$Message('reset-click');
        },
        save_click: function () {
            this.$Message('save-click');
        }
    }
});
var footer = new Vue({
    el: '#footer',
    methods: {
        showMessage: function () {
            this.$Message('message from footer');
        }
    }
});
$.post('d', '{"op":"reload"}', main.update_config);
//$(".content").find(".content-left").css({"color":"red","border":"2px solid red"});
});
