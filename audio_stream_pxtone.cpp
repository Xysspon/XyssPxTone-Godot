/*************************************************************************/
/*  audio_stream_pxtone.cpp                                              */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/* Copyright (c) 2022-2023 Laura K. (alula)                              */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "audio_stream_pxtone.h"

#include "core/io/file_access.h"

constexpr float i16_f32mul = 1.0f / 32768.0f;

int AudioStreamPlaybackPxTone::_mix_internal(AudioFrame *p_buffer, int p_frames) {
	if (!active) {
		return 0;
	}

	int todo = p_frames;

	int frames_mixed_this_step = p_frames;
	state.params.b_loop = pxtn_stream->loop;

	while (todo && active) {
		int16_t imm_buffer[4096];

		int todo_samples = todo * 2;
		int samples = todo_samples > 4096 ? 4096 : todo_samples;
		int filled_bytes = 0;
		bool ret = svc->Moo(state, imm_buffer, samples * sizeof(int16_t), &filled_bytes);
		int filled_frames = filled_bytes / (sizeof(int16_t) * 2);
		todo -= filled_frames;

		for (int i = 0; i < filled_frames; i++) {
			*p_buffer++ = AudioFrame(imm_buffer[2 * i] * i16_f32mul, imm_buffer[2 * i + 1] * i16_f32mul);
		}

		//EOF
		if (!ret || state.end_vomit) {
			frames_mixed_this_step = p_frames - todo;
			//fill remainder with silence
			for (int i = p_frames - todo; i < p_frames; i++) {
				*p_buffer++ = AudioFrame(0, 0);
			}
			active = false;
			todo = 0;
		}
	}

	return frames_mixed_this_step;
}

float AudioStreamPlaybackPxTone::get_stream_sampling_rate() {
	return pxtn_stream->sample_rate;
}

void AudioStreamPlaybackPxTone::start(double p_from_pos) {
	state = mooState();
	svc->tones_ready(state);

	pxtnVOMITPREPARATION prep;
	memset(&prep, 0, sizeof(prep));
	prep.master_volume = 1.0f;
	prep.flags = pxtnVOMITPREPFLAG_loop;
	svc->moo_preparation(&prep, state);

	active = true;
	loops = 0;
	seek(p_from_pos);
	begin_resample();
}

void AudioStreamPlaybackPxTone::stop() {
	active = false;
}

bool AudioStreamPlaybackPxTone::is_playing() const {
	return active;
}

int AudioStreamPlaybackPxTone::get_loop_count() const {
	return loops;
}

double AudioStreamPlaybackPxTone::get_playback_position() const {
	return double(state.smp_count % svc->moo_get_total_sample()) / pxtn_stream->sample_rate;
}

void AudioStreamPlaybackPxTone::seek(double p_time) {
	if (!active) {
		return;
	}

	if (p_time < 0) {
		p_time = 0;
	} else if (p_time >= pxtn_stream->get_length()) {
		p_time = 0;
	}

	state.smp_count = uint32_t(pxtn_stream->sample_rate * p_time);
}

void AudioStreamPlaybackPxTone::tag_used_streams() {
	pxtn_stream->tag_used(get_playback_position());
}

AudioStreamPlaybackPxTone::~AudioStreamPlaybackPxTone() {
	delete svc;
}

Ref<AudioStreamPlayback> AudioStreamPxTone::instantiate_playback() {
	Ref<AudioStreamPlaybackPxTone> pxtns;

	ERR_FAIL_COND_V_MSG(data.is_empty(), pxtns,
			"This AudioStreamPxTone does not have an audio file assigned "
			"to it. AudioStreamPxTone should not be created from the "
			"inspector or with `.new()`. Instead, load an audio file.");

	pxtns.instantiate();
	pxtns->pxtn_stream = Ref<AudioStreamPxTone>(this);
	auto svc = new pxtnService();
	pxtns->svc = svc;

	pxtnERR errorcode = svc->init();
	if (errorcode != pxtnOK) {
		ERR_FAIL_COND_V(errorcode, Ref<AudioStreamPlaybackPxTone>());
	}

	svc->set_destination_quality(2, (int)sample_rate);

	pxtnDescriptor desc;
	desc.set_memory_r(data.ptr(), data.size());

	errorcode = svc->read(&desc);

	if (errorcode != pxtnOK) {
		ERR_FAIL_COND_V(errorcode, Ref<AudioStreamPlaybackPxTone>());
	}

	pxtns->frames_mixed = 0;
	pxtns->active = false;
	pxtns->loops = 0;

	return pxtns;
}

String AudioStreamPxTone::get_stream_name() const {
	return ""; //return stream_name;
}

void AudioStreamPxTone::clear_data() {
	data.clear();
}

void AudioStreamPxTone::set_data(const Vector<uint8_t> &p_data) {
	int src_data_len = p_data.size();
	const uint8_t *src_datar = p_data.ptr();

	pxtnService svc;
	mooState state;
	pxtnDescriptor desc;

	channels = 2;
	sample_rate = 44100;

	desc.set_memory_r(data.ptr(), data.size());

	ERR_FAIL_COND_MSG(svc.init() != pxtnOK, "Failed to initialize PxTone service.");
	svc.set_destination_quality(channels, (int)sample_rate);

	desc.set_memory_r(p_data.ptr(), p_data.size());
	ERR_FAIL_COND_MSG(svc.read(&desc) != pxtnOK, "Failed to decode specified PxTone file.");
	ERR_FAIL_COND(svc.tones_ready(state) != pxtnOK);

	pxtnVOMITPREPARATION prep;
	memset(&prep, 0, sizeof(prep));
	prep.master_volume = 1.0f;
	prep.flags = pxtnVOMITPREPFLAG_loop;
	prep.start_pos_float = 0.0f;
	svc.moo_preparation(&prep, state);
	length = svc.moo_get_total_sample() / sample_rate;
	bpm = (double)svc.master->get_beat_tempo();
	bar_beats = 1;
	beat_count = svc.master->get_beat_num();

	clear_data();

	data.resize(src_data_len);
	memcpy(data.ptrw(), src_datar, src_data_len);
	data_len = src_data_len;
}

Vector<uint8_t> AudioStreamPxTone::get_data() const {
	return data;
}

void AudioStreamPxTone::set_loop(bool p_enable) {
	loop = p_enable;
}

bool AudioStreamPxTone::has_loop() const {
	return loop;
}

void AudioStreamPxTone::set_loop_offset(double p_seconds) {
	loop_offset = p_seconds;
}

double AudioStreamPxTone::get_loop_offset() const {
	return loop_offset;
}

double AudioStreamPxTone::get_length() const {
	return length;
}

bool AudioStreamPxTone::is_monophonic() const {
	return false;
}

double AudioStreamPxTone::get_bpm() const {
	return bpm;
}

int AudioStreamPxTone::get_beat_count() const {
	return beat_count;
}

int AudioStreamPxTone::get_bar_beats() const {
	return bar_beats;
}


void AudioStreamPxTone::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_data", "data"), &AudioStreamPxTone::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &AudioStreamPxTone::get_data);

	ClassDB::bind_method(D_METHOD("set_loop", "enable"), &AudioStreamPxTone::set_loop);
	ClassDB::bind_method(D_METHOD("has_loop"), &AudioStreamPxTone::has_loop);

	ClassDB::bind_method(D_METHOD("set_loop_offset", "seconds"), &AudioStreamPxTone::set_loop_offset);
	ClassDB::bind_method(D_METHOD("get_loop_offset"), &AudioStreamPxTone::get_loop_offset);

	ClassDB::bind_method(D_METHOD("get_bpm"), &AudioStreamPxTone::get_bpm);
	ClassDB::bind_method(D_METHOD("get_beat_count"), &AudioStreamPxTone::get_beat_count);
	ClassDB::bind_method(D_METHOD("get_bar_beats"), &AudioStreamPxTone::get_bar_beats);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_data", "get_data");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bpm", PROPERTY_HINT_RANGE, "0,400,0.01,or_greater"), "", "get_bpm");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "beat_count", PROPERTY_HINT_RANGE, "0,512,1,or_greater"), "", "get_beat_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bar_beats", PROPERTY_HINT_RANGE, "2,32,1,or_greater"), "", "get_bar_beats");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop"), "set_loop", "has_loop");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loop_offset"), "set_loop_offset", "get_loop_offset");
}

AudioStreamPxTone::AudioStreamPxTone() {
}

AudioStreamPxTone::~AudioStreamPxTone() {
	clear_data();
}
