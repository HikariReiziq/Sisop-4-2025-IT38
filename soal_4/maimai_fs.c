#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <zlib.h>

struct maimai_config {
    char *root_dir;
    unsigned char aes_key[32];
};

#define MAI_EXT ".mai"
#define MAI_EXT_LEN 4
#define AES_BLOCK_SIZE 16
#define GZIP_CHUNK 16384
#define STARTER_PREFIX "/starter/"
#define METRO_PREFIX "/metro/"
#define DRAGON_PREFIX "/dragon/"
#define BLACKROSE_PREFIX "/blackrose/"
#define HEAVEN_PREFIX "/heaven/"
#define YOUTH_PREFIX "/youth/"
#define SEVENREF_PREFIX "/7sref/"

static void mkdir_p(const char *dir);
static char* get_real_path(const char *fuse_path, const char *root_dir);
static int route_7sref(const char *path, char *target_area, char *target_file);

static void metro_encode(char *data, size_t size);
static void metro_decode(char *data, size_t size);
static void rot13_encode(char *data, size_t size);
static int gzip_compress(const char *input, size_t in_len, char **output, size_t *out_len);
static int gzip_decompress(const char *input, size_t in_len, char **output, size_t *out_len);

static int aes_encrypt(const unsigned char *plaintext, int plaintext_len,
                      const unsigned char *key, const unsigned char *iv,
                      unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    if(!(ctx = EVP_CIPHER_CTX_new())) return -1;
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;

    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

static int aes_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                      const unsigned char *key, const unsigned char *iv,
                      unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    if(!(ctx = EVP_CIPHER_CTX_new())) return -1;
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len = len;

    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

static void mkdir_p(const char *dir) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static char* get_real_path(const char *fuse_path, const char *root_dir) {
    char target_area[32] = {0};
    char target_file[PATH_MAX] = {0};
    if (strncmp(fuse_path, SEVENREF_PREFIX, strlen(SEVENREF_PREFIX)) == 0) {
        if (route_7sref(fuse_path, target_area, target_file)) {
            size_t len = strlen(root_dir) + strlen("/chiho/") + strlen(target_area) + 
                        strlen(target_file) + 1;
            char *real_path = malloc(len);
            if (real_path) {
                snprintf(real_path, len, "%s/chiho/%s/%s", root_dir, target_area, target_file);
                return real_path;
            }
        }
        return NULL;
    }
    
    const char *prefix = NULL;
    const char *chiho_subdir = NULL;
    const char *ext = "";
    
    if (strncmp(fuse_path, STARTER_PREFIX, strlen(STARTER_PREFIX)) == 0) {
        prefix = STARTER_PREFIX;
        chiho_subdir = "starter";
        if (strlen(fuse_path) > strlen(STARTER_PREFIX)) ext = MAI_EXT;
    }
    else if (strncmp(fuse_path, METRO_PREFIX, strlen(METRO_PREFIX)) == 0) {
        prefix = METRO_PREFIX;
        chiho_subdir = "metro";
    }
    else if (strncmp(fuse_path, DRAGON_PREFIX, strlen(DRAGON_PREFIX)) == 0) {
        prefix = DRAGON_PREFIX;
        chiho_subdir = "dragon";
    }
    else if (strncmp(fuse_path, BLACKROSE_PREFIX, strlen(BLACKROSE_PREFIX)) == 0) {
        prefix = BLACKROSE_PREFIX;
        chiho_subdir = "blackrose";
    }
    else if (strncmp(fuse_path, HEAVEN_PREFIX, strlen(HEAVEN_PREFIX)) == 0) {
        prefix = HEAVEN_PREFIX;
        chiho_subdir = "heaven";
    }
    else if (strncmp(fuse_path, YOUTH_PREFIX, strlen(YOUTH_PREFIX)) == 0) {
        prefix = YOUTH_PREFIX;
        chiho_subdir = "youth";
    }
    else {
        return NULL;
    }
    
    size_t len = strlen(root_dir) + strlen("/chiho/") + strlen(chiho_subdir) + 
                strlen(fuse_path + strlen(prefix)) + strlen(ext) + 1;
    char *real_path = malloc(len);
    if (!real_path) return NULL;
    
    snprintf(real_path, len, "%s/chiho/%s/%s%s", root_dir, chiho_subdir, 
             fuse_path + strlen(prefix), ext);
    return real_path;
}

static int route_7sref(const char *path, char *target_area, char *target_file) {
    const char *filename = path + strlen(SEVENREF_PREFIX);
    char *underscore = strchr(filename, '_');
    
    if (!underscore) return 0;
    
    size_t area_len = underscore - filename;
    if (area_len >= 32) return 0;
    
    strncpy(target_area, filename, area_len);
    target_area[area_len] = '\0';
    strcpy(target_file, underscore + 1);
    
    if (strcmp(target_area, "starter") == 0 ||
        strcmp(target_area, "metro") == 0 ||
        strcmp(target_area, "dragon") == 0 ||
        strcmp(target_area, "blackrose") == 0 ||
        strcmp(target_area, "heaven") == 0 ||
        strcmp(target_area, "youth") == 0) {
        return 1;
    }
    
    return 0;
}

static void metro_encode(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] = (data[i] + (i % 256)) % 256;
    }
}

static void metro_decode(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] = (data[i] - (i % 256) + 256) % 256;
    }
}

static void rot13_encode(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] = 'a' + ((data[i] - 'a' + 13) % 26);
        }
        else if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] = 'A' + ((data[i] - 'A' + 13) % 26);
        }
    }
}

static int gzip_compress(const char *input, size_t in_len, char **output, size_t *out_len) {
    z_stream strm;
    int ret;
    
    *output = malloc(in_len + in_len/10 + 128); // Rough estimate
    if (!*output) return -1;
    
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
                      15 + 16, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        free(*output);
        return -1;
    }
    
    strm.next_in = (Bytef*)input;
    strm.avail_in = in_len;
    strm.next_out = (Bytef*)*output;
    strm.avail_out = in_len + in_len/10 + 128;
    
    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        free(*output);
        return -1;
    }
    
    *out_len = strm.total_out;
    deflateEnd(&strm);
    return 0;
}

static int gzip_decompress(const char *input, size_t in_len, char **output, size_t *out_len) {
    z_stream strm;
    int ret;
    char *out = malloc(GZIP_CHUNK);
    size_t out_size = GZIP_CHUNK;
    
    if (!out) return -1;
    
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = in_len;
    strm.next_in = (Bytef*)input;
    
    ret = inflateInit2(&strm, 15 + 16);
    if (ret != Z_OK) {
        free(out);
        return -1;
    }
    
    *output = NULL;
    *out_len = 0;
    
    do {
        strm.avail_out = out_size - *out_len;
        strm.next_out = (Bytef*)(out + *out_len);
        
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR) break;
        
        *out_len = strm.total_out;
        
        if (strm.avail_out == 0) {
            out_size += GZIP_CHUNK;
            out = realloc(out, out_size);
            if (!out) {
                inflateEnd(&strm);
                return -1;
            }
        }
    } while (ret != Z_STREAM_END);
    
    if (ret != Z_STREAM_END) {
        inflateEnd(&strm);
        free(out);
        return -1;
    }
    
    *output = out;
    inflateEnd(&strm);
    return 0;
}

static int maimai_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    struct maimai_config *cfg = fuse_get_context()->private_data;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth", "7sref"};
    for (size_t i = 0; i < sizeof(areas)/sizeof(areas[0]); i++) {
        char dir_path[32];
        snprintf(dir_path, sizeof(dir_path), "/%s", areas[i]);
        
        if (strcmp(path, dir_path) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    
    int res = lstat(real_path, stbuf);
    free(real_path);
    return res == -1 ? -errno : 0;
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;
    struct maimai_config *cfg = fuse_get_context()->private_data;
    
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    if (strcmp(path, "/") == 0) {
        const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth", "7sref"};
        for (size_t i = 0; i < sizeof(areas)/sizeof(areas[0]); i++) {
            filler(buf, areas[i], NULL, 0, 0);
        }
        return 0;
    }
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    
    DIR *dp = opendir(real_path);
    free(real_path);
    if (!dp) return -errno;
    
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (de->d_name[0] == '.') continue;
        if (strncmp(path, STARTER_PREFIX, strlen(STARTER_PREFIX)) == 0) {
            size_t name_len = strlen(de->d_name);
            if (name_len > MAI_EXT_LEN && 
                strcmp(de->d_name + name_len - MAI_EXT_LEN, MAI_EXT) == 0) {
                char display_name[name_len - MAI_EXT_LEN + 1];
                strncpy(display_name, de->d_name, name_len - MAI_EXT_LEN);
                display_name[name_len - MAI_EXT_LEN] = '\0';
                filler(buf, display_name, NULL, 0, 0);
                continue;
            }
        }
        filler(buf, de->d_name, NULL, 0, 0);
    }
    closedir(dp);
    return 0;
}

static int maimai_open(const char *path, struct fuse_file_info *fi) {
    struct maimai_config *cfg = fuse_get_context()->private_data;
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    
    int res = open(real_path, fi->flags);
    free(real_path);
    if (res == -1) return -errno;
    
    close(res);
    return 0;
}

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void) fi;
    struct maimai_config *cfg = fuse_get_context()->private_data;
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    
    int fd = open(real_path, O_RDONLY);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }
    
    char *file_buf = malloc(size);
    if (!file_buf) {
        close(fd);
        free(real_path);
        return -ENOMEM;
    }
    
    int res = pread(fd, file_buf, size, offset);
    close(fd);
    
    if (res <= 0) {
        free(file_buf);
        free(real_path);
        return res == -1 ? -errno : res;
    }
    
    if (strncmp(path, METRO_PREFIX, strlen(METRO_PREFIX)) == 0) {
        metro_decode(file_buf, res);
    }
    else if (strncmp(path, DRAGON_PREFIX, strlen(DRAGON_PREFIX)) == 0) {
        rot13_encode(file_buf, res);
    }
    else if (strncmp(path, HEAVEN_PREFIX, strlen(HEAVEN_PREFIX)) == 0) {
        // First 16 bytes are IV
        if (res <= AES_BLOCK_SIZE) {
            free(file_buf);
            free(real_path);
            return -EIO;
        }
        
        unsigned char iv[AES_BLOCK_SIZE];
        memcpy(iv, file_buf, AES_BLOCK_SIZE);
        
        char *decrypted = malloc(res - AES_BLOCK_SIZE); // Maximum possible size
        if (!decrypted) {
            free(file_buf);
            free(real_path);
            return -ENOMEM;
        }
        
        int dec_len = aes_decrypt((unsigned char*)file_buf + AES_BLOCK_SIZE, res - AES_BLOCK_SIZE,
                                cfg->aes_key, iv, (unsigned char*)decrypted);
        if (dec_len <= 0) {
            free(decrypted);
            free(file_buf);
            free(real_path);
            return -EIO;
        }
        
        free(file_buf);
        file_buf = decrypted;
        res = dec_len;
    }
    else if (strncmp(path, YOUTH_PREFIX, strlen(YOUTH_PREFIX)) == 0) {
        char *decompressed;
        size_t decompressed_len;
        if (gzip_decompress(file_buf, res, &decompressed, &decompressed_len) != 0) {
            free(file_buf);
            free(real_path);
            return -EIO;
        }
        
        free(file_buf);
        file_buf = decompressed;
        res = decompressed_len;
    }
    
    memcpy(buf, file_buf, res);
    free(file_buf);
    free(real_path);
    
    return res;
}

static int maimai_write(const char *path, const char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void) fi;
    struct maimai_config *cfg = fuse_get_context()->private_data;
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    char *dir_path = strdup(real_path);
    char *dir = dirname(dir_path);
    mkdir_p(dir);
    free(dir_path);
    
    int res;
    char *processed_buf = NULL;
    size_t processed_size = size;
    
    if (strncmp(path, METRO_PREFIX, strlen(METRO_PREFIX)) == 0) {
        processed_buf = malloc(size);
        if (!processed_buf) {
            free(real_path);
            return -ENOMEM;
        }
        memcpy(processed_buf, buf, size);
        metro_encode(processed_buf, size);
    }
    else if (strncmp(path, DRAGON_PREFIX, strlen(DRAGON_PREFIX)) == 0) {
        processed_buf = malloc(size);
        if (!processed_buf) {
            free(real_path);
            return -ENOMEM;
        }
        memcpy(processed_buf, buf, size);
        rot13_encode(processed_buf, size);
    }
    else if (strncmp(path, HEAVEN_PREFIX, strlen(HEAVEN_PREFIX)) == 0) {
        unsigned char iv[AES_BLOCK_SIZE];
        RAND_bytes(iv, AES_BLOCK_SIZE);
        unsigned char temp_buf[size + AES_BLOCK_SIZE];
        int enc_len = aes_encrypt((unsigned char*)buf, size, cfg->aes_key, iv, temp_buf);
        if (enc_len <= 0) {
            free(real_path);
            return -EIO;
        }
        
        processed_size = AES_BLOCK_SIZE + enc_len;
        processed_buf = malloc(processed_size);
        if (!processed_buf) {
            free(real_path);
            return -ENOMEM;
        }
        
        memcpy(processed_buf, iv, AES_BLOCK_SIZE);
        memcpy(processed_buf + AES_BLOCK_SIZE, temp_buf, enc_len);
    }
    else if (strncmp(path, YOUTH_PREFIX, strlen(YOUTH_PREFIX)) == 0) {
        if (gzip_compress(buf, size, &processed_buf, &processed_size) != 0) {
            free(real_path);
            return -EIO;
        }
    }
    else {
        processed_buf = (char*)buf; // No processing for other areas
    }
    
    int fd = open(real_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        if (processed_buf != buf) free(processed_buf);
        free(real_path);
        return -errno;
    }
    
    res = pwrite(fd, processed_buf, processed_size, offset);
    close(fd);
    
    if (processed_buf != buf) free(processed_buf);
    free(real_path);
    
    return res == -1 ? -errno : (res == processed_size ? size : res);
}

static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    struct maimai_config *cfg = fuse_get_context()->private_data;
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    char *dir_path = strdup(real_path);
    char *dir = dirname(dir_path);
    mkdir_p(dir);
    free(dir_path);
    
    int res = creat(real_path, mode);
    free(real_path);
    if (res == -1) return -errno;
    
    close(res);
    return 0;
}

static int maimai_unlink(const char *path) {
    struct maimai_config *cfg = fuse_get_context()->private_data;
    
    char *real_path = get_real_path(path, cfg->root_dir);
    if (!real_path) return -ENOENT;
    
    int res = unlink(real_path);
    free(real_path);
    if (res == -1) return -errno;
    
    return 0;
}

static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .open = maimai_open,
    .read = maimai_read,
    .write = maimai_write,
    .create = maimai_create,
    .unlink = maimai_unlink,
};

int main(int argc, char *argv[]) {
    struct maimai_config cfg;
    
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <root_dir> <mountpoint> [options]\n", argv[0]);
        return 1;
    }
    
    cfg.root_dir = realpath(argv[1], NULL);
    if (!cfg.root_dir) {
        perror("Error resolving root directory path");
        return 1;
    }
    
    RAND_bytes(cfg.aes_key, sizeof(cfg.aes_key));
    
    const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "youth"};
    for (size_t i = 0; i < sizeof(areas)/sizeof(areas[0]); i++) {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/chiho/%s", cfg.root_dir, areas[i]);
        mkdir_p(path);
    }
    
    char *fuse_argv[argc];
    fuse_argv[0] = argv[0];
    fuse_argv[1] = argv[2];  // mountpoint
    for (int i = 3; i < argc; i++) {
        fuse_argv[i-1] = argv[i];
    }
    int fuse_argc = argc - 1;
    
    return fuse_main(fuse_argc, fuse_argv, &maimai_oper, &cfg);
}
