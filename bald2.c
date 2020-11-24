
#ifndef _DEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#else
#ifdef NDEBUG
#error NDEBUG and _DEBUG are uncompatible
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <locale.h>
#include <assert.h>
#ifdef __linux__
#include <ctype.h>
#include <wchar.h>
#include <sys/stat.h>
#else
#include <io.h>
#include <windows.h>
#endif

#define FIELD_SZ	10
#define DEF_FIELD_SZ 5
#define MAX_WORD_SZ	20

#define WUSED_SZ	1024
#define WORDS_SZ	4096

#define INPUT_BUF_SZ 128

#define DIC_FILE_NAME	"dic.dat"

#if WORDS_SZ > 65536
#error WORDS_SZ must be < 65536
#endif

#if defined(__x86_64__) || defined(_WIN64)
#define FMT_LMDF_64 "l"
#else
#define FMT_LMDF_64
#endif

char field [FIELD_SZ][FIELD_SZ];
char mark_field [FIELD_SZ][FIELD_SZ];
int szx = 0, szy = 0;
char dirv [MAX_WORD_SZ];
unsigned char wstat [MAX_WORD_SZ];
char wused [WUSED_SZ];
char *pwused = wused;
char words [WORDS_SZ];
char *pwords;
int nwords;

char buf [INPUT_BUF_SZ];
char buf2 [INPUT_BUF_SZ];
#ifdef __linux__
wchar_t wbuf [INPUT_BUF_SZ];
size_t strconv (const char *src);
void uprint (const char *str);
#endif

void clear_field (void);

unsigned input (const char *prompt);
char *dic = NULL;
size_t dic_l = 0;

long file_length (FILE *f);
void cleanup (void);

struct sqvec {
	int dx, dy;
} sqdir [4] = {
	{  1,  0 },
	{  0,  1 },
	{ -1,  0 },
	{  0, -1 }
};

static const char s_digs[] = "0123456789";
static const char null_s[] = "";

int valid_cell (int x, int y);

struct findw_s {
	int i, j, s, w;
	const char *dic;
	size_t dicl;
	signed char lc, subc, subx, suby, ax, ay, f_lt_n;
};

const char *find_word (struct findw_s *pfws);
void init_words (void);
int add_word (const struct findw_s *pfws);
void print_words (void);

int isalldigits (const char *s, int sl);
char *strupper (char *str);
void dump_field (void);
int levdamdist (const char *s1, int l1, const char *s2, int l2);

#ifdef _WIN32
#ifndef _CRTAPI1
#define _CRTAPI1 __cdecl
#endif
#else
#define _CRTAPI1
#endif

#ifdef _DEBUG
void check_runtime (void);
#endif

extern const char *msgs[];

int _CRTAPI1 main (int argc, const char *argv[], const char *envp[])
{
	int i, j, s, n;
	long l;
	FILE *f;
	char cz [2] = { 0, 0 };
	struct findw_s fws;
	const char *pw;
	char *pwcan = NULL;
	char f_h = 1;

#ifdef _DEBUG
	check_runtime ();
#endif
#ifdef __linux__
	if (!setlocale (LC_ALL, "C.UTF-8"))
		{ perror ("Set locale failed"); abort(); }
#else
	setlocale (LC_ALL, ".OCP");
#endif
	f = fopen (DIC_FILE_NAME, "rb");
	if (f == NULL) {
		puts (msgs [0]);
		perror (NULL);
		return 1;
	}
	l = file_length (f);
	if (l == -1) {
		puts (msgs [3]);
		perror (NULL);
		fclose (f);
		return 1;
	}
	if (l < 2) {
		fclose (f);
		puts (msgs [1]);
		return 1;
	}
	dic = (char*)malloc ((size_t)l);
	if (dic == NULL) {
		fclose (f);
		puts (msgs [2]);
		return 1;
	}
	if (fread (dic, l, 1, f) != 1) {
		puts (msgs [4]);
		perror (NULL);
		fclose (f);
		cleanup ();
		return 1;
	}
	fclose (f);
	dic_l = l;
	if (dic [dic_l - 1] != '\0') {
		puts (msgs [6]);
		dic [dic_l - 1] = '\0';
	}
	for (i = 0, n = 0; i < dic_l; i++)
		if (dic [i] == '\0') n++;
	printf (msgs [5], n, dic_l);

	clear_field (); szx = 0;
	s = input (msgs [7]);
	if (isalldigits (buf, s)) {
		szx = szy = atoi (buf);
		if (szx > FIELD_SZ) {
			puts (msgs [8]);
			szx = szy = FIELD_SZ;
		}
		if (szx < 2) {
			puts (msgs [9]);
			szx = szy = DEF_FIELD_SZ;
		}
		printf (msgs [10], szx, szy);
		s = input (msgs [11]);
	}
#ifdef __linux__
	strconv (buf);
	s = strlen (buf);
#endif
	strupper (buf);
#ifdef _WIN32
	OemToChar (buf, buf);
#endif
	if (szx == 0) {
		if (s > FIELD_SZ) {
			puts (msgs [8]);
			szx = szy = FIELD_SZ;
		} else if (s < 2) {
			puts (msgs [9]);
			szx = szy = DEF_FIELD_SZ;
		} else
			szx = szy = s;
		printf (msgs [10], szx, szy);
	}
	strcpy (wused, buf);
	pwused += s + 1;
	if (s != szx) {
		if (s > szx)
			puts (msgs [12]);
		else	puts (msgs [13]);
	}
	if (s > szx) s = szx;
	memcpy (field [(szy - 1) / 2], buf, s);
	while (1) {
		n = 0;
		memset (wstat, 0, sizeof (wstat));
		memset (&fws, 0, sizeof (fws));
		init_words ();
		while (pw = find_word (&fws), pw) {
			char *pwu;
			for (pwu = wused; pwu < pwused; pwu += strlen (pwu) + 1)
				if (strcmp (pw, pwu) == 0) break;
			if (pwu < pwused)
				continue;
			s = add_word (&fws);
			if (s == 1) {
				l = strlen (&dic [fws.w]);
				if (l < MAX_WORD_SZ && l > 0) {
					wstat [l-1]++;
					if (wstat [l-1] == 0)
						wstat [l-1] --;
				}
				n++;
			}
		}
		print_words ();
		for (i = 1, l = 1; i < MAX_WORD_SZ; i++) {
			if (wstat [i] != 0) {
				for (j = l; j <= i; j++)
					printf ("%2d: %d\n", j+1, wstat [j]);
				l = j;
			}
		}
		dump_field ();
nx_ch:
		if (f_h) {
ln_fmt:
			puts (msgs [16]);
			puts (msgs [17]);
			puts (msgs [18]);
			puts (msgs [19]);
			puts (msgs [20]);
			puts (msgs [29]);
			f_h = 0;
		}
		s =	input (pwcan ? msgs [32] : msgs [28]);
		if (s == 0 || (s == 1 && buf [0] == '\n')) break;
#ifdef __linux__
		strconv (buf);
		s = strlen (buf);
#endif
		strupper (buf);
#ifdef _WIN32
		OemToChar (buf, buf);
#endif
		if (s == 1 && buf [0] == '?' && buf [1] == 0) {
			puts (msgs [30]);
			goto nx_ch;
		}
		if (s > 1 && (buf [0] == '?' || buf [0] == '/')) {
			char c;
			if (strpbrk (&buf[1], s_digs) == NULL) {
				n = 0;
				memset (&fws, 0, sizeof (fws));
				strcpy (buf2, &buf[1]); s--;
				for (i = 0; i < dic_l; i += strlen (&dic[i]) + 1) {
					j = levdamdist (buf2, s, &dic[i], strlen (&dic[i]));
					if (j > 1)
						continue;
					n++;
#ifdef __linux__
					uprint (&dic[i]);
					if (j == 0) {
						fwrite (" ", 1, 1, stdout);
						fwrite (msgs [33], strlen (msgs [33]), 1, stdout);
					}
					puts (null_s);
#else
					strcpy (buf, &dic[i]);
					CharToOem (buf, buf);
					if (j == 0)
						printf ("%s %s\n", buf, msgs [33]);
					else printf ("%s\n", buf);
#endif
				}
#ifdef LANG_RU
				if (n > 0)
					printf (msgs [14], n);
				else puts (msgs [15]);
#else
				if (n > 0)
					printf ("%d", n);
				else fwrite (msgs [14], strlen (msgs [14]), 1, stdout);
				puts (msgs [15]);
#endif
				goto nx_ch;
			}
			if (s > 4 || s < 3) goto ln_fmt;
			cz [0] = buf [s == 3 ? 1 : 2];
			i = atoi (cz) - 1;
			cz [0] = buf [s == 3 ? 2 : 3];
			j = atoi (cz) - 1;
			if (i < 0 || i >= FIELD_SZ) {
				puts (msgs [21]);
				goto ln_fmt;
			}
			if (j < 0 || j >= FIELD_SZ) {
				puts (msgs [22]);
				goto ln_fmt;
			}
			n = 0;
			if (s == 4) c = buf [1];
			else	c = 0;
			memset (&fws, 0, sizeof (fws));
			while (pw = find_word (&fws), pw) {
				if (fws.subx != i || fws.suby != j)
					continue;
				if (c && c != fws.subc)
					continue;
				n++;
#ifdef __linux__
				cz [0] = fws.subc;
				uprint (cz);
				printf (" %d %d: ", fws.subx + 1, fws.suby + 1);
				uprint (pw);
				printf (" %d %d\n", fws.i + 1, fws.j + 1);
#else
				buf [0] = fws.subc;
				strcpy (&buf[1], pw);
				CharToOem (buf, buf);
				printf ("%c %d %d: %s %d %d\n", buf [0],
					fws.subx + 1, fws.suby + 1, &buf[1], fws.i + 1, fws.j + 1);
#endif
			}
#ifdef LANG_RU
			if (n > 0)
				printf (msgs [14], n);
			else puts (msgs [15]);
#else
			if (n > 0)
				printf ("%d", n);
			else fwrite (msgs [14], strlen (msgs [14]), 1, stdout);
			puts (msgs [15]);
#endif
			goto nx_ch;
		}
		if (buf [0] == '#' && ((buf [1] == 'L' && buf [2] == '\0') ||
			strcmp (&buf[1], "LIST") == 0)) {
			n = 0;
			for (pw = wused; pw < pwused; pw += strlen (pw) + 1) {
				n++;
#ifdef __linux__
				uprint (pw);
				fwrite (" ", 1, 1, stdout);
#else
				CharToOem (pw, buf);
				fwrite (buf, strlen (buf), 1, stdout);
				fwrite (" ", 1, 1, stdout);
#endif
			}
			puts (null_s);
#ifdef LANG_RU
			if (n > 0)
				printf (msgs [14], n);
			else puts (msgs [15]);
#else
			if (n > 0)
				printf ("%d", n);
			else fwrite (msgs [14], strlen (msgs [14]), 1, stdout);
			puts (msgs [15]);
#endif
			goto nx_ch;
		}
		if (isalldigits (buf, s)) {
			int vn;

			if (pwcan < pwused) {
				puts (msgs [23]);
				goto nx_ch;
			}
			vn = atoi (buf);
			memset (&fws, 0, sizeof (fws));
			n = 0;
			while (pw = find_word (&fws), pw) {
				if (strcmp (pw, pwcan) != 0)
					continue;
				n++;
				if (n == vn) {
					field [fws.suby][fws.subx] = fws.subc;
					pwused += strlen (pwcan) + 1;
					pwcan = NULL;
					break;
				}
			}
			if (n <= 0 || vn > n) {
				puts (msgs [24]);
				goto nx_ch;
			}
			continue;
		}
		if (s == 3 && isdigit (buf [1]) && isdigit (buf [2])) {
			cz [0] = buf [1];
			i = atoi (cz) - 1;
			cz [0] = buf [2];
			j = atoi (cz) - 1;
			if (i < 0 || i >= FIELD_SZ) {
				puts (msgs [21]);
				goto ch_fmt;
			}
			if (j < 0 || j >= FIELD_SZ) {
				puts (msgs [22]);
ch_fmt:
				puts (msgs [25]);
				goto nx_ch;
			}
			if (buf [0] == '.')
				buf [0] = 0;
			field [j][i] = buf [0];
			pwcan = NULL;
			continue;
		}
		{
			char subc, subx, suby;
			memset (&fws, 0, sizeof (fws));
			n = 0;
			strcpy (buf2, buf);
			while (pw = find_word (&fws), pw) {
				if (strcmp (pw, buf2) != 0)
					continue;
				for (pw = wused; pw < pwused; pw += strlen (pw) + 1)
					if (strcmp (pw, buf2) == 0) break;
				if (pw < pwused)
					continue;
				n++;
				if (n == 1) {
					subc = fws.subc;
					subx = fws.subx;
					suby = fws.suby;
					continue;
				}
				if (n == 2) {
#ifdef __linux__
					cz [0] = subc;
					fwrite (msgs [26], strlen (msgs [26]), 1, stdout);
					uprint (cz);
					printf (" %d %d  ", subx + 1, suby + 1);
#else
					cz [0] = subc;
					CharToOem (cz, cz);
					fwrite (msgs [26], strlen (msgs [26]), 1, stdout);
					printf ("%c %d %d\t", cz [0], subx + 1, suby + 1);
#endif
				}
#ifdef __linux__
				cz [0] = fws.subc;
				printf ("%d) ", n);
				uprint (cz);
				printf (" %d %d  ", fws.subx + 1, fws.suby + 1);
#else
				cz [0] = fws.subc;
				CharToOem (cz, cz);
				printf ("%d) %c %d %d\t", n, cz [0],
					fws.subx + 1, fws.suby + 1);
#endif
			}
			if (n == 0) {
				puts (msgs [31]);
				goto nx_ch;
			}
			if (n > 1)
				puts (null_s);
			if (pwused - wused + s < WUSED_SZ) {
				strcpy (pwused, buf2);
				if (n == 1)
					pwused += strlen (pwused) + 1;
			} else {
				puts (msgs [27]);
				if (n > 1)
					continue;
			}
			if (n > 1) {
				pwcan = pwused;
				goto nx_ch;
			} else
				field [suby][subx] = subc;
		}
	}
	cleanup ();
	return 0;
}

void cleanup (void)
{
	free (dic);
}

void clear_field (void)
{
	memset (field, 0, sizeof (field));
	memset (mark_field, 0, sizeof (mark_field));
}

int valid_cell (int x, int y)
{
	if (x >= 0 && x < szx &&
		y >= 0 && y < szy)
		return 1;
	return 0;
}

unsigned input (const char *prompt)
{
	unsigned l;

	puts (prompt);
	fgets (buf, INPUT_BUF_SZ-1, stdin);
	buf [INPUT_BUF_SZ - 1] = '\0';
	l = strlen (buf);
	if (buf [l-1] == '\n')
		buf [l-1] = '\0', l--;
	return l;
}

int isalldigits (const char *s, int sl)
{
	return strspn (s, s_digs) ==
		(sl != -1 ? sl : strlen (s));
}

const char *find_word (struct findw_s *pfws)
{
	int d;

	if (pfws->dic == NULL) {
		pfws->dic = dic;
		pfws->dicl = dic_l;
	}
	if (pfws->f_lt_n)
		goto dir_bk2;
	for (; pfws->j < szy; pfws->j++, pfws->i = 0)
	for (; pfws->i < szx; pfws->i++) {
		if (!field [pfws->j][pfws->i]) {
			for (d = 0; d < 4; d++) {
				pfws->ax = pfws->i + sqdir [d].dx;
				pfws->ay = pfws->j + sqdir [d].dy;
				if (valid_cell (pfws->ax, pfws->ay) &&
						field [pfws->ay][pfws->ax] != 0) {
					pfws->f_lt_n = 1;
					break;
				}
			}
			if (!pfws->f_lt_n) continue;
		} else
			pfws->f_lt_n = 1;
		pfws->w = 0;
		for (pfws->s = 0; pfws->s < pfws->dicl; pfws->s++, pfws->lc++) {
			if (pfws->w == pfws->s) {
				pfws->ax = pfws->i;
				pfws->ay = pfws->j;
				pfws->subc = 0;
				pfws->lc = 0;
			}
			if (pfws->lc >= MAX_WORD_SZ) goto nx_word;
			if (field [pfws->ay][pfws->ax] == 0) {
				if (pfws->subc == 0) {
					pfws->subc = pfws->dic [pfws->s];
					pfws->subx = pfws->ax;
					pfws->suby = pfws->ay;
				} else goto dir_bk3;
			} else if (field [pfws->ay][pfws->ax] != pfws->dic [pfws->s])
				goto dir_bk3;
			dirv [pfws->lc] = 0;
			mark_field [pfws->ay][pfws->ax] = 1;
			goto nx_lt;
dir_bk2:
			mark_field [pfws->ay][pfws->ax] = 0;
			if (field [pfws->ay][pfws->ax] == 0)
				pfws->subc = 0;
dir_bk3:
			pfws->s--;
			pfws->lc--;
			if (pfws->lc < 0) {
nx_word:
				pfws->s += strlen (&pfws->dic [pfws->s+1]) + 1;
				pfws->w = pfws->s + 1;
				continue;
			}
dir_bk:
			pfws->ax -= sqdir [dirv [pfws->lc]].dx;
			pfws->ay -= sqdir [dirv [pfws->lc]].dy;
			dirv [pfws->lc] ++;
			if (dirv [pfws->lc] >= 4)
				goto dir_bk2;
nx_lt:
			if (pfws->dic [pfws->s + 1] == '\0') {
				if (pfws->subc != 0)
					return &pfws->dic [pfws->w];
				goto dir_bk2;
			}
			pfws->ax += sqdir [dirv [pfws->lc]].dx;
			pfws->ay += sqdir [dirv [pfws->lc]].dy;
			if (!valid_cell (pfws->ax, pfws->ay) ||
					mark_field [pfws->ay][pfws->ax] == 1)
				goto dir_bk;
		}
		pfws->f_lt_n = 0;
	}
	return NULL;
}

struct ltvar_s {
	unsigned short nx;
	char c;
	char x, y;
	char sx, sy;
};

struct widx_s {
	union {
		unsigned ofs;
		struct {
			short dummy;
			char cdummy;
			unsigned char l;
		};
	};
};

#define SIZEOF_LTVAR_S	7
#define WIDX_S_OFFSET(ofs)	((ofs)&0xffffff)

void init_words (void)
{
	nwords = 0; pwords = words;
}

int add_word (const struct findw_s *pfws)
{
	const char *w = &pfws->dic [pfws->w];
	char *pw, f;
	struct ltvar_s *pv;
	struct widx_s *awi = (struct widx_s *)&words [WORDS_SZ];
	size_t l = strlen (w);
	int i;

	if (l > MAX_WORD_SZ) return 0;
	for (i = 1; i <= nwords; i++)
		if (strcmp (w, &words [WIDX_S_OFFSET(awi [-i].ofs)]) == 0) {
			f = 0;
			pv = (struct ltvar_s*)&words
					[WIDX_S_OFFSET(awi [-i].ofs) + awi [-i].l + 1];
			while (1) {
				if (pv->c == pfws->subc && pv->x == pfws->subx &&
						pv->y == pfws->suby) {
					f = 1;
					break;
				}
				if (pv->nx != 0)
					pv = (struct ltvar_s*)((char*)pv + pv->nx);
				else break;
			};
			if (f)
				return -1;
			if (pwords + SIZEOF_LTVAR_S >= (char*)&awi [-nwords])
				return 0;
			pv->nx = pwords - (char*)pv;
			f = 2;
			goto set_pv;
		}
	if (pwords + l + 1 + SIZEOF_LTVAR_S >= (char*)&awi [-nwords-1])
		return 0;
	nwords ++;
	strcpy (pwords, w);
	awi [-nwords].ofs = pwords - words;
	awi [-nwords].l = l;
	pwords += l + 1;
	f = 1;
set_pv:
	pv = (struct ltvar_s*)pwords;
	pv->nx = 0; pv->c = pfws->subc;
	pv->x = pfws->subx; pv->y = pfws->suby;
	pv->sx = pfws->i; pv->sy = pfws->j;
	pwords += SIZEOF_LTVAR_S;
	return f;
}

static int _CRTAPI1 compare_words (const void *w1, const void *w2)
{
	struct widx_s *pw1 = (struct widx_s*)w1,
		*pw2 = (struct widx_s*)w2;

	if (pw1->l < pw2->l) return 1;
	if (pw1->l > pw2->l) return -1;
	return - strcmp (&words[WIDX_S_OFFSET(pw1->ofs)],
		&words[WIDX_S_OFFSET(pw2->ofs)]);
}

void print_words (void)
{
	struct widx_s *awi = (struct widx_s *)&words [WORDS_SZ];
	struct ltvar_s *pv;
	int i, n;
	char *w;
	char cz [2] = { 0, 0 };

	if (nwords) {
		if (nwords > 1)
			qsort (&awi[-nwords], nwords, sizeof (struct widx_s), compare_words);
		for (i = 1; i <= nwords; i++) {
			w = &words[WIDX_S_OFFSET(awi[-i].ofs)];
			pv = (struct ltvar_s*)(w + awi[-i].l + 1);
			if (pv->nx == 0) {
#ifdef __linux__
				cz [0] = pv->c;
				uprint (cz);
				printf (" %d %d:\t", pv->x + 1, pv->y + 1);
				uprint (w);
				printf (" %d %d\n", pv->sx + 1, pv->sy + 1);
#else
				buf [0] = pv->c;
				strcpy (&buf[1], w);
				CharToOem (buf, buf);
				printf ("%c %d %d:\t%s %d %d\n", buf [0],
					pv->x + 1, pv->y + 1, &buf[1], pv->sx + 1, pv->sy + 1);
#endif
			} else {
				fwrite ("\t", 1, 1, stdout);
#ifdef __linux__
				uprint (w);
#else
				CharToOem (w, w);
				fwrite (w, awi[-i].l, 1, stdout);
#endif
				n = 1;
				while (1) {
					cz [0] = pv->c;
					fwrite ("\t", 1, 1, stdout);
#ifdef __linux__
					printf ("%d) ", n);
					uprint (cz);
#else
					CharToOem (cz, cz);
					printf ("%d) %c", n, cz [0]);
#endif
					printf (" %d %d (%d %d)", pv->x, pv->y, pv->sx, pv->sy);
					if (pv->nx != 0)
						pv = (struct ltvar_s*)
							((char*)pv + pv->nx), n++;
					else break;
				}
				puts (null_s);
			}
		}
#ifdef LANG_RU
			printf (msgs [14], nwords);
#else
			printf ("%d", nwords);
			puts (msgs [15]);
#endif	
	} else {
#ifdef LANG_RU
		puts (msgs [15]);
#else
		fwrite (msgs [14], strlen (msgs [14]), 1, stdout);
		puts (msgs [15]);
#endif
	}
#ifdef _DEBUG
	printf ("DBG: words: %dB remain\n", (unsigned)(WORDS_SZ - sizeof (struct widx_s) * nwords -
		(pwords - words)));
#endif
}

#define LEVENSHTEIN_MAX_LENTH 255
#define LEVDROWSIZE (MAX_WORD_SZ+1)

static char levdp1a [LEVDROWSIZE];
static char levdp2a [LEVDROWSIZE];
static char levdp3a [LEVDROWSIZE];

int levdamdist (const char *s1, int l1, const char *s2, int l2)
{
	char *row0 = levdp1a;
	char *row1 = levdp2a;
	char *row2 = levdp3a;
	int i, j;

	if (l1>=LEVDROWSIZE || l2>=LEVDROWSIZE)
		return -1;

	for (j = 0; j <= l2; j++)
		row1[j] = j;
	for (i = 0; i < l1; i++) {
		char *dummy;

		row2[0] = i + 1;
		for (j = 0; j < l2; j++) {
			/* substitution */
			row2[j + 1] = row1[j] + (s1[i] != s2[j] ? 1 : 0);
			/* swap */
			if (i > 0 && j > 0 && s1[i - 1] == s2[j] &&
					s1[i] == s2[j - 1] &&
					row2[j + 1] > row0[j - 1] + 1)
				row2[j + 1] = row0[j - 1] + 1;
			/* deletion */
			if (row2[j + 1] > row1[j + 1] + 1)
				row2[j + 1] = row1[j + 1] + 1;
			/* insertion */
			if (row2[j + 1] > row2[j] + 1)
				row2[j + 1] = row2[j] + 1;
		}

		dummy = row0;
		row0 = row1;
		row1 = row2;
		row2 = dummy;
	}

	return row1[l2];
}

#define A_CYRA_CAP	0xC0
#define A_CYRA_SML	0xE0
#define A_CYRYA_CAP	0xDF
#define A_CYRYA_SML	0xFF

char *strupper (char *str)
{
#ifdef __linux__
	int i, l = strlen (str);
	unsigned char *s = (unsigned char*)str;
	for (i = 0; i < l; i++)
		if (s [i] >= A_CYRA_SML && s [i] <= A_CYRYA_SML)
			s [i] += A_CYRA_CAP - A_CYRA_SML;
	return (char*)s;
#else
	return strupr (str);
#endif
}

void dump_field (void)
{
	int i, j;
	char cz [2];
	cz [1] = '\0';

	fwrite ("  ", 2, 1, stdout);
	for (i = 0; i < szx; i++)
		printf ("%d ", i + 1);
	fwrite ("\n  ", 3, 1, stdout);
	for (i = 0; i < szx; i++)
		fwrite ("--", 2, 1, stdout);
	puts (null_s);
	for (j = 0; j < szy; j++) {
		printf ("%d ", j + 1);
		for (i = 0; i < szx; i++) {
#ifdef __linux__
			cz [0] = field [j][i];
			if (cz [0] == 0)
				cz [0] = '.';
			uprint (cz);
#else
			cz [0] = field [j][i];
			if (cz[0] != 0)
				CharToOem (cz, cz);
			else cz [0] = '.';
			fwrite (&cz[0], 1, 1, stdout);
#endif
			fwrite (" ", 1, 1, stdout);
		}
		puts (null_s);
	}
	fwrite ("  ", 2, 1, stdout);
	for (i = 0; i < szx; i++)
		fwrite ("--", 2, 1, stdout);
	puts (null_s);
}

long file_length (FILE *f)
{
#ifdef __linux__
	struct stat st;
	if (fstat (fileno (f), &st) != 0)
		return -1;
	return st.st_size;
#else
	return filelength (fileno (f));
#endif
}

#ifndef _WIN32

#define U_CYRA_CAP	0x0410
#define U_CYRYA_CAP	0x042f
#define U_CYRA_SML	0x0430
#define U_CYRYA_SML	0x044f

size_t strconv (const char *src)
{
	size_t wl;
	int i, l;

	wl = mbstowcs (wbuf, src, INPUT_BUF_SZ-1);
	wbuf [INPUT_BUF_SZ-1] = L'\0';
	if (wl == -1) return -1;
	for (i = 0; i < wl; i++) {
		if (wbuf [i] >= 0x80) {
			if (wbuf [i] >= U_CYRA_CAP && wbuf [i] <= U_CYRYA_CAP) {
				buf [i] = A_CYRA_CAP + wbuf [i] - U_CYRA_CAP;
				continue;
			}
			if (wbuf [i] >= U_CYRA_SML && wbuf [i] <= U_CYRYA_SML) {
				buf [i] = A_CYRA_SML + wbuf [i] - U_CYRA_SML;
				continue;
			}
			buf [i] = '?';
			continue;
		}
		buf [i] = wbuf [i];
	}
	buf [i] = '\0';
	return wl;
}

void uprint (const char *str)
{
	int i, l = strlen (str);
	size_t sl;
	wchar_t *pwbuf;
	const unsigned char *s = (unsigned char*)str;

	if (l >= INPUT_BUF_SZ) l = INPUT_BUF_SZ - 1;
	for (i = 0; i < l; i++) {
		if (s [i] > 0x80) {
			if (s [i] >= A_CYRA_CAP && s [i] <= A_CYRYA_CAP) {
				wbuf [i] = U_CYRA_CAP + s [i] - A_CYRA_CAP;
				continue;
			}
			if (s [i] >= A_CYRA_SML && s [i] <= A_CYRYA_SML) {
				wbuf [i] = U_CYRA_SML + s [i] - A_CYRA_SML;
				continue;
			}
			wbuf [i] = '?';
			continue;
		}
		wbuf [i] = s [i];
	}
	wbuf [l] = '\0';
	sl = wcstombs (buf, wbuf, sizeof (buf)-1);
	buf [sizeof (buf) - 1] = '\0';
	fwrite (buf, sl, 1, stdout);
}
#endif

#ifdef _DEBUG
void check_runtime (void) {
	assert (sizeof (struct ltvar_s) == SIZEOF_LTVAR_S ||
		sizeof (struct ltvar_s) == SIZEOF_LTVAR_S+1);
}
#endif

#ifndef LANG_RU

const char *msgs[] = {
"Dictionary '" DIC_FILE_NAME "' open error.",
"Dictionary format error: size is too small.",
"Memory allocation error.",
"File I/O error.",
"Dictionary read error.",
"Dictionary size: %d words (%" FMT_LMDF_64 "d bytes)\n",
"Dictionary format error: trailing byte is not 0.",
"Enter start word or field size:",
"Field size is truncated to max value.",
"Field size is too small.",
"Field size: %d x %d\n",
"Enter start word:",
"Start word is truncated.",
"Start word is shorter that field.",
"No",
" words found.",
"Format: <word>",
"\t<letter><x><y>",
"\t?<x><y>",
"\t?<letter><x><y>",
"\t?<word>",
"<x> bad",
"<y> bad",
"No word is pending (position ambiguity).",
"Wrong variant number.",
"Format: <letter><x><y>",
"Word position ambiguity:\n1) ",
"List of words is full.",
"Enter next word or letter (cxy) (empty line is exit):",
"? - help",
"Format: <word>\n"
"\t<letter><x><y>\n"
"\t?<x><y>\n"
"\t?<letter><x><y>\n"
"\t?<word>\n"
"\t+<word>",
"The word can't be placed on field.",
"Enter variant number:",
"(exact)"
};

#else

#ifdef __linux__

const char *msgs[] = {
"╨Ю╤И╨╕╨▒╨║╨░ ╨┐╤А╨╕ ╨╛╤В╨║╤А╤Л╤В╨╕╨╕ ╤Д╨░╨╣╨╗╨░ ╤Б╨╗╨╛╨▓╨░╤А╤П '" DIC_FILE_NAME "'.",
"╨Ю╤И╨╕╨▒╨║╨░ ╤Д╨╛╤А╨╝╨░╤В╨░ ╤Б╨╗╨╛╨▓╨░╤А╤П: ╤А╨░╨╖╨╝╨╡╤А ╤Б╨╗╨╕╤И╨║╨╛╨╝ ╨╝╨░╨╗.",
"╨Ю╤И╨╕╨▒╨║╨░ ╤А╨░╤Б╨┐╤А╨╡╨┤╨╡╨╗╨╡╨╜╨╕╤П ╨┐╨░╨╝╤П╤В╨╕.",
"╨Ю╤И╨╕╨▒╨║╨░ ╨▓╨▓╨╛╨┤╨░/╨▓╤Л╨▓╨╛╨┤╨░.",
"╨Ю╤И╨╕╨▒╨║╨░ ╨┐╤А╨╕ ╤З╤В╨╡╨╜╨╕╨╕ ╤Б╨╗╨╛╨▓╨░╤А╤П.",
"╨а╨░╨╖╨╝╨╡╤А ╤Б╨╗╨╛╨▓╨░╤А╤П: %d ╤Б╨╗╨╛╨▓ (%" FMT_LMDF_64 "d ╨▒╨░╨╣╤В)\n",
"╨Ю╤И╨╕╨▒╨║╨░ ╤Д╨╛╤А╨╝╨░╤В╨░ ╤Б╨╗╨╛╨▓╨░╤А╤П: ╨╖╨░╨╝╤Л╨║╨░╤О╤Й╨╕╨╣ ╨▒╨░╨╣╤В ╨╜╨╡ ╤А╨░╨▓╨╡╨╜ 0.",
"╨Т╨▓╨╡╨┤╨╕╤В╨╡ ╨┐╨╡╤А╨▓╨╛╨╡ ╤Б╨╗╨╛╨▓╨╛ ╨╕╨╗╨╕ ╤А╨░╨╖╨╝╨╡╤А ╨┐╨╛╨╗╤П:",
"╨а╨░╨╖╨╝╨╡╤А ╨┐╨╛╨╗╤П ╤Г╤А╨╡╨╖╨░╨╜ ╨┤╨╛ ╨╝╨░╨║╤Б╨╕╨╝╨░╨╗╤М╨╜╨╛╨│╨╛ ╨╖╨╜╨░╤З╨╡╨╜╨╕╤П.",
"╨а╨░╨╖╨╝╨╡╤А ╨┐╨╛╨╗╤П ╤Б╨╗╨╕╤И╨║╨╛╨╝ ╨╝╨░╨╗.",
"╨а╨░╨╖╨╝╨╡╤А ╨┐╨╛╨╗╤П: %d x %d\n",
"╨Т╨▓╨╡╨┤╨╕╤В╨╡ ╨┐╨╡╤А╨▓╨╛╨╡ ╤Б╨╗╨╛╨▓╨╛:",
"╨Я╨╡╤А╨▓╨╛╨╡ ╤Б╨╗╨╛╨▓╨╛ ╤Г╤А╨╡╨╖╨░╨╜╨╛.",
"╨Я╨╡╤А╨▓╨╛╨╡ ╤Б╨╗╨╛╨▓╨╛ ╨║╨╛╤А╨╛╤З╨╡ ╤А╨░╨╖╨╝╨╡╤А╨░ ╨┐╨╛╨╗╤П.",
"%d ╤Б╨╗╨╛╨▓ ╨╜╨░╨╣╨┤╨╡╨╜╨╛.\n",
"╨б╨╗╨╛╨▓ ╨╜╨╡ ╨╜╨░╨╣╨┤╨╡╨╜╨╛.",
"╨д╨╛╤А╨╝╨░╤В: <╤Б╨╗╨╛╨▓╨╛>",
"\t<╨▒╤Г╨║╨▓╨░><x><y>",
"\t?<x><y>",
"\t?<╨▒╤Г╨║╨▓╨░><x><y>",
"\t?<╤Б╨╗╨╛╨▓╨╛>",
"<x> ╨╜╨╡╨▓╨╡╤А╨╜╨╛",
"<y> ╨╜╨╡╨▓╨╡╤А╨╜╨╛",
"╨Э╨╡╤В ╤Б╨╗╨╛╨▓╨░-╨║╨░╨╜╨┤╨╕╨┤╨░╤В╨░ (╨┐╤А╨╕ ╨╜╨╡╨╛╨┤╨╜╨╛╨╖╨╜╨░╤З╨╜╨╛╤Б╤В╨╕ ╤А╨░╨╖╨╝╨╡╤Й╨╡╨╜╨╕╤П).",
"╨Э╨╡╨▓╨╡╤А╨╜╤Л╨╣ ╨╜╨╛╨╝╨╡╤А ╨▓╨░╤А╨╕╨░╨╜╤В╨░.",
"╨д╨╛╤А╨╝╨░╤В: <╨▒╤Г╨║╨▓╨░><x><y>",
"╨Э╨╡╨╛╨┤╨╜╨╛╨╖╨╜╨░╤З╨╜╨╛╤Б╤В╤М ╤А╨░╨╖╨╝╨╡╤Й╨╡╨╜╨╕╤П ╤Б╨╗╨╛╨▓╨░:\n1) ",
"╨б╨╗╨╛╨▓╨░╤А╤М ╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜╨╜╤Л╤Е ╤Б╨╗╨╛╨▓ ╨╖╨░╨┐╨╛╨╗╨╜╨╡╨╜.",
"╨Т╨▓╨╡╨┤╨╕╤В╨╡ ╤Б╨╗╨╡╨┤╤Г╤О╤Й╨╡╨╡ ╤Б╨╗╨╛╨▓╨╛ ╨╕╨╗╨╕ ╨▒╤Г╨║╨▓╤Г (╨Сxy) (╨┐╤Г╤Б╤В╨░╤П ╤Б╤В╤А╨╛╨║╨░ - ╨▓╤Л╤Е╨╛╨┤).",
"? - ╤Б╨┐╤А╨░╨▓╨║╨░.",
"╨д╨╛╤А╨╝╨░╤В: <╤Б╨╗╨╛╨▓╨╛>\t\t- ╨┤╨╛╨▒╨░╨▓╨╕╤В╤М ╤Б╨╗╨╛╨▓╨╛ ╨╜╨░ ╨┐╨╛╨╗╨╡ (*)\n"
"\t<╨▒╤Г╨║╨▓╨░><x><y>\t- ╨┤╨╛╨▒╨░╨▓╨╕╤В╤М ╨▒╤Г╨║╨▓╤Г ╨╜╨░ ╨┐╨╛╨╗╨╡, '.' - ╨┐╤Г╤Б╤В╨╛╨╡ ╨╝╨╡╤Б╤В╨╛\n"
"\t?<x><y>\t\t- ╨┐╨╛╨┤╨▒╨╛╤А ╤Б╨╗╨╛╨▓ ╨┐╨╛ ╨║╨╛╨╛╤А╨┤╨╕╨╜╨░╤В╨╡ ╨╗╤О╨▒╨╛╨╣ ╨▒╤Г╨║╨▓╤Л\n"
"\t?<╨▒╤Г╨║╨▓╨░><x><y>\t- ╨┐╨╛╨┤╨▒╨╛╤А ╤Б╨╗╨╛╨▓ ╨┐╨╛ ╨║╨╛╨╛╤А╨┤╨╕╨╜╨░╤В╨╡ ╨╖╨░╨┤╨░╨╜╨╜╨╛╨╣ ╨▒╤Г╨║╨▓╤Л\n"
"\t?<╤Б╨╗╨╛╨▓╨╛>\t- ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ ╨╜╨░╨╗╨╕╤З╨╕╤П ╤Б╨╗╨╛╨▓╨░\n"
"\t+<╤Б╨╗╨╛╨▓╨╛>\t- ╨┤╨╛╨▒╨░╨▓╨╕╤В╤М ╨▓ ╤Б╨╗╨╛╨▓╨░╤А╤М ╨╕╤Б╨┐╨╛╨╗╤М╨╖╨╛╨▓╨░╨╜╨╜╤Л╤Е ╤Б╨╗╨╛╨▓\n"
"(*) ╨Х╤Б╨╗╨╕ ╤Б╨╗╨╛╨▓╨╛ ╨┤╨╛╨▒╨░╨▓╨╕╤В╤М ╨╜╨░ ╨┐╨╛╨╗╨╡ ╨╝╨╛╨╢╨╜╨╛ ╨╜╨╡╤Б╨║╨╛╨╗╤М╨║╨╕╨╝╨╕ ╤Б╨┐╨╛╤Б╨╛╨▒╨░╨╝╨╕, ╨╜╤Г╨╢╨╜╨╛ ╨▓╨▓╨╡╤Б╤В╨╕ "
"╨╜╨╛╨╝╨╡╤А ╨▓╨░╤А╨╕╨░╨╜╤В╨░ ╨╕╨╖ ╤Б╨┐╨╕╤Б╨║╨░ ╨┐╤А╨╡╨┤╨╗╨╛╨╢╨╡╨╜╨╜╤Л╤Е.",
"╨б╨╗╨╛╨▓╨╛ ╨╜╨╡╨▓╨╛╨╖╨╝╨╛╨╢╨╜╨╛ ╤А╨░╨╖╨╝╨╡╤Б╤В╨╕╤В╤М ╨╜╨░ ╨┐╨╛╨╗╨╡.",
"╨Т╨▓╨╡╨┤╨╕╤В╨╡ ╨╜╨╛╨╝╨╡╤А ╨▓╨░╤А╨╕╨░╨╜╤В╨░:",
"(╤В╨╛╤З╨╜╨╛)"
};

#else

const char *msgs[] = {
"Ошибка при открытии файла словаря '" DIC_FILE_NAME "'.",
"Ошибка формата словаря: размер слишком мал.",
"Ошибка распределения памяти.",
"Ошибка ввода/вывода.",
"Ошибка при чтении словаря.",
"Размер словаря: %d слов (%" FMT_LMDF_64 "d байт)\n",
"Ошибка формата словаря: замыкающий байт не равен 0.",
"Введите первое слово или размер поля:",
"Размер поля урезан до максимального значения.",
"Размер поля слишком мал.",
"Размер поля: %d x %d\n",
"Введите первое слово:",
"Первое слово урезано.",
"Первое слово короче размера поля.",
"%d слов найдено.\n",
"Слов не найдено.",
"Формат: <слово>",
"\t<буква><x><y>",
"\t?<x><y>",
"\t?<буква><x><y>",
"\t?<слово>",
"<x> неверно",
"<y> неверно",
"Нет слова-кандидата (при неоднозначности размещения).",
"Неверный номер варианта.",
"Формат: <буква><x><y>",
"Неоднозначность размещения слова:\n1) ",
"Словарь использованных слов заполнен.",
"Введите следующее слово или букву (Бxy) (пустая строка - выход).",
"? - справка.",
"Формат: <слово>\t\t- добавить слово на поле (*)\n"
"\t<буква><x><y>\t- добавить букву на поле, '.' - пустое место\n"
"\t?<x><y>\t\t- подбор слов по координате любой буквы\n"
"\t?<буква><x><y>\t- подбор слов по координате заданной буквы\n"
"\t?<слово>\t- проверка наличия слова\n"
"\t+<слово>\t- добавить в словарь использованных слов\n"
"(*) Если слово добавить на поле можно несколькими способами, нужно ввести "
"номер варианта из списка предложенных.",
"Слово невозможно разместить на поле.",
"Введите номер варианта:",
"(точно)"
};

#endif

#endif
