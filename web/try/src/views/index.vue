<style scoped>
    .index {
        width: 100%;
        text-align: center;
    }

    .index h1 {
        height: 150px;
    }

    .index h1 img {
        height: 100%;
    }

    .index h2 {
        color: #666;
        margin-bottom: 200px;
    }

    .index h2 p {
        margin: 0 0 50px;
    }

    .index .ivu-row-flex {
        height: 100%;
    }
</style>
<template>
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
</template>
<script>
    export default {
        data () {
            return {
                value1: 1
            }
        },
        methods: {
            handleStart() {
                this.$Message.info('' + this.value1);
                this.$Modal.info({
                    title: 'Bravo',
                    content: 'Now, enjoy the convenience of iView.'
                });
            }
        }
    };
</script>
