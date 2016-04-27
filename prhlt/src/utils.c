#include <prhlt/config.h>
#include <prhlt/constants.h>
#include <prhlt/utils.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

///Calcula if the number is prime
/**
@param n Number
@return if the number is prime return 1 else 0
*/
INLINE int is_prime(int n) {
  int i = 2;
  while ((i < (n / 2)) && (n % i != 0)) i++;
  return (i == (n / 2));
}

///Calcula if the number is prime
/**
@param n Number
@return if the number is prime return 1 else 0
*/
INLINE int next_prime(int n) {
  while (is_prime(n) != 1) n++;
  return n;
}

///Add two numbers in log
/**
@param a Number one to add
@param b Number two to add
@return Add with two numbers
*/
INLINE float add_log(float a, float b) {
  // Make a the max value and b the min value
  if (b > a) {
    SWAP(a,b,float);
  }
  // If min is INFINITY, return max
  if (is_logzero(b)) {
    return a;
  }
  // Robust sum
  return a + log(1 + exp(b - a));
}

/** Parses a string into a sequence of tokens
 * It should performe like strtok_r except for that delimiter
 * is a string delimiter instead of a set of possible delimiters.
 * @param str the first call, it should be the string we want to parse.
 *            The successive calls must be set to NULL
 * @param delimiter a string delimiter that may have more than one character
 * @param saveptr used to save the status of the tokenizer. In the first call
 *            it is unused. In successive calls you have to make sure you do not
 *            change its value until the tokenization is finished
 * @return the next token or NULL is there are no more tokens
 */
char *tokenize(char *str, const char *delimiter, char **saveptr) {
  char *beginning = str;

  // this is not the first call
  if (beginning == NULL) beginning = *saveptr;

  // no more tokens
  if (beginning == NULL || *beginning == '\0') return NULL;

  if (delimiter != NULL) {
    size_t delimiter_length = strlen(delimiter);

    // find the next delimiter
    char *del = strstr(beginning, delimiter);
    // we found it, so remove it
    if (del != NULL) {
      memset(del, '\0', delimiter_length);
      *saveptr = del + delimiter_length;
      return beginning;
    }
    // we didn't find it, we are in the last token
    else {
      *saveptr = NULL;
      return beginning;
    }
  }
  else {
    *saveptr = NULL;
    return beginning;
  }
}

char * strip(char *str) {
  int gap = 0;
  int len = strlen(str) - 1;
  while (len > 0 && isspace(str[len])) len--;
  str[len + 1] = '\0';
  while (str[gap] != '\0' && isspace(str[gap])) gap++;

  if (gap > 0) {
    for (int i = gap; i <= len; i++) {
      str[i - gap] = str[i];
    }
    str[len - gap + 1] = '\0';
  }
  return str;
}

///Split a sentence with character in words in put the words in result
/**
@param sentence Sentence to split
@param result String with the words
@param character Character to split the sentence
*/
int split(char *sentence,char **result, char *character){
 int n=0;
 char *tk;
 tk=(char *)strtok(sentence,character);
 while(tk){
  result[n]=tk;
  n++;
  tk=(char *)strtok(NULL,character);
 }
 return n;
}


#ifndef HAVE_ERRNO_H
  int __prhlt_errno = 0;
  /** Provides naive implementation of perror */
  void __prhlt_perror(const char *s) {
    fprintf(stderr, "System error: %s\n", s);
  }
#endif

#ifndef HAVE_ISATTY
  /** Provides naive implementation of isatty necessary for flex scanners */
  int __prhlt_isatty(int UNUSED(desc)) {
    return 0;
  }
#endif

#ifndef HAVE_CLOCK
  /** Provides naive implementation of clock */
  clock_t __prhlt_clock(void) {
    return (clock_t) -1;
  }
#endif

#if !defined(HAVE_STRDUP)
  char *__prhlt_strdup(const char *str) {
    register char *copy = (char *) malloc(strlen(str) + 1);
    if (copy != NULL) strcpy(copy, str);
    return copy;
  }
#endif
