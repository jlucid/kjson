/* File: kjson_utils.cpp */

#include "kjson_utils.h"
#include "k.h" // Include the kdb+ header
#include <cstring> // For strcmp, memcpy
#include <cstdio> // For snprintf, sprintf

namespace kjson {

K json_to_kobject_dict(const rapidjson::Value& value)
{
    if (value.IsNull())
    {
        return ktn(0, 0); // Return an empty general list
    }
    else if (value.IsObject())
    {
        // Create a dictionary
        rapidjson::SizeType memberCount = value.MemberCount();
        K keys = ktn(KS, memberCount);   // Create symbol list for keys

        // Preliminary pass to determine the type of the values list
        bool allFloats = true;
        bool allBooleans = true;

        for (rapidjson::Value::ConstMemberIterator itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr)
        {
            if (!itr->value.IsNumber())
            {
                allFloats = false;
            }
            if (!itr->value.IsBool())
            {
                allBooleans = false;
            }
        }

        K valuesList = nullptr;
        if (allFloats)
        {
            valuesList = ktn(KF, memberCount); // Homogeneous float list
        }
        else if (allBooleans)
        {
            valuesList = ktn(KB, memberCount); // Homogeneous boolean list
        }
        else
        {
            valuesList = ktn(0, memberCount); // General list
        }

        if (!keys || !valuesList)
        {
            printf("Failed to allocate keys or values list.\n");
            if (keys) r0(keys);
            if (valuesList) r0(valuesList);
            return krr((S)"Memory allocation error: Unable to create keys or values list");
        }

        rapidjson::SizeType idx = 0;
        for (rapidjson::Value::ConstMemberIterator itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr)
        {
            // Assign the key directly to the symbol list
            kS(keys)[idx] = ss((S)itr->name.GetString());

            // Convert JSON value to K object
            if (allFloats && itr->value.IsNumber())
            {
                kF(valuesList)[idx] = itr->value.GetDouble(); // Assign float value directly
            }
            else if (allBooleans && itr->value.IsBool())
            {
                kG(valuesList)[idx] = itr->value.GetBool(); // Assign boolean value directly
            }
            else
            {
                // Mixed list scenario - convert value using json_to_kobject
                K v = json_to_kobject(itr->value);
                if (!v)
                {
                    printf("Failed to convert value for key: %s\n", itr->name.GetString());
                    r0(keys);
                    r0(valuesList);
                    return krr((S)"Type error: Failed to convert value");
                }
                kK(valuesList)[idx] = v; // Assign value to general list
            }

            idx++;
        }

        K dict = xD(keys, valuesList); // Create the dictionary
        if (!dict)
        {
            printf("Failed to create dictionary.\n");
            r0(keys);
            r0(valuesList);
            return krr((S)"Memory allocation error: Unable to create dictionary");
        }

        return dict;
    }
    else
    {
        return krr((S)"Type error: Input must be a JSON object for a dictionary");
    }
}

K json_to_kobject(const rapidjson::Value& value)
{
    if (value.IsNull())
    {
        return kf(nf); // KDB+ null
    }
    else if (value.IsBool())
    {
        return kb(value.GetBool()); // Boolean atom
    }
    else if (value.IsNumber())
    {
        return kf(value.GetDouble()); // Float atom
    }
    else if (value.IsString())
    {
        return kp((S)value.GetString()); // Character vector
    }
    else if (value.IsArray())
    {
        const rapidjson::SizeType size = value.Size();
        if (size == 0)
        {
            return ktn(0, 0); // Empty general list
        }

        // Create a general list and populate it with elements
        K list = ktn(0, size);
        for (rapidjson::SizeType i = 0; i < size; ++i)
        {
            K elem = json_to_kobject(value[i]);
            if (!elem)
            {
                r0(list);
                return krr((S)"Type error: Failed to convert list element");
            }
            kK(list)[i] = elem;
        }

        // Use vk to collapse the list if it is homogeneous
        return vk(list);
    }
    else if (value.IsObject())
    {
        return json_to_kobject_dict(value); // Convert object to dictionary
    }
    return krr((S)"Unsupported JSON type");
}

} // namespace kjson
