var Soundman = require('../');

var soundman = new Soundman;

soundman.init(function(err) {
	if (err) {
		console.log(err);
		return;
	}
		
	console.log(soundman.isMuted());
	soundman.uninit();
});
