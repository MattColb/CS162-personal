/*
 * Implementation of the word_count interface using Pintos lists.
 *
 * You may modify this file, and are expected to modify it.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PINTOS_LIST
#error "PINTOS_LIST must be #define'd when compiling word_count_l.c"
#endif

#include <string.h>

#include "word_count.h"

void init_words(word_count_list_t* wclist) { /* TODO */
  int i = 1;
}

size_t len_words(word_count_list_t* wclist) {
  // If it is the total of counts, add count
  /* TODO */
  int i = 0;
  if (*wclist == NULL){
    return 0;
  }
  word_count_t* repeat = (*wclist);
  while (repeat != NULL){
    i += 1;
    repeat = repeat->next;
  }
  return i;
}

word_count_t* find_word(word_count_list_t* wclist, char* word) {
  /* TODO */
  if (*wclist == NULL){
    return NULL;
  }
  word_count_t *repeat = (*wclist);
  while (repeat != NULL){
    if (strcmp(repeat->word, word) == 0){
      return repeat;
    }
    repeat = repeat->next;
  }
  return NULL;
}

word_count_t* add_word(word_count_list_t* wclist, char* word) {
  /* TODO */
  if (*wclist == NULL){
    // Initialize instead of return
    return NULL;
  }
  word_count_t *repeat = (*wclist);
  // Loop over and add one to the word

  // If you're at the end, add it 

  return NULL;
}

void fprint_words(word_count_list_t* wclist, FILE* outfile) {
  /* TODO */
  /* Please follow this format: fprintf(<file>, "%i\t%s\n", <count>, <word>); */
  if (*wclist == NULL){
    return NULL;
  }
  word_count_t* repeat = (*wclist);
  while (repeat){
    fprintf(outfile, "%i\t%s\n", );
    repeat = repeat->next;
  }
}

static bool less_list(const struct list_elem* ewc1, const struct list_elem* ewc2, void* aux) {
  /* TODO */
  return false;
}

void wordcount_sort(word_count_list_t* wclist,
                    bool less(const word_count_t*, const word_count_t*)) {
  list_sort(wclist, less_list, less);
}
