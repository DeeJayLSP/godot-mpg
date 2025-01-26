#ifndef VIDEO_STREAM_MPG_H
#define VIDEO_STREAM_MPG_H

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "scene/resources/video_stream.h"

#include "./thirdparty/pl_mpeg.h"

class ImageTexture;

class VideoStreamPlaybackMPG : public VideoStreamPlayback {
	GDCLASS(VideoStreamPlaybackMPG, VideoStreamPlayback);

	plm_t *mpeg = nullptr;

	Ref<FileAccess> file;
	String file_name;
	LocalVector<uint8_t> file_data; // HACK: remove once a proper plm_buffer_t with FileAccess is done
	Ref<ImageTexture> texture;
	Vector<uint8_t> frame_data; // Image creation has a big overhead converting from LocalVector
	Point2i size;

	plm_frame_t *frame_current = nullptr;
	bool frame_pending = false;

	static void dummy_yuv2rgb();
	static void load_callback(plm_buffer_t *buf, void *user);
	static void video_callback(plm_t *self, plm_frame_t *frame, void *user);
	static void audio_callback(plm_t *self, plm_samples_t *samples, void *user);

	int delay_compensation = 0;

	bool playing = false;
	bool paused = false;

	double length = 0;
	double time = 0;
	int channels = 0;
	int mix_rate = 0;
	int audio_track = 0;

protected:
	void clear();

public:
	virtual void play() override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual void set_paused(bool p_paused) override;
	virtual bool is_paused() const override;

	virtual double get_length() const override;

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	void set_file(const String &p_file);

	virtual Ref<Texture2D> get_texture() const override;
	virtual void update(double p_delta) override;

	virtual int get_channels() const override;
	virtual int get_mix_rate() const override;

	virtual void set_audio_track(int p_track) override;

	VideoStreamPlaybackMPG();
	~VideoStreamPlaybackMPG();
};

class VideoStreamMPG : public VideoStream {
	GDCLASS(VideoStreamMPG, VideoStream);

protected:
	static void _bind_methods();

public:
	Ref<VideoStreamPlayback> instantiate_playback() override {
		Ref<VideoStreamPlaybackMPG> pb = memnew(VideoStreamPlaybackMPG);
		pb->set_audio_track(audio_track);
		pb->set_file(file);
		return pb;
	}

	void set_audio_track(int p_track) override { audio_track = p_track; }

	VideoStreamMPG() { audio_track = 0; }
};

class ResourceFormatLoaderMPG : public ResourceFormatLoader {
public:
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
};

#endif // VIDEO_STREAM_MPG_H
