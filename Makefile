cpputest: misc/cpputest/cpputest_build/lib/libCppUTest.a
misc/cpputest/cpputest_build/lib/libCppUTest.a:
	cd "misc/cpputest/cpputest_build"; \
	autoreconf .. -i; \
	../configure; \
	make
