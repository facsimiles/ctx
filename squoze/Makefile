all: squoze squoze-test \
	test-32-utf5-intern \
	test-32-utf5-intern-embed \
	test-32-utf8-intern \
	test-32-utf8-intern-embed \
	test-62-utf5-intern \
	test-62-utf5-intern-embed \
	test-52-utf5-intern \
	test-52-utf5-intern-embed \
	test-64-utf8-intern \
	test-64-utf8-intern-embed \
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

TEST_COMMON_FLAGS=-Wall -O2

test-32-utf5-intern: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=1 \
		                          -DSQUOZE_ID_BITS=32 \
					  -DSQUOZE_EMBEDDED_UTF5=1
test-32-utf5-intern-embed: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=0 \
		                          -DSQUOZE_ID_BITS=32 \
					  -DSQUOZE_EMBEDDED_UTF5=1
test-32-utf8-intern: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=1 \
		                          -DSQUOZE_ID_BITS=32 \
					  -DSQUOZE_EMBEDDED_UTF5=0
test-32-utf8-intern-embed: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=0 \
		                          -DSQUOZE_ID_BITS=32 \
					  -DSQUOZE_EMBEDDED_UTF5=0
test-52-utf5-intern: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=1 \
		                          -DSQUOZE_ID_BITS=52 \
					  -DSQUOZE_EMBEDDED_UTF5=1
test-52-utf5-intern-embed: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=0 \
		                          -DSQUOZE_ID_BITS=52 \
					  -DSQUOZE_EMBEDDED_UTF5=1
test-52-utf8-intern: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=1 \
		                          -DSQUOZE_ID_BITS=52 \
					  -DSQUOZE_EMBEDDED_UTF5=0
test-52-utf8-intern-embed: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=0 \
		                          -DSQUOZE_ID_BITS=52 \
					  -DSQUOZE_EMBEDDED_UTF5=0

test-64-utf8-intern-embed: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=0 \
		                          -DSQUOZE_ID_BITS=64 \
					  -DSQUOZE_EMBEDDED_UTF5=0
test-64-utf8-intern: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=1 \
		                          -DSQUOZE_ID_BITS=64 \
					  -DSQUOZE_EMBEDDED_UTF5=0

test-62-utf5-intern-embed: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=0 \
		                          -DSQUOZE_ID_BITS=62 \
					  -DSQUOZE_EMBEDDED_UTF5=1
test-62-utf5-intern: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) -DSQUOZE_ALWAYS_INTERN=1 \
		                          -DSQUOZE_ID_BITS=62 \
					  -DSQUOZE_EMBEDDED_UTF5=1

report.txt: squoze gen-report.sh
	./gen-report.sh > report.txt
index.html: all
	rm index.html || true
	./test-head >> index.html
	for a in 4 6 8 10 12 23;do export MAX_WORD_LEN=$$a;make report-part.html;done 
	cat extra-index.html >> index.html || true
	./test-foot >> index.html
report-part.html: all
	./test-header               >> index.html
	./test-32-utf8-intern       >> index.html
	./test-64-utf8-intern-embed >> index.html
	./test-32-utf8-intern-embed >> index.html
	./test-32-utf5-intern-embed >> index.html
	./test-52-utf5-intern-embed >> index.html
	./test-62-utf5-intern-embed >> index.html
	./test-32-utf5-intern       >> index.html
	./test-52-utf5-intern       >> index.html
	./test-62-utf5-intern       >> index.html
	./test-footer               >> index.html
report-32bit.html: all
	rm report-32bit.html || true
	./test-head >> report-32bit.html
	for a in 4 6 8 10 12 23;do export MAX_WORD_LEN=$$a;make report-part-32bit.html;done 
	./test-foot >> report-32bit.html
report-part-32bit.html: all
	./test-header               >> report-32bit.html
	./test-32-utf8-intern       >> report-32bit.html
	./test-32-utf8-intern-embed >> report-32bit.html
	./test-32-utf5-intern-embed >> report-32bit.html
	./test-32-utf5-intern       >> report-32bit.html
	./test-footer               >> report-32bit.html

	#sh -c 'diff -u reference.txt report.txt'
update-reference: report.txt
	cp report.txt reference.txt
