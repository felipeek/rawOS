#include "../screen.h"
#include "../alloc/kalloc.h"
#include "util.h"

#include "printf.h"

#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define _ADDRESSOF(v) (&(v))
#define rawos_va_arg(ap, t) (*(t*)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define rawos_va_start(ap, v) ((void)(ap = (rawos_va_list)_ADDRESSOF(v) + _INTSIZEOF(v)))
#define rawos_va_end(ap) ((void)(ap = (rawos_va_list)0))
typedef char* rawos_va_list;

// Conversions

s32
u32_to_str_base16(u32 value, s32 bitsize, s32 leading_zeros, s32 capitalized, u8 buffer[16]) {
    s32 i = 0;
    u32 mask = 0xf0000000 >> (32 - bitsize);
    u8 cap = 0x57;
    if (capitalized) {
        cap = 0x37; // hex letters capitalized
    }
    for (; i < (bitsize / 4); i += 1) {
        u32 f = (value & mask) >> (bitsize - 4);
        if(f > 9) buffer[i] = (u8)f + 0x57;//0x37;
        else buffer[i] = (u8)f + 0x30;
        value = value << 4;
    }
    return i;
}

// TODO(fek): WTF?
void foozle(char* dst, char* src, u32 size) {
    for(int i = 0; i < size; ++i){
        dst[i] = *(src+i);
    }
}

int
u32_to_str(u32 val, u8* buffer) {
    u8 b[32] = {0};
    int sum = 0;

    if (val == 0) {
        buffer[0] = '0';
        return 1;
    }

    char* auxbuffer = &b[16];
    char* start = auxbuffer + sum;

    while (val != 0) {
        u32 rem = val % 10;
        val /= 10;
        *auxbuffer = '0' + (u8)rem;
        auxbuffer -= 1;
    }

    int size = start - auxbuffer;

    foozle(&buffer[sum], auxbuffer + 1, (u32)size);
    return size;
}

int
s32_to_str(s32 val, u8* buffer) {
    u8 b[32] = {0};
    s32 sum = 0;

    if (val == 0) {
        buffer[0] = '0';
        return 1;
    }

    if (val < 0) {
        val = -val;
        buffer[0] = '-';
        sum = 1;
    }
    char* auxbuffer = &b[16];
    char* start = auxbuffer + sum;

    while (val != 0) {
        s32 rem = val % 10;
        val /= 10;
        *auxbuffer = '0' + (u8)rem;
        auxbuffer -= 1;
    }

    int size = start - auxbuffer;
    foozle(&buffer[sum], auxbuffer + 1, (u32)size);
    return size;
}

// TODO(psv): very big numbers are incorrect
s32
r32_to_str(r32 v, u8 buffer[32]) {
    u32 l = 0;
    if (v < 0.0) {
        buffer[l] = '-';
        l += 1;
        v = -v;
    }

    l += s32_to_str((s32)v, ((u8*)buffer + l));
    s32 precision = 6;

    r32 fractional_part = v - ((r32)((s32)v));

    buffer[l] = '.';
    l += 1;

    if (fractional_part == 0.0) {
        buffer[l] = '0';
        l += 1;
        return l;
    }

    while (precision > 0 && fractional_part > 0.0) {
        fractional_part *= 10.0;
        buffer[l] = (u8)fractional_part + '0';
        fractional_part = fractional_part - ((r32)((u8)fractional_part));
        l += 1;
        precision -= 1;
    }
    return l;
}

// TODO(psv): very big numbers are incorrect
s32
r64_to_str(r64 v, u8 buffer[64]) {
    s32 l = 0;
    if (v < 0.0) {
        buffer[l] = '-';
        l += 1;
        v = -v;
    }

    l += s32_to_str((s32)v, ((u8*)buffer + l));
    s32 precision = 6;

    r64 fractional_part = v - ((r64)((s32)v));

    buffer[l] = '.';
    l += 1;

    if (fractional_part == 0.0) {
        buffer[l] = '0';
        l += 1;
        return l;
    }

    while (precision > 0 && fractional_part > 0.0) {
        fractional_part *= 10.0;
        buffer[l] = (u8)fractional_part + '0';
        fractional_part = fractional_part - ((r64)((s32)fractional_part));
        l += 1;
        precision -= 1;
    }
    return l;
}

// --------------------------------
// ---------- Catstring -----------
// --------------------------------

#define MAX(A, B) (((A) > (B)) ? (A) : (B))

static void
buffer_grow(catstring * s)
{
  if (s->capacity < 32)
  {
    s->data = kalloc_realloc(s->data, s->capacity, s->capacity + 32);
    s->capacity = s->capacity + 32;
  }
  else
  {
    s->data = kalloc_realloc(s->data, s->capacity, s->capacity * 2);
    s->capacity = s->capacity * 2;
  }
}

static void
buffer_grow_by(catstring* s, int amount)
{
  u32 a = (s->capacity + amount) * 2;
  s->data = kalloc_realloc(s->data, s->capacity, a);
  s->capacity = a;
}

static int
catsprint_string_length(catstring* buffer, const char* str, int length)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(MAX(32, length));
    buffer->capacity = MAX(32, length);
  }

  int n = 0;

  if ((buffer->length + length) >= buffer->capacity)
  {
    buffer_grow_by(buffer, length);
  }
  foozle(buffer->data + buffer->length, str, length);
  buffer->length += length;

  return length;
}

static int
catsprint_string_length_escaped(catstring* buffer, const char* str, int length)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(MAX(32, length));
    buffer->capacity = MAX(32, length);
  }

  int n = 0;

  if ((buffer->length + length) >= buffer->capacity)
  {
    buffer_grow_by(buffer, length * 2);
  }

  char *at = buffer->data + buffer->length;
  for (int i = 0; i < length; ++i)
  {
    char c = *(str + i);
    if (c == '\n') {
      *at = '\\';
      at++;
      *at = 'n';
      buffer->length++;
    }
    else
    {
      *at = c;
    }
    at++;
  }
  buffer->length += length;

  return length;
}

static int
catsprint_string(catstring* buffer, const char* str)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(32);
    buffer->capacity = 32;
  }

  int n = 0;
  for (const char *at = str;; ++at, ++n)
  {
    // push to stream
    if (buffer->length >= buffer->capacity)
    {
      buffer_grow(buffer);
    }

    buffer->data[buffer->length] = *at;
    if (!(*at))
      break;
    buffer->length++;
  }

  return n;
}

static int
catsprint_decimal_unsigned(catstring* buffer, u32 value)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(32);
    buffer->capacity = 32;
  }

  u8 mem[32] = { 0 };
  s32 length = u32_to_str(value, mem);
  return catsprint_string_length(buffer, (const char*)mem, length);
}

static int
catsprint_decimal_signed(catstring* buffer, s32 value)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(32);
    buffer->capacity = 32;
  }

  u8 mem[32] = { 0 };
  s32 length = s32_to_str(value, mem);
  return catsprint_string_length(buffer, mem, length);
}

static int
catsprint_hexadecimal(catstring* buffer, u32 value)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(32);
    buffer->capacity = 32;
  }

  char mem[16] = { 0 };
  int length = u32_to_str_base16(value, 32, 1, 0, mem);
  return catsprint_string_length(buffer, mem, length);
}

static int
catsprint_double(catstring* buffer, double value)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(32);
    buffer->capacity = 32;
  }

  u8 mem[64] = { 0 };
  int length = r64_to_str(value, mem);
  return catsprint_string_length(buffer, mem, length);
}

int
catsprint_list(catstring* buffer, char* str, rawos_va_list args)
{
  if (buffer->capacity == 0)
  {
    buffer->data = kalloc_alloc(32);
    buffer->capacity = 32;
  }
  int XXX = 0;

  int len = 0;
  for (char *at = str;;)
  {
    // push to stream
    if (buffer->length >= buffer->capacity)
    {
      buffer_grow(buffer);
    }

    int n = 0;

    if (*at == '%')
    {
      at++;                     // skip
      switch (*at)
      {
        case '%':{
          // just push character
          buffer->data[buffer->length] = *at;
          if (!(*at))
            break;
          buffer->length++;
          len++;
          at++;
        } break;
        case 's':{
          at++;
          if (*at == '+')
          {
            at++;
            int length = rawos_va_arg(args, int);
            // push string
            n = catsprint_string_length(buffer, rawos_va_arg(args, const char *), length);
          }
          else if (*at == '*')
          {
            at++;
            int length = rawos_va_arg(args, int);
            // push string
            n = catsprint_string_length_escaped(buffer, rawos_va_arg(args, const char *), length);
          }
          else
          {
            // push string
            n = catsprint_string(buffer, rawos_va_arg(args, const char *));
          }
        } break;
        case 'u':{
          at++;
          n = catsprint_decimal_unsigned(buffer, rawos_va_arg(args, u32));
        } break;
        case 'd':{
          at++;
          n = catsprint_decimal_signed(buffer, rawos_va_arg(args, int));
        } break;
        case 'x':{
          at++;
          n = catsprint_hexadecimal(buffer, rawos_va_arg(args, u32));
        } break;
        case 'f':{
          at++;
          n = catsprint_double(buffer, rawos_va_arg(args, double));
        } break;
      }
      len += n;
    }
    else if(*at == '\\' && at[1] == '0')
    {
      buffer->data[buffer->length] = 0;
      if (!(*at))
        break;
      buffer->length++;
      len++;
      at+=2;
    }
    else
    {
      buffer->data[buffer->length] = *at;
      if (!(*at))
        break;
      buffer->length++;
      len++;
      at++;
    }
  }

  return XXX;
}

// %%
// %s string, %
int
catsprint(catstring* buffer, char* str, ...)
{
  rawos_va_list args;
  rawos_va_start(args, str);

  int l = catsprint_list(buffer, str, args);

  rawos_va_end(args);

  return l;
}
#if 0
int
catstring_append(catstring* to, catstring* s)
{
  if (to->capacity == 0)
  {
    to->data = calloc(1, MAX(32, s->length));
    to->capacity = MAX(32, s->length);
  }

  int n = 0;

  if ((to->length + s->length) >= to->capacity)
  {
    buffer_grow_by(to, s->length);
  }
  memcpy(to->data + to->length, s->data, s->length);
  to->length += s->length;

  return s->length;
}

catstring
catstring_copy(catstring * s)
{
  catstring result = { 0 };

  result.data = calloc(1, s->capacity);
  result.capacity = s->capacity;
  result.length = s->length;
  memcpy(result.data, s->data, s->length);

  return result;
}

catstring
catstring_new(const char* str, int length)
{
  catstring result = { 0 };

  result.data = calloc(1, length);
  result.capacity = length;
  memcpy(result.data, str, length);

  return result;
}

void
catstring_print(catstring* s)
{
  if (!s->data)
    return;
  printf("%.*s", s->length, s->data);
}
#endif
void
catstring_free(catstring* s)
{
  kalloc_free(s->data);
  s->capacity = 0;
  s->length = 0;
}