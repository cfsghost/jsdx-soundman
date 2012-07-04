var _soundman = require('../build/Release/jsdx_soundman');

var Soundman = module.exports = function() {};

Soundman.prototype.init = function(callback) {
	_soundman.PulseAudioInit.apply(this, [ callback ]);
};

Soundman.prototype.uninit = function() {
	_soundman.PulseAudioUninit.apply(this, []);
};

Soundman.prototype.setVolume = function(value) {
	return _soundman.setVolume.apply(this, [ value ]);
};

Soundman.prototype.getVolume = function() {
	return _soundman.getVolume();
};
