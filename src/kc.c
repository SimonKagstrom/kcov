/*
 * Copyright (C) 2010 Simon Kagstrom, Thomas Neumann
 *
 * See COPYING for license details
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>    /* POSIX basename */
#include <libelf.h>
#include <dwarf.h>
#include <elfutils/libdw.h>

#include <kc.h>
#include <utils.h>
#include <report.h>

static gint cmp_line(gconstpointer _a, gconstpointer _b)
{
	const struct kc_line *a = _a;
	const struct kc_line *b = _b;
	int ret;

	ret = strcmp(a->file->filename, b->file->filename) == 0;
	if (ret)
		ret = (a->lineno - b->lineno) == 0;

	return ret;
}

static guint hash_line(gconstpointer _a)
{
	const struct kc_line *a = _a;

	return g_str_hash(a->file->filename) + g_int_hash(&a->lineno);
}


void kc_add_addr(struct kc *kc, unsigned long addr_in, int line_nr, const char *filename)
{
	struct kc_line *table_line;
	struct kc_addr *addr;
	struct kc_line *line;
	struct kc_file *file;

	file = g_hash_table_lookup(kc->files, filename);
	if (!file) {
		file = kc_file_new(filename);
		g_hash_table_insert(kc->files, (gpointer*)file->filename, file);
	}

	line = kc_line_new(file, line_nr);
	table_line = g_hash_table_lookup(kc->lines, line);

	if (table_line) {
		kc_line_free(line);
		line = table_line;
	}

	addr = kc_addr_new(addr_in);
	if (!table_line)
		g_hash_table_insert(kc->lines, line, line);
	g_hash_table_insert(kc->addrs, (gpointer*)&addr->addr, addr);

	kc_line_add_addr(line, addr);
	kc_file_add_line(file, line);
}

struct kc_addr *kc_lookup_addr(struct kc *kc, unsigned long addr)
{
	return g_hash_table_lookup(kc->addrs, (gpointer *)&addr);
}


static int lookup_elf_type(struct kc *kc, const char *filename, struct Elf *elf)
{
	Elf_Scn *scn = NULL;
	Elf32_Ehdr *hdr32;
	Elf64_Ehdr *hdr64;
	size_t shstrndx;
	int is_32;

	kc->type = PTRACE_FILE;
	if (!elf_getshdrstrndx(elf, &shstrndx) < 0)
		return -1;

	hdr32 = elf32_getehdr(elf);
	hdr64 = elf64_getehdr(elf);
	if (hdr32 == NULL && hdr64 == NULL) {
		error("Can't get elf header\n");
		return -1;
	}
	if (hdr32 != NULL && hdr64 != NULL) {
		error("Both 32- and 64-bit???\n");
		return -1;
	}

	is_32 = hdr32 != NULL;
	if (is_32)
		kc->e_machine = (unsigned int)hdr32->e_machine;
	else
		kc->e_machine = (unsigned int)hdr64->e_machine;

	while ( (scn = elf_nextscn(elf, scn)) != NULL ) {
		const char *name;

		if (is_32) {
			Elf32_Shdr *shdr = elf32_getshdr(scn);

			if (!shdr)
				continue;

			name = elf_strptr(elf, shstrndx, shdr->sh_name);
		}
		else {
			Elf64_Shdr *shdr = elf64_getshdr(scn);

			if (!shdr)
				continue;

			name = elf_strptr(elf, shstrndx, shdr->sh_name);
		}

		/* Linux kernel module? */
		if (strcmp(name, ".modinfo") == 0) {
			char *cpy = xstrdup(filename);
			char *bn = xstrdup(basename(cpy));
			char *p;

			/* Remove .ko and fixup module names */
			p = strrchr(bn, '.');
			if (p)
				*p = '\0';
			for (p = bn; *p; p++) {
				if (*p == '-')
					*p = '_';
			}

			free((void *)kc->module_name);
			kc->type = KPROBE_COVERAGE;
			kc->module_name = bn;

			free(cpy);
			return 0;
		}
		else if (strcmp(name, "__ksymtab") == 0) {
			kc->type = KPROBE_COVERAGE;
			return 0;
		}
	}

	return 0;
}

static const char *lookup_filename_by_pid(pid_t pid)
{
	char linkpath[80 + 1];
	char path[1024];
	ssize_t err;

	memset(path, 0, sizeof(path));
	snprintf(linkpath, sizeof(linkpath) - 1, "/proc/%d/exe", pid);
	linkpath[sizeof(linkpath) - 1 ] = '\0';

	err = readlink(linkpath, path, sizeof(path));
	if (err < 0)
		return NULL;

	/* This allocation will be lost, but that's OK - this function
	 * is only called once at startup anyway. */
	return xstrdup(path);
}

int kc_is_elf(const char *filename)
{
	Elf32_Ehdr hdr;
	int ret = 0;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return ret;

	/* Compare the header with the ELF magic */
	if (read(fd, &hdr, sizeof(hdr)) == sizeof(hdr))
		ret = memcmp(hdr.e_ident, ELFMAG, strlen(ELFMAG)) == 0;

	close(fd);

	return ret;
}

struct kc *kc_open_elf(const char *filename, pid_t pid)
{
	Dwarf_Off offset = 0;
	Dwarf_Off last_offset = 0;
	size_t hdr_size;
	struct Elf *elf;
	Dwarf *dbg;
	struct kc *kc;
	int err;
	int fd;

	if (pid != 0 && filename == NULL) {
		filename = lookup_filename_by_pid(pid);

		if (!filename) {
			error("Can't lookup filename of PID %d\n", pid);
			return NULL;
		}
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	/* Initialize libdwarf */
	dbg = dwarf_begin(fd, DWARF_C_READ);
	if (!dbg) {
		error("No debug symbols in %s.\n"
				"Kcov needs programs built with debugging information (-g)\n", filename);
		close(fd);
		return NULL;
	}

	/* Zeroes everything */
	kc = xmalloc(sizeof(struct kc));
	kc->module_name = xstrdup("");
	kc->in_file = filename;

	kc->addrs = g_hash_table_new(g_int_hash, g_int_equal);
	kc->lines = g_hash_table_new(hash_line, cmp_line);
	kc->files = g_hash_table_new(g_str_hash, g_str_equal);
	kc->sort_type = FILENAME;

	elf = dwarf_getelf(dbg);
	if (!elf) {
		error("Can't get ELF\n");
		goto out_err;
	}

	err = lookup_elf_type(kc, filename, elf);
	if (err < 0)
		goto out_err;
	if (pid != 0)
		kc->type = PTRACE_PID;

	/* Iterate over the headers */
	while (dwarf_nextcu(dbg, offset, &offset, &hdr_size, 0, 0, 0) == 0) {
		Dwarf_Lines* line_buffer;
		size_t line_count;
		Dwarf_Die die;
		int i;

		if (dwarf_offdie(dbg, last_offset + hdr_size, &die) == NULL)
			goto out_err;

		last_offset = offset;

		/* Get the source lines */
		if (dwarf_getsrclines(&die, &line_buffer, &line_count) != 0)
			continue;

		/* Store them */
		for (i = 0; i < line_count; i++) {
			Dwarf_Line *line;
			int line_nr;
			const char* line_source;
			Dwarf_Word mtime, len;
			bool is_code;
			Dwarf_Addr addr;

			if ( !(line = dwarf_onesrcline(line_buffer, i)) )
				goto out_err;
			if (dwarf_lineno(line, &line_nr) != 0)
				goto out_err;
			if (!(line_source = dwarf_linesrc(line, &mtime, &len)) )
				goto out_err;
			if (dwarf_linebeginstatement(line, &is_code) != 0)
				goto out_err;

			if (dwarf_lineaddr(line, &addr) != 0)
				goto out_err;

			if (line_nr && is_code) {
				kc_add_addr(kc, addr, line_nr, line_source);
			}
		}
	}

	/* Shutdown libdwarf */
	if (dwarf_end(dbg) != 0)
		goto out_err;

out_ok:
	close(fd);

	return kc;

out_err:
	close(fd);
	free(kc);

	return NULL;
}

void kc_close(struct kc *kc)
{
	free(kc);
}



static uint64_t get_file_checksum(struct kc *kc)
{
	struct stat st;

	if (lstat(kc->in_file, &st) < 0)
		return 0;

	return ((uint64_t)st.st_mtim.tv_sec << 32) | ((uint64_t)st.st_mtim.tv_nsec);
}

void kc_db_marshal(struct kc_data_db *db)
{
}

void kc_db_unmarshal(struct kc_data_db *db)
{
}

void kc_read_db(struct kc *kc)
{
	struct kc_data_db *db;
	uint64_t checksum;
	size_t sz;
	int i;

	db = read_file(&sz, "%s/%s/kcov.db", kc->out_dir, kc->binary_filename);
	if (!db)
		return;
	kc_db_unmarshal(db);
	checksum = get_file_checksum(kc);
	if (db->elf_checksum != checksum)
		goto out;

	for (i = 0; i < db->n_addrs; i++) {
		struct kc_addr *db_addr = &db->addrs[i];
		struct kc_addr *tgt;

		kc_addr_unmarshall(db_addr);

		tgt = kc_lookup_addr(kc, db_addr->addr);
		if (!tgt)
			continue;

		tgt->hits += db_addr->hits;
	}

out:
	free(db);
}

void kc_write_db(struct kc *kc)
{
	struct kc_data_db *db;
	struct kc_addr *val;
	GHashTableIter iter;
	unsigned long key;
	guint sz;
	int cnt = 0;

	sz = g_hash_table_size(kc->addrs);
	db = xmalloc(sizeof(struct kc_data_db) + sizeof(struct kc_addr) * sz);
	db->n_addrs = sz;
	db->elf_checksum = get_file_checksum(kc);

	g_hash_table_iter_init(&iter, kc->addrs);
	while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&val)) {
		struct kc_addr *db_addr = &db->addrs[cnt];

		memcpy(db_addr, val, sizeof(struct kc_addr));
		kc_addr_marshall(db_addr);

		cnt++;
	}
	kc_db_marshal(db);
	write_file(db, sizeof(struct kc_data_db) + sz * sizeof(struct kc_addr),
			"%s/%s/kcov.db", kc->out_dir, kc->binary_filename);

	free(db);
}

int kc_coverage_db_read(const char *dir, struct kc_coverage_db *dst)
{
	struct kc_coverage_db *db;
	size_t sz;

	db = read_file(&sz, "%s/kcov_coverage.db", dir);
	if (!db)
		return -1;
	if (sz != sizeof(struct kc_coverage_db)) {
		warning("Coverage db size wrong: %u vs %u\n",
				sz, sizeof(struct kc_coverage_db));
		free(db);
		return -1;
	}

	*dst = *db;
	kc_coverage_db_unmarshal(dst);
	free(db);

	return 0;
}

void kc_coverage_db_write(const char *dir, struct kc_coverage_db *src)
{
	struct kc_coverage_db db;

	db = *src;
	kc_coverage_db_marshal(&db);

	write_file(&db, sizeof(struct kc_coverage_db),
			"%s/kcov_coverage.db", dir);
}

void kc_coverage_db_marshal(struct kc_coverage_db *db)
{
}

void kc_coverage_db_unmarshal(struct kc_coverage_db *db)
{
}
