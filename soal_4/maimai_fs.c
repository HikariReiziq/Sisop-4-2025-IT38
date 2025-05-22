#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <zlib.h>

#define AES_BLOCK_SIZE 16
#define MAX_PATH_LEN 1024
#define BLOCK_SIZE 4096
#define AES_KEY "SEGAMaimai2025ChihoSEGAMaimai2025Chiho"
#define IV_FILE_EXT ".iv"

struct maimai_context {
    char* base_path;    // ke chiho/
    char* mount_path;   // ke fuse_dir/
    int debug;
};

char* get_real_path(const char* path);
void shift_bytes(char* data, size_t size, int direction);
void rot13_transform(char* data, size_t size);
int aes_encrypt(const char* input, size_t input_len, char* output, const unsigned char* iv);
int aes_decrypt(const char* input, size_t input_len, char* output, const unsigned char* iv);
int gzip_compress(const char* input, size_t input_len, char* output, size_t* output_len);
int gzip_decompress(const char* input, size_t input_len, char* output, size_t* output_len);

char* get_real_path(const char* path) {
    struct maimai_context* mc = fuse_get_context()->private_data;
    char* real_path = malloc(MAX_PATH_LEN);

    if (!real_path) return NULL;

    if (strcmp(path, "/") == 0) {
        snprintf(real_path, MAX_PATH_LEN, "%s", mc->base_path);
        return real_path;
    }

    if (strstr(path, "/7sref/") != NULL) {
        char* filename = strrchr(path, '/') + 1;
        char* underscore = strchr(filename, '_');

        if (!underscore) {
            free(real_path);
            return NULL;
        }

        *underscore = '\0';
        char* area = filename;
        char* real_filename = underscore + 1;

        snprintf(real_path, MAX_PATH_LEN, "%s/%s/%s",
            mc->base_path, area, real_filename);
        *underscore = '_';
    }
    else {
        snprintf(real_path, MAX_PATH_LEN, "%s%s", mc->base_path, path);

        if (strstr(path, "/starter/") != NULL) {
            strcat(real_path, ".mai");
        }
    }

    if (mc->debug) {
        fprintf(stderr, "Path transform: %s -> %s\n", path, real_path);
    }

    return real_path;
}

// fuse

static int maimai_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) {
    (void)fi;
    struct maimai_context* mc = fuse_get_context()->private_data;
    int res = 0;
    char* real_path = NULL;

    memset(stbuf, 0, sizeof(struct stat));

    if (mc->debug) fprintf(stderr, "getattr: %s\n", path);


    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strcmp(path, "/7sref") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strstr(path, "/7sref/") != NULL) {
        real_path = get_real_path(path);
        if (!real_path) return -ENOENT;
    }
    else {
        real_path = malloc(MAX_PATH_LEN);
        snprintf(real_path, MAX_PATH_LEN, "%s%s", mc->base_path, path);

        if (strstr(path, "/starter/") != NULL) {
            strcat(real_path, ".mai");
        }
    }

    res = lstat(real_path, stbuf);
    free(real_path);

    if (res == -1) return -errno;
    return 0;
}

static int maimai_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    struct maimai_context* mc = fuse_get_context()->private_data;
    DIR* dp;
    struct dirent* de;

    (void)offset;
    (void)fi;
    (void)flags;

    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0, 0);
        filler(buf, "..", NULL, 0, 0);
        filler(buf, "starter", NULL, 0, 0);
        filler(buf, "metro", NULL, 0, 0);
        filler(buf, "dragon", NULL, 0, 0);
        filler(buf, "blackrose", NULL, 0, 0);
        filler(buf, "heaven", NULL, 0, 0);
        filler(buf, "youth", NULL, 0, 0);
        filler(buf, "7sref", NULL, 0, 0);
        return 0;
    }

    if (strcmp(path, "/7sref") == 0) {
        filler(buf, ".", NULL, 0, 0);
        filler(buf, "..", NULL, 0, 0);
        return 0;
    }

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char display_name[MAX_PATH_LEN];
        strcpy(display_name, de->d_name);

        if (strstr(path, "/starter") != NULL) {
            char* ext = strrchr(display_name, '.');
            if (ext && strcmp(ext, ".mai") == 0) {
                *ext = '\0';
            }
        }

        if (filler(buf, display_name, &st, 0, 0)) break;
    }

    char* real_path = get_real_path(path);
    if (!real_path) return -ENOENT;

    dp = opendir(real_path);
    if (dp == NULL) {
        free(real_path);
        return -errno;
    }

    if (strstr(path, "/starter") != NULL) {
        while ((de = readdir(dp)) != NULL) {
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;

            char display_name[MAX_PATH_LEN];
            strcpy(display_name, de->d_name);
            char* ext = strrchr(display_name, '.');
            if (ext && strcmp(ext, ".mai") == 0) {
                *ext = '\0';
            }

            if (filler(buf, display_name, &st, 0, 0)) break;
        }
    }
    else {
        while ((de = readdir(dp)) != NULL) {
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;

            if (filler(buf, de->d_name, &st, 0, 0)) break;
        }
    }

    closedir(dp);
    free(real_path);
    return 0;
}

static int maimai_open(const char* path, struct fuse_file_info* fi) {
    int res;

    char* real_path = get_real_path(path);
    if (!real_path) return -ENOENT;

    res = open(real_path, fi->flags);
    if (res == -1) {
        free(real_path);
        return -errno;
    }

    close(res);
    free(real_path);
    return 0;
}

static int maimai_read(const char* path, char* buf, size_t size, off_t offset,
    struct fuse_file_info* fi) {
    char temp_buf[BLOCK_SIZE];
    int fd;
    int res;

    char* real_path = get_real_path(path);
    if (!real_path) return -ENOENT;

    fd = open(real_path, O_RDONLY);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }

    res = pread(fd, temp_buf, size, offset);
    if (res == -1) {
        close(fd);
        free(real_path);
        return -errno;
    }

    if (strstr(path, "/metro/") != NULL) {
        shift_bytes(temp_buf, res, -1);
        memcpy(buf, temp_buf, res);
    }
    else if (strstr(path, "/dragon/") != NULL) {
        rot13_transform(temp_buf, res);
        memcpy(buf, temp_buf, res);
    }
    else if (strstr(path, "/heaven/") != NULL) {
        char iv_path[MAX_PATH_LEN];
        int ret = snprintf(iv_path, MAX_PATH_LEN, "%s%s", real_path, IV_FILE_EXT);
        if (ret < 0 || ret >= MAX_PATH_LEN) {
            fprintf(stderr, "Path too long\n");
            close(fd);
            free(real_path);
            return -ENAMETOOLONG;
        }

        unsigned char iv[AES_BLOCK_SIZE];
        FILE* iv_file = fopen(iv_path, "rb");
        if (!iv_file) {
            close(fd);
            free(real_path);
            return -EIO;
        }
        fread(iv, 1, AES_BLOCK_SIZE, iv_file);
        fclose(iv_file);

        char* decrypted = malloc(res);
        if (!decrypted) {
            close(fd);
            free(real_path);
            return -ENOMEM;
        }
        int dec_len = aes_decrypt(temp_buf, res, decrypted, iv);
        memcpy(buf, decrypted, dec_len);
        free(decrypted);
        res = dec_len;
    }
    else if (strstr(path, "/youth/") != NULL) {
        size_t decompressed_size = size * 4;
        char* decompressed = malloc(decompressed_size);
        if (!decompressed) {
            close(fd);
            free(real_path);
            return -ENOMEM;
        }

        char* compressed_data = malloc(res);
        if (!compressed_data) {
            free(decompressed);
            close(fd);
            free(real_path);
            return -ENOMEM;
        }

        res = pread(fd, compressed_data, res, offset);
        if (res == -1) {
            free(decompressed);
            free(compressed_data);
            close(fd);
            free(real_path);
            return -errno;
        }

        size_t actual_decompressed = decompressed_size;
        int decompress_res = gzip_decompress(compressed_data, res, decompressed, &actual_decompressed);
        free(compressed_data);

        if (decompress_res != Z_OK) {
            free(decompressed);
            close(fd);
            free(real_path);
            return -EIO;
        }

        if (actual_decompressed > size) {
            free(decompressed);
            close(fd);
            free(real_path);
            return -ENOMEM;
        }

        memcpy(buf, decompressed, actual_decompressed);
        free(decompressed);
        res = actual_decompressed;
    }
    else {
        memcpy(buf, temp_buf, res);
    }

    close(fd);
    free(real_path);
    return res;
}

static int maimai_write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    struct maimai_context* mc = fuse_get_context()->private_data;
    int fd;

    (void)fi;
    (void)offset;

    char* real_path = get_real_path(path);
    if (!real_path) return -ENOENT;

    if (mc->debug) fprintf(stderr, "write: %s -> %s\n", path, real_path);

    fd = open(real_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }

    char temp_buf[BLOCK_SIZE];
    memset(temp_buf, 0, sizeof(temp_buf));

    fprintf(stderr, "maimai_write: offset = %lld, size = %zu, path = %s\n", (long long)offset, size, path);

    if (strstr(path, "/metro/") != NULL) {
        fprintf(stderr, "maimai_write (metro): size = %zu\n", size);
        memcpy(temp_buf, buf, size);
        shift_bytes(temp_buf, size, 1);
        if (write(fd, temp_buf, size) != size) {
            close(fd);
            free(real_path);
            fprintf(stderr, "maimai_write: write error (metro)\n");
            return -EIO;
        }
    }
    else if (strstr(path, "/dragon/") != NULL) {
        fprintf(stderr, "maimai_write (dragon): size = %zu\n", size);
        memcpy(temp_buf, buf, size);
        rot13_transform(temp_buf, size);
        if (write(fd, temp_buf, size) != size) {
            close(fd);
            free(real_path);
            fprintf(stderr, "maimai_write: write error (dragon)\n");
            return -EIO;
        }
    }
    else if (strstr(path, "/heaven/") != NULL) {
        fprintf(stderr, "maimai_write (heaven): size = %zu\n", size);
        unsigned char iv[AES_BLOCK_SIZE];
        RAND_bytes(iv, AES_BLOCK_SIZE);

        char iv_path[MAX_PATH_LEN];
        int ret = snprintf(iv_path, MAX_PATH_LEN, "%s%s", real_path, IV_FILE_EXT);
        if (ret < 0 || ret >= MAX_PATH_LEN) {
            close(fd);
            free(real_path);
            fprintf(stderr, "Path too long\n");
            return -ENAMETOOLONG;
        }

        FILE* iv_file = fopen(iv_path, "wb");
        if (!iv_file) {
            close(fd);
            free(real_path);
            return -EIO;
        }
        if (fwrite(iv, 1, AES_BLOCK_SIZE, iv_file) != AES_BLOCK_SIZE) {
            fclose(iv_file);
            close(fd);
            free(real_path);
            return -EIO;
        }
        fclose(iv_file);

        char* encrypted = malloc(size + AES_BLOCK_SIZE);
        if (!encrypted) {
            close(fd);
            free(real_path);
            return -ENOMEM;
        }
        int enc_len = aes_encrypt(buf, size, encrypted, iv);
        if (enc_len < 0) {
            free(encrypted);
            close(fd);
            free(real_path);
            fprintf(stderr, "AES Encryption Error\n");
            return -EIO;
        }
        if (write(fd, encrypted, enc_len) != enc_len) {
            free(encrypted);
            close(fd);
            free(real_path);
            return -EIO;
        }
        free(encrypted);
    }
    else if (strstr(path, "/youth/") != NULL) {
        size_t compressed_len;
        unsigned long bound = compressBound(size);
        fprintf(stderr, "maimai_write: size = %zu, compressBound(size) = %lu\n", size, bound);

        char* compressed = malloc(bound);
        if (!compressed) {
            close(fd);
            free(real_path);
            fprintf(stderr, "maimai_write: malloc error (compressed)\n");
            return -ENOMEM;
        }

        compressed_len = bound;

        if (gzip_compress(buf, size, compressed, &compressed_len) != Z_OK) {
            free(compressed);
            close(fd);
            free(real_path);
            fprintf(stderr, "maimai_write: gzip_compress error\n");
            return -EIO;
        }

        fprintf(stderr, "maimai_write: compressed_len = %zu\n", compressed_len);

        ssize_t bytes_written = write(fd, compressed, compressed_len);
        fprintf(stderr, "maimai_write: bytes_written = %zd\n", bytes_written);
        if (bytes_written != compressed_len) {
            free(compressed);
            close(fd);
            free(real_path);
            fprintf(stderr, "maimai_write: write error (compressed data)\n");
            return -EIO;
        }
        free(compressed);
    }
    else {
        fprintf(stderr, "maimai_write (other): size = %zu\n", size);
        if (write(fd, buf, size) != size) {
            close(fd);
            free(real_path);
            fprintf(stderr, "maimai_write: write error (other)\n");
            return -EIO;
        }
    }

    close(fd);
    free(real_path);
    return size;
}

static int maimai_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
    int res;

    char* real_path = get_real_path(path);
    if (!real_path) return -ENOENT;

    res = open(real_path, fi->flags, mode);
    if (res == -1) {
        free(real_path);
        return -errno;
    }

    close(res);
    free(real_path);
    return 0;
}

static int maimai_unlink(const char* path) {
    int res;

    char* real_path = get_real_path(path);
    if (!real_path) return -ENOENT;

    res = unlink(real_path);
    if (res == -1) {
        free(real_path);
        return -errno;
    }

    if (strstr(path, "/heaven/") != NULL) {
        char iv_path[MAX_PATH_LEN];
        int ret = snprintf(iv_path, MAX_PATH_LEN, "%s%s", real_path, IV_FILE_EXT);
        if (ret < 0 || ret >= MAX_PATH_LEN) {
            free(real_path);
            fprintf(stderr, "Path too long\n");
            return -ENAMETOOLONG;
        }
        unlink(iv_path);
    }

    free(real_path);
    return 0;
}

void shift_bytes(char* data, size_t size, int direction) {
    for (size_t i = 0; i < size; i++) {
        int shift = (i % 256);
        if (direction > 0) {
            data[i] = (data[i] + shift) % 256;
        }
        else {
            data[i] = (data[i] - shift + 256) % 256;
        }
    }
}

void rot13_transform(char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] = 'a' + ((data[i] - 'a' + 13) % 26);
        }
        else if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] = 'A' + ((data[i] - 'A' + 13) % 26);
        }
    }
}

int aes_encrypt(const char* input, size_t input_len, char* output, const unsigned char* iv) {
    EVP_CIPHER_CTX* ctx;
    int len;
    int ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return -1;
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
        (const unsigned char*)AES_KEY, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (1 != EVP_EncryptUpdate(ctx, (unsigned char*)output, &len,
        (const unsigned char*)input, input_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, (unsigned char*)output + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

int aes_decrypt(const char* input, size_t input_len, char* output, const unsigned char* iv) {
    EVP_CIPHER_CTX* ctx;
    int len;
    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return -1;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
        (const unsigned char*)AES_KEY, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (1 != EVP_DecryptUpdate(ctx, (unsigned char*)output, &len,
        (const unsigned char*)input, input_len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, (unsigned char*)output + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

int gzip_compress(const char* src, size_t srcLen, char* dest, size_t* destLen) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return -1;

    strm.next_in = (Bytef*)src;
    strm.avail_in = srcLen;
    strm.next_out = (Bytef*)dest;
    strm.avail_out = *destLen;

    int ret = deflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_ERROR) {
        deflateEnd(&strm);
        fprintf(stderr, "gzip_compress: zlib stream error\n");
        return -1;
    }
    if (ret == Z_BUF_ERROR) {
        deflateEnd(&strm);
        fprintf(stderr, "gzip_compress: output buffer too small\n");
        return -1;
    }
    if (ret != Z_OK && ret != Z_STREAM_END) {
        deflateEnd(&strm);
        fprintf(stderr, "gzip_compress: deflate error %d\n", ret);
        return -1;
    }

    *destLen = strm.total_out;

    deflateEnd(&strm);
    return Z_OK;
}

int gzip_decompress(const char* src, size_t srcLen, char* dest, size_t* destLen) {
    z_stream strm;
    int ret;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef*)src;
    strm.avail_in = srcLen;
    strm.next_out = (Bytef*)dest;
    strm.avail_out = *destLen;

    if (inflateInit2(&strm, 15 + 32) != Z_OK) {
        fprintf(stderr, "gzip_decompress: inflateInit2 failed\n");
        return -1;
    }

    ret = inflate(&strm, Z_FINISH);

    if (ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
        inflateEnd(&strm);
        fprintf(stderr, "gzip_decompress: inflate error %d\n", ret);
        return ret;
    }

    *destLen = strm.total_out;
    inflateEnd(&strm);

    return Z_OK;
}

// main

static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .open = maimai_open,
    .read = maimai_read,
    .write = maimai_write,
    .create = maimai_create,
    .unlink = maimai_unlink,
};

int main(int argc, char* argv[]) {
    struct maimai_context* mc = malloc(sizeof(struct maimai_context));
    mc->base_path = realpath("chiho", NULL);
    mc->mount_path = realpath("fuse_dir", NULL);
    mc->debug = 1;

    mkdir("chiho", 0755);
    mkdir("chiho/starter", 0755);
    mkdir("chiho/metro", 0755);
    mkdir("chiho/dragon", 0755);
    mkdir("chiho/blackrose", 0755);
    mkdir("chiho/heaven", 0755);
    mkdir("chiho/youth", 0755);

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    if (!RAND_poll()) {
        fprintf(stderr, "RAND_poll failed\n");
        return 1;
    }

    return fuse_main(argc, argv, &maimai_oper, mc);
}
