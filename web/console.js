$(document).ready(function()
{
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
$("#debug-button").click(function()
    {
        $(".menu").toggle();
    });
});
