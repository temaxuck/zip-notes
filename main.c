#include <stdio.h>
#define ZIP_IMPLEMENTATION
#include "zip.h"

/* #define TARGET_PATH "./build/archive.zip" */
#define TARGET_PATH "./example.xlsx"

void debug_print_cd_record(ZipReader *r, CDFH_Record h, size_t i);

int unzip(char *target_path) {
	ZipReader r = zip_reader_init(target_path);
	printf("[EOCD] Comment: %s\n", zip_reader_read_eocd_comment(&r));

	zip_reader_begin(&r); {
		CDFH_Record cd_record = {0};
		for (size_t i = 0; zip_reader_next(&r, &cd_record); i++) {
			debug_print_cd_record(&r,cd_record, i);
		}
	} zip_reader_end(&r);

	
	zip_reader_finish(&r);
	return 0;
}

int main() {
	if (unzip(TARGET_PATH) != 0) printf("ERROR: Failed to unzip %s\n", TARGET_PATH);
	return 0;
}


void debug_print_cd_record(ZipReader *r, CDFH_Record h, size_t i) {
	printf("#################### CD ENTRY ####################\n");
	printf("[CD#%zu] File:                   \"%s\"\n", i, zip_reader_cd_file_name(r));
	printf("[CD#%zu] Magic:                  0x%x\n"  , i, h.magic);
	printf("[CD#%zu] Version No:             0x%x\n"  , i, h.version);
	printf("[CD#%zu] Min version No:         0x%x\n"  , i, h.min_version);
	printf("[CD#%zu] General purpose flag:   0b%b\n"  , i, h.general_purpose_flag);
	printf("[CD#%zu] Compression method:     0x%x\n"  , i, h.compression_method);
	printf("[CD#%zu] Last modified time:     %u\n"    , i, h.last_modified_time);
	printf("[CD#%zu] Last modified date:     %u\n"    , i, h.last_modified_date);
	printf("[CD#%zu] CRC32 uncompressed:     0x%x\n"  , i, h.crc32_uncompressed);
	printf("[CD#%zu] File size compressed:   %u\n"    , i, h.file_size_compressed);
	printf("[CD#%zu] File size uncompressed: %u\n"    , i, h.file_size_uncompressed);
	printf("[CD#%zu] File name length:       %u\n"    , i, h.file_name_size);
	printf("[CD#%zu] Extra field length:     %u\n"    , i, h.xfield_size);
	printf("[CD#%zu] Comment length:         %u\n"    , i, h.comment_size);
	printf("[CD#%zu] Start disk No:          %u\n"    , i, h.start_disk_no);
	printf("[CD#%zu] Internal attributes:    0x%x\n"  , i, h.internal_attrs);
	printf("[CD#%zu] External attributes:    0x%x\n"  , i, h.external_attrs);
	printf("[CD#%zu] Contents at:            0x%x\n"  , i, h.loc_relative_offset); // if disk number matches with current
} 
