all: squoze squoze-test sqz-test index.html
clean:
	rm -f squoze squoze-test sqz-test test*
squoze: squoze.c squoze.h Makefile
	gcc -Wall squoze.c -o squoze -O2 -g
sqz-test: sqz-test.c squoze.h Makefile
	gcc -Wall sqz-test.c -o sqz-test -g
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

TEST_COMMON_FLAGS=-Wall -O3

test-squoze32-utf5: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) \
		-DSQUOZE_ID_BITS=32 \
		-DSQUOZE_ID_UTF5=1

# this is the same as using just fallback/murmurhash
test-murmurhash: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) \
		-DSQUOZE_ID_BITS=32 \
		-DSQUOZE_ID_UTF5=0 \
		-DSQUOZE_ID_MURMUR=1
test-squoze32-utf8: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) \
		-DSQUOZE_ID_BITS=32 \
		-DSQUOZE_ID_UTF5=0
test-squoze52-utf5: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) \
		-DSQUOZE_ID_BITS=52 \
		-DSQUOZE_ID_UTF5=1

test-squoze64-utf8: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) \
		-DSQUOZE_ID_BITS=64 \
		-DSQUOZE_ID_UTF5=0

test-squoze62-utf5: squoze-test.c squoze.h Makefile
	gcc $< -o $@ $(TEST_COMMON_FLAGS) \
		-DSQUOZE_ID_BITS=62 \
		-DSQUOZE_ID_UTF5=1

report.txt: squoze gen-report.sh
	./gen-report.sh > report.txt
index.html: \
	test-squoze32-utf5 \
	test-squoze62-utf5 \
	test-squoze52-utf5 \
	test-murmurhash    \
	test-squoze32-utf8 \
	test-squoze64-utf8 \
	test-head          \
	test-foot          \
	test-header        \
	test-footer
	rm index.html || true
	./test-head >> index.html
	for a in 3 4 6 8 10 12 23;do export MAX_WORD_LEN=$$a;make report-part.html;done 
	echo "<h3>32bit ARM benchmarks on a RPI3</h3>" >> index.html
	echo "<p>As large words do not embed in 32bit, but 4-6 characters still covers many of the most common words in english, as well as many common keyword tokens in programming.</p>" >> index.html
	cat extra-index.html >> index.html || true
	./test-foot >> index.html
report-part.html: all
	./test-header        >> index.html
	./test-murmurhash    >> index.html
	./test-squoze64-utf8 >> index.html
	./test-squoze32-utf8 >> index.html
	./test-squoze32-utf5 >> index.html
	./test-squoze52-utf5 >> index.html
	./test-squoze62-utf5 >> index.html
	./test-footer        >> index.html
report-32bit.html:         \
	test-squoze32-utf5 \
	test-murmurhash    \
	test-squoze32-utf8 \
	test-head          \
	test-foot          \
	test-header        \
	test-footer
	rm report-32bit.html || true
	for a in 3 4 8 10 12;do export MAX_WORD_LEN=$$a;make report-part-32bit.html;done 
report-part-32bit.html:
	./test-header         >> report-32bit.html
	./test-murmurhash     >> report-32bit.html
	./test-squoze32-utf8  >> report-32bit.html
	./test-squoze32-utf5  >> report-32bit.html
	./test-footer         >> report-32bit.html



update-reference: report.txt
	cp report.txt reference.txt
rpi-cp:
	scp squoze*.[ch] Makefile pi@raspberrypi:squoze
rpi-test:
	ssh pi@raspberrypi make -C squoze report-32bit.html -j5
rpi-get:
	scp pi@raspberrypi:squoze/report-32bit.html extra-index.html
README: index.html
	w3m -dump index.html > README
updateweb: ../squoze.tar.xz
	cp index.html squoze.h ../squoze.tar.xz ~/squoze.org
	syncsquoze
../squoze.tar.xz: squoze.c squoze.h Makefile squoze-test.c
	(cd ..;tar cvf squoze.tar.xz squoze/squoze.[ch]\
                                     squoze/Makefile \
                                     squoze/squoze-test.c )
