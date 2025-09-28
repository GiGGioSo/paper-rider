#include "pr_common.h"

char *read_whole_file(const char *path) {
    FILE *file = NULL;
    char *file_content = NULL;
    int32 result = 0; // if != 0, there was an error

    {
        file = fopen(path, "rb");
        if (file == NULL) return_defer(1);

        if (fseek(file, 0, SEEK_END)) return_defer(2);
        int64 file_length = ftell(file);
        if (file_length == -1L) return_defer(3);

        file_content = (char *) malloc(file_length + 1);
        if (file_content == NULL) return_defer(4);

        if (fread(file_content, sizeof(char), file_length, file) != (uint64) file_length) {
            return_defer(5);
        }
        file_content[file_length] = '\0';
    }

    defer:
    if (file) fclose(file);
    if (result != 0 && file_content != NULL) {
        free(file_content);
        file_content = NULL;
    }

    return file_content;
}
