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
        config: "NULL"
    },
    methods: {
        showMessage: function () {
            this.$Message('message from main');
        },
        change_ca_path: function (e) {
            this.$Message('Change CA path: ' + e);
        },
        update_config: function (data, status) {
            config = $.parseJSON(data);
            this.$refs.ca_path.currentValue = config.ca;
            this.$refs.port.currentValue = config.port;
            this.$Message('' + config.cmd[0]);
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
