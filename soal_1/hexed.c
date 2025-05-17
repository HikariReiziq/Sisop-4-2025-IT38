
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define ZIP_FILE "anomali.zip"
#define DOWNLOAD_CMD "wget -q --show-progress -O " ZIP_FILE " \"https://drive.google.com/uc?export=download&id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5\""
#define UNZIP_CMD "unzip -j -o " ZIP_FILE " -d anomali/" // -j untuk menghilangkan struktur folder ZIP
#define REMOVE_ZIP_CMD "rm -f " ZIP_FILE

#define SOURCE_DIR "anomali"
#define IMAGE_DIR "anomali/image"
#define LOG_FILE "anomali/conversion.log"
#define HEX_BUFFER_SIZE 1000000

void get_current_timestamp(char *date_str, char *time_str) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_str, 11, "%Y-%m-%d", t);
    strftime(time_str, 9, "%H:%M:%S", t);
}

void log_conversion(const char *filename, const char *image_filename, int success) {
    char date_str[11], time_str[9];
    get_current_timestamp(date_str, time_str);

    mkdir("anomali", 0700);
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        if(success) {
            fprintf(log, "[%s][%s]: Successfully converted %s to %s\n",
                    date_str, time_str, filename, image_filename);
        } else {
            fprintf(log, "[%s][%s]: FAILED to convert %s\n",
                    date_str, time_str, filename);
        }
        fclose(log);
    }
}

void hex_to_bin(const char *hex_str, unsigned char *bin_data, size_t *bin_size) {
    size_t len = strlen(hex_str);
    *bin_size = 0;
    
    // Handle panjang ganjil dengan menambahkan '0' di akhir
    if(len % 2 != 0) {
        len++;
        char *fixed_hex = malloc(len + 1);
        strcpy(fixed_hex, hex_str);
        fixed_hex[len - 1] = '0';
        fixed_hex[len] = '\0';
        hex_str = fixed_hex;
    }

    *bin_size = len / 2;
    for(size_t i = 0; i < *bin_size; i++) {
        if(sscanf(&hex_str[i*2], "%2hhx", &bin_data[i]) != 1) {
            *bin_size = 0;
            break;
        }
    }
}

void create_directories() {
    mkdir("anomali", 0700);
    mkdir(IMAGE_DIR, 0700);
}

void process_file(const char *filepath) {
    char *filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : (char *)filepath;

    FILE *f = fopen(filepath, "r");
    if(!f) {
        log_conversion(filename, "", 0);
        return;
    }

    // Baca seluruh file
    char hex_str[HEX_BUFFER_SIZE] = {0};
    size_t read_size = fread(hex_str, 1, sizeof(hex_str) - 1, f);
    fclose(f);

    // Filter hanya karakter hex valid
    size_t hex_len = 0;
    for(size_t i = 0; i < read_size; i++) {
        if(isxdigit(hex_str[i])) {
            hex_str[hex_len++] = tolower(hex_str[i]);
        }
    }
    hex_str[hex_len] = '\0';

    // Konversi
    unsigned char bin_data[HEX_BUFFER_SIZE];
    size_t bin_size;
    hex_to_bin(hex_str, bin_data, &bin_size);

    // Generate nama output
    char output_name[256];
    strncpy(output_name, filename, strlen(filename) - 4);
    output_name[strlen(filename) - 4] = '\0';

    char date_str[11], time_str[9];
    get_current_timestamp(date_str, time_str);

    char image_filename[512];
    snprintf(image_filename, sizeof(image_filename), "%s/%s_image_%s_%s.png",
             IMAGE_DIR, output_name, date_str, time_str);

    // Tulis file gambar meski data tidak valid
    FILE *img = fopen(image_filename, "wb");
    if(img) {
        int write_ok = 1;
        if(bin_size > 0) {
            if(fwrite(bin_data, 1, bin_size, img) != bin_size) {
                write_ok = 0;
            }
        } else {
            // Tulis data kosong jika konversi gagal
            unsigned char dummy = 0xFF;
            fwrite(&dummy, 1, 1, img);
            write_ok = 0;
        }
        fclose(img);
        log_conversion(filename, image_filename, write_ok);
    } else {
        log_conversion(filename, "", 0);
    }
}

int main() {
    system("rm -rf anomali"); // Hapus folder lama kalo ada
    
    printf("Downloading...\n");
    if(system(DOWNLOAD_CMD)) return 1;
    
    printf("Unzipping...\n");
    if(system(UNZIP_CMD)) return 1;
    system(REMOVE_ZIP_CMD);
    
    create_directories();

    // Proses semua file .txt
    DIR *dir = opendir(SOURCE_DIR);
    if(!dir) return 1;
    
    struct dirent *entry;
    int count = 0;
    while((entry = readdir(dir)) != NULL) {
        if(strstr(entry->d_name, ".txt")) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", SOURCE_DIR, entry->d_name);
            printf("Processing: %s\n", entry->d_name); // Debug
            process_file(filepath);
            count++;
        }
    }
    closedir(dir);
    
    printf("Total files processed: %d\n", count);
    system("tree anomali");
    return 0;
}
