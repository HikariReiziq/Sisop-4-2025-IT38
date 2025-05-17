#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

char *ROOT_DIR;
#define LOG_FILE "/app/antink-logs/it24.log"

int is_dangerous(const char *name) {
    return strcasestr(name, "nafis") || strcasestr(name, "kimcun");
}

void reverse(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

void write_log(const char *msg, const char *file) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t now = time(NULL);
        char *time_str = strtok(ctime(&now), "\n");
        fprintf(log, "[%s] %s : %s\n", time_str, msg, file);
        fclose(log);
    }
}

static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);
    return lstat(fullpath, stbuf);
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);
    DIR *dp = opendir(fullpath);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            filler(buf, de->d_name, NULL, 0);
            continue;
        }

        char display_name[1024];
        strcpy(display_name, de->d_name);

        if (is_dangerous(display_name)) {
            write_log("ALERT! Anomaly detected", display_name);
            reverse(display_name);
        }

        filler(buf, display_name, NULL, 0);
    }

    closedir(dp);
    return 0;
}

void rot13(char *s) {
    while (*s) {
        if (isalpha(*s)) {
            char base = islower(*s) ? 'a' : 'A';
            *s = ((*s - base + 13) % 26) + base;
        }
        s++;
    }
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);

    int res = open(fullpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);

    FILE *f = fopen(fullpath, "r");
    if (!f)
        return -errno;

    fseek(f, offset, SEEK_SET);
    size_t len = fread(buf, 1, size, f);
    fclose(f);

    // Dapatkan nama file dari path
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    // Hanya ROT13 jika file .txt dan tidak berbahaya
    if (!is_dangerous(filename) && strstr(filename, ".txt")) {
        buf[len] = '\0'; // null-terminate buffer sebelum rot13
        rot13(buf);
    }

    return len;
}

static const struct fuse_operations operations = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mount_point>\n", argv[0]);
        exit(1);
    }

    umask(0);
    ROOT_DIR = realpath(argv[1], NULL);
    if (!ROOT_DIR) {
        perror("realpath");
        exit(1);
    }

    char *fuse_argv[] = { argv[0], "-f", argv[2] };
    return fuse_main(3, fuse_argv, &operations, NULL);
}
