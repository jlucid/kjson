#include "kjson_serialisation.h"
#include "kjson_utils.h"
#include <cmath>
#include <ctime>
#include <cstring>  // For memcpy, memcmp
#include <arpa/inet.h>  // For ntohl, etc.
#include <cassert>
#include <cstdio>  // For snprintf
#include <sstream>  // For std::ostringstream
#include <string>
#include <array>
#include "rapidjson/error/en.h"  // For GetParseError_En

namespace kjson {

extern "C" {
    #include "k.h"
    extern K vk(K);
}

using GUID = std::array<unsigned char, 16>;

// Generic function to serialise vectors
template<typename Writer, typename T, typename EmitFunction>
void serialise_vector(Writer& w, K x, bool isvec, int i, EmitFunction emit_func) {
    if (isvec) {
        if (i >= 0) {
            emit_func(w, reinterpret_cast<T*>(x->G0)[i]);
        } else {
            w.StartArray();
            for (int idx = 0; idx < x->n; ++idx) {
                emit_func(w, reinterpret_cast<T*>(x->G0)[idx]);
            }
            w.EndArray();
        }
    } else {
        emit_func(w, *reinterpret_cast<T*>(&x->g));
    }
}

template<typename Writer, typename EmitFunction>
void serialise_vector(Writer& w, K x, bool isvec, int i, EmitFunction emit_func, U*)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_func(w, kU(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; ++idx)
            {
                emit_func(w, kU(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_func(w, kU(x)[0]);
    }
}

template<typename Writer, typename EmitFunction>
void serialise_vector_enum(Writer& w, K x, bool isvec, int i, EmitFunction emit_func) {
    if (isvec) {
        if (i >= 0) {
            emit_func(w, kI(x)[i]);
        } else {
            w.StartArray();
            for (int idx = 0; idx < x->n; ++idx) {
                emit_func(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    } else {
        emit_func(w, x->i);
    }
}

template<typename Writer>
void serialise_keyed_table(Writer& w, K keys, K values) {
    const K kdict = keys->k;
    const K kkeys = kK(kdict)[0];
    const K kvalues = kK(kdict)[1];

    const K vdict = values->k;
    const K vkeys = kK(vdict)[0];
    const K vvalues = kK(vdict)[1];

    const int krows = kK(kvalues)[0]->n;
    const int vrows = kK(vvalues)[0]->n;

    assert(vrows == krows);

    w.StartArray();
    for (int row = 0; row < krows; ++row) {
        w.StartObject();
        for (int col = 0; col < kkeys->n; ++col) {
            serialise_atom(w, kkeys, col);
            serialise_atom(w, kK(kvalues)[col], row);
        }
        for (int col = 0; col < vkeys->n; ++col) {
            serialise_atom(w, vkeys, col);
            serialise_atom(w, kK(vvalues)[col], row);
        }
        w.EndObject();
    }
    w.EndArray();
}

template<typename Writer>
void emit_sym(Writer& w, S s) {
    if (s) {
        w.String(s);
    } else {
        w.Null();
    }
}

template<typename Writer>
void serialise_sym(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, S s) {
        emit_sym(w, s);
    };
    serialise_vector<Writer, S>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_enum_sym(Writer& w, K sym, int sym_idx)
{
    switch (sym_idx)
    {
        case(wi):
        case(ni):   
            w.Null(); 
            break;
        default:    
            w.String((char*)kS(sym)[sym_idx]);  // Retrieve symbol from global sym
    }
}

template<typename Writer>
void serialise_enum_sym(Writer& w, K x, bool isvec, int i)
{
    K domain = k(0, (S)"sym", (K)0);
    if (!domain || domain->t != KS)  // Ensure the domain is a symbol list
    {
        if (domain) r0(domain);
        w.Null();
        return;
    }

    auto emit_func = [&](Writer& w, J idx) {
        if (idx == nj || idx < 0 || idx >= domain->n)
        {
            w.Null();
        }
        else
        {
            w.String(kS(domain)[idx]);
        }
    };

    // Use the serialise_vector function with T = J (long)
    serialise_vector<Writer, J>(w, x, isvec, i, emit_func);

    r0(domain);  // Decrement reference count for domain
}

template<typename Writer>
void emit_char(Writer& w, C c) {
    w.String(&c, 1);
}

template<typename Writer>
void serialise_char(Writer& w, K x, bool isvec, int i) {
    if (isvec) {
        if (i == -1) {
            w.String(reinterpret_cast<char*>(kC(x)), x->n);
        } else {
            emit_char(w, kC(x)[i]);
        }
    } else {
        emit_char(w, x->g);
    }
}

template<typename Writer>
void emit_bool(Writer& w, G g) {
    w.Bool(g != 0);
}

template<typename Writer>
void serialise_bool(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, G g) {
        emit_bool(w, g);
    };
    serialise_vector<Writer, G>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_byte(Writer& w, G n) {
    const char* hex = "0123456789abcdef";
    char buff[3] = {hex[(n >> 4) & 0xF], hex[n & 0xF], '\0'};
    w.String(buff, 2);
}

template<typename Writer>
void serialise_byte(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, G n) {
        emit_byte(w, n);
    };
    serialise_vector<Writer, G>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_short(Writer& w, H n) {
    switch (n) {
        case wh:
        case nh:
            w.Null();
            break;
        default:
            w.Int(n);
            break;
    }
}

template<typename Writer>
void serialise_short(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, H n) {
        emit_short(w, n);
    };
    serialise_vector<Writer, H>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_int(Writer& w, I n) {
    switch (n) {
        case wi:
        case ni:
            w.Null();
            break;
        default:
            w.Int(n);
            break;
    }
}

template<typename Writer>
void serialise_int(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, I n) {
        emit_int(w, n);
    };
    serialise_vector<Writer, I>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_long(Writer& w, J n) {
    switch (n) {
        case wj:
        case nj:
            w.Null();
            break;
        default:
            w.Int64(n);
            break;
    }
}

template<typename Writer>
void serialise_long(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, J n) {
        emit_long(w, n);
    };
    serialise_vector<Writer, J>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_double(Writer& w, F n) {
    static const char* INF_STR = "Inf";
    static const char* NINF_STR = "-Inf";

    if (std::isnan(n)) {
        w.Null();
    } else if (std::isinf(n)) {
        w.String(n == INFINITY ? INF_STR : NINF_STR);
    } else {
        double intpart;
        if (std::modf(n, &intpart) == 0.0) {
            if (n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max()) {
                w.Int(static_cast<int>(n));
            } else if (n >= std::numeric_limits<int64_t>::min() && n <= std::numeric_limits<int64_t>::max()) {
                w.Int64(static_cast<int64_t>(n));
            } else {
                w.Double(n);
            }
        } else {
            w.Double(n);
        }
    }
}

template<typename Writer>
void serialise_float(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, E n) {
        emit_double(w, n);
    };
    serialise_vector<Writer, E>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void serialise_double(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, F n) {
        emit_double(w, n);
    };
    serialise_vector<Writer, F>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_date_custom(Writer& w, I n) {
    if (n == ni) {
        w.Null();
    } else {
        time_t tt = (n + 10957) * 86400;
        struct tm timinfo;
        gmtime_r(&tt, &timinfo);
        char buff[11];
        int written = snprintf(buff, sizeof(buff), "%04d-%02d-%02d",
                               timinfo.tm_year + 1900,
                               timinfo.tm_mon + 1,
                               timinfo.tm_mday);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_date(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, I n) {
        emit_date_custom(w, n);
    };
    serialise_vector<Writer, I>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_time_custom(Writer& w, I n) {
    if (n == ni) {
        w.Null();
    } else {
        int millis = n % 1000;
        int total_seconds = n / 1000;
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;
        char buff[13];
        int written = snprintf(buff, sizeof(buff), "%02d:%02d:%02d.%03d", hours, minutes, seconds, millis);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_time(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, I n) {
        emit_time_custom(w, n);
    };
    serialise_vector<Writer, I>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_timestamp_custom(Writer& w, J n) {
    if (n == nj) {
        w.Null();
    } else {
        constexpr long long SEC_IN_DAY = 86400LL;
        constexpr long long NANOS_IN_SEC = 1000000000LL;
        constexpr long long DAYS_1970_TO_2000 = 10957LL;

        long long total_seconds = n / NANOS_IN_SEC;
        long long nanoseconds = n % NANOS_IN_SEC;

        total_seconds += DAYS_1970_TO_2000 * SEC_IN_DAY;

        time_t tt = static_cast<time_t>(total_seconds);
        struct tm timinfo;
        gmtime_r(&tt, &timinfo);

        char buff[30];
        int written = snprintf(buff, sizeof(buff), "%04d-%02d-%02dT%02d:%02d:%02d.%09lld",
                               timinfo.tm_year + 1900, timinfo.tm_mon + 1, timinfo.tm_mday,
                               timinfo.tm_hour, timinfo.tm_min, timinfo.tm_sec, nanoseconds);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_timestamp(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, J n) {
        emit_timestamp_custom(w, n);
    };
    serialise_vector<Writer, J>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_timespan_custom(Writer& w, J n) {
    if (n == nj) {
        w.Null();
    } else {
        constexpr long long NANOS_IN_SEC = 1000000000LL;
        constexpr long long SECS_IN_MIN = 60LL;
        constexpr long long MINS_IN_HOUR = 60LL;
        constexpr long long HOURS_IN_DAY = 24LL;

        bool is_negative = n < 0;
        long long total_seconds = std::abs(n) / NANOS_IN_SEC;
        long long nanoseconds = std::abs(n) % NANOS_IN_SEC;

        long long days = total_seconds / (HOURS_IN_DAY * MINS_IN_HOUR * SECS_IN_MIN);
        long long remaining_seconds = total_seconds % (HOURS_IN_DAY * MINS_IN_HOUR * SECS_IN_MIN);
        long long hours = remaining_seconds / (MINS_IN_HOUR * SECS_IN_MIN);
        remaining_seconds %= (MINS_IN_HOUR * SECS_IN_MIN);
        long long minutes = remaining_seconds / SECS_IN_MIN;
        long long seconds = remaining_seconds % SECS_IN_MIN;

        char buff[32];
        int written = snprintf(buff, sizeof(buff),
                               "%s%lldD%02lld:%02lld:%02lld.%09lld",
                               is_negative ? "-" : "", days, hours, minutes, seconds, nanoseconds);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_timespan(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, J n) {
        emit_timespan_custom(w, n);
    };
    serialise_vector<Writer, J>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_datetime_custom(Writer& w, F n) {
    if (std::isnan(n)) {
        w.Null();
    } else {
        time_t tt = static_cast<time_t>((n + 10957) * 86400);
        struct tm timinfo;
        gmtime_r(&tt, &timinfo);

        long millis = static_cast<long>(round((n - floor(n)) * 86400000)) % 1000;

        char buff[25];
        int written = snprintf(buff, sizeof(buff),
                               "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
                               timinfo.tm_year + 1900, timinfo.tm_mon + 1, timinfo.tm_mday,
                               timinfo.tm_hour, timinfo.tm_min, timinfo.tm_sec, millis);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_datetime(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, F n) {
        emit_datetime_custom(w, n);
    };
    serialise_vector<Writer, F>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_month_custom(Writer& w, I n) {
    if (n == ni) {
        w.Null();
    } else {
        int year = n / 12 + 2000;
        int month = n % 12 + 1;

        char buff[8];
        int written = snprintf(buff, sizeof(buff), "%04d-%02d", year, month);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_month(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, I n) {
        emit_month_custom(w, n);
    };
    serialise_vector<Writer, I>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_minute_custom(Writer& w, I n) {
    if (n == ni) {
        w.Null();
    } else {
        int hours = n / 60;
        int minutes = n % 60;

        char buff[6];
        int written = snprintf(buff, sizeof(buff), "%02d:%02d", hours, minutes);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_minute(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, I n) {
        emit_minute_custom(w, n);
    };
    serialise_vector<Writer, I>(w, x, isvec, i, emit_func);
}

template<typename Writer>
void emit_second_custom(Writer& w, I n) {
    if (n == ni) {
        w.Null();
    } else {
        int hours = n / 3600;
        int minutes = (n % 3600) / 60;
        int seconds = n % 60;

        char buff[9];
        int written = snprintf(buff, sizeof(buff), "%02d:%02d:%02d", hours, minutes, seconds);
        if (written > 0 && written < static_cast<int>(sizeof(buff))) {
            w.String(buff, written);
        } else {
            w.Null();
        }
    }
}

template<typename Writer>
void serialise_second(Writer& w, K x, bool isvec, int i) {
    auto emit_func = [](Writer& w, I n) {
        emit_second_custom(w, n);
    };
    serialise_vector<Writer, I>(w, x, isvec, i, emit_func);
}

template<typename Writer>
inline void emit_guid_custom(Writer& w, const U guid_raw)
{
    static const U null_guid = {0};

    if (memcmp(&guid_raw, &null_guid, sizeof(U)) == 0)
    {
        w.Null();
    }
    else
    {
        union {
            struct {
                uint32_t data1;
                uint16_t data2;
                uint16_t data3;
                uint16_t data4;
                uint16_t data5;
                uint32_t data6;
            };
            U raw;
        } guid;

        guid.raw = guid_raw;
        char buff[37]; // 8-4-4-4-12\0
        snprintf(buff, sizeof(buff), "%08x-%04x-%04x-%04x-%04x%08x",
                 ntohl(guid.data1), ntohs(guid.data2), ntohs(guid.data3), ntohs(guid.data4),
                 ntohs(guid.data5), ntohl(guid.data6));
        w.String(buff, 36);
    }
}

template<typename Writer>
void serialise_guid(Writer& w, K x, bool isvec, int i)
{
    auto emit_func = [](Writer& w, U u) {
        emit_guid_custom(w, u);
    };
    serialise_vector(w, x, isvec, i, emit_func, (U*)nullptr);
}

template<typename Writer>
void serialise_dict(Writer& w, K x, bool /*isvec*/, int /*i*/) {
    const K keys = kK(x)[0];
    const K values = kK(x)[1];

    if (keys->t == XT && values->t == XT) {
        serialise_keyed_table(w, keys, values);
    } else {
        w.StartObject();
        for (int idx = 0; idx < keys->n; ++idx) {
            serialise_atom(w, keys, idx);
            serialise_atom(w, values, idx);
        }
        w.EndObject();
    }
}

template<typename Writer>
void serialise_list(Writer& w, K x, bool isvec, int i) {
    if (isvec) {
        if (i >= 0) {
            K v = kK(x)[i];
            serialise_atom(w, v, -1);
        } else {
            w.StartArray();
            for (int idx = 0; idx < x->n; ++idx) {
                K v = kK(x)[idx];
                serialise_atom(w, v, -1);
            }
            w.EndArray();
        }
    } else {
        w.Null();
    }
}

template<typename Writer>
void serialise_table(Writer& w, K x, bool isvec, int i) {
    const K dict = x->k;
    const K keys = kK(dict)[0];
    const K values = kK(dict)[1];

    if (i >= 0) {
        w.StartObject();
        for (int col = 0; col < keys->n; ++col) {
            serialise_atom(w, keys, col);
            serialise_atom(w, kK(values)[col], i);
        }
        w.EndObject();
    } else {
        const int rows = kK(values)[0]->n;

        w.StartArray();
        for (int row = 0; row < rows; ++row) {
            w.StartObject();
            for (int col = 0; col < keys->n; ++col) {
                serialise_atom(w, keys, col);
                serialise_atom(w, kK(values)[col], row);
            }
            w.EndObject();
        }
        w.EndArray();
    }
}

template<typename Writer>
void serialise_atom(Writer& w, const K x, int i) {
    bool isvec = x->t >= 0;

    switch (x->t) {
        case 0:
            serialise_list(w, x, isvec, i);
            break;
        case -KS:
        case KS:
            serialise_sym(w, x, isvec, i);
            break;
        case -KC:
        case KC:
            serialise_char(w, x, isvec, i);
            break;
        case -KB:
        case KB:
            serialise_bool(w, x, isvec, i);
            break;
        case -KG:
        case KG:
            serialise_byte(w, x, isvec, i);
            break;
        case -KH:
        case KH:
            serialise_short(w, x, isvec, i);
            break;
        case -KI:
        case KI:
            serialise_int(w, x, isvec, i);
            break;
        case -KJ:
        case KJ:
            serialise_long(w, x, isvec, i);
            break;
        case -KE:
        case KE:
            serialise_float(w, x, isvec, i);
            break;
        case -KF:
        case KF:
            serialise_double(w, x, isvec, i);
            break;
        case -KD:
        case KD:
            serialise_date(w, x, isvec, i);
            break;
        case -KZ:
        case KZ:
            serialise_datetime(w, x, isvec, i);
            break;
        case -KP:
        case KP:
            serialise_timestamp(w, x, isvec, i);
            break;
        case -KN:
        case KN:
            serialise_timespan(w, x, isvec, i);
            break;
        case -KM:
        case KM:
            serialise_month(w, x, isvec, i);
            break;
        case -KU:
        case KU:
            serialise_minute(w, x, isvec, i);
            break;
        case -KV:
        case KV:
            serialise_second(w, x, isvec, i);
            break;
        case -KT:
        case KT:
            serialise_time(w, x, isvec, i);
            break;
        case -UU:
        case UU:
            serialise_guid(w, x, isvec, i);
            break;
        case XT:
            serialise_table(w, x, isvec, i);
            break;
        case XD:
            serialise_dict(w, x, isvec, i);
            break;
        case 20:
	case -20:
            serialise_enum_sym(w, x, isvec, i);
            break;
        default:
            w.Null();
            break;
    }
}

}  // namespace kjson

extern "C" {

K handle_parse_error(const rapidjson::Document& document) {
    std::string errMsg = std::string("Parse error: ") + GetParseError_En(document.GetParseError()) +
                         " at offset " + std::to_string(document.GetErrorOffset());
    return krr(const_cast<S>(errMsg.c_str()));
}

K jtok(K json_string) {
    if (json_string->t != KC) {
        return krr(const_cast<S>("Type error: Input must be a char vector (string)"));
    }

    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char*>(kC(json_string)), json_string->n);

    if (document.HasParseError()) {
        return handle_parse_error(document);
    }

    try {
        return kjson::json_to_kobject(document);
    } catch (const std::exception& e) {
        return krr(const_cast<S>(e.what()));
    }
}

K ktoj(K x) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.SetMaxDecimalPlaces(5);

    try {
        kjson::serialise_atom(writer, x, -1);

        size_t len = buffer.GetSize();
        const char* str = buffer.GetString();

        return kpn(const_cast<S>(str), len);
    } catch (const std::exception& e) {
        return krr(const_cast<S>(e.what()));
    }
}

}  // extern "C"
