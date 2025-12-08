#include "pr_common.h"

unsigned char *read_whole_file(const char *path) {
    FILE *file = NULL;
    unsigned char *file_content = NULL;
    int32 result = 0; // if != 0, there was an error

    {
        file = fopen(path, "rb");
        if (file == NULL) return_defer(1);

        if (fseek(file, 0, SEEK_END)) return_defer(2);
        int64 file_length = ftell(file);
        if (file_length == -1L) return_defer(3);

        if (fseek(file, 0, SEEK_SET)) return_defer(4);

        printf("file_length(%ld); ", file_length);

        file_content = (unsigned char *) malloc(file_length + 1);
        if (file_content == NULL) return_defer(5);
    
        uint64 read_length =
            fread(file_content, sizeof(char), file_length, file);
        printf("read_length(%zu); ", read_length);
        if (read_length != (uint64) file_length) {
            return_defer(6);
        }
        file_content[file_length] = '\0';
    }

    defer:
    if (file) fclose(file);
    if (result != 0 && file_content != NULL) {
        printf("read_whole_file(%s): %d\n", path, result);
        free(file_content);
        file_content = NULL;
    }

    return file_content;
}
