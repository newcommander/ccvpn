$(document).ready(function()
{
var tmp = 'tmp';
var header = new Vue({
    el: '#header',
    methods: {
        showMessage: function () {
            this.$Message('message from header')
        }
    }
});
var main = new Vue({
    el: '#main',
    methods: {
        showMessage: function () {
            this.$Message('message from main');
        },
        change_ca_path: function (e) {
            this.$Message('Change CA path: ' + e);
            tmp = e;
        }
    }
});
var footer = new Vue({
    el: '#footer',
    methods: {
        showMessage: function () {
            this.$Message('message from footer')
        }
    }
});
$("#debug-button").click(function () {
    $(".menu").toggle();
    main.$Message("" + main.$refs.deamon_mode.checkStatus + ", " + main.$refs.ca_path.currentValue + ', ' + tmp);
    main.$refs.ca_path.currentValue = "nihao";
    tmp = main.$refs.ca_path.currentValue;
    $.post("d", "{json_data}");
});
//$(".content").find(".content-left").css({"color":"red","border":"2px solid red"});
});
