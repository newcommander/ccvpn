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
            this.$refs.deamon_mode.checkStatus = true;  //TODO
            this.$refs.ask_passwd.checkStatus = true;  //TODO
            this.$refs.client_2_client.checkStatus = true;  //TODO
            this.$refs.port.currentValue = config.port;
            this.$refs.ca_file.currentValue = config.ca;
            this.$refs.cert_file.currentValue = config.cert;
            this.$refs.key_file.currentValue = config.key;
            this.$refs.dh_file.currentValue = config.dh;
            this.$refs.vpn_subnet.currentValue = config.server;
            //this.$refs.ip_pool.currentValue = config.ifconfig-pool-persist;
            this.$refs.private_subnet.currentValue = config.route;
            this.$refs.status_file.currentValue = config.status;
            this.$refs.sys_log.currentValue = config.log-append;
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
