
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <wchar.h>
#endif

#define FIELD_SZ	10
#define DEF_FIELD_SZ 5
#define MAX_WORD_SZ 20

#define INPUT_BUF_SZ 128

char field [FIELD_SZ][FIELD_SZ];
char mark_field [FIELD_SZ][FIELD_SZ];
int szx = 0, szy = 0;
char dirv [MAX_WORD_SZ];

char buf [INPUT_BUF_SZ];
#ifndef _WIN32
wchar_t wbuf [INPUT_BUF_SZ];
size_t strconv (const char *src);
void uprint (const char *s);
#endif

void clear_field (void);

void input (const char *prompt);
char *dic = NULL;
size_t dic_l = 0;

long filelength (FILE *f);
void cleanup (void);

struct sqvec {
	int dx, dy;
} sqdir [4] = {
	{  1,  0 },
	{  0,  1 },
	{ -1,  0 },
	{  0, -1 }
};

int valid_cell (int x, int y);

char *strupper (char *s);
void dump_field (void);

int main (int argc, const char *argv[], const char *envp[])
{
	int i, j, s, d, w, n, lc;
	int ax, ay, subx, suby;
	long l;
	FILE *f;
	char f_lt_n, subc;
	char cz [2] = { 0, 0 };

#ifdef _WIN32
	setlocale (LC_ALL, ".OCP");
#endif
	f = fopen ("dic.dat", "rb");
	if (f == NULL) {
		puts ("Dictionary open error.");
		return 1;
	}
	l = filelength (f);
	dic = (char*)malloc ((size_t)l);
	if (dic == NULL) {
		fclose (f);
		puts ("Memory allocation error.");
		return 1;
	}
	if (l == -1) {
		fclose (f);
		cleanup ();
		puts ("File I/O error.");
		return 1;
	}
	if (fread (dic, l, 1, f) != 1) {
		fclose (f);
		cleanup ();
		puts ("Dictionary read error.");
		return 1;
	}
	dic_l = l;
	fclose (f);
	printf ("Dictionary size: %d bytes\n", dic_l);

	clear_field ();
	input ("Enter field size:");
	szx = szy = atoi (buf);
	if (szx > FIELD_SZ) {
		puts ("Field size is truncated to max value.");
		szx = szy = FIELD_SZ;
	}
	if (szx < 2) {
		puts ("Field size is too small.");
		szx = szy = DEF_FIELD_SZ;
	}
	printf ("Field size: %d x %d\n", szx, szy);
	input ("Enter start word:");
	s = strlen (buf);
	if (buf [s-1] == '\n')
		buf [s-1] = '\0', s--;
#ifndef _WIN32
	strconv (buf);
	s = strlen (buf);
#endif
	if (s != szx) {
		if (s > szx)
			puts ("Start word is truncated.");
		else	puts ("Start word is shorter that field.");
	}
	if (s > szx) s = szx;
	strupper (buf);
#ifdef _WIN32
	OemToChar (buf, buf);
#endif
	memcpy (field [(szy - 1) / 2], buf, s);
	while (1) {
		n = 0;
	for (j = 0; j < szy; j++)
		for (i = 0; i < szx; i++) {
			f_lt_n = 0;
			for (d = 0; d < 4; d++) {
				ax = i + sqdir [d].dx;
				ay = j + sqdir [d].dy;
				if (valid_cell (ax, ay) && field [ay][ax] != 0) {
					f_lt_n = 1;
					break;
				}
			}
			if (!(field [j][i] || f_lt_n))
				continue;
			w = 0;
			for (s = 0; s < dic_l; s++, lc++) {
				if (w == s) {
					ax = i; ay = j;
					subc = 0; lc = 0;
					memset (mark_field, 0, sizeof (mark_field));
				}
				if (lc >= MAX_WORD_SZ) goto nx_word;
				if (field [ay][ax] == 0) {
					if (subc == 0) {
						subc = dic [s];
						subx = ax; suby = ay;
					} else goto dir_bk2;
				} else if (field [ay][ax] != dic [s])
					goto dir_bk2;
				dirv [lc] = 0;
				mark_field [ay][ax] = 1;
				goto nx_lt;
dir_bk2:
				s--; lc--;
				if (lc < 0) {
nx_word:
					s += strlen (&dic [s+1]) + 1;
					w = s + 1;
					continue;
				}
dir_bk:
				ax -= sqdir [dirv [lc]].dx;
				ay -= sqdir [dirv [lc]].dy;
				dirv [lc] ++;
				if (dirv [lc] >= 4) {
					if (lc > 0) {
						mark_field [ay][ax] = 0;
						if (field [ay][ax] == 0) subc = 0;
					}
					goto dir_bk2;
				}
nx_lt:
				if (dic [s + 1] == '\0') {
					if (subc != 0) {
#ifdef _WIN32
						buf [0] = subc;
						strcpy (&buf[1], &dic [w]);
						CharToOem (buf, buf);
						printf ("%c %d %d: %s %d %d\n", buf [0],
							subx + 1, suby + 1, &buf[1], i + 1, j + 1);
#else
						cz [0] = subc;
						uprint (cz);
						printf (" %d %d: ", subx + 1, suby + 1);
						uprint (&dic [w]);
						printf (" %d %d\n", i + 1, j + 1);
#endif
						n++;
					}
					goto dir_bk2;
				}
				ax += sqdir [dirv [lc]].dx;
				ay += sqdir [dirv [lc]].dy;
				if (!valid_cell (ax, ay) || mark_field [ay][ax] == 1)
					goto dir_bk;
			}
		}
		if (n > 0)
			printf ("%d", n);
		else printf ("No");
		puts (" words found.");
		dump_field ();
nx_ch:
		input ("Enter next letter (cxy) or exit (empty line):");
		s = strlen (buf);
		if (s == 0 || (s == 1 && buf [0] == '\n')) break;
		if (buf [s-1] == '\n')
			buf [s-1] = '\0', s--;
#ifndef _WIN32
		strconv (buf);
		s = strlen (buf);
#endif
		strupper (buf);
#ifdef _WIN32
		OemToChar (buf, buf);
#endif
		if (s != 3) {
			puts ("Format: <letter><x><y>");
			goto nx_ch;
		}
		cz [0] = buf [1];
		ax = atoi (cz) - 1;
		cz [0] = buf [2];
		ay = atoi (cz) - 1;
		if (ax < 0 || ax >= FIELD_SZ) {
			puts ("<x> bad");
			goto nx_ch;
		}
		if (ay < 0 || ay >= FIELD_SZ) {
			puts ("<y> bad");
			goto nx_ch;
		}
		field [ay][ax] = buf [0];
	}
	cleanup ();
	return 0;
}

void clear_field (void)
{
	memset (field, 0, sizeof (field));
}

int valid_cell (int x, int y)
{
	if (x >= 0 && x < szx &&
		y >= 0 && y < szy)
		return 1;
	return 0;
}

void input (const char *prompt)
{
	puts (prompt);
	fgets (buf, INPUT_BUF_SZ, stdin);
	buf [INPUT_BUF_SZ - 1] = '\0';
}

#define A_CYRA_CAP	0xC0
#define A_CYRA_SML	0xE0
#define A_CYRYA_CAP	0xDF
#define A_CYRYA_SML	0xFF

char *strupper (char *s)
{
#ifndef _WIN32
	int i, l = strlen (s);
	for (i = 0; i < l; i++)
		if (s [i] >= A_CYRA_SML && s [i] <= A_CYRYA_SML)
			s [i] += A_CYRA_CAP - A_CYRA_SML;
	return s;
#else
	return _strupr (s);
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
	puts ("");
	for (j = 0; j < szy; j++) {
		printf ("%d ", j + 1);
		for (i = 0; i < szx; i++) {
#ifndef _WIN32
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
		puts ("");
	}
	fwrite ("  ", 2, 1, stdout);
	for (i = 0; i < szx; i++)
		fwrite ("--", 2, 1, stdout);
	puts ("");
}

long filelength (FILE *f)
{
#ifndef _WIN32
	long l, lt = ftell (f);
	if (lt == -1) return -1;
	if (fseek (f, 0, SEEK_END) != 0)
		return -1;
	l = ftell (f);
	fseek (f, lt, SEEK_SET);
	return l;
#else
	return _filelength (fileno (f));
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
	const char *psrc = src;

	mbstate_t mbs;
	memset (&mbs, 0, sizeof (mbs));
	l = strlen (src);
	wl = mbsrtowcs (wbuf, &psrc, INPUT_BUF_SZ, &mbs);
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

void uprint (const char *s)
{
	int i, l = strlen (s);
	size_t sl;
	wchar_t *pwbuf;
	mbstate_t mbs;

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
	wbuf [i] = '\0';
	memset (&mbs, 0, sizeof (mbs));
	pwbuf = wbuf;
	sl = wcsrtombs (buf, (const wchar_t**)&pwbuf, sizeof (buf), &mbs);
	buf [sizeof (buf) - 1] = '\0';
	fwrite (buf, sl, 1, stdout);
}
#endif

void cleanup (void)
{
	free (dic);
}
