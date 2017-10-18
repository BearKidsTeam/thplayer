const app1 = new Moon({
	el: "#app1",
	data: {
		osname: "windows"
	},
	methods: {
		setos: function(os) {
			this.set('osname', os);
		}
	}
});
// osname can be: windows, mac, linux
function getOS() {
	var userAgent = window.navigator.userAgent,
		platform = window.navigator.platform,
		macosPlatforms = ['Macintosh', 'MacIntel', 'MacPPC', 'Mac68K'],
		windowsPlatforms = ['Win32', 'Win64', 'Windows', 'WinCE'],
//		iosPlatforms = ['iPhone', 'iPad', 'iPod'],
		os = 'linux'; // default

	if (macosPlatforms.indexOf(platform) !== -1) {
		os = 'mac';
//	} else if (iosPlatforms.indexOf(platform) !== -1) {
//		os = 'iOS';
	} else if (windowsPlatforms.indexOf(platform) !== -1) {
		os = 'windows';
//	} else if (/Android/.test(userAgent)) {
//		os = 'Android';
	} else if (!os && /Linux/.test(platform)) {
		os = 'linux';
	}

	return os;
}
app1.set('osname', getOS());