/*
 * Copyright (C) 2010 Thomas Neumann, Simon Kagstrom
 *
 * See COPYING for license details
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

#include <utils.h>
#include <kc.h>


static const char icon_ruby[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x18, 0x10, 0x5d, 0x57, 0x34, 0x6e, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0x35, 0x2f, 0x00, 0x00, 0x00, 0xd0, 0x33, 0x9a, 0x9d, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const char icon_amber[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x28, 0x04, 0x98, 0xcb, 0xd6, 0xe0, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0xe0, 0x50, 0x00, 0x00, 0x00, 0xa2, 0x7a, 0xda, 0x7e, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const char icon_emerald[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x25,
0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f, 0x22, 0x2b,
0xc9, 0xf5, 0x03, 0x33, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x12, 0x00, 0x00,
0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, 0xb1,
0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0x1b, 0xea, 0x59, 0x0a, 0x0a,
0x0a, 0x0f, 0xba, 0x50, 0x83, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x60, 0x00,
0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
0x42, 0x60, 0x82 };

static const char icon_snow[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x11, 0x0f,
0x1e, 0x1d, 0x75, 0xbc, 0xef, 0x55, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b,
0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d,
0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b, 0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45,
0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x55, 0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41,
0x54, 0x78, 0xda, 0x63, 0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0xe5, 0x27, 0xde, 0xfc, 0x00, 0x00,
0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

static const char icon_glass[] =
  { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
0x25, 0xdb, 0x56, 0xca, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, 0xb1, 0x8f, 0x0b,
0xfc, 0x61, 0x05, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4c, 0x54, 0x45, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
0x55, 0xc2, 0xd3, 0x7e, 0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4e, 0x53, 0x00, 0x40, 0xe6, 0xd8, 0x66,
0x00, 0x00, 0x00, 0x01, 0x62, 0x4b, 0x47, 0x44, 0x00, 0x88, 0x05, 0x1d, 0x48, 0x00, 0x00, 0x00, 0x09,
0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x12, 0x00, 0x00, 0x0b, 0x12, 0x01, 0xd2, 0xdd, 0x7e, 0xfc,
0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xd2, 0x07, 0x13, 0x0f, 0x08, 0x19, 0xc4, 0x40,
0x56, 0x10, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x00, 0x00, 0x00,
0x02, 0x00, 0x01, 0x48, 0xaf, 0xa4, 0x71, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
0x60, 0x82 };

const char css_text[] = "/* Based upon the lcov CSS style, style files can be reused */\n"
		"body { color: #000000; background-color: #FFFFFF; }\n"
		"a:link { color: #284FA8; text-decoration: underline; }\n"
		"a:visited { color: #00CB40; text-decoration: underline; }\n"
		"a:active { color: #FF0040; text-decoration: underline; }\n"
		"td.title { text-align: center; padding-bottom: 10px; font-size: 20pt; font-weight: bold; }\n"
		"td.ruler { background-color: #6688D4; }\n"
		"td.headerItem { text-align: right; padding-right: 6px; font-family: sans-serif; font-weight: bold; }\n"
		"td.headerValue { text-align: left; color: #284FA8; font-family: sans-serif; font-weight: bold; }\n"
		"td.versionInfo { text-align: center; padding-top:  2px; }\n"
		"pre.source { font-family: monospace; white-space: pre; }\n"
		"span.lineNum { background-color: #EFE383; }\n"
		"span.lineCov { background-color: #CAD7FE; }\n"
		"span.linePartCov { background-color: #FFEA20; }\n"
		"span.lineNoCov { background-color: #FF6230; }\n"
		"td.tableHead { text-align: center; color: #FFFFFF; background-color: #6688D4; font-family: sans-serif; font-size: 120%; font-weight: bold; }\n"
		"td.coverFile { text-align: left; padding-left: 10px; padding-right: 20px; color: #284FA8; background-color: #DAE7FE; font-family: monospace; }\n"
		"td.coverBar { padding-left: 10px; padding-right: 10px; background-color: #DAE7FE; }\n"
		"td.coverBarOutline { background-color: #000000; }\n"
		"td.coverPerHi { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; font-weight: bold; }\n"
		"td.coverPerLeftMed { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; font-weight: bold; }\n"
		"td.coverPerLeftLo { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #FF0000; font-weight: bold; }\n"
		"td.coverPerLeftHi { text-align: left; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; font-weight: bold; }\n"
		"td.coverNumHi { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #A7FC9D; }\n"
		"td.coverPerMed { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; font-weight: bold; }\n"
		"td.coverNumMed { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FFEA20; }\n"
		"td.coverPerLo { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FF0000; font-weight: bold; }\n"
		"td.coverNumLo { text-align: right; padding-left: 10px; padding-right: 10px; background-color: #FF0000; }\n";

static const char **allocated_objs;
static int n_allocs;
static int cur_allocation = 0;
static void cleanup_allocations(void)
{
	int i;

	for (i = 0; i < n_allocs; i++) {
		free((void*)allocated_objs[i]);
		allocated_objs[i] = NULL;
	}
	cur_allocation = 0;
}

void *add_allocation(void *x)
{
	if (cur_allocation >= n_allocs) {
		int last = n_allocs;
		int i;

		n_allocs = n_allocs + 512;
		allocated_objs = xrealloc(allocated_objs,
				n_allocs * sizeof(const char *));
		for (i = last; i < n_allocs; i++)
			allocated_objs[i] = NULL;
	}

	allocated_objs[cur_allocation] = x;
	cur_allocation++;

	return x;
}

static void write_pngs(const char *dir)
{
	xwrite_file(icon_ruby, sizeof(icon_ruby), "%s/ruby.png", dir);
	xwrite_file(icon_amber, sizeof(icon_amber), "%s/amber.png", dir);
	xwrite_file(icon_emerald, sizeof(icon_emerald), "%s/emerald.png", dir);
	xwrite_file(icon_snow, sizeof(icon_snow), "%s/snow.png", dir);
	xwrite_file(icon_glass, sizeof(icon_glass), "%s/glass.png", dir);
}

static const char *construct_bar(double percent)
{
   const char* color = "ruby.png";
   char buf[255];
   int width = (int)(percent+0.5);

   if (percent >= 50)
	   color = "emerald.png";
   else if (percent>=15)
	   color = "amber.png";
   else if (percent <= 1)
	   color = "snow.png";

   xsnprintf(buf, sizeof(buf),
		   "<img src=\"%s\" width=\"%d\" height=\"10\" alt=\"%.1f%%\"/><img src=\"snow.png\" width=\"%d\" height=\"10\" alt=\"%.1f%%\"/>",
		   color, width, percent, 100 - width, percent);

   return add_allocation(xstrdup(buf));
}

static void write_css(const char *dir)
{
	xwrite_file(css_text, sizeof(css_text), "%s/bcov.css", dir);
}

static char *escape_helper(char *dst, char *what)
{
	int len = strlen(what);

	strcpy(dst, what);

	return dst + len;
}

static const char *outfile_name_from_kc_file(struct kc_file *kc_file)
{
	char *p = strrchr(kc_file->filename, '/');
	char *out;

	if (!p)
		return NULL;

	out = xmalloc(strlen(p) + 8);
	xsnprintf(out, strlen(p) + 8, "%s.html", p + 1);
	add_allocation(out);

	return out;
}

static const char *tmp_outfile_name_from_kc_file(struct kc_file *kc_file)
{
	char *p = strrchr(kc_file->filename, '/');
	char *out;

	if (!p)
		return NULL;

	out = xmalloc(strlen(p) + 16);
	xsnprintf(out, strlen(p) + 16, "%s.html.tmp", p + 1);
	add_allocation(out);

	return out;
}


static const char *dir_concat_report(const char *dir, const char *filename)
{
	return add_allocation((void*)dir_concat(dir, filename));
}

static const char *escape_html(const char *s)
{
	char buf[4096];
	char *dst = buf;
	size_t len = strlen(s);
	size_t i;

	memset(buf, 0, sizeof(buf));
	for (i = 0; i < len; i++) {
		char c = s[i];

		switch (c) {
		case '<':
			dst = escape_helper(dst, "&lt;");
			break;
		case '>':
			dst = escape_helper(dst, "&gt;");
			break;
		case '&':
			dst = escape_helper(dst, "&amp;");
			break;
		case '\"':
			dst = escape_helper(dst, "&quot;");
			break;
		case '\'':
			dst = escape_helper(dst, "&#039;");
			break;
		case '/':
			dst = escape_helper(dst, "&#047;");
			break;
		case '\\':
			dst = escape_helper(dst, "&#092;");
			break;
		case '\n': case '\r':
			dst = escape_helper(dst, " ");
			break;
		default:
			*dst = c;
			dst++;
			break;
		}
	}

	return add_allocation(strdup(buf));
}

static void write_header(FILE *fp, struct kc *kc, struct kc_file *file,
		int lines, int active_lines, int do_write_command)
{
	const char *percentage_text = "Lo";
	double percentage;
	char date_buf[80];
	time_t t;
	struct tm *tm;

	percentage = lines == 0 ? 0 : ((float)active_lines / (float)lines) * 100;

	if (percentage > kc->low_limit && percentage < kc->high_limit)
		percentage_text = "Med";
	else if (percentage >= kc->high_limit)
		percentage_text = "Hi";

	t = time(NULL);
	tm = localtime(&t);
	strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", tm);
	fprintf(fp,
			"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
			"<html>\n"
			"<head>\n"
			"  <title>Coverage - %s</title>\n"
			"  <link rel=\"stylesheet\" type=\"text/css\" href=\"bcov.css\"/>\n"
			"</head>\n"
			"<body>\n"
			"<table width=\"100%%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"  <tr><td class=\"title\">Coverage Report</td></tr>\n"
			"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
			"  <tr>\n"
			"    <td width=\"100%%\">\n"
			"      <table cellpadding=\"1\" border=\"0\" width=\"100%%\">\n",
			do_write_command ? escape_html(kc->module_name) : "overview" /* Title */
	);
	if (do_write_command)
		fprintf(fp,
				"        <tr>\n"
				"          <td class=\"headerItem\" width=\"20%%\">Command:</td>\n"
				"          <td class=\"headerValue\" width=\"80%%\" colspan=6>%s</td>\n"
				"        </tr>\n",
				escape_html(kc->module_name) /* Command */
		);
	fprintf(fp,
			"        <tr>\n"
			"          <td class=\"headerItem\" width=\"20%%\">Date:</td>\n"
			"          <td class=\"headerValue\" width=\"15%%\">%s</td>\n"
			"          <td width=\"5%%\"></td>\n"
			"          <td class=\"headerItem\" width=\"20%%\">Instrumented&nbsp;lines:</td>\n"
			"          <td class=\"headerValue\" width=\"10%%\">%d</td>\n"
			"        </tr>\n"
			"        <tr>\n"
			"          <td class=\"headerItem\" width=\"20%%\">Code&nbsp;covered:</td>\n"
			"          <td class=\"coverPerLeft%s\" width=\"15%%\">%.1f %%</td>\n"
			"          <td width=\"5%%\"></td>\n"
			"          <td class=\"headerItem\" width=\"20%%\">Executed&nbsp;lines:</td>\n"
			"          <td class=\"headerValue\" width=\"10%%\">%d</td>\n"
			"        </tr>\n"
			"      </table>\n"
			"    </td>\n"
			"  </tr>\n"
			"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
			"</table>\n",
			date_buf,
			lines,
			percentage_text,
			percentage,
			active_lines
	);
}

static void write_footer(FILE *fp)
{
	fprintf(fp,
			"<table width=\"100%%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
			"  <tr><td class=\"ruler\"><img src=\"glass.png\" width=\"3\" height=\"3\" alt=\"\"/></td></tr>\n"
			"  <tr><td class=\"versionInfo\">Generated by: <a href=\"http://simonkagstrom.github.com/kcov/index.html\">Kcov</a> (based on <a href=\"http://bcov.sourceforge.net\">bcov</a>)</td></tr>\n"
			"</table>\n"
			"<br/>\n"
			"</body>\n"
			"</html>\n");
}

static int write_file_report(const char *dir, struct kc *kc, struct kc_file *kc_file)
{
	const char *tmp_outfile_name = tmp_outfile_name_from_kc_file(kc_file);
	const char *outfile_name = outfile_name_from_kc_file(kc_file);
	FILE *out_fp;
	FILE *fp;
	int lineno = 1;
	int total_hits = 0;

	if (!tmp_outfile_name)
		return -1;

	fp = fopen(kc_file->filename, "r");
	if (!fp)
		return -1;

	out_fp = fopen(dir_concat_report(dir, tmp_outfile_name), "w");
	if (!out_fp)
		return -1;

	fprintf(out_fp, "<pre class=\"source\">\n");

	/* Read the lines of the source file */
	while (!feof(fp)) {
		char *line = NULL;
		struct kc_line *kc_line = kc_file_lookup_line(kc_file, lineno);
		size_t sz = 0;

		if (getline(&line, &sz, fp) < 0) {
			if (line)
				free(line);
			break;
		}

		fprintf(out_fp,
				"<span class=\"lineNum\">%7d </span>",
				lineno);

		if (kc_line) {
			const char *line_class = "lineNoCov";
			int hits = 0;
			int i;

			/* Count hits */
			for (i = 0; i < kc_line->n_addrs; i++) {
				struct kc_addr *addr = kc_line->addrs[i];

				hits += addr->hits;
			}
			/* Full, partial or no coverage? */
			if (hits >= kc_line->possible_hits)
				line_class = "lineCov";
			else if (hits != 0)
				line_class = "linePartCov";
			fprintf(out_fp,
					"<span class=\"%s\"> %4u / %-4u  : ",
					line_class, hits, kc_line->possible_hits);
			if (hits)
				total_hits++;
		}
		else
			fprintf(out_fp,	"              : ");

		fprintf(out_fp, "%s</span>\n", escape_html(line));

		free(line);
		lineno++;
	}
	fprintf(out_fp, "</pre>\n");

	write_footer(out_fp);

	fclose(fp);
	fclose(out_fp);


	fp = fopen(dir_concat_report(dir, "tmp"), "w");
	if (!fp)
		return -1;
	write_header(fp, kc, NULL, g_hash_table_size(kc_file->lines),
			total_hits, 1);
	fclose(fp);

	concat_files(dir_concat_report(dir, outfile_name),
			dir_concat_report(dir, "tmp"),
			dir_concat_report(dir, tmp_outfile_name));
	unlink(dir_concat_report(dir, "tmp"));
	unlink(dir_concat_report(dir, tmp_outfile_name));

	return 0;
}


static int kc_file_cmp(const void *pa, const void *pb)
{
	struct kc_file *a = *(struct kc_file **)pa;
	struct kc_file *b = *(struct kc_file **)pb;

	return strcmp(a->filename, b->filename);
}

static int kc_percentage_cmp(const void *pa, const void *pb)
{
	struct kc_file *a = *(struct kc_file **)pa;
	struct kc_file *b = *(struct kc_file **)pb;

	return (int)(a->percentage - b->percentage);
}

static int cmp_path(const char *src_file_path, const char *cmp_realpath)
{
	struct stat sb;
	char *src_file_realpath = realpath(src_file_path, NULL);
	int ret = 0;

	if (!src_file_realpath)
		return 0;

	if (stat(cmp_realpath, &sb))
		return 0;

	if (S_ISDIR(sb.st_mode)) {
		size_t plen = strlen(cmp_realpath);

		/* If the paths are equal, return true */
		if (strstr(src_file_realpath, cmp_realpath) == src_file_realpath &&
				(src_file_realpath[plen] == '\0' || src_file_realpath[plen] == '/'))
			ret = 1;
	} else {
		if (strcmp(src_file_realpath, cmp_realpath) == 0)
			ret = 1;
	}
	free(src_file_realpath);

	return ret;
}

static int report_path(const char *path, struct kc *kc)
{
	if (!kc->only_report_paths && !kc->only_report_patterns)
		return 1;

	const char **p;
	size_t i;

	if (kc->only_report_paths)
	{
		p = kc->only_report_paths;
		for (i = 0; p[i] != NULL; i++)
		{
			if (cmp_path(path, p[i]))
				return 1;
		}
	}

	if (kc->only_report_patterns)
	{
		p = kc->only_report_patterns;
		i = 0;
		while (p[i])
		{
			if (strstr(path, p[i]))
				return 1;

			i++;
		}
	}

	return 0;
}

static int exclude_path(const char *path, struct kc *kc)
{
	if (!kc->exclude_paths && !kc->exclude_patterns)
		return 0;

	const char **p;
	size_t i;

	if (kc->exclude_paths)
	{
		struct stat sb;
		p = kc->exclude_paths;
		for (i = 0; p[i] != NULL; i++)
		{
			if (cmp_path(path, p[i]))
				return 1;
		}
	}

	if (kc->exclude_patterns)
	{
		p = kc->exclude_patterns;
		i = 0;
		while (p[i])
		{
			if (strstr(path, p[i]))
				return 1;

			i++;
		}
	}

	return 0;
}

static int write_index(const char *dir, struct kc *kc)
{
	GHashTableIter iter;
	const char *key;
	struct kc_file *kc_file;
	struct kc_file **files;
	int total_lines = 0;
	int total_active_lines = 0;
	int n_files = 0;
	FILE *fp;
	int file_idx;

	fp = fopen(dir_concat_report(dir, "index.html.main"), "w");
	if (!fp)
		return -1;

	fprintf(fp,
			"<center>\n"
			"  <table width=\"80%%\" cellpadding=\"2\" cellspacing=\"1\" border=\"0\">\n"
			"    <tr>\n"
			"      <td width=\"50%%\"><br/></td>\n"
			"      <td width=\"15%%\"></td>\n"
			"      <td width=\"15%%\"></td>\n"
			"      <td width=\"20%%\"></td>\n"
			"    </tr>\n"
			"    <tr>\n"
			"      <td class=\"tableHead\">Filename</td>\n"
			"      <td class=\"tableHead\" colspan=\"3\">Coverage</td>\n"
			"    </tr>\n");

	/* Sort the files by name */
	files = add_allocation(xmalloc(g_hash_table_size(kc->files) * sizeof(struct kc_file *)));
	g_hash_table_iter_init(&iter, kc->files);
	while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&kc_file)) {
		files[n_files] = kc_file;
		n_files++;
	}
	qsort(files, g_hash_table_size(kc->files), sizeof(struct kc_file *),
			kc->sort_type == COVERAGE_PERCENT ? kc_percentage_cmp : kc_file_cmp);
	for (file_idx = 0; file_idx < n_files; file_idx++) {
		double percentage = 0;
		const char *percentage_text = "Lo";
		int active_lines = 0;
		GHashTableIter line_iter;
		int n_lines;
		struct kc_line *line_val;
		int line_key;

		kc_file = files[file_idx];

		if (!report_path(kc_file->filename, kc))
			continue;

		if (exclude_path(kc_file->filename, kc))
			continue;

		if (!file_exists(kc_file->filename))
			continue;

		/* Count the percentage of hits */
		n_lines = g_hash_table_size(kc_file->lines);
		g_hash_table_iter_init(&line_iter, kc_file->lines);
		while (g_hash_table_iter_next(&line_iter,
				(gpointer*)&line_key, (gpointer*)&line_val)) {
			int i;

			for (i = 0; i < line_val->n_addrs; i++) {
				if (line_val->addrs[i]->hits > 0) {
					/* Only care about full lines */
					active_lines++;
					break;
				}
			}
		}

		if (n_lines != 0)
			percentage = ((double)active_lines / (double)n_lines) * 100;
		kc_file->percentage = percentage;

		total_lines += n_lines;
		total_active_lines += active_lines;

		if (percentage > kc->low_limit && percentage < kc->high_limit)
			percentage_text = "Med";
		else if (percentage >= kc->high_limit)
			percentage_text = "Hi";

		fprintf(fp,
				"    <tr>\n"
				"      <td class=\"coverFile\"><a href=\"%s\">%s</a></td>\n"
				"      <td class=\"coverBar\" align=\"center\">\n"
				"        <table border=\"0\" cellspacing=\"0\" cellpadding=\"1\"><tr><td class=\"coverBarOutline\">%s</td></tr></table>\n"
				"      </td>\n"
				"      <td class=\"coverPer%s\">%.1f&nbsp;%%</td>\n"
				"      <td class=\"coverNum%s\">%d&nbsp;/&nbsp;%d&nbsp;lines</td>\n"
				"    </tr>\n",
				outfile_name_from_kc_file(kc_file),
				kc_file->filename,
				construct_bar(percentage),
				percentage_text, percentage,
				percentage_text, active_lines, n_lines);
	}
	fprintf(fp,
			"  </table>\n"
			"</center>\n"
			"<br/>\n");

	write_footer(fp);
	fclose(fp);

	kc->total_coverage.n_lines = total_lines;
	kc->total_coverage.n_covered_lines = total_active_lines;

	fp = fopen(dir_concat_report(dir, "index.html.hdr"), "w");
	if (!fp)
		return -1;
	write_header(fp, kc, NULL, total_lines, total_active_lines, 1);
	fclose(fp);

	concat_files(dir_concat_report(dir, "index.html"),
			dir_concat_report(dir, "index.html.hdr"),
			dir_concat_report(dir, "index.html.main"));
	unlink(dir_concat_report(dir, "index.html.hdr"));
	unlink(dir_concat_report(dir, "index.html.main"));

	return 0;
}

static int write_global_index(struct kc *kc, const char *dir_name)
{
	struct dirent *de;
	int total_active_lines = 0;
	int total_lines = 0;
	DIR *dir;
	FILE *fp;

	fp = fopen(dir_concat_report(dir_name, "index.html.main"), "w");
	if (!fp)
		return -1;

	fprintf(fp,
			"<center>\n"
			"  <table width=\"80%%\" cellpadding=\"2\" cellspacing=\"1\" border=\"0\">\n"
			"    <tr>\n"
			"      <td width=\"50%%\"><br/></td>\n"
			"      <td width=\"15%%\"></td>\n"
			"      <td width=\"15%%\"></td>\n"
			"      <td width=\"20%%\"></td>\n"
			"    </tr>\n"
			"    <tr>\n"
			"      <td class=\"tableHead\">Filename</td>\n"
			"      <td class=\"tableHead\" colspan=\"3\">Coverage</td>\n"
			"    </tr>\n");

	dir = opendir(dir_name);
	panic_if(!dir, "Can't open directory %s\n", dir_name);

	de = readdir(dir);
	while (de)
	{
		struct kc_coverage_db db;
		const char *percentage_text = "Lo";
		double percentage = 0;
		char buf[2048];
		int r;

		r = snprintf(buf, sizeof(buf), "%s/%s", dir_name, de->d_name);
		panic_if(r == sizeof(buf), "Buffer overflow");

		/* Read the coverage database but skip if it isn't there */
		if (kc_coverage_db_read(buf, &db) < 0) {
			de = readdir(dir);
			continue;
		}
		total_lines += db.n_lines;
		total_active_lines += db.n_covered_lines;

		if (db.n_covered_lines != 0)
			percentage = ((double)db.n_covered_lines / (double)db.n_lines) * 100;

		if (percentage > kc->low_limit && percentage < kc->high_limit)
			percentage_text = "Med";
		else if (percentage >= kc->high_limit)
			percentage_text = "Hi";

		fprintf(fp,
				"    <tr>\n"
				"      <td class=\"coverFile\"><a href=\"%s/index.html\">%s</a></td>\n"
				"      <td class=\"coverBar\" align=\"center\">\n"
				"        <table border=\"0\" cellspacing=\"0\" cellpadding=\"1\"><tr><td class=\"coverBarOutline\">%s</td></tr></table>\n"
				"      </td>\n"
				"      <td class=\"coverPer%s\">%.1f&nbsp;%%</td>\n"
				"      <td class=\"coverNum%s\">%d&nbsp;/&nbsp;%d&nbsp;lines</td>\n"
				"    </tr>\n",
				de->d_name,
				db.name,
				construct_bar(percentage),
				percentage_text, percentage,
				percentage_text, db.n_covered_lines, db.n_lines);

		de = readdir(dir);
	}
	closedir(dir);

	fprintf(fp,
			"  </table>\n"
			"</center>\n"
			"<br/>\n");

	write_footer(fp);
	fclose(fp);

	fp = fopen(dir_concat_report(dir_name, "index.html.hdr"), "w");
	panic_if (!fp, "Can't open header file - there won't be any report\n");
	write_header(fp, kc, NULL, total_lines, total_active_lines, 0);
	fclose(fp);

	concat_files(dir_concat_report(dir_name, "index.html"),
			dir_concat_report(dir_name, "index.html.hdr"),
			dir_concat_report(dir_name, "index.html.main"));
	unlink(dir_concat_report(dir_name, "index.html.hdr"));
	unlink(dir_concat_report(dir_name, "index.html.main"));

	return 0;
}

static void write_report(const char *dir, struct kc *kc)
{
	GHashTableIter iter;
	const char *key;
	struct kc_file *val;

	memset(&kc->total_coverage, 0, sizeof(kc->total_coverage));
	strncpy(kc->total_coverage.name, kc->module_name, sizeof(kc->total_coverage.name) - 1);
	/* Write an index first */
	write_index(dir, kc);

	g_hash_table_iter_init(&iter, kc->files);
	while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&val)) {
		if (file_exists(val->filename))
			write_file_report(dir, kc, val);
	}

	kc_coverage_db_write(dir, &kc->total_coverage);
	write_global_index(kc, kc->out_dir);

	cleanup_allocations();
}



static pthread_t thread;
static const char *g_dir;
static struct kc *g_kc;
static int should_exit;

static void *report_thread(void *priv)
{
	struct kc *kc = priv;
	const char *binary_dir;
	int i = 0;

	binary_dir = dir_concat(kc->out_dir, kc->binary_filename);

	mkdir(kc->out_dir, 0755);

	mkdir(binary_dir, 0755);

	write_css(kc->out_dir);
	write_pngs(kc->out_dir);
	write_css(binary_dir);
	write_pngs(binary_dir);

	while (1) {
		if (i % 5 == 0)
			write_report(binary_dir, kc);
		if (should_exit)
			break;
		sleep(1);
		i++;
		sync();
	}
	/* Do this twice to collect percentages from last round */
	write_report(binary_dir, kc);
	write_report(binary_dir, kc);

	kc_write_db(kc);

	free((void *)binary_dir);

	pthread_exit(NULL);
}

void stop_report_thread(void)
{
	should_exit = 1;
	pthread_join(thread, NULL);
	cleanup_allocations();

	if (g_kc->type == PTRACE_PID || g_kc->type == PTRACE_FILE)
		ptrace_detach(g_kc);
}


void run_report_thread(struct kc *kc)
{
	int ret;

	g_dir = kc->out_dir;
	g_kc = kc;
	ret = pthread_create(&thread, NULL, report_thread, kc);
	panic_if (ret < 0, "Can't create thread");
}
