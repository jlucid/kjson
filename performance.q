/ Load your functions
libpath: `:kjson
ktoj: libpath 2:(`ktoj;1)
jtok: libpath 2:(`jtok;1)

input:`a`b`c`d`e`f`g!(.z.p;10.0 20.0;`oa`op`sd;.z.d;.z.p;("hjk";"ghmb";"ghj");([] sym:`aa`bb`cc;price:10 20 30f;size:100 200 300;flag:101b))
output:.j.j input

show "Running .j.j to input"
\ts:100000 .j.j input
show "Running ktoj to input"
\ts:100000 ktoj input
show "Running .j.k on json"
\ts:100000 .j.k output
show "Running jtok on json"
\ts:100000 jtok output
