/ Load your functions
libpath: `:kjson
ktoj: libpath 2:(`ktoj;1)
jtok: libpath 2:(`jtok;1)

/ Initialize the lists as general lists
objects: enlist ();                           / List to hold objects
description: enlist "Empty list";             / List to hold descriptions

/ Simple Atoms
objects,: 0N;                                  description,: "Null Value"
objects,: `byte$1234;                          description,: "Byte Value"
objects,: 42i;                                 description,: "Integer Atom"
objects,: 2e;                                  description,: "Real Atom"
objects,: 12j;                                 description,: "Long Atom"
objects,: 3h;                                  description,: "Short Atom"
objects,: 3.14159;                             description,: "Float Atom"
objects,: 3.0;                                 description,: "Float Atom, drop decimals if .0"
objects,: 1b;                                  description,: "Boolean Atom"
objects,: "q";                                 description,: "Char Atom"
objects,: `hello;                              description,: "Symbol Atom"
objects,: "Hello, world!";                     description,: "String (Char Vector)"
objects,: 2021.09.15;                          description,: "Date Atom"
objects,: 2024.10m;                            description,: "Month Atom"
objects,: 12:34:56.789;                        description,: "Time Atom"
objects,: 12:10;                               description,: "Minute Atom"
objects,: 00:00:10;                            description,: "Second Atom"
objects,: 2021.09.15D12:34:56.789;             description,: "Timestamp Atom"
objects,: 2024.10.21T19:24:23.353;             description,: "Datetime Atom"
objects,: "G"$"0a4c8e56-1f0b-4e19-9eab-5f61f5c88f0a"; description,: "GUID Atom"
objects,: 0D15:00:31.539282370;                description,: "Timespan Atom"

/ Lists
objects,: (1;2;3;4;5);                         description,: "Integer List"
objects,: (1.1;2.2;3.3);                       description,: "Float List"
objects,: (0b;1b;0b);                          description,: "Boolean List"
objects,: `foo`bar`baz;                        description,: "Symbol List"
objects,: (1;"two";3.0);                       description,: "Mixed List"
objects,: enlist ((1;2;1b);(`a;"hhj";.z.p));   description,: "List of Lists"

/ Dictionaries
objects,: enlist `a`b!(10.0;20.01);            description,: "Dictionary with Floats"
objects,: enlist `a`b!(0b;1b);                 description,: "Dictionary with Booleans"
objects,: enlist `int`float`bool!(42;3.14;1b); description,: "Dictionary with Mixed Types"
objects,: enlist `a`b`c!(1 2 3f;3 4 5j;101b);  description,: "Dictionary with lists of different types"
objects,: enlist `nums`letters!(1 2 3;"abc");  description,: "Dictionary with Lists 1"
objects,: enlist `a`b`c!((1;2;3);12345;("as";"hj")); description,: "Dictionary with Lists 2"

/ Tables
objects,: enlist ([] a:1 2 3; b:"abc");                         description,: "Simple Table"
objects,: enlist ([] int:1 2 3; float:1.1 2.2 3.3; sym:`x`y`z); description,: "Table with Various Column Types"
objects,: enlist ([int:1 2 3]; float:1.1 2.2 3.3; sym:`x`y`z);  description,: "Keyed Table"

/ Nested Structures
objects,: enlist `outer`inner!( `a`b!(1;2);3); description,: "Nested Dictionary"
objects,: enlist ((`a`b!(1;2));(`a`b!(3;4)));  description,: "List of Dictionaries"

/ Enumeration
sym:`a`b`c
y:`a`b`c`b`a`b`c`c`c`c`c`c`c
s:`sym$y
objects,:s;                                    description,:"Enumerations"

/ Known failure: infinity not supported in ktoj
// objects,: -0w 0 1 2 3 0w;                   description,: "List containing plus and minus infinity"

/ JSON conversion checks

/ Check K to JSON conversion
ktojCheck:{[x;y]
  $[(ktoj x) ~ .j.j x;
    show "K to JSON - Passed: ", y;
    [show "Failed: ", y; 0N! (.j.j x; ktoj x)]]
 }

/ Check JSON to K conversion
jtokCheck:{[x;y]
  $[(jtok ktoj x) ~ .j.k .j.j x;
    show "JSON to K - Passed: ", y;
    [show "Failed: ", y; 0N! (.j.k .j.j x; jtok ktoj x)]]
 }

/ Run checks on all objects
ktojCheck[;]'[objects; description]
jtokCheck[;]'[objects; description]
