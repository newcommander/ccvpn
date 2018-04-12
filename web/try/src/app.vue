<template>
    <div>
        <router-view></router-view>
		<Input v-model="value2" placeholder="Enter something..." style="width: 300px"></Input>
<Row type="flex" justify="start" align="top">
	<Col span="6">
		<div class="sidebar">
			<div class="menu">
				<div class="index">
					<h1>
						<img src="https://raw.githubusercontent.com/iview/iview/master/assets/logo.png">
					</h1>
					<h2>
						<p>Welcome to your iView app!</p>
						<Button type="ghost" @click="handleStart">Start iView</Button>
					</h2>
				</div>
			</div>
		</div>
	</Col>
	<Col span=18>
		<div class="content">
			<div class="content-title">Server Configuration</div>
			<div class="content-base">
				<div class="content-buttons" align="right">
					<Button type="primary" size="large" @click="reload_click">Reload</Button>
					<Button type="primary" size="large" @click="reset_click">Reset</Button>
					<Button type="primary" size="large" @click="save_click">Save</Button>
				</div>
				<div class="content-container">
					<div class="content-l">
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
									<h4>Deamon mode</h4>
								</div>
								<div class="content-item-switch">
									<Switch v-model="deamon_mode" size="large">
										<span slot="open">ON</span>
										<span slot="close">OFF</span>
									</Switch>
								</div>
							</div>
						</div>
					</div>
					<div class="content-r">
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
									<h4>Ask password</h4>
								</div>
								<div class="content-item-switch">
									<Switch v-model="ask_passwd" size="large">
										<span slot="open">ON</span>
										<span slot="close">OFF</span>
									</Switch>
								</div>
							</div>
						</div>
					</div>
				</div>
				<InputNumber :max="10" :min="1" v-model="value1"></InputNumber>
			</div>
		</div>
	</Col>
</Row>
    </div>
</template>
<script>
    export default {
        data() {
            return {
				value1: 2
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
