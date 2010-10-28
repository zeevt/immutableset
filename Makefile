ifeq ($(CC), icc)
	GPP = icpc
	CFLAGS = -std=c99 -Wall -g -gdwarf-2 -pipe $(OPT_FLAGS)
	CXXFLAGS = -std=c++0x -Wall -g -gdwarf-2 -pipe -fno-rtti -fno-exceptions -fvisibility=hidden $(OPT_FLAGS)
	LDFLAGS = -Wl,-O1 -Wl,--as-needed
	PGO_GEN = -prof-gen -O3
	PGO_USE = -prof-use -O3
	OPT_FLAGS = -DNDEBUG -O2 -mssse3 -mtune=core2 -ansi-alias
	DEBUG_FLAGS =
else ifeq ($(CC), clang)
	GPP = clang++
	CFLAGS = -std=c99 -Wall -pedantic -g -pipe $(OPT_FLAGS)
	CXXFLAGS = -std=c++98 -Wall -pedantic -g -pipe -fvisibility=hidden $(OPT_FLAGS)
	LDFLAGS = -Wl,-O1 -Wl,--as-needed -fwhole-program
	PGO_GEN =
	PGO_USE =
	OPT_FLAGS = -DNDEBUG -O3 -march=core2 -mtune=core2 -fomit-frame-pointer
	DEBUG_FLAGS =
else
	CC = gcc
	GPP = g++
	CFLAGS = -std=c99 -Wall -Wextra -Wlogical-op -Wc++-compat -pedantic -Wno-unused-result -g -pipe $(OPT_FLAGS)
	CXXFLAGS = -std=c++0x -Wall -Wextra -Wlogical-op -pedantic -Wno-unused-result -g -pipe -fno-rtti -fno-exceptions -fvisibility=hidden $(OPT_FLAGS)
	LDFLAGS = -Wl,-O1 -Wl,--as-needed -fwhole-program
	PGO_GEN = -fprofile-generate -O3
	PGO_USE = -fprofile-use -O3
	OPT_FLAGS = -DNDEBUG -O2 -march=native -fexcess-precision=fast
endif

all: tester_pgo

IMPLS = linear linear_sse std_bsearch my_bsearch unroll_bsearch bbst ahnentafel skiplist hash ghash hopscotch

tester.o: tester.cpp mapped_file.h unix_utils.h readonly_set.h readonly_set_cfg.h $(IMPLS:%=%.h)
	$(GPP) $(CXXFLAGS) -D_POSIX_C_SOURCE=199309L -c -o $@ $<

unix_mapped_file.o: unix_mapped_file.c print_utils.h readonly_set_cfg.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c %.h print_utils.h readonly_set.h readonly_set_cfg.h
	$(CC) $(CFLAGS) -c -o $@ $<

tester: tester.o unix_mapped_file.o print_utils.o abstract_rs.o choose_hash_func_mask.o slogaemie.o next_permutation.o hash_tools.o $(IMPLS:%=%.o)
	$(GPP) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -ldl -lrt

tester_pgo: unix_mapped_file.o print_utils.o abstract_rs.o choose_hash_func_mask.o slogaemie.o next_permutation.o hash_tools.o $(IMPLS:%=%.o)
	$(GPP) $(CXXFLAGS) $(PGO_GEN) -D_POSIX_C_SOURCE=199309L -c -o tester.o tester.cpp
	$(GPP) $(CXXFLAGS) $(LDFLAGS) $(PGO_GEN) -o tester tester.o $^ -ldl -lrt
	$(MAKE) compare size=1000
	$(GPP) $(CXXFLAGS) $(PGO_USE) -D_POSIX_C_SOURCE=199309L -c -o tester.o tester.cpp
	$(GPP) $(CXXFLAGS) $(LDFLAGS) $(PGO_USE) -o tester tester.o $^ -ldl -lrt


ifeq (${data_file},)
data_file=test_data.bin
data_file_misses=test_data_misses.bin
endif

.PHONY: regen clean

%_generated.c: tester regen ${data_file}
	./tester generate-source ${data_file} $* $@

%_generated_instrumented.so: %_generated.c
	$(CC) $(CFLAGS) $(PGO_GEN) -fPIC -c -o $*_generated.o $<
	$(CC) $(LDFLAGS) $(PGO_GEN) -shared -o $@ $*_generated.o

%_generated.gcda: %_generated_instrumented.so
	./tester benchmark ${data_file} ${data_file_misses} ./$*_generated_instrumented.so | egrep -o 'GOOD|BAD'

%_generated_$(CC)_pgo.so: %_generated.c %_generated.gcda
	$(CC) $(CFLAGS) $(PGO_USE) -fPIC -c -o $*_generated.o $<
	$(CC) $(LDFLAGS) $(PGO_USE) -shared -o $@ $*_generated.o
ifeq ($(CC),icc)
	rm *.dyn pgopti.dpi pgopti.dpi.lock
endif

%_generated.so: %_generated.o
	$(CC) $(LDFLAGS) -shared -o $@ $<

%_generated.o: %_generated.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

${data_file}: tester regen
	if [ "${data_file}" = "test_data.bin" ] ; then ./tester generate-data ${data_file} ${data_file_misses} ${size} ; fi
	sleep 1s

ifndef IMPLS_BENCH
IMPLS_BENCH = $(IMPLS)
endif

compare: tester ${data_file} $(IMPLS_BENCH:%=%_generated_$(CC)_pgo.so)
	taskset 1 ./tester benchmark ${data_file} ${data_file_misses} $(IMPLS_BENCH:%=./%_generated_$(CC)_pgo.so)

benchmark_data.txt: regen
	if [ -z "${seq}" ]; then echo "ERROR! Provide seq parameter like seq='\`seq 100 100 1000\`'" ; false; fi
	for i in ${seq} ; do \
		$(MAKE) compare size=$$i | \
		grep -v 'instrumented' | \
		grep './tester benchmark' -A $$((3 * $(shell echo $(IMPLS_BENCH) | wc -w))) | \
		grep -v '.tester' > benchmark_data.temp ; \
		for impl in $(IMPLS_BENCH) ; do \
			grep $${impl}_generated_$(CC)_pgo.so -B 2 benchmark_data.temp | \
			grep ' hits ' | \
			egrep -o '[012345789]+\.[0123456789]+' > val1.temp ; \
			grep $${impl}_generated_$(CC)_pgo.so -B 2 benchmark_data.temp | \
			grep ' misses ' | \
			egrep -o '[012345789]+\.[0123456789]+' > val2.temp ; \
			strip --strip-unneeded $${impl}_generated_$(CC)_pgo.so -o so.temp ; \
			size -A so.temp | egrep '^Total' | \
			awk '{print $$2}' > val4.temp ; \
			echo $$i > val5.temp ; \
			echo $(CC) > val6.temp ; \
			echo $${impl} > val7.temp ; \
			./tablize.py val5.temp val7.temp val6.temp val1.temp val2.temp val4.temp >> benchmark_data.txt ; \
		done ; \
	done ; \
	rm -f *.temp

choose_hash_func_mask: choose_hash_func_mask.c slogaemie.c next_permutation.c
	$(CC) $(CFLAGS) -c -o slogaemie.o slogaemie.c
	$(CC) $(CFLAGS) -c -o next_permutation.o next_permutation.c
	$(CC) $(CFLAGS) -DTEST -c -o choose_hash_func_mask.o choose_hash_func_mask.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ choose_hash_func_mask.o slogaemie.o next_permutation.o
	./$@

choose_hash_func_mask_pgo: choose_hash_func_mask.c slogaemie.c next_permutation.c
	$(CC) $(CFLAGS) $(PGO_GEN) -c -o slogaemie.o slogaemie.c
	$(CC) $(CFLAGS) $(PGO_GEN) -c -o next_permutation.o next_permutation.c
	$(CC) $(CFLAGS) $(PGO_GEN) -DTEST -c -o choose_hash_func_mask.o choose_hash_func_mask.c
	$(CC) $(CFLAGS) $(PGO_GEN) $(LDFLAGS) -o $@ choose_hash_func_mask.o slogaemie.o next_permutation.o
	./$@
	$(CC) $(CFLAGS) $(PGO_USE) -c -o slogaemie.o slogaemie.c
	$(CC) $(CFLAGS) $(PGO_USE) -c -o next_permutation.o next_permutation.c
	$(CC) $(CFLAGS) $(PGO_USE) -DTEST -c -o choose_hash_func_mask.o choose_hash_func_mask.c
	$(CC) $(CFLAGS) $(PGO_USE) $(LDFLAGS) -o $@ choose_hash_func_mask.o slogaemie.o next_permutation.o

cpu_fast:
	cpufreq-set -c 0 -g userspace
	cpufreq-set -c 1 -g userspace
	cpufreq-set -c 0 -f 2.33Ghz
	cpufreq-set -c 1 -f 2.33Ghz

cpu_cool:
	cpufreq-set -c 0 -g ondemand
	cpufreq-set -c 1 -g ondemand

clean:
	rm -f test_data.bin test_data_misses.bin tester *.so *_generated.c \
	choose_hash_func_mask choose_hash_func_mask_pgo \
	*.o *.bc *.gcda *.debug *.dyn pgopti.dpi pgopti.dpi.lock perf.data
