all: squoze squoze-test \
	test-32-utf5-intern \
	test-32-utf5-intern-embed \
	test-32-utf8-intern \
	test-32-utf8-intern-embed \
	test-62-utf5-intern \
	test-62-utf5-intern-embed \
	test-62-utf8-intern \
	test-62-utf8-intern-embed \
	test-head \
	test-foot \
	test-header \
	test-footer
clean:
	rm -f squoze squoze-test
squoze: squoze.c squoze.h Makefile
	gcc -Wall squoze.c -o squoze -O2 -g
squoze-test: squoze-test.c squoze.h Makefile
	gcc -Wall squoze-test.c -o squoze-test -g
test-head: squoze-test.c squoze.h Makefile squoze-test.c
	gcc -Wall $< -o $@ -O3 -DHEAD=1
test-foot: squoze-test.c squoze.h Makefile squoze-test.c
	gcc -Wall $< -o $@ -O3 -DFOOT=1
test-header: squoze-test.c squoze.h Makefile squoze-test.c
	gcc -Wall $< -o $@ -O3 -DHEADER=1
test-footer: squoze-test.c squoze.h Makefile squoze-test.c
	gcc -Wall $< -o $@ -O3 -DFOOTER=1

test-32-utf5-intern: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=1 -DSQUOZE_ID_BITS=32 -DSQUOZE_EMBEDDED_UTF5=1
test-32-utf5-intern-embed: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=0 -DSQUOZE_ID_BITS=32 -DSQUOZE_EMBEDDED_UTF5=1
test-32-utf8-intern: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=1 -DSQUOZE_ID_BITS=32 -DSQUOZE_EMBEDDED_UTF5=0
test-32-utf8-intern-embed: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=0 -DSQUOZE_ID_BITS=32 -DSQUOZE_EMBEDDED_UTF5=0
test-52-utf5-intern: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=1 -DSQUOZE_ID_BITS=52 -DSQUOZE_EMBEDDED_UTF5=1
test-52-utf5-intern-embed: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=0 -DSQUOZE_ID_BITS=52 -DSQUOZE_EMBEDDED_UTF5=1
test-52-utf8-intern: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=1 -DSQUOZE_ID_BITS=52 -DSQUOZE_EMBEDDED_UTF5=0
test-52-utf8-intern-embed: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=0 -DSQUOZE_ID_BITS=52 -DSQUOZE_EMBEDDED_UTF5=0

test-62-utf8-intern-embed: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=0 -DSQUOZE_ID_BITS=64 -DSQUOZE_EMBEDDED_UTF5=0
test-62-utf5-intern-embed: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=0 -DSQUOZE_ID_BITS=64 -DSQUOZE_EMBEDDED_UTF5=1
test-62-utf5-intern: squoze-test.c squoze.h Makefile
	gcc -Wall $< -o $@ -O3 -DSQUOZE_ALWAYS_INTERN=1 -DSQUOZE_ID_BITS=64 -DSQUOZE_EMBEDDED_UTF5=1

report.txt: squoze gen-report.sh
	./gen-report.sh > report.txt
report.html: all
	rm report.html || true
	./test-head >> report.html
	for a in 7 9 12 21;do export MAX_WORD_LEN=$$a;make report-part.html;done 
	./test-foot >> report.html
report-part.html: all
	./test-header >> report.html
	./test-62-utf8-intern-embed >> report.html
	./test-62-utf5-intern-embed  >> report.html
	./test-32-utf5-intern-embed  >> report.html
	./test-32-utf8-intern  >>report.html
	./test-32-utf5-intern  >> report.html
	./test-62-utf5-intern >> report.html
	./test-footer >> report.html

	#sh -c 'diff -u reference.txt report.txt'
update-reference: report.txt
	cp report.txt reference.txt
