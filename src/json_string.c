/*
  This file is part of YACJP.

  YacJP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.

  YacJP is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with YacJP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdarg.h>

#include "json_value.h"

struct json_string_impl {
     struct json_string fn;

     int capacity;
     char *string;
};

static void accept(struct json_string_impl *this, json_visitor_t *visitor) {
     visitor->visit_string(visitor, (json_string_t*)this);
}

static char *get(struct json_string_impl *this) {
     return this->string;
}

static void set(struct json_string_impl *this, char *format, ...) {
     va_list args;
     int c = this->capacity;
     int n;

     va_start(args, format);
     n = vsnprintf(c ? this->string : "", c, format, args);
     va_end(args);

     if (n >= c) {
          if (c) {
               do {
                    c *= 2;
               } while (n >= c);
               this->string = realloc(this->string, c);
          }
          else {
               c = 4;
               this->string = malloc(c);
          }
          this->capacity = c;
          va_start(args, format);
          vsnprintf(this->string, c, format, args);
          va_end(args);
     }
}

static void free_(struct json_string_impl *this) {
     if (this->string) free(this->string);
     free(this);
}

__PUBLIC__ json_string_t *json_new_string() {
     struct json_string_impl *result = (struct json_string_impl *)malloc(sizeof(struct json_string_impl));
     if (!result) return NULL;
     result->fn.accept    = (json_string_accept_fn)accept;
     result->fn.free      = (json_string_free_fn  )free_ ;
     result->fn.get       = (json_string_get_fn   )get   ;
     result->fn.set       = (json_string_set_fn   )set   ;
     result->capacity     = 0;
     result->string       = NULL;
     return (json_string_t*)result;
}
