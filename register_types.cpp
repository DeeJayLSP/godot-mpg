#include "register_types.h"

#include "video_stream_mpg.h"

static Ref<ResourceFormatLoaderMPG> resource_loader_mpg;

void initialize_mpg_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	resource_loader_mpg.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_mpg, true);

	GDREGISTER_CLASS(VideoStreamMPG);
}

void uninitialize_mpg_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ResourceLoader::remove_resource_format_loader(resource_loader_mpg);
	resource_loader_mpg.unref();
}
