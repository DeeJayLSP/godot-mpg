#define PL_MPEG_IMPLEMENTATION
#define PLM_MALLOC(sz) memalloc(sz)
#define PLM_REALLOC(p, sz) memrealloc(p, sz)
#define PLM_FREE(p) memfree(p)

#include "video_stream_mpg.h"

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "scene/resources/image_texture.h"

#include "thirdparty/misc/yuv2rgb.h"

void VideoStreamPlaybackMPG::load_callback(plm_buffer_t *buf, void *user) {
	Ref<FileAccess> fa = *(Ref<FileAccess> *)user;

	plm_buffer_discard_read_bytes(buf);

	uint64_t bytes_available = buf->capacity - buf->length;
	uint64_t bytes_read = fa->get_buffer(buf->bytes + buf->length, bytes_available);
	buf->length += bytes_read;

	if (bytes_read == 0) {
		buf->has_ended = TRUE;
	}
}

void VideoStreamPlaybackMPG::seek_callback(plm_buffer_t *buf, size_t pos, void *user) {
	Ref<FileAccess> fa = *(Ref<FileAccess> *)user;
	fa->seek(pos);
}

size_t VideoStreamPlaybackMPG::tell_callback(plm_buffer_t *buf, void *user) {
	Ref<FileAccess> fa = *(Ref<FileAccess> *)user;
	return fa->get_position();
}

void VideoStreamPlaybackMPG::video_callback(plm_t *self, plm_frame_t *frame, void *user) {
	VideoStreamPlaybackMPG *ps = (VideoStreamPlaybackMPG *)user;
	ps->frame_current = frame;
	ps->frame_pending = true;
}

void VideoStreamPlaybackMPG::audio_callback(plm_t *self, plm_samples_t *samples, void *user) {
	VideoStreamPlaybackMPG *ps = (VideoStreamPlaybackMPG *)user;
	if (ps->mix_callback) {
		ps->mix_callback(ps->mix_udata, samples->interleaved, samples->count);
	}
}

void VideoStreamPlaybackMPG::set_file(const String &p_file) {
	ERR_FAIL_COND(playing);

	file_name = p_file;
	file = FileAccess::open(p_file, FileAccess::READ);
	ERR_FAIL_COND_MSG(file.is_null(), "Cannot open file: " + p_file);

	plm_buffer_t *buffer = plm_buffer_create_with_callbacks(load_callback, seek_callback, tell_callback, file->get_length(), &file);
	mpeg = plm_create_with_buffer(buffer, TRUE);

	plm_set_video_decode_callback(mpeg, video_callback, this);
	plm_set_audio_decode_callback(mpeg, audio_callback, this);

	int x = plm_get_width(mpeg);
	int y = plm_get_height(mpeg);

	Ref<Image> img = Image::create_empty(x, y, false, Image::FORMAT_RGBA8);
	texture->set_image(img);

	playing = false;
}

void VideoStreamPlaybackMPG::clear() {
	if (file.is_null()) {
		return;
	}

	if (mpeg) {
		plm_destroy(mpeg);
	}

	file.unref();
	playing = false;
}

void VideoStreamPlaybackMPG::play() {
	if (playing) {
		stop();
	}

	playing = true;
	plm_set_audio_lead_time(mpeg, (double)GLOBAL_GET("audio/video/video_delay_compensation_ms") / 1000.0);
}

void VideoStreamPlaybackMPG::stop() {
	if (playing) {
		clear();
		set_file(file_name);
	}
	playing = false;
}

bool VideoStreamPlaybackMPG::is_playing() const {
	return playing;
}

void VideoStreamPlaybackMPG::set_paused(bool p_paused) {
	paused = p_paused;
}

bool VideoStreamPlaybackMPG::is_paused() const {
	return paused;
}

double VideoStreamPlaybackMPG::get_length() const {
	return mpeg != nullptr ? plm_get_duration(mpeg) : 0.0;
}

double VideoStreamPlaybackMPG::get_playback_position() const {
	return mpeg != nullptr ? plm_get_time(mpeg) - plm_get_audio_lead_time(mpeg) : 0.0;
}

void VideoStreamPlaybackMPG::seek(double p_time) {
	if (mpeg != nullptr) {
		seek_pos = p_time;
	}
}

Ref<Texture2D> VideoStreamPlaybackMPG::get_texture() const {
	return texture;
}

void VideoStreamPlaybackMPG::update(double p_delta) {
	if (file.is_null() || !playing || paused || mpeg == nullptr) {
		return;
	}

	if (plm_has_ended(mpeg)) {
		stop();
		return;
	}

	if (seek_pos != -1.0) {
		plm_seek(mpeg, seek_pos, FALSE);
		seek_pos = -1.0;
	} else {
		plm_decode(mpeg, p_delta);
	}

	if (frame_pending) {
		int x = frame_current->width;
		int y = frame_current->height;
		frame_data.resize((x * y) << 2);
		yuv420_2_rgb8888(frame_data.ptrw(), frame_current->y.data, frame_current->cb.data, frame_current->cr.data, x, y, x, x >> 1, x << 2);
		Ref<Image> img = Image::create_from_data(x, y, false, Image::FORMAT_RGBA8, frame_data);
		texture->update(img);

		frame_pending = false;
	}
}

int VideoStreamPlaybackMPG::get_channels() const {
	return mpeg != nullptr ? mpeg->audio_buffer->mode == PLM_AUDIO_MODE_MONO ? 1 : 2 : 0;
}

int VideoStreamPlaybackMPG::get_mix_rate() const {
	return mpeg != nullptr ? plm_get_samplerate(mpeg) : 0;
}

void VideoStreamPlaybackMPG::set_audio_track(int p_track) {
	if (mpeg != nullptr) {
		plm_set_audio_stream(mpeg, p_track);
	}
}

VideoStreamPlaybackMPG::VideoStreamPlaybackMPG() {
	texture.instantiate();
	PLM_UNUSED(yuv444_2_rgb8888);
	PLM_UNUSED(yuv422_2_rgb8888);
}

VideoStreamPlaybackMPG::~VideoStreamPlaybackMPG() {
	clear();
}

void VideoStreamMPG::_bind_methods() {}

Ref<VideoStreamPlayback> VideoStreamMPG::instantiate_playback() {
	Ref<VideoStreamPlaybackMPG> pb;
	pb.instantiate();
	pb->set_file(file);
	pb->set_audio_track(audio_track);
	return pb;
}

void VideoStreamMPG::set_audio_track(int p_track) {
	audio_track = p_track;
}

Ref<Resource> ResourceFormatLoaderMPG::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, r_error);
	ERR_FAIL_COND_V_MSG(f.is_null(), Ref<Resource>(), "Cannot open file '" + p_path + "'.");

	Ref<VideoStreamMPG> mpg_stream;
	mpg_stream.instantiate();
	mpg_stream->set_file(p_path);
	return mpg_stream;
}

void ResourceFormatLoaderMPG::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("mpg");
}

bool ResourceFormatLoaderMPG::handles_type(const String &p_type) const {
	return ClassDB::is_parent_class(p_type, "VideoStream");
}

String ResourceFormatLoaderMPG::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "mpg") {
		return "VideoStreamMPG";
	}
	return "";
}
