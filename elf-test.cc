#include <gelf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* First Elf_Data buffer for section .foo (ELF_T_WORD, align 4) */
uint32_t hash_words[] = {
	0x01234567,
	0x89abcdef,
	0xdeadc0de
};

/* Second Elf_Data buffer for section .foo (ELF_T_BYTE, align 1) */
char data_string[] = "helloworld";

/* Third Elf_Data buffer for section .foo (ELF_T_WORD, align 4) */
uint32_t checksum[] = {
	0xffffeeee
};

char string_table[] = {
	/* Offset 0 */   '\0',
	/* Offset 1 */   '.', 'f' ,'o', 'o', '\0',
	/* Offset 6 */   '.', 's' , 'h' , 's' , 't',
	'r', 't', 'a', 'b', '\0'
};

int
main(int argc, char **argv)
{
	Elf *e;
	Elf_Scn *scn;
	Elf_Data *data, *data1, *data2, *data3;
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;
	Elf64_Shdr *shdr;
	int fd;

	if (argc != 2) {
		fprintf(stderr, "usage: %s: file\n", argv[0]);
		exit(1);
	}

	if ((fd = open(argv[1], O_RDWR, 0644)) < 0) {
		fprintf(stderr, "open %s failed\n", argv[1]);
		exit(1);
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "elf_version failed\n");
		exit(1);
	}

	if ((e = elf_begin(fd, ELF_C_RDWR, NULL)) == NULL) {
		fprintf(stderr, "elf_begin failed\n");
		exit(1);
	}

	ehdr = elf64_getehdr(e);
	printf("XXX: %d\n", ehdr->e_shnum);

	/*
	 * Create .foo section (which has three Elf_Data buffer)
	 */
	if ((scn = elf_newscn(e)) == NULL) {
		fprintf(stderr, "elf_newscn failed\n");
		exit(1);
	}

	if ((data1 = elf_newdata(scn)) == NULL) {
		fprintf(stderr, "elf_newdata failed\n");
		exit(1);
	}

	data1->d_align = 4;
	data1->d_off = 0;
	data1->d_buf = hash_words;
	/* Elf_Data elftype != section elftype. */
	data1->d_type = ELF_T_WORD;
	data1->d_size = sizeof(hash_words);
	data1->d_version = EV_CURRENT;

	if ((shdr = elf64_getshdr(scn)) == NULL) {
		fprintf(stderr, "elf64_getshdr failed\n");
		exit(1);
	}

	shdr->sh_name = 1;
	shdr->sh_type = SHT_HASH; /* section elftype is ELF_T_BYTE */
	shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	shdr->sh_entsize = 0;
	shdr->sh_addralign = 4;	/* Set .foo section alignment to 4. */

	ehdr = elf64_getehdr(e);
	printf("XXX: %d\n", ehdr->e_shnum);

	if (elf_update(e, ELF_C_NULL) < 0) {
		fprintf(stderr, "NULL call failed\n");
		exit(1);
	}
	/*
	 * Write out the ELF object.
	 */

	if (elf_update(e, ELF_C_WRITE) < 0) {
		fprintf(stderr, "elf_update failed: %s\n",
		    elf_errmsg(elf_errno()));
		exit(1);
	}

	elf_end(e);
	close(fd);

	exit(0);
}

