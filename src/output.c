/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Paul Corbett.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)output.c	5.7 (Berkeley) 5/24/93";
#endif /* not lint */

#include "defs.h"
#include <string.h>

static int nvectors;
static int nentries;
static int **froms;
static int **tos;
static int *tally;
static int *width;
static int *state_count;
static int *order;
static int *base;
static int *pos;
static int maxtable;
static int *table;
static int *check;
static int lowzero;
static int high;

void output_version (const char * prefix) {
  printf("// created by jay 1.0.2 (c) 2002-2004 ats@cs.rit.edu\n"
         "// skeleton %s\n", prefix), outline += 2;
}

void output_yyLhs (const char * prefix) {
  int i, j;
  
  printf("//yyLhs %d\n", nrules-2), ++ outline;
  
  printf("%s%6d,", prefix, symbol_value[start_symbol]), j = 1;
  for (i = 3; i < nrules; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", symbol_value[rlhs[i]]), ++ j;
  }
  putchar('\n'), ++ outline;
}

void output_yyLen (const char * prefix) {
  int i, j;
  
  printf("//yyLen %d\n", nrules-2), ++ outline;
  
  printf("%s%6d,", prefix, 2), j = 1;
  for (i = 3; i < nrules; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", rrhs[i + 1] - rrhs[i] - 1), ++ j;
  }
  putchar('\n'), ++ outline;
}

void output_yyDefRed (const char * prefix) {
  int i, j;
  
  printf("//yyDefRed %d\n", nstates), ++ outline;

  printf("%s%6d,", prefix, defred[0] ? defred[0] - 2 : 0), j = 1;
  for (i = 1; i < nstates; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", (defred[i] ? defred[i] - 2 : 0)), ++ j;
  }
  putchar('\n'), ++ outline;
}

void token_actions() {
    register int i, j;
    register int shiftcount, reducecount;
    register int max, min;
    register int *actionrow, *r, *s;
    register action *p;

    actionrow = NEW2(2*ntokens, int);
    for (i = 0; i < nstates; ++i)
    {
	if (parser[i])
	{
	    for (j = 0; j < 2*ntokens; ++j)
	    actionrow[j] = 0;

	    shiftcount = 0;
	    reducecount = 0;
	    for (p = parser[i]; p; p = p->next)
	    {
		if (p->suppressed == 0)
		{
		    if (p->action_code == SHIFT)
		    {
			++shiftcount;
			actionrow[p->symbol] = p->number;
		    }
		    else if (p->action_code == REDUCE && p->number != defred[i])
		    {
			++reducecount;
			actionrow[p->symbol + ntokens] = p->number;
		    }
		}
	    }

	    tally[i] = shiftcount;
	    tally[nstates+i] = reducecount;
	    width[i] = 0;
	    width[nstates+i] = 0;
	    if (shiftcount > 0)
	    {
		froms[i] = r = NEW2(shiftcount, int);
		tos[i] = s = NEW2(shiftcount, int);
		min = MAXINT;
		max = 0;
		for (j = 0; j < ntokens; ++j)
		{
		    if (actionrow[j])
		    {
			if (min > symbol_value[j])
			    min = symbol_value[j];
			if (max < symbol_value[j])
			    max = symbol_value[j];
			*r++ = symbol_value[j];
			*s++ = actionrow[j];
		    }
		}
		width[i] = max - min + 1;
	    }
	    if (reducecount > 0)
	    {
		froms[nstates+i] = r = NEW2(reducecount, int);
		tos[nstates+i] = s = NEW2(reducecount, int);
		min = MAXINT;
		max = 0;
		for (j = 0; j < ntokens; ++j)
		{
		    if (actionrow[ntokens+j])
		    {
			if (min > symbol_value[j])
			    min = symbol_value[j];
			if (max < symbol_value[j])
			    max = symbol_value[j];
			*r++ = symbol_value[j];
			*s++ = actionrow[ntokens+j] - 2;
		    }
		}
		width[nstates+i] = max - min + 1;
	    }
	}
    }
    FREE(actionrow);
}

void save_column(int symbol, int default_state) {
    register int i;
    register int m;
    register int n;
    register int *sp;
    register int *sp1;
    register int *sp2;
    register int count;
    register int symno;

    m = goto_map[symbol];
    n = goto_map[symbol + 1];

    count = 0;
    for (i = m; i < n; i++)
    {
	if (to_state[i] != default_state)
	    ++count;
    }
    if (count == 0) return;

    symno = symbol_value[symbol] + 2*nstates;

    froms[symno] = sp1 = sp = NEW2(count, int);
    tos[symno] = sp2 = NEW2(count, int);

    for (i = m; i < n; i++)
    {
	if (to_state[i] != default_state)
	{
	    *sp1++ = from_state[i];
	    *sp2++ = to_state[i];
	}
    }

    tally[symno] = count;
    width[symno] = sp1[-1] - sp[0] + 1;
}


void goto_actions (const char * prefix) {
    register int i, j, k;

    state_count = NEW2(nstates, int);

    k = default_goto(start_symbol + 1);

  printf("//yyDgoto %d\n", 1 + nsyms - (start_symbol+2)), ++ outline;
  
  printf("%s%6d,", prefix, k), j = 1;
  save_column(start_symbol + 1, k);
  for (i = start_symbol + 2; i < nsyms; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    k = default_goto(i);
    printf("%6d,", k), ++ j;
    save_column(i, k);
  }
  putchar('\n'), ++ outline;

  FREE(state_count);
}

void sort_actions() {
  register int i;
  register int j;
  register int k;
  register int t;
  register int w;

  order = NEW2(nvectors, int);
  nentries = 0;

  for (i = 0; i < nvectors; i++)
    {
      if (tally[i] > 0)
	{
	  t = tally[i];
	  w = width[i];
	  j = nentries - 1;

	  while (j >= 0 && (width[order[j]] < w))
	    j--;

	  while (j >= 0 && (width[order[j]] == w) && (tally[order[j]] < t))
	    j--;

	  for (k = nentries - 1; k > j; k--)
	    order[k + 1] = order[k];

	  order[j + 1] = i;
	  nentries++;
	}
    }
}


void pack_table() {
    register int i;
    register int place;
    register int state;

    base = NEW2(nvectors, int);
    pos = NEW2(nentries, int);

    maxtable = 1000;
    table = NEW2(maxtable, int);
    check = NEW2(maxtable, int);

    lowzero = 0;
    high = 0;

    for (i = 0; i < maxtable; i++)
	check[i] = -1;

    for (i = 0; i < nentries; i++)
    {
	state = matching_vector(i);

	if (state < 0)
	    place = pack_vector(i);
	else
	    place = base[state];

	pos[i] = place;
	base[order[i]] = place;
    }

    for (i = 0; i < nvectors; i++)
    {
	if (froms[i])
	    FREE(froms[i]);
	if (tos[i])
	    FREE(tos[i]);
    }

    FREE(froms);
    FREE(tos);
    FREE(pos);
}


void output_yyDgoto (const char * prefix) {
    nvectors = 2*nstates + nvars;

    froms = NEW2(nvectors, int *);
    tos = NEW2(nvectors, int *);
    tally = NEW2(nvectors, int);
    width = NEW2(nvectors, int);

    token_actions();
    FREE(lookaheads);
    FREE(LA);
    FREE(LAruleno);
    FREE(accessing_symbol);

    goto_actions(prefix);
    FREE(goto_map + ntokens);
    FREE(from_state);
    FREE(to_state);

    sort_actions();
    pack_table();
}


int default_goto(int symbol) {
    register int i;
    register int m;
    register int n;
    register int default_state;
    register int max;

    m = goto_map[symbol];
    n = goto_map[symbol + 1];

    if (m == n) return (0);

    for (i = 0; i < nstates; i++)
	state_count[i] = 0;

    for (i = m; i < n; i++)
	state_count[to_state[i]]++;

    max = 0;
    default_state = 0;
    for (i = 0; i < nstates; i++)
    {
	if (state_count[i] > max)
	{
	    max = state_count[i];
	    default_state = i;
	}
    }

    return (default_state);
}





/*  The function matching_vector determines if the vector specified by	*/
/*  the input parameter matches a previously considered	vector.  The	*/
/*  test at the start of the function checks if the vector represents	*/
/*  a row of shifts over terminal symbols or a row of reductions, or a	*/
/*  column of shifts over a nonterminal symbol.  Berkeley Yacc does not	*/
/*  check if a column of shifts over a nonterminal symbols matches a	*/
/*  previously considered vector.  Because of the nature of LR parsing	*/
/*  tables, no two columns can match.  Therefore, the only possible	*/
/*  match would be between a row and a column.  Such matches are	*/
/*  unlikely.  Therefore, to save time, no attempt is made to see if a	*/
/*  column matches a previously considered vector.			*/
/*									*/
/*  Matching_vector is poorly designed.  The test could easily be made	*/
/*  faster.  Also, it depends on the vectors being in a specific	*/
/*  order.								*/

int matching_vector(int vector) {
    register int i;
    register int j;
    register int k;
    register int t;
    register int w;
    register int match;
    register int prev;

    i = order[vector];
    if (i >= 2*nstates)
	return (-1);

    t = tally[i];
    w = width[i];

    for (prev = vector - 1; prev >= 0; prev--)
    {
	j = order[prev];
	if (width[j] != w || tally[j] != t)
	    return (-1);

	match = 1;
	for (k = 0; match && k < t; k++)
	{
	    if (tos[j][k] != tos[i][k] || froms[j][k] != froms[i][k])
		match = 0;
	}

	if (match)
	    return (j);
    }

    return (-1);
}



int pack_vector(int vector) {
    register int i, j, k, l;
    register int t;
    register int loc;
    register int ok;
    register int *from;
    register int *to;
    int newmax;

    i = order[vector];
    t = tally[i];
    assert(t);

    from = froms[i];
    to = tos[i];

    j = lowzero - from[0];
    for (k = 1; k < t; ++k)
	if (lowzero - from[k] > j)
	    j = lowzero - from[k];
    for (;; ++j)
    {
	if (j == 0)
	    continue;
	ok = 1;
	for (k = 0; ok && k < t; k++)
	{
	    loc = j + from[k];
	    if (loc >= maxtable)
	    {
		if (loc >= MAXTABLE)
		    fatal("maximum table size exceeded");

		newmax = maxtable;
		do { newmax += 200; } while (newmax <= loc);
		table = (int *) REALLOC(table, newmax*sizeof(int));
		if (table == 0) no_space();
		check = (int *) REALLOC(check, newmax*sizeof(int));
		if (check == 0) no_space();
		for (l  = maxtable; l < newmax; ++l)
		{
		    table[l] = 0;
		    check[l] = -1;
		}
		maxtable = newmax;
	    }

	    if (check[loc] != -1)
		ok = 0;
	}
	for (k = 0; ok && k < vector; k++)
	{
	    if (pos[k] == j)
		ok = 0;
	}
	if (ok)
	{
	    for (k = 0; k < t; k++)
	    {
		loc = j + from[k];
		table[loc] = to[k];
		check[loc] = from[k];
		if (loc > high) high = loc;
	    }

	    while (check[lowzero] != -1)
		++lowzero;

	    return (j);
	}
    }
}

void output_yySindex (const char * prefix) {
  int i, j;

  printf("//yySindex %d\n", nstates), ++ outline;
  
  printf("%s%6d,", prefix, base[0]), j = 1;
  for (i = 1; i < nstates; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", base[i]), ++ j;
  }
  putchar('\n'), ++ outline;
}

void output_yyRindex (const char * prefix) {
  int i, j;

  printf("//yyRindex %d\n", nstates), ++ outline;
  
  printf("%s%6d,", prefix, base[nstates]), j = 1;
  for (i = nstates + 1; i < 2*nstates; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", base[i]), ++ j;
  }
  putchar('\n'), ++ outline;
}

void output_yyGindex (const char * prefix) {
  int i, j;

  printf("//yyGindex %d\n", 1 + nvectors-1 - (2*nstates + 1)), ++ outline;
  
  printf("%s%6d,", prefix, base[2*nstates]), j = 1;
  for (i = 2*nstates + 1; i < nvectors - 1; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", base[i]), ++ j;
  }
  putchar('\n'), ++ outline;

  FREE(base);
}

void output_yyTable (const char * prefix) {
  int i, j;

  printf("//yyTable %d\n", high+1), ++ outline;
  
  printf("%s%6d,", prefix, table[0]), j = 1, ++ outline;
  for (i = 1; i <= high; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", table[i]), ++ j;
  }
  putchar('\n'), ++ outline;

  FREE(table);
}

void output_yyCheck (const char * prefix) {
  int i, j;

  printf("//yyCheck %d\n", high+1), ++ outline;
  
  printf("%s%6d,", prefix, check[0]), j = 1;
  for (i = 1; i <= high; ++ i) {
    if (j >= 10) printf("\n%s", prefix), j = 0, ++ outline;
    printf("%6d,", check[i]), ++ j;
  }
  putchar('\n'), ++ outline;

  FREE(check);
}


int is_C_identifier(char *name) {
    register char *s;
    register int c;

    s = name;
    c = *s;
    if (c == '"')
    {
	c = *++s;
	if (!isalpha(c) && c != '_' && c != '$')
	    return (0);
	while ((c = *++s) != '"')
	{
	    if (!isalnum(c) && c != '_' && c != '$')
		return (0);
	}
	return (1);
    }

    if (!isalpha(c) && c != '_' && c != '$')
	return (0);
    while ((c = *++s))
    {
	if (!isalnum(c) && c != '_' && c != '$')
	    return (0);
    }
    return (1);
}


void output_defines(char *prefix) {
    register int c, i;
    register char *s;

    for (i = 2; i < ntokens; ++i)
    {
	s = symbol_name[i];
	if (is_C_identifier(s))
	{
	    printf("  %s ", prefix);
	    c = *s;
	    if (c == '"')
	    {
		while ((c = *++s) != '"')
		{
		    putchar(c);
		}
	    }
	    else
	    {
		do
		{
		    putchar(c);
		}
		while ((c = *++s));
	    }
	    ++outline;
	    printf(" = %d;\n", symbol_value[i]);
	}
    }

    ++outline;
    printf("  %s yyErrorCode = %d;\n", prefix, symbol_value[1]);
}


void output_stored_text(FILE *file, char *name) {
    register int c;
    register FILE *in;

    fflush(file);
    in = fopen(name, "r");
    if (in == NULL)
	open_error(name);
    if ((c = getc(in)) != EOF) {
      if (c ==  '\n')
	++outline;
      putchar(c);
      while ((c = getc(in)) != EOF)
      {
	if (c == '\n')
	    ++outline;
    	putchar(c);
      }
      printf(line_format, ++outline + 1, "-");
    }
    fclose(in);
}

void output_yyFinal (const char * prefix) {
  printf("  %s %d;\n", prefix, final_state), ++ outline;
}

void output_yyNames (const char * prefix) {
  int i, j, max;

  max = 0;
  for (i = 2; i < ntokens; ++i)
    if (symbol_value[i] > max)
      max = symbol_value[i];

  printf("//yyNames %d %d\n", max+1, 1 + ntokens-2), ++ outline;
  printf("%s 0 end-of-file\n", prefix), ++ outline;
  for (i = 2; i < ntokens; ++ i)
    printf("%s %d %s\n", prefix, symbol_value[i], symbol_name[i]), ++ outline;
}

void output_yyNames_strings () {
  int i, j, k, max;
  char **symnam, *s;

  max = 0;
  for (i = 2; i < ntokens; ++i)
    if (symbol_value[i] > max)
      max = symbol_value[i];

  symnam = (char **) MALLOC((max+1)*sizeof(char *));
  if (symnam == 0) no_space();
  
  for (i = 0; i < max; ++i) /* no need to init [max] */
    symnam[i] = 0;
  for (i = ntokens - 1; i >= 2; --i)
    symnam[symbol_value[i]] = symbol_name[i];
  symnam[0] = "end-of-file";
  
  j = 0; fputs("    ", stdout);
  for (i = 0; i <= max; ++i) {
    if ((s = symnam[i])) {
      if (s[0] == '"') {
        k = 7;
        while (*++s != '"') {
          ++k;
          if (*s == '\\') {
            k += 2;
            if (*++s == '\\') ++k;
          }
        }
        j += k;
        if (j > 70) {
          printf("\n    "), ++ outline;
          j = k;
        }
        printf("\"\\\"");
        s = symnam[i];
        while (*++s != '"') {
          if (*s == '\\') {
            printf("\\\\");
            if (*++s == '\\') printf("\\\\");
            else putchar(*s);
          } else
            putchar(*s);
        }
        printf("\\\"\",");
      } else if (s[0] == '\'') {
        if (s[1] == '"') {
          j += 7;
          if (j > 70) {
            printf("\n    "), ++ outline;
            j = 7;
          }
          printf("\"'\\\"'\",");
        } else {
          k = 5;
          while (*++s != '\'') {
            ++k;
            if (*s == '\\') {
              k += 2;
              if (*++s == '\\')
                ++k;
            }
          }
          j += k;
          if (j > 70) {
            printf("\n    "), ++ outline;
            j = k;
          }
          printf("\"'");
          s = symnam[i];
          while (*++s != '\'') {
            if (*s == '\\') {
              printf("\\\\");
              if (*++s == '\\') printf("\\\\");
              else putchar(*s);
            } else
              putchar(*s);
          }
          printf("'\",");
        }
      } else {
        k = strlen(s) + 3;
        j += k;
        if (j > 70) {
          printf("\n    "), ++ outline;
          j = k;
        }
        putchar('"');
        do { putchar(*s); } while (*++s);
        printf("\",");
      }
    } else {
      j += 5;
      if (j > 70) {
        printf("\n    "), ++ outline;
        j = 5;
      }
      printf("null,");
    }
  }
  putchar('\n'), ++ outline;
  FREE(symnam);
}

void output_yyRule (const char * prefix) {
  int i, j;

  printf("//yyRule %d\n", nrules - 2), ++ outline;
  for (i = 2; i < nrules; ++ i) {
    printf("%s %s:", prefix, symbol_name[rlhs[i]]);
    for (j = rrhs[i]; ritem[j] > 0; ++ j)
      printf(" %s", symbol_name[ritem[j]]);
    putchar('\n'), ++ outline;
  }
}

void output_yyRule_strings () {
  int i, j;
  char *s;
  char * prefix = tflag ? "" : "//t";

  for (i = 2; i < nrules; ++i) {
    printf("%s    \"%s :", prefix, symbol_name[rlhs[i]]);
    for (j = rrhs[i]; ritem[j] > 0; ++j) {
      s = symbol_name[ritem[j]];
      if (s[0] == '"') {
        printf(" \\\"");
        while (*++s != '"') {
          if (*s == '\\') {
            if (s[1] == '\\') printf("\\\\\\\\");
            else printf("\\\\%c", s[1]);
            ++s;
          } else
            putchar(*s);
        }
        printf("\\\"");
      } else if (s[0] == '\'') {
        if (s[1] == '"')
          printf(" '\\\"'");
        else if (s[1] == '\\') {
          if (s[2] == '\\') printf(" '\\\\\\\\");
          else printf(" '\\\\%c", s[2]);
          s += 2;
          while (*++s != '\'')
            putchar(*s);
          putchar('\'');
        } else
          printf(" '%c'", s[1]);
      } else
        printf(" %s", s);
    }
    printf("\",\n"), ++ outline;
  }
}

void output_trailing_text() {
    register int c, last;
    register FILE *in;

    if (line == 0)
	return;

    in = input_file;
    c = *cptr;
    if (c == '\n')
    {
	++lineno;
	if ((c = getc(in)) == EOF)
	    return;
        ++outline;
	printf(line_format, lineno, input_file_name);
	if (c == '\n')
	    ++outline;
	putchar(c);
	last = c;
    }
    else
    {
	++outline;
	printf(line_format, lineno, input_file_name);
	do { putchar(c); } while ((c = *++cptr) != '\n');
	++outline;
	printf("\n      ");
	last = '\n';
    }

    while ((c = getc(in)) != EOF)
    {
	if (c == '\n')
	    ++outline;
	putchar(c);
	last = c;
    }

    if (last != '\n')
    {
	++outline;
	printf("\n      ");
    }
    printf(line_format, ++outline + 1, "-");
}


void output_semantic_actions() {
    register int c, last;

    fclose(action_file);
    action_file = fopen(action_file_name, "r");
    if (action_file == NULL)
	open_error(action_file_name);

    if ((c = getc(action_file)) == EOF)
	return;

    last = c;
    if (c == '\n')
	++outline;
    putchar(c);
    while ((c = getc(action_file)) != EOF)
    {
	if (c == '\n')
	    ++outline;
	putchar(c);
	last = c;
    }

    if (last != '\n')
    {
	++outline;
	printf("\n      ");
    }

    printf(line_format, ++outline + 1, "-");
}


void free_itemsets() {
    register core *cp, *next;

    FREE(state_table);
    for (cp = first_state; cp; cp = next)
    {
	next = cp->next;
	FREE(cp);
    }
}


void free_shifts() {
    register shifts *sp, *next;

    FREE(shift_table);
    for (sp = first_shift; sp; sp = next)
    {
	next = sp->next;
	FREE(sp);
    }
}



void free_reductions() {
    register reductions *rp, *next;

    FREE(reduction_table);
    for (rp = first_reduction; rp; rp = next)
    {
	next = rp->next;
	FREE(rp);
    }
}

void output () {
  int lno = 0;
  char buf [128];

  free_itemsets();
  free_shifts();
  free_reductions();

  while (fgets(buf, sizeof buf, stdin) != NULL) {
    char * cp;
    ++ lno;
    if (buf[strlen(buf)-1] != '\n')
      fprintf(stderr, "jay: line %d is too long\n", lno), done(1);
    switch (buf[0]) {
    case '#':	continue;
    case 't':	if (!tflag) fputs("//t", stdout);
    case '.':	break;
    default:
      cp = strtok(buf, " \t\r\n");
      if (cp) {
        char * prefix = strtok(NULL, "\r\n");
        if (!prefix) prefix = "";
        
        if (strcmp(cp, "actions") == 0)       output_semantic_actions();
        else if (strcmp(cp, "epilog") == 0)   output_trailing_text();
        else if (strcmp(cp, "local") == 0)    output_stored_text(local_file, local_file_name);
        else if (strcmp(cp, "prolog") == 0)   output_stored_text(prolog_file, prolog_file_name);
        else if (strcmp(cp, "tokens") == 0)   output_defines(prefix);
        else if (strcmp(cp, "version") == 0)  output_version(prefix);
        else if (strcmp(cp, "yyCheck") == 0)  output_yyCheck(prefix);
        else if (strcmp(cp, "yyDefRed") == 0) output_yyDefRed(prefix);
        else if (strcmp(cp, "yyDgoto") == 0)  output_yyDgoto(prefix); 
        else if (strcmp(cp, "yyFinal") == 0)  output_yyFinal(prefix);
        else if (strcmp(cp, "yyGindex") == 0) output_yyGindex(prefix);
        else if (strcmp(cp, "yyLen") == 0)    output_yyLen(prefix);
        else if (strcmp(cp, "yyLhs") == 0)    output_yyLhs(prefix);
        else if (strcmp(cp, "yyNames-strings") == 0) output_yyNames_strings();
        else if (strcmp(cp, "yyNames") == 0)   output_yyNames(prefix);
        else if (strcmp(cp, "yyRindex") == 0) output_yyRindex(prefix);
        else if (strcmp(cp, "yyRule-strings") == 0) output_yyRule_strings();
        else if (strcmp(cp, "yyRule") == 0)   output_yyRule(prefix);
        else if (strcmp(cp, "yySindex") == 0) output_yySindex(prefix);
        else if (strcmp(cp, "yyTable") == 0)  output_yyTable(prefix);
        else
          fprintf(stderr, "jay: unknown call (%s) in line %d\n", cp, lno);
      }
      continue;
    }
    fputs(buf+1, stdout), ++ outline;
  }
  free_parser();
}


