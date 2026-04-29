#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace tinygltf {

    bool WriteImageData(const std::string* basepath, const std::string* filename,
        const Image* image, bool embedImages,
        const FsCallbacks* fs, const URICallbacks* uri_cb,
        std::string* out_data, void* user_data) {
        return false;
    }
}