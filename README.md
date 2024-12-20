# KJSON Library for KDB+ Integration

This library provides tools to convert JSON data to KDB+ objects and vice versa, facilitating seamless integration between JSON and KDB+ data. The library uses RapidJSON to parse and serialize JSON and is built to work efficiently with KDB+ native data types. The library took as a starting point the [qrapidjson](https://github.com/lmartinking/qrapidjson) library which provided a means of converting K objects to JSON. 
This implementation was built using chatGPT o1-preview. It has been tested against the K objects given in the unittests.q file. 
In terms of performance, tests show the converters to be 2-4x faster than native .j.j and .j.k functions. 


## Features
- Convert JSON strings to KDB+ objects.
- Serialize KDB+ objects to JSON strings.
- Support for various KDB+ data types, including lists, dictionaries, tables, timestamps, and more.
- Utilizes RapidJSON for fast and efficient parsing and serialization.

## Requirements
- **KDB+**: Version 4.1 or higher.
- **RapidJSON**: Included as a dependency for parsing and serialization.
- **C++ Compiler**: Requires a C++11 or higher compatible compiler.

**Note:** While the library has been tested against the K objects given in the unittests.q file, it may still contain bugs or limitations. Users are encouraged to thoroughly test in their specific environments before deploying to production.- Operating System: Linux (tested on Ubuntu)

## Tested Environment
- **KDB+ Version**: 4.1
- **Operating System**: Linux (tested on Ubuntu)
- **Compiler**: GCC version supporting C++20 standard

## Building the Library
1. Clone the repository:
   ```sh
   git clone https://github.com/jlucid/kjson.git
   cd kjson
   ```
2. Compile the shared library using the provided Makefile:
   ```sh
   make
   ```
   Alternatively, you can compile manually using:
   ```sh
   g++ -std=c++20 -O3 -DNDEBUG -fPIC -I. -DKXVER=3 json_serialisation.cpp kjson_utils.cpp -o kjson.so -shared
   ```

## Usage
1. Load the compiled shared library (`kjson.so`) into your KDB+ process:
   ```q
   \ Load the shared library into KDB+
   libpath:`:kjson  
   ktoj:libpath 2:(`ktoj;1)
   jtok:libpath 2:(`jtok;1)
   ```
2. Example usage in KDB+:
   ```q
    a:.j.k .j.j (([] sym:`a`b;p:1 2);([] rv:1 2;g:3 4;s:10b))
    b:jtok ktoj (([] sym:`a`b;p:1 2);([] rv:1 2;g:3 4;s:10b))
    a~b
    1b
   ```

## License
This project is licensed under the GPL 3.0 License. 


