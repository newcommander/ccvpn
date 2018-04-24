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
					<Button type="primary" size="large" @click="fake_load_click">Fake load</Button>
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
									<Switch v-model="daemon_mode" size="large">
										<span slot="open">ON</span>
										<span slot="close">OFF</span>
									</Switch>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Client to Client</h4>
								</div>
								<div class="content-item-switch">
									<Switch v-model="client_to_client" size="large">
										<span slot="open">ON</span>
										<span slot="close">OFF</span>
									</Switch>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Port</h4>
								</div>
								<div class="content-item-text">
                                    <InputNumber :min="1024" :max="65535" :value="1024" v-model="port"></InputNumber>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>CA</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="ca" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Server Cert</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="server_cert" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Cipher</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="cipher" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Client configuration directroy</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="ccd" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Device type</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="dev_type" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Diffie Hellman parameters</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="dh_file" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>ifconfig-pool-persist</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="ipp" placeholder="ifconfig-pool-persist"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Keep alive</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="keepalive_interval" placeholder="none"></Input>
                                    <Input v-model="keepalive_timeout" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Key</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="key" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Log append</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="log_append" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Management</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="mng_addr" placeholder="none"></Input>
                                    <Input v-model="mng_port" placeholder="none"></Input>
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
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
									<h4>Persist key</h4>
								</div>
								<div class="content-item-switch">
									<Switch v-model="persist_key" size="large">
										<span slot="open">ON</span>
										<span slot="close">OFF</span>
									</Switch>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
									<h4>Persist tun</h4>
								</div>
								<div class="content-item-switch">
									<Switch v-model="persist_tun" size="large">
										<span slot="open">ON</span>
										<span slot="close">OFF</span>
									</Switch>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Protocol</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="protocol" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Push</h4>
								</div>
								<div class="content-item-text">
                                    ......
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Route</h4>
								</div>
								<div class="content-item-text">
                                    <Table border :columns="route_table_title" :data="route_table_items" no-data-text="No Data" no-filtered-data-text="No Filtered Data"></Table>
                                    <Table border :columns="new_route_table_title" :data="new_route_table_items" no-data-text="No Data" :show-header=false></Table>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Server</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="server_subnet" placeholder="none"></Input>
                                    <Input v-model="server_netmask" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Status</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="status" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Verbosity</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="verb" placeholder="none"></Input>
								</div>
							</div>
						</div>
						<div class="content-item">
							<div class="content-item-base">
								<div class="content-item-title">
                                    <h4>Additional</h4>
								</div>
								<div class="content-item-text">
                                    <Input v-model="additional" placeholder="none"></Input>
								</div>
							</div>
						</div>
					</div>
				</div>
			</div>
		</div>
	</Col>
</Row>
</template>
<script>
    export default {
        data () {
            return {
                config: {},
                daemon_mode: false,
                ask_passwd: false,
                route_table_title: [
                    { title: 'Subnet', key: 'subnet' },
                    { title: 'Netmask', key: 'netmask' },
                    {
                        title: ' ',
                        key: 'action',
                        width: 70,
                        align: 'center',
                        render: (h, params) => {
                            return h('div', [
                                h('Button', {
                                    props: {
                                        type: 'error',
                                        size: 'small'
                                    },
                                    on: {
                                        click: () => {
                                            main.remove_route_item(params.index);
                                        }
                                    }
                                }, 'Del')
                            ]);
                        }
                    }
                ],
                route_table_items: [
                    { "netmask" : "255.255.255.1",	"subnet" : "192.168.1.2" },
                    { "netmask" : "255.255.255.1",	"subnet" : "192.168.3.4" }
                ],
                new_route_subnet: "pp",
                new_route_netmask: "hh",
                new_route_table_title: [
                {
                    title: 'Subnet',
                    key: 'subnet',
                    render: (h, params) => {
                        return h('div', [
                            h('Input', {
                                props: {
                                    placeholder: 'subnet'
                                },
                                on: {
                                    input: (value) => {
                                        this.new_subnet = value;
                                    }
                                }
                            }, ' ')
                        ]);
                    }
                },
                {
                    title: 'Netmask',
                    key: 'netmask',
                    render: (h, params) => {
                        return h('div', [
                            h('Input', {
                                props: {
                                    placeholder: 'netmask'
                                },
                                on: {
                                    input: (value) => {
                                        this.new_netmask = value;
                                    }
                                }
                            }, ' ')
                        ]);
                    }
                },
                {
                    title: ' ',
                    key: 'action',
                    width: 70,
                    align: 'center',
                    render: (h, params) => {
                        return h('div', [
                            h('Button', {
                                props: {
                                    type: 'primary',
                                    size: 'small'
                                },
                                on: {
                                    click: () => {
                                        main.route_table_items.push({ "subnet" : this.new_subnet,  "netmask" : this.new_netmask });
                                        main.refresh_new_route_table();
                                    }
                                }
                            }, 'Add')
                        ]);
                    }
                }],
                new_route_table_items: [ {} ]
            }
        },
        methods: {
            handleStart() {
                this.$Modal.info({
                    title: 'Bravo',
                    content: 'Now, enjoy the convenience of iView.'
                });
            },
            fake_load_click() {
                $.post('/w/d', '{"op":"reload"}', this.update_config);
            },
            update_config(data, status) {
                var i;
                this.config = data;
                for (i in this.config.cmd) {
                    if (this.config.cmd[i] == "daemon")
                        this.daemon_mode = true;
                    else if (this.config.cmd[i] == "askpass")
                        this.ask_passwd = true;
                }
                this.$Message.info('' + JSON.stringify(this.config));
            }
        }
    };
</script>
