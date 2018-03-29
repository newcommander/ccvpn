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
        num: 1,
        route_table_title: [
            { title: 'subnet', key: 'subnet' },
            { title: 'netmask', key: 'netmask' }
        ],
        route_table_items: [
			{ "netmask" : "255.255.255.1",	"subnet" : "192.168.1.2" },
			{ "netmask" : "255.255.255.1",	"subnet" : "192.168.3.4" }
		],
        new_route_subnet: "pp",
        new_route_netmask: "",
        deamon_mode: false,
        client_to_client: false,
        ask_passwd: false,
        persist_key: false,
        persist_tun: false
    },
    methods: {
        iview_button_change (status) {
            this.$Message.info('开关状态：' + status);
        },
        showMessage: function () {
            this.$Message('message from main');
        },
        change_ca_path: function (e) {
            this.$Message('Change CA path: ' + this.num);
            this.num += 1;
        },
        new_route_click: function () {
            this.$Modal.prompt({
				title: 'New Route', content: 'subnet:'
			}).then((data) => {
				this.new_route_subnet = data.value;

				this.$Modal.prompt({
					title: 'New Route', content: 'netmask:'
				}).then((data1) => {
					this.new_route_netmask = data1.value;
					//this.route_table_items.push({ "subnet": this.new_route_subnet, "netmask": this.new_route_netmask });
					//this.$Message(this.new_route_subnet + " " + this.new_route_netmask);
				}).catch();
			}).catch();
        },
        del_route_click: function () {
        },
        update_config: function (data, status) {
            config = data;
            for (i in config.cmd) {
                if (config.cmd[i] == "daemon")
                    this.deamon_mode = true;
                else if (config.cmd[i] == "askpass")
                    this.ask_passwd = true;
                else if (config.cmd[i] == "client_to_client")
                    this.client_to_client = true;
                else if (config.cmd[i] == "persist_key")
                    this.persist_key = true;
                else if (config.cmd[i] == "persist_tun")
                    this.persist_tun = true;
            }
            this.$refs.port.currentValue = config.port;
            this.$refs.port.currentValue = 3726;
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
//            this.route_table_items = config.route;
//					this.route_table_items = [
//						{ "netmask" : "255.255.255.1",	"subnet" : "192.168.1.2" },
//						{ "netmask" : "255.255.255.1",	"subnet" : "192.168.3.4" }
//					];
        },
        reload_click: function () {
            var lala = '{"op":"reload"}';
            $.post('d', lala, this.update_config);
        },
        reset_click: function () {
            this.$Message.info('reset-click');
        },
        save_click: function () {
            this.$Message.info('save-click');
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
