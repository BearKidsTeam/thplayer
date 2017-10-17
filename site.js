const app1 = new Moon({
	el: "#app1",
	data: {
		msg: "Hello Moon!",
		osname: "windows",
		msg_map: {
			windows: "Windows supported!",
			mac: "osx is a bug",
			linux: "Linux fully powered!"
		}
	},
	methods: {
		setos: function(os) {
			this.set('osname', os);
			this.set('msg', this.get('msg_map')[os]);
		}
	}
});
app1.set('msg', "Changed Message!");