#ifndef KJSON_UTILS_H
#define KJSON_UTILS_H

#define KXVER 3
#include "k.h"
#include "rapidjson/document.h" // Add this for rapidjson::Value

// Ensure `vk` function uses C linkage to avoid name mangling
extern "C" {
    extern K vk(K);  // Declare vk function for collapsing homogeneous lists
}

namespace kjson {
    // Utility functions used for JSON to K and K to JSON conversion
    K json_to_kobject(const rapidjson::Value& value);
    K json_to_kobject_dict(const rapidjson::Value& value);
}

#endif // KJSON_UTILS_H
