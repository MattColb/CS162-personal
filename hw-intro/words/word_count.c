/*

Copyright © 2019 University of California, Berkeley

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

word_count provides lists of words and associated count

Functional methods take the head of a list as first arg.
Mutators take a reference to a list as first arg.
*/

#include "word_count.h"
#include <string.h>

/* Basic utilities */

char *new_string(char *str) {
  char *new_str = (char *) malloc(strlen(str) + 1);
  if (new_str == NULL) {
    return NULL;
  }
  return strcpy(new_str, str);
}

int init_words(WordCount **wclist) {
  /* Initialize word count.
     Returns 0 if no errors are encountered
     in the body of this function; 1 otherwise.
  */
  if (wclist == NULL){
    return 1;
  }
  *wclist = NULL;
  return 0;
}

ssize_t len_words(WordCount *wchead) {
  /* Return -1 if any errors are
     encountered in the body of
     this function.
  */
  if (!wchead){
    return -1;
  }

  WordCount *current = wchead;
  size_t len = 0;
  while (current){
    len ++;
    current = current->next;
  }
  return (ssize_t)len;
}

WordCount *find_word(WordCount *wchead, char *word) {
  if (!word){
    return NULL;
  }

  /* Return count for word, if it exists */
  WordCount *wc = wchead;
  while (wc){
    if (wc->word && strcmp(word, wc->word) == 0){
      return wc;
    }
    wc = wc->next;
  }
  return NULL;
}

int add_word(WordCount **wclist, char *word) {
  /* If word is present in word_counts list, increment the count.
     Otherwise insert with count 1.
     Returns 0 if no errors are encountered in the body of this function; 1 otherwise.
  */
  if (wclist == NULL){
    return 1;
  }

  if (*wclist == NULL) {
    *wclist = (WordCount *)malloc(sizeof(WordCount));
    if (*wclist == NULL) {
        return 1;  // Memory allocation failure
    }
    (*wclist)->next = NULL;
    (*wclist)->count = 1;
    (*wclist)->word = strdup(word);  // Copy the word to avoid pointer issues
    if ((*wclist)->word == NULL) {
        free(*wclist);  // Clean up on failure
        *wclist = NULL;
        return 1;
    }
    return 0;
}


 WordCount *current = *wclist;
 while (current){
  if (strcmp(current->word, word) == 0){
    current->count ++;
    return 0;
  }
  if (current->next == NULL){
    WordCount *new_word = (WordCount *)malloc(sizeof(WordCount));
    if (new_word == NULL){
      return 1;
    }
    new_word->next = NULL;
    new_word->count = 1;
    new_word->word = strdup(word);
    current->next = new_word;
    return 0;
  }
  current = (current)->next;
 }
 return 0;
}

void fprint_words(WordCount *wchead, FILE *ofile) {
  /* print word counts to a file */
  WordCount *wc;
  for (wc = wchead; wc; wc = wc->next) {
    fprintf(ofile, "%i\t%s\n", wc->count, wc->word);
  }
}
