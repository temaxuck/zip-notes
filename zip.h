#ifndef ZIP_H
#  define ZIP_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define ZIP_DYNAMIC_FIELD_MAX_SIZE 0xffff

#define ZIP_CDFH_RECORD_SIZE         offsetof(CDFH_Record, _pad)
#define ZIP_EOCD_RECORD_SIZE         offsetof(EOCD_Record, _pad)
#define ZIP_LOC_RECORD_MAGIC_NUMBER  0x04034b50
#define ZIP_CDFH_RECORD_MAGIC_NUMBER 0x02014b50
#define ZIP_EOCD_RECORD_MAGIC_NUMBER 0x06054b50

//===----------------------------------------------------------------------===
// Zip record types
//===----------------------------------------------------------------------===
/**
 * End of central directory record
 */
typedef struct __attribute__((packed)) {
	uint32_t magic;
	uint16_t cur_disk_no;
	uint16_t cd_start_disk_no;
	uint16_t cur_disk_cd_record_count;
	uint16_t total_cd_record_count;
	uint32_t cd_size;
	uint32_t cd_offset;
	uint16_t comment_size;
	char     _pad[2];
} EOCD_Record;

/**
 * Central directory file header
 */
typedef struct __attribute__((packed)) {
	uint32_t magic;
	uint16_t version;
	uint16_t min_version;
	uint16_t general_purpose_flag;
	uint16_t compression_method;
	uint16_t last_modified_time;
	uint16_t last_modified_date;
	uint32_t crc32_uncompressed;
	uint32_t file_size_compressed;
	uint32_t file_size_uncompressed;
	uint16_t file_name_size;
	uint16_t xfield_size;
	uint16_t comment_size;
	uint16_t start_disk_no;
	uint16_t internal_attrs;
	uint32_t external_attrs;
	uint32_t loc_relative_offset;
	char     _pad[2];
} CDFH_Record;

/**
 * Local file header
 */
typedef struct __attribute__((packed)) {
	uint32_t magic;
	uint16_t min_version;
	uint16_t general_purpose_flag;
	uint16_t compression_method;
	uint16_t last_modified_time;
	uint16_t last_modified_date;
	uint32_t crc32_uncompressed;
	uint32_t file_size_compressed;
	uint32_t file_size_uncompressed;
	uint16_t file_name_size;
	uint16_t xfield_size;
	char     _pad[2];
} LOC_Record;

//===-----------------------------------------------------------------------===
// Zip compression
//===-----------------------------------------------------------------------===
typedef enum {
	ZIP_COMPRESSION_METHOD_NO_COMPRESSION	   = 0,
	ZIP_COMPRESSION_METHOD_SHRINK			   = 1,
	ZIP_COMPRESSION_METHOD_REDUCED_1		   = 2,
	ZIP_COMPRESSION_METHOD_REDUCED_2		   = 3,
	ZIP_COMPRESSION_METHOD_REDUCED_3		   = 4,
	ZIP_COMPRESSION_METHOD_REDUCED_4		   = 5,
	ZIP_COMPRESSION_METHOD_IMPLODE			   = 6,
	ZIP_COMPRESSION_METHOD_RESERVED_TOKENIZING = 7,
	ZIP_COMPRESSION_METHOD_DEFLATE			   = 8,
	ZIP_COMPRESSION_METHOD_DEFLATE64		   = 9,
	ZIP_COMPRESSION_METHOD_LIB_IMPLODING	   = 10,
	ZIP_COMPRESSION_METHOD_RESERVED			   = 11,
	ZIP_COMPRESSION_METHOD_BZIP2			   = 12,
	ZIP_COMPRESSION_METHOD_COUNT,
} ZipCompressionMethod;

//===----------------------------------------------------------------------===
// Zip reader
//===----------------------------------------------------------------------===
typedef struct {
	int fd;
	off_t offset;
	ssize_t file_size;

	EOCD_Record eocd;
	off_t eocd_offset;

	CDFH_Record last_cd;
	off_t last_cd_offset;
	off_t next_cd_offset;
} ZipReader;

ZipReader zip_reader_init             (char *target_path);
void      zip_reader_finish           (ZipReader *r);
void      zip_reader_begin            (ZipReader *r);
bool      zip_reader_next             (ZipReader *r, CDFH_Record *record);
char*     zip_reader_cd_file_name     (ZipReader *r);
char*     zip_reader_cd_xfield        (ZipReader *r);
char*     zip_reader_cd_comment       (ZipReader *r);
void      zip_reader_end              (ZipReader *r);
char*     zip_reader_read_eocd_comment(ZipReader *r);
#endif // ZIP_H

#ifdef ZIP_IMPLEMENTATION
#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//===----------------------------------------------------------------------===
// Zip reader
//===----------------------------------------------------------------------===
void zip_reader__seek(ZipReader *r, off_t offset) {
	assert(offset >= 0 && offset <= r->file_size && "Out of bounds");
	r->offset = offset;
	assert(lseek(r->fd, r->offset, SEEK_SET) != -1 && "Failed to lseek");
}

void zip_reader__peek(ZipReader *r, void *dest, size_t count) {
	assert(r->offset + count <= (size_t)r->file_size && "Reading out of bounds");
	assert(pread(r->fd, dest, count, r->offset) != -1 && "Failed to pread");
}

void zip_reader__read(ZipReader *r, void *dest, size_t count) {
	assert(r->offset + count <= (size_t)r->file_size && "Reading out of bounds");
	ssize_t nread = read(r->fd, dest, count);
	assert(nread != -1 && "Failed to read");
	r->offset += nread;
}

char *zip_reader__read_dynamic_field(ZipReader *r, size_t size) {
	static char field[ZIP_DYNAMIC_FIELD_MAX_SIZE + 1] = {0};

	assert(size <= ZIP_DYNAMIC_FIELD_MAX_SIZE && "Size exceeds max allowed field size");
	zip_reader__read(r, field, size);
	memset(field + size, 0, ZIP_DYNAMIC_FIELD_MAX_SIZE - size);

	return field;
}

void zip_reader__init_cd(ZipReader *r) {
	assert(r->file_size >= (off_t)ZIP_EOCD_RECORD_SIZE && "Bad zip");
	zip_reader__seek(r, r->file_size - ZIP_EOCD_RECORD_SIZE);

	uint32_t buf = 0;

	// find eocd record
	while (1) {
		zip_reader__peek(r, &buf, sizeof(buf));
		r->eocd_offset = r->offset;
		if (buf == ZIP_EOCD_RECORD_MAGIC_NUMBER) break;
		zip_reader__seek(r, r->offset - 1);
	}

	EOCD_Record eocd = {0};
	zip_reader__peek(r, &eocd, ZIP_EOCD_RECORD_SIZE);
	assert(eocd.cd_offset <= r->file_size && "Bad zip");
	r->eocd = eocd;

	r->next_cd_offset = -1;
	r->last_cd_offset = -1;
}

ZipReader zip_reader_init(char *target_path) {
	ZipReader r = {0};

	r.fd = open(target_path, O_RDONLY);
	assert(r.fd != -1 && "Failed to open file");

	struct stat statbuf = {0};
	assert(fstat(r.fd, &statbuf) == 0 && "Failed to read file stats");
	r.file_size = statbuf.st_size;

	zip_reader__init_cd(&r);

	return r;
}

void zip_reader_finish(ZipReader *r) {
	close(r->fd);
}

void zip_reader_begin(ZipReader *r) {
	r->last_cd = (CDFH_Record){0};
	r->last_cd_offset = -1;
	r->next_cd_offset = r->eocd.cd_offset;
	zip_reader__seek(r, r->next_cd_offset);
}

bool zip_reader_next(ZipReader *r, CDFH_Record *record) {
	if (r->next_cd_offset == -1) return false;

	r->last_cd_offset = r->offset;

	zip_reader__read(r, record, ZIP_CDFH_RECORD_SIZE);
	assert(record->magic == ZIP_CDFH_RECORD_MAGIC_NUMBER && "Bad zip");
	assert(memcpy(&r->last_cd, record, sizeof(*record)) != NULL && "Failed to memcpy");

	size_t offset = r->offset + record->file_name_size + record->xfield_size + record->comment_size;
	zip_reader__seek(r, offset);
	
	if (r->offset >= r->eocd.cd_size + r->eocd.cd_offset) r->next_cd_offset = -1;
	else r->next_cd_offset = r->offset;

	return true;
}

char* zip_reader_cd_file_name(ZipReader *r) {
	assert(r->last_cd_offset != -1 && "Not in central directory mode");
	off_t prev_pos = r->offset;

	off_t offset = r->last_cd_offset + ZIP_CDFH_RECORD_SIZE;
	zip_reader__seek(r, offset);
	char *filename = zip_reader__read_dynamic_field(r, r->last_cd.file_name_size);

	zip_reader__seek(r, prev_pos);
	return filename;
}

char* zip_reader_cd_xfield(ZipReader *r) {
	assert(r->last_cd_offset != -1 && "Not in central directory mode");
	off_t prev_pos = r->offset;

	off_t offset = r->last_cd_offset + ZIP_CDFH_RECORD_SIZE + r->last_cd.file_name_size;
	zip_reader__seek(r, offset);
	char *filename = zip_reader__read_dynamic_field(r, r->last_cd.xfield_size);

	zip_reader__seek(r, prev_pos);
	return filename;
}

char* zip_reader_cd_comment(ZipReader *r) {
	assert(r->last_cd_offset != -1 && "Not in central directory mode");
	off_t prev_pos = r->offset;

	off_t offset = r->last_cd_offset + ZIP_CDFH_RECORD_SIZE + r->last_cd.file_name_size + r->last_cd.xfield_size;
	zip_reader__seek(r, offset);
	char *filename = zip_reader__read_dynamic_field(r, r->last_cd.comment_size);

	zip_reader__seek(r, prev_pos);
	return filename;
}

void zip_reader_end(ZipReader *r) {
	r->last_cd_offset = -1;
	r->next_cd_offset = -1;
}

char *zip_reader_read_eocd_comment(ZipReader *r) {
	off_t prev_pos = r->offset;

	zip_reader__seek(r, r->eocd_offset + ZIP_EOCD_RECORD_SIZE);
	char *comment = zip_reader__read_dynamic_field(r, r->eocd.comment_size);

	zip_reader__seek(r, prev_pos);
	return comment;
}
#endif // ZIP_IMPLEMENTATION
