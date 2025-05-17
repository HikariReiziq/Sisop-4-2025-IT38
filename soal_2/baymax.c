#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <stdarg.h>
void ensure_directory_exists(const char *path);
#define FRAGMENT_SIZE 1024
#define RELIC_DIR "relic"
#define LOG_FILE "activity.log"
#define ZIP_FILE "relic.zip"
#define DOWNLOAD_CMD "wget -q --show-progress -O " ZIP_FILE " \"https://drive.google.com/uc?export=download&id=1MHVhFT57Wa9Zcx69Bv9j5ImHc9rXMH1c\""
#define UNZIP_CMD "unzip -j -o " ZIP_FILE " -d " RELIC_DIR
#define REMOVE_ZIP_CMD "rm -f " ZIP_FILE

static const char *virtual_file = "Baymax.jpeg";
static char *last_read_file = NULL;
static char current_written_file[256] = "";

void log_activity(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        log = fopen(LOG_FILE, "w");  // Force create if not exists
        if (!log) return;
        fclose(log);
        log = fopen(LOG_FILE, "a");
    }
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}

void create_directories() {
    mkdir("bebas", 0700);
    // Hapus cleanup yang tidak perlu
    ensure_directory_exists(RELIC_DIR); // Cukup pastikan direktori ada
}

void ensure_directory_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0755);
    }
}

size_t get_virtual_file_size(const char *name) {
    char path[256];
    size_t total = 0;
    for (int i = 0; i < 14; i++) {  // Fixed to 14 fragments
        snprintf(path, sizeof(path), "%s/%s.%03d", RELIC_DIR, name, i);
        FILE *f = fopen(path, "rb");
        if (!f) break;
        fseek(f, 0, SEEK_END);
        total += ftell(f);
        fclose(f);
    }
    return total;
}

static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path + 1, virtual_file) == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = get_virtual_file_size(virtual_file);
        return 0;
    }

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    return -ENOENT;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, virtual_file, NULL, 0, FUSE_FILL_DIR_PLUS);

    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path + 1, virtual_file) != 0)
        return -ENOENT;

    free(last_read_file);
    last_read_file = strdup(virtual_file);
    log_activity("READ: %s", virtual_file);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path + 1, virtual_file) != 0) return -ENOENT;

    size_t total = 0;
    char frag_path[256];
    
    // Iterasi semua fragment (000-013)
    for (int i = 0; i < 14; i++) {
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELIC_DIR, virtual_file, i);
        FILE *f = fopen(frag_path, "rb");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        size_t fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        // Baca data dari fragment jika offset berada dalam rentang
        if (offset < total + fsize) {
            size_t read_offset = (offset > total) ? (offset - total) : 0;
            size_t to_read = fsize - read_offset;
            if (to_read > size) to_read = size;

            fseek(f, read_offset, SEEK_SET);
            size_t actual_read = fread(buf, 1, to_read, f);
            total += actual_read;
            buf += actual_read;
            size -= actual_read;
        } else {
            total += fsize; // Skip fragment jika offset di luar
        }

        fclose(f);
        if (size == 0) break;
    }
    return total;
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    strncpy(current_written_file, path + 1, sizeof(current_written_file));
    
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/temp_input_%s_%d", current_written_file, getpid());
    FILE *tmp = fopen(tmp_path, "wb");
    if (!tmp) return -EIO;
    fclose(tmp);
    return 0;
}

static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/temp_input_%s_%d", path + 1, getpid());
    FILE *tmp = fopen(tmp_path, "ab");
    if (!tmp) return -EIO;
    size_t written = fwrite(buf, 1, size, tmp);
    fclose(tmp);
    return written;
}

static int fs_release(const char *path, struct fuse_file_info *fi) {
    const char *filename = current_written_file;
    if (filename[0] == '\0') return -EINVAL;

    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/temp_input_%s_%d", filename, getpid());
    FILE *tmp = fopen(tmp_path, "rb");
    if (!tmp) return -EIO;

    int count = 0;
    char fragment_list[4096] = {0};

    while (1) {
        char chunk[FRAGMENT_SIZE];
        size_t r = fread(chunk, 1, FRAGMENT_SIZE, tmp);
        if (r == 0) break;

        char frag_path[256];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELIC_DIR, filename, count);
        FILE *f = fopen(frag_path, "wb");
        if (!f) {
            fclose(tmp);
            remove(tmp_path);
            return -EIO;
        }
        fwrite(chunk, 1, r, f);
        fclose(f);

        if (count > 0) strncat(fragment_list, ", ", sizeof(fragment_list) - strlen(fragment_list));
        strncat(fragment_list, frag_path + strlen(RELIC_DIR) + 1, sizeof(fragment_list) - strlen(fragment_list));
        count++;
    }
    fclose(tmp);
    remove(tmp_path);

    log_activity("WRITE: %s -> %s", filename, fragment_list);

    if (last_read_file && strcmp(last_read_file, filename) != 0) {
        log_activity("COPY: %s -> %s", last_read_file, filename);
        free(last_read_file);
        last_read_file = NULL;
    }

    current_written_file[0] = '\0';
    return 0;
}

static struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open = fs_open,
    .read = fs_read,
    .create = fs_create,
    .write = fs_write,
    .release = fs_release,
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        static char *default_argv[] = { NULL, "bebas", "-o", "allow_other", NULL };
        default_argv[0] = argv[0];
        argv = default_argv;
        argc = 4;
    }

    printf("Downloading...\n");
    system(DOWNLOAD_CMD);

    printf("Unzipping...\n");
    system(UNZIP_CMD);
    system(REMOVE_ZIP_CMD);
    
    create_directories();
    ensure_directory_exists(RELIC_DIR);
    ensure_directory_exists("bebas");
    
    return fuse_main(argc, argv, &fs_oper, NULL);
}
