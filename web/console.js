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
        },
        buttonclick: function () {
            this.$Message('button-click');
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
    var lala = '{' +
        '"item1":"data1",' +
        '"item2":"data2",' +
        '"data1":[1,2,3],' +
        '"data2":[4,5,6]' +
    '}';
    $(".menu").toggle();
    main.$refs.ca_path.currentValue = "nihao";
    $.post("d", lala, function(data,status) {
        lala = $.parseJSON("" + data);
        main.$Message("" + lala.keepalive);
    });
});
//$(".content").find(".content-left").css({"color":"red","border":"2px solid red"});
});
