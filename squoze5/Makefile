all: squoze5 squoze5-test
clean:
	rm -f squoze5 squoze5-test
squoze5: squoze5.c squoze5.h Makefile
	gcc -Wall squoze5.c -o squoze5 -O2 -g
squoze5-test: squoze5-test.c squoze5.h Makefile
	gcc -Wall squoze5-test.c -O2 -o squoze5-test
report.txt: squoze5 gen-report.sh
	./gen-report.sh > report.txt
test: squoze5-test report.txt
	./squoze5-test
	sh -c 'diff -u reference.txt report.txt'
	wc *.txt
update-reference: report.txt
	cp report.txt reference.txt
