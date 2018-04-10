const routers = [{
    path: '/',
    meta: {
        title: 'Console'
    },
    component: (resolve) => require(['./views/index.vue'], resolve)
}];
export default routers;
