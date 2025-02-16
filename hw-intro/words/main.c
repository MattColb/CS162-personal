/*

  Word Count using dedicated lists

*/

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

*/

#include <assert.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "word_count.h"

/* Global data structure tracking the words encountered */
WordCount *word_counts = NULL;

/* The maximum length of each word in a file */
#define MAX_WORD_LEN 64

/*
 * 3.1.1 Total Word Count
 *
 * Returns the total amount of words found in infile.
 * Useful functions: fgetc(), isalpha().
 */
int num_words(FILE* infile) {
  if (infile ==NULL){
    perror("Error opening file");
    return -1;
  }

  int ch;
  int num_words = 0;
  bool curr_word = false;

  while ((ch = fgetc(infile)) != EOF){
    // This may be wrong
    if ((isalpha(ch) != 0)){
      if (!curr_word){
        num_words++;
        curr_word = true;
      }
    }else{
      curr_word = false;
    }
  }

  return num_words;
}

/*
 * 3.1.2 Word Frequency Count
 *
 * Given infile, extracts and adds each word in the FILE to `wclist`.
 * Useful functions: fgetc(), isalpha(), tolower(), add_word().
 * 
 * As mentioned in the spec, your code should not panic or
 * segfault on errors. Thus, this function should return
 * 1 in the event of any errors (e.g. wclist or infile is NULL)
 * and 0 otherwise.
 */
int count_words(WordCount **wclist, FILE *infile) {
  if (infile ==NULL){
    perror("Error opening file");
    return 1;
  }

  
  char word[MAX_WORD_LEN + 1];  // Buffer to store words
  int word_len = 0;
  int ch;
  bool in_word = false;

  while ((ch = fgetc(infile)) != EOF) {
      if (isalpha(ch)) {  
          // Convert to lowercase and add to word buffer
          if (word_len < MAX_WORD_LEN) {
              word[word_len++] = tolower(ch);
          }
          in_word = true;
      } else if (in_word) {  
          // Word ended, add to list
          word[word_len] = '\0';  // Null-terminate
          if (add_word(wclist, word) != 0) {  // Check for memory errors
              return 1;
          }
          word_len = 0;
          in_word = false;
      }
  }

  // Handle case where file ends with a word (no punctuation)
  if (in_word && word_len > 0) {
      word[word_len] = '\0';
      if (add_word(wclist, word) != 0) {
          return 1;
      }
  }

  return 0;
}

/*
 * Comparator to sort list by frequency.
 * Useful function: strcmp().
 */
 static bool wordcount_less(const WordCount *wc1, const WordCount *wc2) {
     if (wc1->count != wc2->count) {
         return wc1->count < wc2->count;  // Higher count comes first
     }
     return strcmp(wc1->word, wc2->word) > 0;  // Sort alphabetically if counts are equal
 }
 

// In trying times, displays a helpful message.
static int display_help(void) {
	printf("Flags:\n"
	    "--count (-c): Count the total amount of words in the file, or STDIN if a file is not specified. This is default behavior if no flag is specified.\n"
	    "--frequency (-f): Count the frequency of each word in the file, or STDIN if a file is not specified.\n"
	    "--help (-h): Displays this help message.\n");
	return 0;
}

/*
 * Handle command line flags and arguments.
 */
int main(int argc, char *argv[]) {
  // Default mode: count total words
  bool count_mode = true;
  bool freq_mode = false;
  int total_words = 0;
  WordCount *word_counts = NULL;  // Declare the word count list

  FILE *infile = NULL;

  // Command-line options
  int option;
  static struct option long_options[] = {
      {"count", no_argument, 0, 'c'},
      {"frequency", no_argument, 0, 'f'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
  };

  // Parse command-line arguments
  while ((option = getopt_long(argc, argv, "cfh", long_options, NULL)) != -1) {
      switch (option) {
          case 'c':
              count_mode = true;
              freq_mode = false;
              break;
          case 'f':
              count_mode = false;
              freq_mode = true;
              break;
          case 'h':
              return display_help();
      }
  }

  if (!count_mode && !freq_mode) {
      printf("Please specify a mode.\n");
      return display_help();
  }

  /* Initialize word count structure */
  init_words(&word_counts);

  /* Determine input source (files or STDIN) */
  if (optind >= argc) {
      // No files specified, read from STDIN
      infile = stdin;
      if (count_mode) {
          total_words += num_words(infile);
      } else {
          count_words(&word_counts, infile);
      }
  } else {
      // Process each input file
      for (int i = optind; i < argc; i++) {
          infile = fopen(argv[i], "r");
          if (!infile) {
              perror("Error opening file");
              continue;  // Skip file but continue processing others
          }
          if (count_mode) {
              total_words += num_words(infile);
          } else {
              count_words(&word_counts, infile);
          }
          fclose(infile);
      }
  }

  /* Output results */
  if (count_mode) {
      printf("The total number of words is: %i\n", total_words);
  } else {
      wordcount_sort(&word_counts, wordcount_less);
      printf("The frequencies of each word are: \n");
      fprint_words(word_counts, stdout);
  }

  return 0;
}