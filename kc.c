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
#include <libdwarf.h>

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


static void dwarf_error_handler(Dwarf_Error error, Dwarf_Ptr userData)
{
	char *msg = dwarf_errmsg(error);

	printf("Dwarf: %s\n", msg);
}

static void lookup_elf_type(struct kc *kc, const char *filename, struct Elf *elf)
{
	Elf_Scn *scn = NULL;
	Elf32_Ehdr *hdr = elf32_getehdr(elf);
	size_t shstrndx;
	int is_32;

	kc->type = PTRACE_FILE;
	if (!hdr || elf_getshdrstrndx(elf, &shstrndx) < 0)
		return;

	is_32 = hdr->e_ident[EI_CLASS] == ELFCLASS32;

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
			return;
		}
		else if (strcmp(name, "__ksymtab") == 0) {
			kc->type = KPROBE_COVERAGE;
			return;
		}
	}
}


struct kc *kc_open_elf(const char *filename)
{
	Dwarf_Unsigned header;
	struct Elf *elf;
	Dwarf_Debug dbg;
	struct kc *kc;
	int err;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	/* Initialize libdwarf */
	err = dwarf_init(fd, DW_DLC_READ, dwarf_error_handler, 0, &dbg,0);
	if (err == DW_DLV_ERROR) {
		close(fd);
		return NULL;
	}

	/* Zeroes everything */
	kc = xmalloc(sizeof(struct kc));
	kc->module_name = xstrdup("");

	kc->addrs = g_hash_table_new(g_int_hash, g_int_equal);
	kc->lines = g_hash_table_new(hash_line, cmp_line);
	kc->files = g_hash_table_new(g_str_hash, g_str_equal);
	kc->sort_type = FILENAME;

	if (err == DW_DLV_NO_ENTRY)
		goto out_ok;

	err = dwarf_get_elf(dbg, &elf, NULL);
	lookup_elf_type(kc, filename, elf);

	/* Iterate over the headers */
	while (dwarf_next_cu_header(dbg, 0, 0, 0, 0,
			&header, 0) == DW_DLV_OK) {
		Dwarf_Line* line_buffer;
		Dwarf_Signed line_count;
		Dwarf_Die die;
		int i;

		if (dwarf_siblingof(dbg, 0, &die, 0) != DW_DLV_OK)
			goto out_err;

		/* Get the source lines */
		if (dwarf_srclines(die, &line_buffer, &line_count, 0) != DW_DLV_OK)
			continue;

		/* Store them */
		for (i = 0; i < line_count; i++) {
			Dwarf_Unsigned line_nr;
			char* line_source;
			Dwarf_Bool is_code;
			Dwarf_Addr addr;

			if (dwarf_lineno(line_buffer[i], &line_nr, 0) != DW_DLV_OK)
				goto out_err;
			if (dwarf_linesrc(line_buffer[i], &line_source, 0) != DW_DLV_OK)
				goto out_err;
			if (dwarf_linebeginstatement(line_buffer[i], &is_code, 0) != DW_DLV_OK)
				goto out_err;

			if (dwarf_lineaddr(line_buffer[i], &addr, 0) != DW_DLV_OK)
				goto out_err;

			if (line_nr && is_code) {
				kc_add_addr(kc, addr, line_nr, line_source);
			}

			dwarf_dealloc(dbg, line_source, DW_DLA_STRING);
		}

		/* Release the memory */
		for (i = 0; i < line_count; i++)
			dwarf_dealloc(dbg,line_buffer[i], DW_DLA_LINE);
		dwarf_dealloc(dbg,line_buffer, DW_DLA_LIST);
	}

	/* Shutdown libdwarf */
	if (dwarf_finish(dbg, 0) != DW_DLV_OK)
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

	db = read_file(&sz, "%s/kcov.db", kc->out_dir);
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
	write_file(kc->out_dir, "kcov.db", db,
			sizeof(struct kc_data_db) + sz * sizeof(struct kc_addr));

	free(db);
}
