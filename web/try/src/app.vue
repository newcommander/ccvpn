<template>
    <div>
        <router-view></router-view>
    </div>
</template>
<script>
    export default {
        data() {
            return {

            };
        },
        mounted() {
			this.$Message.info('lala haha');
			$.post('/w/d', '{"op":"reload"}', this.update_config);
        },
        beforeDestroy() {

        },
        methods: {
			update_config: function (data, status) {
				this.$Message.info('' + JSON.stringify(data));
				this.value1 = 3;
			}
        }
    };
</script>
