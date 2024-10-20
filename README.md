# KJSON Library for KDB+ Integration

This library provides tools to convert JSON data to KDB+ objects and vice versa, facilitating seamless integration between JSON and KDB+ data. The library uses RapidJSON to parse and serialize JSON and is built to work efficiently with KDB+ native data types.

## Features
- Convert JSON strings to KDB+ objects.
- Serialize KDB+ objects to JSON strings.
- Support for various KDB+ data types, including lists, dictionaries, tables, timestamps, and more.
- Utilizes RapidJSON for fast and efficient parsing and serialization.

## Requirements
- **KDB+**: Version 4.1 or higher.
- **RapidJSON**: Included as a dependency for parsing and serialization.
- **C++ Compiler**: Requires a C++20-compatible compiler.

## Tested Environment
- **KDB+ Version**: 4.1
- **Operating System**: Linux (tested on Ubuntu)
- **Compiler**: GCC version supporting C++20 standard

## Building the Library
1. Clone the repository:
   ```sh
   git clone <repository_url>
   cd <repository_folder>
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
   system "l kjson.so"
   ktoj: `libc` (`ktoj; 1)
   jtok: `libc` (`jtok; 1)
   ```
2. Example usage in KDB+:
   ```q
   jsonString: "{"a":1,"b":2}"
   result: jtok jsonString
   ```

## License
This project is licensed under the MIT License.

## Contact
For any questions or contributions, please contact the repository owner at: [your email address]

