/*
Copyright (c) 2010 Zeev Tarantov (zeev.tarantov@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#ifdef WIN32
#include "winnt_utils.h"
#include "inttypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#define HINSTANCE void*
#include <strings.h>
#include "unix_utils.h"
#endif

#include <set>
#ifdef __cplusplus
extern "C" {
#endif

#include "mapped_file.h"

#include "skiplist.h"
#include "my_bsearch.h"
#include "std_bsearch.h"
#include "linear.h"
#include "linear_sse.h"
#include "hash.h"
#include "bbst.h"
#include "ahnentafel.h"
#include "unroll_bsearch.h"
#include "ghash.h"
#include "hopscotch.h"

#ifdef __cplusplus
}
#endif

static const struct 
{
  const char* name;
  const struct readonly_set_ops* rs_ops;
}
implementations[] =
{
  { "skiplist"       , &skiplist       },
  { "my_bsearch"     , &my_bsearch     },
  { "std_bsearch"    , &std_bsearch    },
  { "linear"         , &linear         },
  { "linear_sse"     , &linear_sse     },
  { "hash"           , &hash           },
  { "bbst"           , &bbst           },
  { "ahnentafel"     , &ahnentafel     },
  { "unroll_bsearch" , &unroll_bsearch },
  { "ghash"          , &ghash          },
  { "hopscotch"      , &hopscotch      }
};


/* http://groups.google.com/group/comp.lang.c/browse_thread/thread/a9915080a4424068/ */
/* http://www.jstatsoft.org/v08/i14/paper */
static uint32_t prng_state[] = {123456789,362436069,521288629,88675123,886756453};
static uint32_t xorshift(void)
{
  uint32_t t = prng_state[0]^(prng_state[0]>>7);
  prng_state[0]=prng_state[1]; prng_state[1]=prng_state[2]; prng_state[2]=prng_state[3]; prng_state[3]=prng_state[4];
  prng_state[4]=(prng_state[4]^(prng_state[4]<<6))^(t^(t<<13));
  return (prng_state[1]+prng_state[1]+1)*prng_state[4];
}


typedef const item_t* (*is_in_set_t)(const item_t item);

static int test_rnd_hit(is_in_set_t is_in_set, const item_t* test_data, int size)
{
  int res = 1;
  for (int i = 0; i < size; i++)
  {
    const item_t* actual = is_in_set(test_data[i]);
    if (!actual)
      printf("%" PR_item_t ": null\n", test_data[i]);
    if (actual && (test_data[i] != *actual))
      printf("%" PR_item_t ": %" PR_item_t "\n", test_data[i], *actual);
    res &= actual && (test_data[i] == *actual);
  }
  return res;
}

static int test_rnd_miss(is_in_set_t is_in_set, const item_t* test_data, int size)
{
  int res = 1;
  for (int i = 0; i < size; i++)
  {
    const item_t* actual = is_in_set(test_data[i]);
    if (actual)
      printf("found %" PR_item_t ", expected NULL!\n", *actual);
    res &= !actual;
  }
  return res;
}

static item_t get_rand_in_range(item_t range_min, item_t range_max)
{
  if ((item_t)(range_max - range_min + 1) == 0)
  {
#if item_bits == 64
    return (item_t)xorshift() << 32 | xorshift();
#elif item_bits == 32
    return xorshift();
#elif item_bits == 16
    return xorshift() & 0xffff;
#endif
  }
  else
  {
    item_t needed_range = (range_max - range_min + 1);
#if item_bits == 64
    if (needed_range > UINT32_MAX)
    {
      return ((item_t)xorshift() << 32 | xorshift()) % needed_range + range_min;
    }
    else
    {
      return xorshift() % needed_range + range_min;
    }
#else
    return xorshift() % needed_range + range_min;
#endif
  }
}

static int make_test_data(item_t* array, int size, item_t range_min, item_t range_max)
{
  if ((range_max - range_min) < ((item_t)size - 1))
  {
    printf("Impossible to have %d unique items in range [%" PR_item_t ", %" PR_item_t "]!\n", size, range_min, range_max);
    return 1;
  }

  std::set<item_t> the_set;

  for (int i = 0; i < size; )
  {
    item_t candidate = get_rand_in_range(range_min, range_max);
    std::pair<std::set<item_t>::iterator, bool> pair = the_set.insert(candidate);
    if (pair.second)
      i++;
  }

  for (std::set<item_t>::const_iterator iter = the_set.begin();
       iter != the_set.end();
       ++iter)
    *(array++) = *iter;

  return 0;
}

static int make_test_misses_data(item_t* array, int size)
{
  int o_index = 0;
  if (0 != array[0])
    array[o_index++] = array[0] - 1;
  for (int i_index = 1; i_index < size; i_index++)
    if (array[i_index] - 1 > array[i_index - 1] + 1)
      array[o_index++] = array[i_index] - 1;
  array[o_index++] = array[size - 1] + 2;
  return o_index;
}

static item_t* array_iter(void* opaque)
{
  item_t** pp = (item_t**)opaque;
  return (*pp)++;
}

static int generate(const struct readonly_set_ops* rs_ops, int size, const item_t* array, const char* c_path)
{
  FILE* stream;
  struct readonly_set* rs;
  const item_t* p;
  if (NULL == (rs = rs_ops->allocate(size)))
    goto end;
  p = &array[0];
  rs_ops->fill(rs, &array_iter, &p);
  stream = fopen(c_path, "w");
  if (!stream)
    goto cleanup;
  rs_ops->generate_c(rs, stream);
  fclose(stream);
  rs_ops->cleanup(rs);
  return 0;
cleanup:
  rs_ops->cleanup(rs);
end:
  return 1;
}

static int test_so(const char* filename, const struct mapped_file* mf1, const struct mapped_file* mf2, int times)
{
  timestamp_t tp1, tp2;
  HINSTANCE hDLL = dlopen(filename, RTLD_LAZY);
  if (!hDLL)
  {
    fprintf(stderr, "Error! dlopen: %s\n", dlerror());
    return 2;
  }
  dlerror();
  is_in_set_t is_in_set = (is_in_set_t)dlsym(hDLL, "is_in_set");
  {
    const char* err = dlerror();
    if (!is_in_set || err)
    {
      fprintf(stderr, "Error! dlsym: %s\n", err);
      return 2;
    }
  }
  int res = 1;
  get_timestamp(tp1);
  for (int i = 0; i < times; i++)
    res &= test_rnd_hit(is_in_set, (item_t*)mf1->data, mf1->length / sizeof(item_t));
  get_timestamp(tp2);
  timestamp_diff(tp1, tp2);
  printf("%d iterations of hits took " PRINTF_TIMESTAMP_STR " seconds\n", times, PRINTF_TIMESTAMP_VAL(tp1));
  get_timestamp(tp1);
  for (int i = 0; i < times; i++)
    res &= test_rnd_miss(is_in_set, (item_t*)mf2->data, mf2->length / sizeof(item_t));
  get_timestamp(tp2);
  timestamp_diff(tp1, tp2);
  printf("%d iterations of misses took " PRINTF_TIMESTAMP_STR " seconds\n", times, PRINTF_TIMESTAMP_VAL(tp1));
  {
    int err = dlclose(hDLL);
    if (err)
    {
      fprintf(stderr, "Error! dlclose: %s\n", dlerror());
      return 2;
    }
  }
  return !res;
}

static const struct readonly_set_ops* get_set_by_name(const char* name)
{
  for (unsigned i = 0; i < sizeof(implementations)/sizeof(*implementations); i++)
    if (0 == strcasecmp(implementations[i].name, name))
      return implementations[i].rs_ops;
  fprintf(stderr, "Error: unknown impl \"%s\". Use the 'list' command to see available impls.\n", name);
  return NULL;
}

static const struct
{
  const char* name;
  int num_required_arguments;
  const char* desc;
}
command_line_options[] =
{
  { "list", 0,
    "lists available implementations" },
  { "generate-data", 3,
    "data_file data_file_misses size [min range] [max range] - outputs size elements of random data in range (default: [0 - ~0])" },
  { "generate-source", 3,
    "data_file impl1 c_file1 [impl n c_file n] - outputs generated C source for given set" },
  { "test", 2,
    "test_data test_data_misses shared_lib_1 [shared_lib n] - tests shared libs against test_data" },
  { "benchmark", 2,
    "test_data test_data_misses shared_lib_1 [shared_lib n] - performs 'test' 1000 times" }
};

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    puts("Available commands:");
    for (unsigned i = 0; i < sizeof(command_line_options)/sizeof(*command_line_options); i++)
      printf("%s - %s.\n", command_line_options[i].name, command_line_options[i].desc);
    return 0;
  }
  int command = -1;
  for (unsigned i = 0; i < sizeof(command_line_options)/sizeof(*command_line_options); i++)
  {
    if (0 == strcasecmp(command_line_options[i].name, argv[1]))
    {
      command = i;
      break;
    }
  }
  if (-1 == command)
  {
    fprintf(stderr, "Error: unknown command.\n");
    return 1;
  }
  if (argc < command_line_options[command].num_required_arguments + 2)
  {
    fprintf(stderr, "Error: At least %d arguments required.\n", command_line_options[command].num_required_arguments);
    return 1;
  }
  switch (command)
  {
    case 0:
      for (unsigned i = 0; i < sizeof(implementations)/sizeof(*implementations); i++)
        puts(implementations[i].name);
      return 0;
    case 1:
    {
      //TODO: read optional range values
      int size = atoi(argv[4]), misses_size;
      FILE* stream;
      item_t* array = (item_t*)malloc(sizeof(item_t) * (size + 1));
      if (!array) return 1;
      if (make_test_data(array, size, 0, (item_t)~0))
        goto cleanup;
      stream = fopen(argv[2], "wb");
      fwrite(array, size, sizeof(item_t), stream);
      fclose(stream);
      misses_size = make_test_misses_data(array, size);
      stream = fopen(argv[3], "wb");
      fwrite(array, misses_size, sizeof(item_t), stream);
      fclose(stream);
      free(array);
      return 0;
      cleanup:
      free(array);
      return 1;
    }
    case 2:
    {
      struct mapped_file* mf = mmap_file(argv[2]);
      if (!mf)
        return 2;
      int size = (int)(mf->length / sizeof(item_t)), res = 1;
      for (int i = 0; i <= size - 2; i++)
      {
        if (((item_t*)mf->data)[i] >= ((item_t*)mf->data)[i+1])
        {
          fprintf(stderr, "Error: data file %s is not strictly ordered (item #%d >= item #%d).\n", argv[2], i+1, i+2);
          munmap_file(mf);
          return 1;
        }
      }
      for (int i = 3; i+2 <= argc; i += 2)
      {
        const struct readonly_set_ops* rs_ops = get_set_by_name(argv[i]);
        if (!rs_ops) continue;
        res &= generate(rs_ops, size, (const item_t*)mf->data, argv[i+1]);
      }
      munmap_file(mf);
      return res;
    }
    case 3:
    case 4:
    {
      struct mapped_file* mf1 = mmap_file(argv[2]);
      if (!mf1)
        return 2;
      struct mapped_file* mf2 = mmap_file(argv[3]);
      if (!mf2)
        return 2;
      int res = 0, times = 1;
      if (4 == command) times = 1000;
      init_timestamper();
      for (int i = 4; i < argc; i++)
      {
        int curr_res = test_so(argv[i], mf1, mf2, times);
        if (curr_res != 2)
        {
          res |= curr_res;
          printf("Rand test of %s with size=%d: %s\n", argv[i], (int)(mf1->length / sizeof(item_t)), curr_res ? "BAD" : "GOOD");
        }
      }
      munmap_file(mf1);
      munmap_file(mf2);
      return res;
    }
  }
}
