var _soundman = require('../build/Release/jsdx_soundman');

var Soundman = module.exports = function() {};

Soundman.prototype.init = function(callback) {
	_soundman.PulseAudioInit.apply(this, [ callback ]);
};

Soundman.prototype.setVolume = function() {
};

Soundman.prototype.getVolume = function() {
	return _soundman.getVolume();
};