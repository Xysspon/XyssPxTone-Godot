/*************************************************************************/
/*  audio_stream_pxtone.h                                                */
/*************************************************************************/
/* Copyright (c) 2007-2025 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2025 Godot Engine contributors (cf. AUTHORS.md).   */
/* Copyright (c) 2022-2025 Alula, Xysspon LLC                            */
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

#ifndef AUDIO_STREAM_PXTONE_H
#define AUDIO_STREAM_PXTONE_H

#include "core/io/resource_loader.h"
#include "servers/audio/audio_stream.h"

#include "pxtone/pxtnService.h"

class AudioStreamPxTone;

class AudioStreamPlaybackPxTone : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamPlaybackPxTone, AudioStreamPlaybackResampled);

	pxtnService *svc = nullptr;
	mooState state{};
	uint32_t frames_mixed = 0;
	bool active = false;
	int loops = 0;

	friend class AudioStreamPxTone;

	Ref<AudioStreamPxTone> pxtn_stream;

protected:
	virtual int _mix_internal(AudioFrame *p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; //times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	AudioStreamPlaybackPxTone() {}
	~AudioStreamPlaybackPxTone();
};

class AudioStreamPxTone : public AudioStream {
	GDCLASS(AudioStreamPxTone, AudioStream);
	OBJ_SAVE_TYPE(AudioStream) //children are all saved as AudioStream, so they can be exchanged
	RES_BASE_EXTENSION("ptstr");

	friend class AudioStreamPlaybackPxTone;

	PackedByteArray data;
	uint32_t data_len = 0;

	float sample_rate = 1.0;
	float length = 0.0;
	float loop_offset = 0.0;
	double bpm = 0;
	int channels = 1;
	int beat_count = 0;
	int bar_beats = 4;
	bool loop = false;

	void clear_data();

protected:
	static void _bind_methods();

public:
	void set_loop(bool p_enable);
	virtual bool has_loop() const override;

	void set_loop_offset(double p_seconds);
	double get_loop_offset() const;

	virtual double get_bpm() const override;
	virtual int get_beat_count() const override;
	virtual int get_bar_beats() const override;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual String get_stream_name() const override;

	void set_data(const Vector<uint8_t> &p_data);
	Vector<uint8_t> get_data() const;

	virtual double get_length() const override;

	virtual bool is_monophonic() const override;

	AudioStreamPxTone();
	virtual ~AudioStreamPxTone();
};

#endif // AUDIO_STREAM_PXTONE_H
