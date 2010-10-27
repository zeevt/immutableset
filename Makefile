ifeq ($(CC), icc)
	CFLAGS = -std=c99 -Wall -g -gdwarf-2 -pipe -ipo -mssse3 -mtune=core2 -ffunction-sections -ansi-alias
	CXXFLAGS = -std=c++0x -Wall -g -gdwarf-2 -pipe -ipo -mssse3 -mtune=core2 -ffunction-sections
	LDFLAGS = -Wl,-O1 -Wl,--as-needed
	PGO_GEN = -prof-gen
	PGO_USE = -prof-use
	OPT_FLAGS = -DNDEBUG -O3
	DEBUG_FLAGS =
else ifeq ($(CC), clang)
	CFLAGS = -std=c99 -Wall -pedantic -g -pipe -march=core2 -mtune=core2 -fomit-frame-pointer -ffunction-sections -funroll-loops
	LDFLAGS = -Wl,-O1 -Wl,--as-needed -fwhole-program
	PGO_GEN =
	PGO_USE =
	OPT_FLAGS = -DNDEBUG -O3
	DEBUG_FLAGS =
else
	CC = gcc
	CFLAGS = -std=c99 -Wall -Wextra -Wlogical-op -Wc++-compat -pedantic -Wno-unused-result -g -pipe -march=native -fexcess-precision=fast -ffunction-sections -flto
	CXXFLAGS = -std=c++0x -Wall -Wextra -Wlogical-op -pedantic -Wno-unused-result -g -pipe -march=native -fexcess-precision=fast -flto
	LDFLAGS = -Wl,-O1 -Wl,--as-needed -fwhole-program -flto -fwhopr
	PGO_GEN = -fprofile-generate -O3
	PGO_USE = -fprofile-use -O3
	ifeq ("${DEBUG}", "yes")
		OPT_FLAGS =
		DEBUG_FLAGS = -O0 -ggdb
	else
		OPT_FLAGS = -DNDEBUG -O2
		DEBUG_FLAGS =
	endif
endif

all: tester_pgo

IMPLS = linear linear_sse std_bsearch my_bsearch unroll_bsearch bbst ahnentafel skiplist hash ghash hopscotch

tester.o: tester.c tree.h mapped_file.h unix_utils.h readonly_set.h readonly_set_cfg.h $(IMPLS:%=%.h)
	$(CC) $(CFLAGS) $(OPT_FLAGS) -D_POSIX_C_SOURCE=199309L -c -o $@ $<

unix_mapped_file.o: unix_mapped_file.c print_utils.h readonly_set_cfg.h
	$(CC) $(CFLAGS) $(OPT_FLAGS) -c -o $@ $<

%.o: %.c %.h print_utils.h readonly_set.h readonly_set_cfg.h
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -c -o $@ $<

tester: tester.o unix_mapped_file.o print_utils.o abstract_rs.o choose_hash_func_mask.o slogaemie.o next_permutation.o hash_tools.o $(IMPLS:%=%.o)
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(LDFLAGS) $(DEBUG_FLAGS) -o $@ $^ -ldl -lrt

tester_pgo: unix_mapped_file.o print_utils.o abstract_rs.o choose_hash_func_mask.o slogaemie.o next_permutation.o hash_tools.o $(IMPLS:%=%.o)
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_GEN) -D_POSIX_C_SOURCE=199309L -c -o tester.o tester.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(LDFLAGS) $(PGO_GEN) -o tester tester.o $^ -ldl -lrt
	$(MAKE) compare size=1000
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_USE) -D_POSIX_C_SOURCE=199309L -c -o tester.o tester.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(LDFLAGS) $(PGO_USE) -o tester tester.o $^ -ldl -lrt


ifeq (${data_file},)
data_file=test_data.bin
data_file_misses=test_data_misses.bin
endif

.PHONY: regen clean

%_generated.c: tester regen ${data_file}
	./tester generate-source ${data_file} $* $@

%_generated_instrumented.so: %_generated.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_GEN) $(DEBUG_FLAGS) -fPIC -c -o $*_generated.o $<
	$(CC) $(LDFLAGS) $(PGO_GEN) -shared -o $@ $*_generated.o

%_generated.gcda: %_generated_instrumented.so
	./tester benchmark ${data_file} ${data_file_misses} ./$*_generated_instrumented.so | egrep -o 'GOOD|BAD'

%_generated_pgo.so: %_generated.c %_generated.gcda
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_USE) $(DEBUG_FLAGS) -fPIC -c -o $*_generated.o $<
	$(CC) $(LDFLAGS) $(PGO_USE) -shared -o $@ $*_generated.o
ifeq ($(CC),icc)
	rm *.dyn pgopti.dpi pgopti.dpi.lock
endif

%_generated.so: %_generated.o
	$(CC) $(LDFLAGS) -shared -o $@ $<

%_generated.o: %_generated.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -fPIC -c -o $@ $<

${data_file}: tester regen
	if [ "${data_file}" = "test_data.bin" ] ; then ./tester generate-data ${data_file} ${data_file_misses} ${size} ; fi
	sleep 1s

ifndef IMPLS_BENCH
IMPLS_BENCH = $(IMPLS)
endif

compare: tester ${data_file} $(IMPLS_BENCH:%=%_generated_pgo.so)
	taskset 1 ./tester benchmark ${data_file} ${data_file_misses} $(IMPLS_BENCH:%=./%_generated_pgo.so)

benchmark_data.txt: regen
	if [ -z "${seq}" ]; then echo "ERROR! Provide seq parameter like seq='\`seq 100 100 1000\`'" ; false; fi
	for i in ${seq} ; do \
		$(MAKE) compare size=$$i | \
		grep -v 'instrumented' | \
		grep './tester benchmark' -A $$((3 * $(shell echo $(IMPLS_BENCH) | wc -w))) | \
		grep -v '.tester' > benchmark_data.temp ; \
		for impl in $(IMPLS_BENCH) ; do \
			grep $${impl}_generated_pgo.so -B 2 benchmark_data.temp | \
			grep ' hits ' | \
			egrep -o '[012345789]+\.[0123456789]+' > val1.temp ; \
			grep $${impl}_generated_pgo.so -B 2 benchmark_data.temp | \
			grep ' misses ' | \
			egrep -o '[012345789]+\.[0123456789]+' > val2.temp ; \
			size -A $${impl}_generated_pgo.so | egrep '^Total' | \
			awk '{print $$2}' > val4.temp ; \
			echo $$i > val5.temp ; \
			echo $(CC) > val6.temp ; \
			echo $${impl} > val7.temp ; \
			./tablize.py val5.temp val7.temp val6.temp val1.temp val2.temp val4.temp >> benchmark_data.txt ; \
		done ; \
	done ; \
	rm -f *.temp

choose_hash_func_mask: choose_hash_func_mask.c slogaemie.c next_permutation.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -c -o slogaemie.o slogaemie.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -c -o next_permutation.o next_permutation.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -DTEST -c -o choose_hash_func_mask.o choose_hash_func_mask.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) $(LDFLAGS) -o $@ choose_hash_func_mask.o slogaemie.o next_permutation.o
	./$@

choose_hash_func_mask_pgo: choose_hash_func_mask.c slogaemie.c next_permutation.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_GEN) $(DEBUG_FLAGS) -c -o slogaemie.o slogaemie.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_GEN) $(DEBUG_FLAGS) -c -o next_permutation.o next_permutation.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_GEN) $(DEBUG_FLAGS) -DTEST -c -o choose_hash_func_mask.o choose_hash_func_mask.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_GEN) $(DEBUG_FLAGS) $(LDFLAGS) -o $@ choose_hash_func_mask.o slogaemie.o next_permutation.o
	./$@
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_USE) $(DEBUG_FLAGS) -c -o slogaemie.o slogaemie.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_USE) $(DEBUG_FLAGS) -c -o next_permutation.o next_permutation.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_USE) $(DEBUG_FLAGS) -DTEST -c -o choose_hash_func_mask.o choose_hash_func_mask.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(PGO_USE) $(DEBUG_FLAGS) $(LDFLAGS) -o $@ choose_hash_func_mask.o slogaemie.o next_permutation.o

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
