#include "kjson_serialisation.h"
#include "kjson_utils.h"
#include <cmath>
#include <ctime>
#include <cstring> // For memcpy, memcmp
#include <arpa/inet.h> // For ntohl, etc.
#include <cassert>
#include <cstdio> // For snprintf
#include "rapidjson/error/en.h" // For GetParseError_En

namespace kjson {

using GUID = unsigned char[16];

extern "C" {
    #include "k.h"
    extern K vk(K);
}

template<typename Writer>
void serialise_keyed_table(Writer& w, K keys, K values)
{
    const K kdict = keys->k;
    const K kkeys = kK(kdict)[0];
    const K kvalues = kK(kdict)[1];

    const K vdict = values->k;
    const K vkeys = kK(vdict)[0];
    const K vvalues = kK(vdict)[1];

    const int krows = kK(kvalues)[0]->n;
    const int vrows = kK(vvalues)[0]->n;

    assert(vrows == krows);

    // Serialize keyed table as an array of objects
    w.StartArray();
    for (int row = 0; row < krows; row++)
    {
        w.StartObject();
        for (int col = 0; col < kkeys->n; col++)
        {
            serialise_atom(w, kkeys, col);
            serialise_atom(w, kK(kvalues)[col], row);
        }
        for (int col = 0; col < vkeys->n; col++)
        {
            serialise_atom(w, vkeys, col);
            serialise_atom(w, kK(vvalues)[col], row);
        }
        w.EndObject();
    }
    w.EndArray();
}

template<typename Writer>
void serialise_sym(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            w.String((char*)kS(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int i = 0; i < x->n; i++)
            {
                w.String((char*)kS(x)[i]);
            }
            w.EndArray();
        }
    }
    else
    {
        w.String(x->s);
    }
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
    // Dynamically retrieve the global `sym` domain
    K sym = k(0, (S)"sym", (K)0);
    if (!sym || sym->t != KS)  // Ensure sym is a valid symbol list
    {
        w.Null();
        return;
    }

    if (isvec)
    {
        if (i >= 0)
        {
            // Access symbol using the correct index from x
            emit_enum_sym(w, sym, kJ(x)[i]);  // Use kJ(x) for enumerations
        }
        else
        {
            w.StartArray();  // Serialize the entire array
            for (int idx = 0; idx < x->n; ++idx)
            {
                // Correctly access the symbol based on the enumerated index
                emit_enum_sym(w, sym, kJ(x)[idx]);  // Use kJ(x) for correct ordering
            }
            w.EndArray();
        }
    }
    else
    {
        // Handle scalar enumerated symbol
        emit_enum_sym(w, sym, x->j);  // Use x->j for scalar access
    }
}

template<typename Writer>
void serialise_char(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i == -1)
        {
            w.String((char*)kC(x), x->n);
        }
        else
        {
            w.String((char*)&kC(x)[i], 1);
        }
    }
    else
    {
        w.String((char*)&x->g, 1);
    }
}

template<typename Writer>
void serialise_bool(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            w.Bool(kG(x)[i] != 0);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                w.Bool(kG(x)[idx] != 0);
            }
            w.EndArray();
        }
    }
    else
    {
        w.Bool(x->g != 0);
    }
}

template<typename Writer>
inline void emit_byte(Writer& w, unsigned char n)
{
    const char* hex = "0123456789abcdef";
    char buff[3]; // Two hex digits + null terminator

    buff[0] = hex[(n >> 4) & 0xF];
    buff[1] = hex[n & 0xF];
    buff[2] = '\0';

    w.String(buff, 2);
}

template<typename Writer>
void serialise_byte(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_byte(w, kG(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_byte(w, kG(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_byte(w, x->g);
    }
}

template<typename Writer>
inline void emit_short(Writer& w, int n)
{
    switch (n)
    {
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
void serialise_short(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_short(w, kH(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_short(w, kH(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_short(w, x->h);
    }
}

template<typename Writer>
inline void emit_int(Writer& w, int n)
{
    switch (n)
    {
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
void serialise_int(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_int(w, kI(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_int(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_int(w, x->i);
    }
}

template<typename Writer>
inline void emit_long(Writer& w, long long n)
{
    switch (n)
    {
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
void serialise_long(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_long(w, kJ(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_long(w, kJ(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_long(w, x->j);
    }
}

template<typename Writer>
inline void emit_double(Writer& w, double n)
{

    static const char* INF_STR = "Inf";
    static const char* NINF_STR = "-Inf";

    if (std::isnan(n))
    {
        w.Null();
    }
    else if (std::isinf(n))
    {
        w.String(n == INFINITY ? INF_STR : NINF_STR);
    }
    else
    {
        double intpart;
        if (std::modf(n, &intpart) == 0.0)
        {
            // Value is an integer
            if (n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max())
            {
                w.Int(static_cast<int>(n));
            }
            else if (n >= std::numeric_limits<int64_t>::min() && n <= std::numeric_limits<int64_t>::max())
            {
                w.Int64(static_cast<int64_t>(n));
            }
            else
            {
                w.Double(n);
            }
        }
        else
        {
            w.Double(n);
        }
    }
}

template<typename Writer>
void serialise_float(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_double(w, kE(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_double(w, kE(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_double(w, x->e);
    }
}

template<typename Writer>
void serialise_double(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_double(w, kF(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_double(w, kF(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_double(w, x->f);
    }
}

template<typename Writer>
inline void emit_date_custom(Writer& w, int n)
{
    if (n == ni) // Handle missing values
    {
        w.Null();
    }
    else
    {
        // Convert `n` to time since epoch and populate `tm` structure
        time_t tt = (n + 10957) * 86400; // Adjusting the value for KDB+/Q date format
        struct tm timinfo;
        gmtime_r(&tt, &timinfo);

        // Buffer large enough for the formatted date (YYYY-MM-DD) and null terminator
        char buff[12]; // YYYY-MM-DD + \0 -> 11 characters, using 12 for extra safety

        // Use snprintf to safely format the date, ensuring no buffer overflow
        int written = snprintf(buff, sizeof(buff), "%04d-%02d-%02d",
                               timinfo.tm_year + 1900,
                               timinfo.tm_mon + 1,
                               timinfo.tm_mday);

        // Ensure no truncation occurred and the output is valid
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the formatted date string
        }
        else
        {
            w.Null(); // Handle error case if formatting failed
        }
    }
}

template<typename Writer>
void serialise_date(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_date_custom(w, kI(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_date_custom(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_date_custom(w, x->i);
    }
}

template<typename Writer>
inline void emit_time_custom(Writer& w, int n)
{
    if (n == ni) // Handle missing values
    {
        w.Null();
    }
    else
    {
        // Calculate hours, minutes, seconds, and milliseconds
        int millis = n % 1000;
        int total_seconds = n / 1000;
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;
        int seconds = total_seconds % 60;

        // Use a buffer large enough for the formatted time string
        char buff[16]; // HH:MM:SS.mmm\0 (total 12 characters + null terminator)

        // Safely format the time string with snprintf
        int written = snprintf(buff, sizeof(buff), "%02d:%02d:%02d.%03d", hours, minutes, seconds, millis);

        // Check if snprintf succeeded without truncation
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the correctly formatted time string
        }
        else
        {
            w.Null(); // Handle error in case of truncation or formatting failure
        }
    }
}

template<typename Writer>
void serialise_time(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_time_custom(w, kI(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_time_custom(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_time_custom(w, x->i);
    }
}

template<typename Writer>
inline void emit_timestamp_custom(Writer& w, long long n)
{
    if (n == nj) // Handle missing values
    {
        w.Null();
    }
    else
    {
        // Constants for conversion
        constexpr long long SEC_IN_DAY = 86400LL;
        constexpr long long NANOS_IN_SEC = 1000000000LL;
        constexpr long long DAYS_1970_TO_2000 = 10957LL;

        // Calculate seconds and nanoseconds
        long long total_seconds = n / NANOS_IN_SEC;
        long long nanoseconds = n % NANOS_IN_SEC;

        // Adjust for the epoch difference (from 2000-01-01 to 1970-01-01)
        total_seconds += DAYS_1970_TO_2000 * SEC_IN_DAY;

        // Convert to time structure
        time_t tt = static_cast<time_t>(total_seconds);
        struct tm timinfo;
        gmtime_r(&tt, &timinfo);

        // Buffer large enough for the full timestamp string
        char buff[40]; // YYYY-MM-DDTHH:MM:SS.nnnnnnnnn (29 characters + null terminator)

        // Safely format the timestamp string
        int written = snprintf(buff, sizeof(buff), "%04d-%02d-%02dT%02d:%02d:%02d.%09lld",
                               timinfo.tm_year + 1900, timinfo.tm_mon + 1, timinfo.tm_mday,
                               timinfo.tm_hour, timinfo.tm_min, timinfo.tm_sec, nanoseconds);

        // Check for truncation and emit the formatted string
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the formatted timestamp
        }
        else
        {
            w.Null(); // Handle error in case of truncation or formatting failure
        }
    }
}

template<typename Writer>
void serialise_timestamp(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_timestamp_custom(w, kJ(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_timestamp_custom(w, kJ(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_timestamp_custom(w, x->j);
    }
}

template<typename Writer>
inline void emit_timespan_custom(Writer& w, long long n)
{
    if (n == nj)
    {
        w.Null();
    }
    else
    {
        // Convert nanoseconds to days, hours, minutes, seconds, and nanoseconds
        constexpr long long NANOS_IN_SEC = 1000000000LL;
        constexpr long long SECS_IN_MIN = 60LL;
        constexpr long long MINS_IN_HOUR = 60LL;
        constexpr long long HOURS_IN_DAY = 24LL;

        long long total_seconds = n / NANOS_IN_SEC;
        long long nanoseconds = n % NANOS_IN_SEC;

        // Determine sign of the timespan
        bool is_negative = false;
        if (n < 0)
        {
            is_negative = true;
            total_seconds = -total_seconds;
            nanoseconds = -nanoseconds;
        }

        // Calculate days, hours, minutes, and seconds
        long long days = total_seconds / (HOURS_IN_DAY * MINS_IN_HOUR * SECS_IN_MIN);
        long long remaining_seconds = total_seconds % (HOURS_IN_DAY * MINS_IN_HOUR * SECS_IN_MIN);
        long long hours = remaining_seconds / (MINS_IN_HOUR * SECS_IN_MIN);
        remaining_seconds %= (MINS_IN_HOUR * SECS_IN_MIN);
        long long minutes = remaining_seconds / SECS_IN_MIN;
        long long seconds = remaining_seconds % SECS_IN_MIN;

        // Format the string as "0D01:02:03.456789123"
        char buff[32]; // Enough space for the string representation
        snprintf(buff, sizeof(buff),
                 "%s%lldD%02lld:%02lld:%02lld.%09lld",
                 is_negative ? "-" : "", days, hours, minutes, seconds, nanoseconds);

        // Write to the writer
        w.String(buff, strlen(buff));
    }
}

template<typename Writer>
void serialise_timespan(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_timespan_custom(w, kJ(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_timespan_custom(w, kJ(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_timespan_custom(w, x->j);
    }
}

template<typename Writer>
inline void emit_datetime_custom(Writer& w, double n)
{
    if (std::isnan(n)) // Handle NaN (missing value)
    {
        w.Null();
    }
    else
    {
        // Convert days since 2000-01-01 to seconds since UNIX epoch
        time_t tt = static_cast<time_t>((n + 10957) * 86400);
        struct tm timinfo;
        gmtime_r(&tt, &timinfo);

        // Calculate and round milliseconds
        long millis = static_cast<long>(round((n - floor(n)) * 86400000)) % 1000;

        // Buffer large enough for full datetime string
        char buff[30]; // YYYY-MM-DDTHH:MM:SS.mmm (23 characters + null terminator)

        // Format the datetime string safely
        int written = snprintf(buff, sizeof(buff),
                               "%04d-%02d-%02dT%02d:%02d:%02d.%03ld",
                               timinfo.tm_year + 1900, timinfo.tm_mon + 1, timinfo.tm_mday,
                               timinfo.tm_hour, timinfo.tm_min, timinfo.tm_sec, millis);

        // Check for truncation and emit the string
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the correctly formatted datetime
        }
        else
        {
            w.Null(); // Handle error if truncation or formatting failed
        }
    }
}

template<typename Writer>
void serialise_datetime(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_datetime_custom(w, kF(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_datetime_custom(w, kF(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_datetime_custom(w, x->f);
    }
}

template<typename Writer>
inline void emit_month_custom(Writer& w, const int n)
{
    if (n == ni) // Handle missing value
    {
        w.Null();
    }
    else
    {
        // Calculate year and month
        int year = n / 12 + 2000;
        int month = n % 12 + 1;

        // Buffer large enough for YYYY-MM format
        char buff[8]; // YYYY-MM (7 characters + null terminator)

        // Safely format the month string
        int written = snprintf(buff, sizeof(buff), "%04d-%02d", year, month);

        // Check for truncation and emit the string
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the formatted year-month string
        }
        else
        {
            w.Null(); // Handle error if truncation or formatting failed
        }
    }
}

template<typename Writer>
void serialise_month(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_month_custom(w, kI(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_month_custom(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_month_custom(w, x->i);
    }
}

template<typename Writer>
inline void emit_minute_custom(Writer& w, const int n)
{
    if (n == ni) // Handle missing value
    {
        w.Null();
    }
    else
    {
        // Calculate hours and minutes
        int hours = n / 60;
        int minutes = n % 60;

        // Buffer large enough for HH:MM format
        char buff[6]; // HH:MM (5 characters + null terminator)

        // Safely format the time string
        int written = snprintf(buff, sizeof(buff), "%02d:%02d", hours, minutes);

        // Check for truncation and emit the string
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the formatted time string
        }
        else
        {
            w.Null(); // Handle error if truncation or formatting failed
        }
    }
}

template<typename Writer>
void serialise_minute(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_minute_custom(w, kI(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_minute_custom(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_minute_custom(w, x->i);
    }
}

template<typename Writer>
inline void emit_second_custom(Writer& w, const int n)
{
    if (n == ni) // Handle missing value
    {
        w.Null();
    }
    else
    {
        // Calculate hours, minutes, and seconds
        int hours = n / 3600;
        int minutes = (n % 3600) / 60;
        int seconds = n % 60;

        // Buffer large enough for HH:MM:SS format
        char buff[9]; // HH:MM:SS (8 characters + null terminator)

        // Safely format the time string
        int written = snprintf(buff, sizeof(buff), "%02d:%02d:%02d", hours, minutes, seconds);

        // Check for truncation and emit the string
        if (written > 0 && written < sizeof(buff))
        {
            w.String(buff, written); // Emit the formatted time string
        }
        else
        {
            w.Null(); // Handle error if truncation or formatting failed
        }
    }
}

template<typename Writer>
void serialise_second(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            emit_second_custom(w, kI(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_second_custom(w, kI(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_second_custom(w, x->i);
    }
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
    if (isvec)
    {
        if (i >= 0)
        {
            emit_guid_custom(w, kU(x)[i]);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                emit_guid_custom(w, kU(x)[idx]);
            }
            w.EndArray();
        }
    }
    else
    {
        emit_guid_custom(w, kU(x)[0]);
    }
}

template<typename Writer>
void serialise_dict(Writer& w, K x, bool isvec, int i)
{
    const K keys = kK(x)[0];
    const K values = kK(x)[1];

    if (keys->t == XT && values->t == XT)
    {
        serialise_keyed_table(w, keys, values);
    }
    else
    {
        w.StartObject();
        for (int idx = 0; idx < keys->n; idx++)
        {
            serialise_atom(w, keys, idx);
            serialise_atom(w, values, idx);
        }
        w.EndObject();
    }
}

template<typename Writer>
void serialise_list(Writer& w, K x, bool isvec, int i)
{
    if (isvec)
    {
        if (i >= 0)
        {
            K v = kK(x)[i];
            serialise_atom(w, v);
        }
        else
        {
            w.StartArray();
            for (int idx = 0; idx < x->n; idx++)
            {
                K v = kK(x)[idx];
                serialise_atom(w, v);
            }
            w.EndArray();
        }
    }
    else
    {
        w.Null();
    }
}

template<typename Writer>
void serialise_table(Writer& w, K x, bool isvec, int i)
{
    const K dict = x->k;
    const K keys = kK(dict)[0];
    const K values = kK(dict)[1];

    if (i >= 0)
    {
        w.StartObject();
        for (int col = 0; col < keys->n; col++)
        {
            serialise_atom(w, keys, col);
            serialise_atom(w, kK(values)[col], i);
        }
        w.EndObject();
    }
    else
    {
        const int rows = kK(values)[0]->n;

        w.StartArray();
        for (int row = 0; row < rows; row++)
        {
            w.StartObject();
            for (int col = 0; col < keys->n; col++)
            {
                serialise_atom(w, keys, col);
                serialise_atom(w, kK(values)[col], row);
            }
            w.EndObject();
        }
        w.EndArray();
    }
}

// Template function to serialize K objects (atoms, lists, dictionaries, etc.)
template<typename Writer>
void serialise_atom(Writer& w, const K x, int i)
{
    bool isvec = x->t >= 0;

    switch (x->t)
    {
        case 0:
            serialise_list(w, x, isvec, i);
            break;
        case KS:
        case -KS:
            serialise_sym(w, x, isvec, i);
            break;
        case KC:
        case -KC:
            serialise_char(w, x, isvec, i);
            break;
        case KB:
        case -KB:
            serialise_bool(w, x, isvec, i);
            break;
        case KG:
        case -KG:
            serialise_byte(w, x, isvec, i);
            break;
        case KH:
        case -KH:
            serialise_short(w, x, isvec, i);
            break;
        case KI:
        case -KI:
            serialise_int(w, x, isvec, i);
            break;
        case KT:
        case -KT:
            serialise_time(w, x, isvec, i);
            break;
        case KN:
        case -KN:
            serialise_timespan(w, x, isvec, i);
            break;
        case KP:
        case -KP:
            serialise_timestamp(w, x, isvec, i);
            break;
        case KZ:
        case -KZ:
            serialise_datetime(w, x, isvec, i);
            break;
        case KD:
        case -KD:
            serialise_date(w, x, isvec, i);
            break;
        case KJ:
        case -KJ:
            serialise_long(w, x, isvec, i);
            break;
        case UU:
        case -UU:
            serialise_guid(w, x, isvec, i);
            break;
        case KE:
        case -KE:
            serialise_float(w, x, isvec, i);
            break;
        case KM:
        case -KM:
            serialise_month(w, x, isvec, i);
            break;
        case KF:
        case -KF:
            serialise_double(w, x, isvec, i);
            break;
        case KU:
        case -KU:
            serialise_minute(w, x, isvec, i); 
	        break;
        case KV:
        case -KV:
            serialise_second(w, x, isvec, i); 
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


} // namespace kjson

extern "C" {

// Helper function to handle parsing errors
K handle_parse_error(const rapidjson::Document& document) {
    std::string errMsg = std::string("Parse error: ") + GetParseError_En(document.GetParseError()) +
                         " at offset " + std::to_string(document.GetErrorOffset());
    return krr((S)errMsg.c_str());
}

// KDB+ interface function: JSON to K (General)
K jtok(K json_string) {
    if (json_string->t != KC) {
        return krr((S)"Type error: Input must be a char vector (string)");
    }

    // Parse the JSON string
    rapidjson::Document document;
    document.Parse((const char*)kC(json_string), json_string->n);

    if (document.HasParseError()) {
        return handle_parse_error(document);
    }

    // Convert JSON value to K object
    return kjson::json_to_kobject(document);
}

// KDB+ interface function: K object to JSON
K ktoj(K x)
{
    // Use RapidJSON's StringBuffer and Writer for serialization
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    // Set decimal precision to 5 places (optional)
    writer.SetMaxDecimalPlaces(5);

    // Serialize the K object
    kjson::serialise_atom(writer, x);  // Assuming serialise_atom exists in the kjson namespace

    // Get the size of the buffer
    size_t len = buffer.GetSize();
    const char* str = buffer.GetString();

    // Marshal to KDB+, passing both the string and its length for safety
    return kpn((S)str, len);  // Use kpn to ensure proper string length is handled
}

} // extern "C"
