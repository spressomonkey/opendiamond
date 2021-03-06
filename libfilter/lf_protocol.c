/*
 *  The OpenDiamond Platform for Interactive Search
 *
 *  Copyright (c) 2002-2007 Intel Corporation
 *  Copyright (c) 2006 Larry Huston <larry@thehustons.net>
 *  Copyright (c) 2006-2010 Carnegie Mellon University
 *  All rights reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#define _GNU_SOURCE

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lf_protocol.h"

static void error_stdio(FILE *f, const char *msg) {
  if (feof(f)) {
    //    g_warning("EOF");
    exit(EXIT_SUCCESS);
  } else {
    perror(msg);
    exit(EXIT_FAILURE);
  }
}

int lf_get_size(FILE *in) {
  char *line = NULL;
  size_t n;
  int result;

  if (getline(&line, &n, in) == -1) {
    free(line);
    error_stdio(in, "Can't read size");
  }

  // if there is no string, then return -1
  if (strlen(g_strchomp(line)) == 0) {
    result = -1;
  } else {
    result = atoi(line);
  }

  free(line);

  //  g_message("size: %d", result);
  return result;
}

char *lf_get_string(FILE *in) {
  int size = lf_get_size(in);

  if (size == -1) {
    return NULL;
  }

  char *result = g_malloc(size + 1);
  result[size] = '\0';

  if (size > 0) {
    if (fread(result, size, 1, in) != 1) {
      error_stdio(in, "Can't read string");
    }
  }

  // read trailing '\n'
  getc(in);

  return result;
}

char **lf_get_strings(FILE *in) {
  GSList *list = NULL;

  char *str;
  while ((str = lf_get_string(in)) != NULL) {
    list = g_slist_prepend(list, str);
  }

  // convert to strv
  int len = g_slist_length(list);
  char **result = g_new(char *, len + 1);
  result[len] = NULL;

  list = g_slist_reverse(list);

  int i = 0;
  while (list != NULL) {
    result[i++] = list->data;
    list = g_slist_delete_link(list, list);
  }

  return result;
}

void *lf_get_binary(FILE *in, int *len_OUT) {
  int size = lf_get_size(in);
  *len_OUT = size;

  uint8_t *binary = NULL;

  if (size > 0) {
    binary = g_malloc(size);

    if (fread(binary, size, 1, in) != 1) {
      error_stdio(in, "Can't read binary");
    }
  }

  if (size != -1) {
    // read trailing '\n'
    getc(in);
  }

  return binary;
}

void lf_send_binary(FILE *out, int len, const void *data) {
  if (fprintf(out, "%d\n", len) == -1) {
    error_stdio(out, "Can't write binary length");
  }
  if (fwrite(data, len, 1, out) != 1) {
    error_stdio(out, "Can't write binary");
  }
  if (fprintf(out, "\n") == -1) {
    error_stdio(out, "Can't write end of binary");
  }
  if (fflush(out) != 0) {
    error_stdio(out, "Can't flush");
  }
}


void lf_send_tag(FILE *out, const char *tag) {
  if (fprintf(out, "%s\n", tag) == -1) {
    error_stdio(out, "Can't write tag");
  }
  if (fflush(out) != 0) {
    error_stdio(out, "Can't flush");
  }
}

void lf_send_int(FILE *out, int i) {
  char *str = g_strdup_printf("%d", i);
  lf_send_string(out, str);
  g_free(str);
}

void lf_send_string(FILE *out, const char *str) {
  int len = strlen(str);
  if (fprintf(out, "%d\n%s\n", len, str) == -1) {
    error_stdio(out, "Can't write string");
  }
  if (fflush(out) != 0) {
    error_stdio(out, "Can't flush");
  }
}

bool lf_get_boolean(FILE *in) {
  char *str = lf_get_string(in);
  if (str == NULL) {
    g_warning("Can't get boolean");
    exit(EXIT_FAILURE);
  }

  bool result = (strcmp(str, "true") == 0);
  g_free(str);

  return result;
}

void lf_send_blank(FILE *out) {
  if (fprintf(out, "\n") == -1) {
    error_stdio(out, "Can't write blank");
  }
  if (fflush(out) != 0) {
    error_stdio(out, "Can't flush");
  }
}

void lf_get_blank(FILE *in) {
  if (lf_get_string(in) != NULL) {
    g_warning("Expecting blank");
    exit(EXIT_FAILURE);
  }
}


double lf_get_double(FILE *in) {
  char *s = lf_get_string(in);
  if (s == NULL) {
    g_warning("Expecting double");
    exit(EXIT_FAILURE);
  }

  double d = g_ascii_strtod(s, NULL);
  g_free(s);

  return d;
}

void lf_send_double(FILE *out, double d) {
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  lf_send_string(out, g_ascii_dtostr (buf, sizeof (buf), d));
}
