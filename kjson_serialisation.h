#ifndef KJSON_SERIALISATION_H
#define KJSON_SERIALISATION_H

#define KXVER 3
#include "k.h"
#include "kjson_utils.h"           // Include utils to cover dependent functions
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace kjson {

// Forward declarations for serialization functions
template <typename Writer>
void serialise_atom(Writer& w, const K x, int i = -1);

// Add other forward declarations if required
template<typename Writer>
void serialise_dict(Writer& w, K x, bool isvec, int i);
template<typename Writer>
void serialise_keyed_table(Writer& w, K keys, K values);
// Include more as needed...

} // namespace kjson


extern "C" {
    K __attribute__((visibility("default"))) jtok(K json_string);
    K __attribute__((visibility("default"))) ktoj(K x);
}


#endif // KJSON_SERIALISATION_H
