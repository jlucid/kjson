/ Load your functions
libpath:`:kjson
ktoj:libpath 2:(`ktoj;1)
jtok:libpath 2:(`jtok;1)

/ Initialize the lists as general lists
objects:enlist  ()      / empty general list
description:enlist ()  / empty general list

/ Simple Atoms
objects,:  42;                               description,:  "Integer Atom"
objects,:  3.14159;                          description,:  "Float Atom"
objects,:  1b;                               description,:  "Boolean Atom"
objects,:  "q";                              description,:  "Char Atom"
objects,:  `hello;                           description,:  "Symbol Atom"
objects,:  "Hello, world!";                  description,:  "String (Char Vector)"
objects,:  2021.09.15;                       description,:  "Date Atom"
objects,:  12:34:56.789;                     description,:  "Time Atom"
objects,:  2021.09.15D12:34:56.789;          description,:  "Timestamp Atom"
objects,:  "G"$"0a4c8e56-1f0b-4e19-9eab-5f61f5c88f0a"; description,:  "GUID Atom"

/ Lists
objects,:  (1;2;3;4;5);                      description,:  "Integer List"
objects,:  (1.1;2.2;3.3);                    description,:  "Float List"
objects,:  (0b;1b;0b);                       description,:  "Boolean List"
objects,:  `foo`bar`baz;                     description,:  "Symbol List"
objects,:  (1;"two";3.0);                    description,:  "Mixed List"

/ Dictionaries
objects,:enlist  `a`b!(10.0;20.01);                description,:  "Dictionary with Floats"
objects,:enlist  `a`b!(0b;1b);                     description,:  "Booleans"
objects,:enlist  `a`b`c!(1;2;3);                   description,:  "Simple Dictionary"
objects,:enlist  `int`float`bool!(42;3.14;1b);     description,:  "Dictionary with Mixed Types"
objects,:enlist  `nums`letters!(1 2 3;"abc");      description,:  "Dictionary with Lists"

/ Tables
objects,:enlist  ([] a:1 2 3; b:"abc");            description,:  "Simple Table"
objects,:enlist  ([] int:1 2 3; float:1.1 2.2 3.3; sym:`x`y`z); description,:  "Table with Various Column Types"

/ Nested Structures
objects,:enlist  `outer`inner!( `a`b!(1;2);3); description,:  "Nested Dictionary"
objects,:enlist  ((`a`b!(1;2));(`a`b!(3;4)));      description,:  "List of Dictionaries"

/ Edge Cases
objects,:  0N;                               description,:  "Null Value"

/ Timespan
objects,:  0D15:00:31.539282370;               description,:  "Timespan Atom"
ktojCheck:{[x;y]
  $[(ktoj x)~.j.j x; show "K to JSON - Passed: ", y;[show "Failed: ", y;0N! (.j.j x;ktoj x)]]
 }

jtokCheck:{[x;y]
  $[(jtok ktoj x)~.j.k .j.j x; show "JSON to K - Passed: ", y;[show "Failed: ", y;0N! (.j.k .j.j x;jtok ktoj x)]]
 }

objects:1 _ objects
description:1 _ description

ktojCheck[;]'[objects; description]
jtokCheck[;]'[objects; description]
