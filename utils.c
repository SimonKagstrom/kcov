#include <stdio.h>
#include <utils.h>
#include <sys/types.h>
#include <sys/stat.h>

int write_file(const char *dir, const char *filename,
		const char* data, size_t len)
{
	char buf[255];
	FILE *fp;
	int ret = 0;

	xsnprintf(buf, sizeof(buf), "%s/%s", dir, filename);
	fp = fopen(buf, "w");
	if (!fp)
		return -1;

	if (fwrite(data, sizeof(uint8_t), len, fp) != len)
		ret = -1;
	fclose(fp);

	return ret;
}

const char *dir_concat(const char *dir, const char *filename)
{
	size_t len = strlen(dir) + strlen(filename) + 4;
	char *out = xmalloc(len);

	xsnprintf(out, len, "%s/%s", dir, filename);

	return out;
}

int file_exists(const char *path)
{
	struct stat st;

	return lstat(path, &st) == 0;
}

static void read_write(FILE *dst, FILE *src)
{
	char buf[1024];

	while (!feof(src)) {
		int n = fread(buf, sizeof(char), sizeof(buf), src);

		fwrite(buf, sizeof(char), n, dst);
	}
}

int concat_files(const char *dst_name, const char *file_a, const char *file_b)
{
	FILE *dst;
	FILE *s1, *s2;
	int ret = -1;

	dst = fopen(dst_name, "w");
	if (!dst)
		return -1;
	s1 = fopen(file_a, "r");
	if (!s1)
		goto out_dst;
	s2 = fopen(file_b, "r");
	if (!s2)
		goto out_s1;

	read_write(dst, s1);
	read_write(dst, s2);

	fclose(s2);
out_s1:
	fclose(s1);
out_dst:
	fclose(dst);

	return ret;
}
