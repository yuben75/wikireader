/*
 * GPL from wikipedia-iphonw
 * Patrick Collison <patrick@collison.ie>
 * Josef Weidendorfer <Josef.Weidendorfer@gmx.de>
 */
/*
 * Copyright (c) 1995 Wolfram Schneider <wosch@FreeBSD.org>. Berlin.
 * Copyright (c) 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
 *
 * $FreeBSD: src/usr.bin/locate/locate/fastfind.c,v 1.12.2.1 2001/02/22 13:28:52 wosch Exp $
 * $DragonFly: src/usr.bin/locate/locate/fastfind.c,v 1.3 2005/08/04 17:31:23 drhodus Exp $
 */

#include <ctype.h>
#include <string.h>
#include "lsearcher.h"

int kill_switch;
ucaddr_t addr = 0;

void kill_search() {
  kill_switch = 1;
}

int check_bigram_char(int ch) {
    /* legal bigram: 0, ASCII_MIN ... ASCII_MAX */
    if (ch == 0 || (ch >= ASCII_MIN && ch <= ASCII_MAX))
        return(ch);

    fatal("locate database header corrupt, bigram char outside 0, %d-%d: %d",  
        ASCII_MIN, ASCII_MAX, ch);
    exit(1);
}

char *tolower_word(char *word) {
	register char *p;

    for(p = word; *p != '\0'; p++)
        *p = TOLOWER(*p);

    return(word);
}

int matches = 0;

bool handle_match(uchar_t *s) {
  printf("%s\n", s);
  return true;
}

void init_index(lindex *l, FILE *db_file, FILE *prefix_file) {
    uchar_t *p, *s;
    int c;

    for (c = 0, p = l->bigram1, s = l->bigram2; c < NBG; c++) {
        p[c] = check_bigram_char(getc(db_file));
        s[c] = check_bigram_char(getc(db_file));
    }

    /* database */
    l->db_file = db_file;
    fgetpos(l->db_file, &l->db_start);

    /* prefix table */
    if (prefix_file)
        fread(&l->prefixdb, sizeof(uint32_t), CHAR_MAX, prefix_file);
}

void load_index(lindex *l, char *path, char *ppath) {
    FILE *db = NULL;
    FILE *offset_file = NULL;

    debug("load_index(0x%p, %s, %s)", l, path, ppath);

    db = fopen(path, "r");
    if(ppath) offset_file = fopen(ppath, "r");

    init_index(l, db, offset_file);

    if (offset_file)
        fclose(offset_file);
}

/*
 * extract the start positions of 0-9 and A-Z from the index
 * the switch happens when the count is 0, meaning nothing
 * from the previous string should be taken...
 */
void scan(lindex *l, char *scan_file) {
    int c;
    int count = 0;
    off_t file_offset = 0;
    uchar_t path[MAXSTR];
    uchar_t *p;

    /* don't bother supporting >2gb... an index file that large
     * would have search times so high as to be useless 
     */

    debug("scanning through plenty of chars...");

    file_offset = 1;
    c = getc(l->db_file);
    for (; c != EOF; ) {
        off_t this_offset = file_offset - 1;

        /* go forward or backward */
        if (c == SWITCH) { /* big step, an integer */
            count +=  getw(l->db_file) - OFFSET;
            file_offset += sizeof(int);
        } else {	   /* slow step, =< 14 chars */
            count += c - OFFSET;
        }

        /* overlay old path */
        p = path + count;


        /* nothing got reused -> first char is different */
        if (count == 0) {
            for (;;) {
                c = getc(l->db_file);
                ++file_offset;
                /*
                 * == UMLAUT: 8 bit char followed
                 * <= SWITCH: offset
                 * >= PARITY: bigram
                 * rest:      single ascii char
                 *
                 * offset < SWITCH < UMLAUT < ascii < PARITY < bigram
                 */
                if (c < PARITY) {
                    if (c <= UMLAUT) {
                        if (c == UMLAUT) {
                            c = getc(l->db_file);
                            ++file_offset;
                        } else
                            break; /* SWITCH */
                    }
                    *p++ = c;
                } else {		
                    /* bigrams are parity-marked */
                    TO7BIT(c);
                    *p++ = l->bigram1[c];
                    *p++ = l->bigram2[c];
                }
            }

            *p-- = '\0';
            if(isalnum(path[0])) {
                /* ignore unicode chars for now */
                l->prefixdb[path[0]] = this_offset;
                debug("%c starts at 0x%x", path[0], (int)this_offset);
            }
        } else {
            /* skip stuff... until the next switch... */
            for (;;) {
                c = getc(l->db_file);
                ++file_offset;
                if (c < PARITY && c <= UMLAUT) {
                    if (c == UMLAUT) {
                        c = getc(l->db_file);
                        ++file_offset;
                    } else
                        break; /* SWITCH */
                }
            }
        }
    }

    /* write it out */
    FILE *fp = fopen(scan_file, "w");
    if (!fp)
        return;

    fwrite(l->prefixdb, sizeof(uint32_t), CHAR_MAX, fp);
    fclose(fp);
}

uchar_t *pathprep(char *path) {
    char *patend;
    size_t patlen = strlen(path);
    char *patptr = malloc(patlen + 2);
    bzero(patptr, patlen + 2);
    *patptr = '\0';
    strncpy(patptr, path, patlen);
    patend = patptr + patlen - 1;
    *(patptr + patlen) = '\0';
    return (uchar_t *)patend;
}

int search(lindex *l, char *pathpart, resultf f, donef df, bool icase, bool strict) {
	register uchar_t *p, *s, *patend, *q, *foundchar;
	register int c, cc;
	int count, found;
    uchar_t *cutoff = NULL, path[MAXSTR];

    kill_switch = 0;

    /* use a lookup table for case insensitive search */
    uchar_t table[UCHAR_MAX + 1];

    if(icase)
        tolower_word(pathpart);

    patend = pathprep(pathpart);
    cc = *patend;

    /* set patend char to true */
    for (c = 0; c < UCHAR_MAX + 1; c++)
        table[c] = 0;

    if(icase) {
        table[TOLOWER(*patend)] = 1;
        table[toupper(*patend)] = 1;
    } else
        table[*patend] = 1;

    /* main loop */
    found = count = 0;
    foundchar = 0;

    debug("checking for prefixdb... %p", l->prefixdb);
    int offset = -1;
    bool skip = false;

    fsetpos(l->db_file, &l->db_start);
    offset = l->prefixdb[toupper(*pathpart)];
    debug("offset: %d", offset);
    if(strict && l->prefixdb && (offset > 0)) {
        debug("using prefix db");
        fseek(l->db_file, offset, SEEK_CUR);
        skip = true;
    }

	c = getc(l->db_file);
	for (; c != EOF; ) {
        if(kill_switch)
            return -1;

        if (c == SWITCH) {
			int local_count =  getw(l->db_file) - OFFSET;
            if(!skip)
                count += local_count;
        } else if(!skip) {
            count += c - OFFSET;
        }

        skip = false;

        p = path + count;
        foundchar = p - 1;

		for (;;) {
            if(kill_switch)
                return -1;

			c = getc(l->db_file);
			/*
			 * == UMLAUT: 8 bit char followed
			 * <= SWITCH: offset
			 * >= PARITY: bigram
			 * rest:      single ascii char
			 *
			 * offset < SWITCH < UMLAUT < ascii < PARITY < bigram
			 */
			if (c < PARITY) {
				if (c <= UMLAUT) {
					if (c == UMLAUT) {
						c = getc(l->db_file);
					} else
						break; /* SWITCH */
				}
				if (table[c])
					foundchar = p;
				*p++ = c;
			}
			else {		
				/* bigrams are parity-marked */
				TO7BIT(c);

				if (table[l->bigram1[c]] || table[l->bigram2[c]])
				    foundchar = p + 1;

				*p++ = l->bigram1[c];
				*p++ = l->bigram2[c];
			}
		}

        if (found) {
            cutoff = path;
            *p-- = '\0';
            foundchar = p;
        } else if (foundchar >= path + count) {
            *p-- = '\0';
            cutoff = path + count;
        } else if(!strict) {
            continue;
        } else { 
            *p-- = '\0';
        }

        found = 0;
        if(strict) {
            for(s = (uchar_t *)path, q = (uchar_t *)pathpart; *q; s++, q++)
                if((icase && TOLOWER(*s) != *q) || (!icase && *s != *q))
                    break;
            if(*q == '\0') {
                if(!f(path))
                    return 0;
            }
        } else {
            for (s = foundchar; s >= cutoff; s--) {
                if (*s == cc || (icase && TOLOWER(*s) == cc)) {
                    for (p = patend - 1, q = s - 1; *p != '\0'; p--, q--)
                        if (*q != *p && (!icase || TOLOWER(*q) != *p))
                            break;
                    if (*p == '\0' && \
                            (icase ? \
                             strncasecmp((const char *)path, pathpart, strlen(pathpart)) : \
                             strncmp((const char *)path, pathpart, strlen(pathpart)))) {
                        found = 1;
                        if(!f(path))
                            return 0;
                        break;
                    }
                }
            }
        }
    }

    if(df)
        df();
    return 0;
}

#ifdef INCLUDE_MAIN
void usage(char *prog) {
  fatal("%s -f <indexFile> [-c <scanFile>] [-s <search>]", prog);
}

int main(int argc, char **argv) {
  extern char *optarg;
  char scanFile[MAXSTR], indexFile[MAXSTR], needle[MAXSTR];
  unsigned char ch; 
  bool doScan = false, doSearch = false, haveScanFile = false, icase = true, twoRuns = false;
  lindex l;
  memset(&l, 0, sizeof(l));

  debug = false;

  while ((ch = getopt(argc, argv, "dac:s:f:hnp")) != 255) {
    switch (ch) {
    case 'c':
      haveScanFile = true;
      strncpy(scanFile, optarg, MAXSTR);
      break;
    case 'n':
      doScan = true;
    case 'd':
      debug = true;
      break;
    case 'f':
      strncpy(indexFile, optarg, MAXSTR);
      break;
    case 's':
      doSearch = true;
      strncpy(needle, optarg, MAXSTR);
      break;
    case 'a':
      icase = false;
      break;
    case 'p':
      twoRuns = true;
      break;
    case 'h':
    default:
      usage(argv[0]); 
    }
  }

  if(indexFile) 
    load_index(&l, indexFile, (haveScanFile && doSearch) ? scanFile : NULL);
  else {
    debug("no index file");
    usage(argv[0]);
  }

  if(doScan)
    scan(&l, scanFile);
  else if(doSearch) {
    search(&l, needle, handle_match, NULL, icase, true);
    if (twoRuns)
        search(&l, needle, handle_match, NULL, icase, false);
  }else {
    debug("no action");
    usage(argv[0]);
  }

  return 0;
}
#endif
