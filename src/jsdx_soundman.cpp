#include <v8.h>
#include <node.h>
#include <pthread.h>
#include <ev.h>
#include <pulse/pulseaudio.h>
#include <list>
#include <string>

#include "jsdx_soundman.hpp"

namespace JSDXSoundman {
 
	using namespace node;
	using namespace v8;
	using namespace std;

	typedef enum state {
		CONNECTING,
		CONNECTED,
		ERROR
	} PulseAudioState;

	pa_mainloop *mainloop;
	pa_mainloop_api *mainloop_api;
	pa_context *context;
	PulseAudioState state;

	void _PulseAudioStateCallback(pa_context *context, void *data)
	{
		switch(pa_context_get_state(context)) {
		case PA_CONTEXT_READY:
			state = CONNECTED;
			break;
		case PA_CONTEXT_FAILED:
			state = ERROR;
			break;
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_TERMINATED:
			break;
		}
	}

	void _PulseAudioInit(uv_work_t *req)
	{
		int ret;

		mainloop = pa_mainloop_new();
		mainloop_api = pa_mainloop_get_api(mainloop);
		context = pa_context_new(mainloop_api, "Sound Manager");
		pa_context_set_state_callback(context, &_PulseAudioStateCallback, NULL);

		/* Connect to PulseAudio server */
		pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);

		state = CONNECTING;
		while (state == CONNECTING) {
			pa_mainloop_iterate(mainloop, 1, &ret);
		}
	}

	void _PulseAudioInitCompleted(uv_work_t *req)
	{
		HandleScope scope;

		NodeCallback *callback = (NodeCallback *)req->data;

		TryCatch try_catch;
		if (state == ERROR) {
			/* Prepare arguments */
			Local<Value> argv[1] = {
				Local<Value>::New(Exception::Error(String::New("Failed to connect")))
			};

			callback->cb->Call(callback->Holder, 1, argv);
		} else {
			/* Prepare arguments */
			Local<Value> argv[1] = {
				Local<Value>::New(Null())
			};

			callback->cb->Call(callback->Holder, 1, argv);
		}

		delete callback;
		delete req;

		if (try_catch.HasCaught())
			FatalException(try_catch);
	}

	Handle<Value> PulseAudioInit(const Arguments& args)
	{
		HandleScope scope;

		if (!args[0]->IsFunction())
			return Undefined();

		/* Process callback function */
		NodeCallback *callback = new NodeCallback();
		callback->Holder = Persistent<Object>::New(args.Holder());
		callback->cb = Persistent<Function>::New(Local<Function>::Cast(args[0]));

		/* Prepare structure for PulseAudio thread */
		uv_work_t *req = new uv_work_t;
		req->data = callback;

		uv_queue_work(uv_default_loop(), req, _PulseAudioInit, _PulseAudioInitCompleted);

		uv_run(uv_default_loop());

		return Undefined();
	}

	void _GetPulseAudioSinkName_cb(pa_context *context, const pa_server_info *info, void *data)
	{
		std::string *default_sink_name = (std::string*) data;

		*default_sink_name = info->default_sink_name;
	}

	void _SinkListCallback(pa_context *c, const pa_sink_info *sink, int eol, void *data)
	{
		if (eol != 0)
			return;

		std::list<pa_sink_info *> *sinks = (std::list<pa_sink_info *> *) data;

		sinks->push_back((pa_sink_info *)sink);
	}

	pa_sink_info *_GetPulseAudioSink(std::string sink_name)
	{
		int ret;
		std::list<pa_sink_info *> sinks;

		pa_operation* op = pa_context_get_sink_info_by_name(context, sink_name.c_str(), &_SinkListCallback, &sinks);

		while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
			pa_mainloop_iterate(mainloop, 1, &ret);
		}

		pa_operation_unref(op);

		if (sinks.empty())
			return NULL;

		return *(sinks.begin());
	}

	pa_sink_info *_GetPulseAudioDefaultSink()
	{
		int ret;
		std::string sink_name;

		pa_operation* op = pa_context_get_server_info(context, &_GetPulseAudioSinkName_cb, &sink_name);

		while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
			pa_mainloop_iterate(mainloop, 1, &ret);
		}

		pa_operation_unref(op);

		return _GetPulseAudioSink(sink_name);
	}

	Handle<Value> GetVolume(const Arguments& args)
	{
		HandleScope scope;

		/* Get default sink */
		pa_sink_info *sink = _GetPulseAudioDefaultSink();
		if (sink == NULL)
			return scope.Close(Integer::New(-1));

		/* Figure percentage of volume */
		return scope.Close(Integer::New((int)((pa_cvolume_avg(&(sink->volume)) * 100) / PA_VOLUME_NORM)));
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "PulseAudioInit", PulseAudioInit);
		NODE_SET_METHOD(target, "getVolume", GetVolume);
	}

	NODE_MODULE(jsdx_soundman, init);
}
