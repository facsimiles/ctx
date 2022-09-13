all: squoze
clean:
	rm -f squoze squoze-test
squoze: squoze.c squoze.h Makefile
	gcc -Wall squoze.c -o squoze -O2 -g
squoze-test: squoze-test.c squoze.h Makefile
	gcc -Wall squoze-test.c -o squoze-test -g
report.txt: squoze gen-report.sh
	./gen-report.sh > report.txt
test: report.txt squoze-test
	./squoze-test
	sh -c 'diff -u reference.txt report.txt'
	wc *.txt
update-reference: report.txt
	cp report.txt reference.txt
