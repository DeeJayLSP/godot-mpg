#define PL_MPEG_IMPLEMENTATION
#define PLM_MALLOC(sz) memalloc(sz)
#define PLM_REALLOC(p, sz) memrealloc(p, sz)
#define PLM_FREE(p) memfree(p)

#include "video_stream_mpg.h"

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "scene/resources/image_texture.h"

#include "thirdparty/misc/yuv2rgb.h"

void VideoStreamPlaybackMPG::buffer_data(plm_buffer_t *buf, void *user) { // TODO: fix this and use custom buffer
	PLM_UNUSED(user);
	Ref<FileAccess> fa = *(Ref<FileAccess> *)buf->load_callback_user_data;

	if (buf->discard_read_bytes) {
		plm_buffer_discard_read_bytes(buf);
	}

	uint64_t bytes_available = buf->capacity - buf->length;
	uint64_t bytes_read = fa->get_buffer(buf->bytes + buf->length, bytes_available);
	buf->length += bytes_read;

	if (bytes_read == 0) {
		buf->has_ended = TRUE;
	}
}

void VideoStreamPlaybackMPG::video_write(plm_t *self, plm_frame_t *frame, void *user) {
	PLM_UNUSED(user);
	VideoStreamPlaybackMPG *ps = (VideoStreamPlaybackMPG *)self->video_decode_callback_user_data;
	ps->frame_current = frame;
	ps->frame_pending = true;
}

void VideoStreamPlaybackMPG::audio_write(plm_t *self, plm_samples_t *samples, void *user) {
	PLM_UNUSED(user);
	VideoStreamPlaybackMPG *ps = (VideoStreamPlaybackMPG *)self->audio_decode_callback_user_data;

	if (ps->mix_callback) {
		ps->mix_callback(ps->mix_udata, samples->interleaved, samples->count);
	}
}

void VideoStreamPlaybackMPG::set_file(const String &p_file) {
	ERR_FAIL_COND(playing);

	file_name = p_file;
	file = FileAccess::open(p_file, FileAccess::READ);
	ERR_FAIL_COND_MSG(file.is_null(), "Cannot open file: " + p_file);

	// TODO: Custom plm_buffer_t using FileAccess. Not working properly yet.
	// plm_buffer_t *buffer = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
	// buffer->total_size = file->get_length();
	// plm_buffer_set_load_callback(buffer, buffer_data, &file);
	// mpeg = plm_create_with_buffer(buffer, TRUE);

	// HACK: Temporarily create with memory instead.
	const uint64_t len = file->get_length();
	file_data.resize(len);
	file->get_buffer(file_data.ptr(), len);
	file->close();
	mpeg = plm_create_with_memory(file_data.ptr(), file_data.size(), FALSE);

	plm_set_video_decode_callback(mpeg, video_write, this);
	plm_set_audio_decode_callback(mpeg, audio_write, this);

	if (plm_get_num_audio_streams(mpeg) > 0) {
		plm_set_audio_stream(mpeg, audio_track);
		channels = mpeg->audio_buffer->mode == PLM_AUDIO_MODE_MONO ? 1 : 2;
		mix_rate = plm_get_samplerate(mpeg);
	} else {
		plm_set_audio_enabled(mpeg, FALSE);
	}

	size.x = plm_get_width(mpeg);
	size.y = plm_get_height(mpeg);

	if (texture->get_image().is_null()) {
		Ref<Image> img = Image::create_empty(size.x, size.y, false, Image::FORMAT_RGBA8);
		texture->set_image(img);
	}

	length = plm_get_duration(mpeg);

	time = 0;
	playing = false;
}

void VideoStreamPlaybackMPG::clear() {
	if (file.is_null()) {
		return;
	}

	if (mpeg) {
		plm_destroy(mpeg);
	}

	file_data.clear();

	file.unref();
	playing = false;
}

void VideoStreamPlaybackMPG::play() {
	if (!playing) {
		time = 0;
	} else {
		stop();
	}

	playing = true;
	delay_compensation = GLOBAL_GET("audio/video/video_delay_compensation_ms");
	delay_compensation /= 1000.0;
}

void VideoStreamPlaybackMPG::stop() {
	if (playing) {
		clear();
		set_file(file_name);
	}
	playing = false;
	time = 0;
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
	return 0;
}

double VideoStreamPlaybackMPG::get_playback_position() const {
	return time - delay_compensation;
}

void VideoStreamPlaybackMPG::seek(double p_time) {
	WARN_PRINT_ONCE("Seeking in MPG videos is not implemented yet (it's only supported for GDExtension-provided video streams).");
}

Ref<Texture2D> VideoStreamPlaybackMPG::get_texture() const {
	return texture;
}

void VideoStreamPlaybackMPG::update(double p_delta) {
	if (file.is_null()) {
		return;
	}

	if (!playing || paused) {
		return;
	}
	time += p_delta;

	plm_decode(mpeg, p_delta);

	if (frame_pending) {
		frame_data.resize((size.x * size.y) << 2);
		yuv420_2_rgb8888(frame_data.ptrw(), frame_current->y.data, frame_current->cb.data, frame_current->cr.data, size.x, size.y, size.x, size.x >> 1, size.x << 2);

		if (false) { // MPEG-1 only supports 4:2:0, these are here to prevent werror from yelling
			yuv444_2_rgb8888(nullptr, nullptr, nullptr, nullptr, 0, 0, 0, 0, 0);
			yuv422_2_rgb8888(nullptr, nullptr, nullptr, nullptr, 0, 0, 0, 0, 0);
		}

		Ref<Image> img = memnew(Image(size.x, size.y, false, Image::FORMAT_RGBA8, frame_data));
		texture->update(img);

		frame_pending = false;
	}

	if (plm_has_ended(mpeg)) {
		stop();
	}
}

int VideoStreamPlaybackMPG::get_channels() const {
	return channels;
}

int VideoStreamPlaybackMPG::get_mix_rate() const {
	return mix_rate;
}

void VideoStreamPlaybackMPG::set_audio_track(int p_track) {
	audio_track = p_track;
}

VideoStreamPlaybackMPG::VideoStreamPlaybackMPG() {
	texture.instantiate();
}

VideoStreamPlaybackMPG::~VideoStreamPlaybackMPG() {
	clear();
}

void VideoStreamMPG::_bind_methods() {}

Ref<Resource> ResourceFormatLoaderMPG::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	VideoStreamMPG *stream = memnew(VideoStreamMPG);
	stream->set_file(p_path);

	Ref<VideoStreamMPG> mpg_stream = Ref<VideoStreamMPG>(stream);

	if (r_error) {
		*r_error = OK;
	}

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
