# Laporan Penjelasan dan Penyelesaian Soal Modul 4

بِسْمِ اللهِ الرَّحْمٰنِ الرَّحِيْمِ

Segala puji bagi Allah, Rabb semesta alam. Shalawat dan salam semoga tercurah kepada Nabi Muhammad ﷺ, keluarga, sahabat, serta siapa saja yang mengikuti jalan beliau hingga hari kiamat.

Laporan ini disusun sebagai bentuk tanggung jawab akademik dan ikhtiar dalam menuntut ilmu, sebagaimana sabda Nabi ﷺ:

"Barangsiapa menempuh jalan untuk menuntut ilmu, niscaya Allah akan mudahkan baginya jalan menuju surga."

(HR. Muslim)

Semoga apa yang tertulis dalam laporan ini menjadi sebab keberkahan, ilmu yang bermanfaat, dan amal yang diterima di sisi Allah ﷻ.

Semoga Allah memudahkan langkah kita semua dalam menuntut ilmu, mengamalkannya, serta menjaganya agar tidak sekadar jadi hafalan di otak, tapi bekal untuk akhirat.


## Anggota Kelompok
| Nama                      | NRP        |
|---------------------------|------------|
|Muhammad Hikari Reiziq R.  | 5027241079 |
|Dira Muhammad Ilyas S. A.  | 5027241033 |
|Thariq Kyran Aryunaldi     | 5027241073 |


Soal no 3 tentang sistem **AntiNK (Anti Nafis Kimcun)** yang:

- Dibuat menggunakan **Docker + FUSE** di container `antink-server`.
- Memiliki container `antink-logger` untuk **monitoring log real-time**.
- Menggunakan **`docker-compose`** untuk menyusun infrastruktur dengan volume:
    - `it24_host` (folder file asli)
    - `antink_mount` (mount point hasil FUSE)
    - `antink-logs` (folder untuk log)

### Tujuan Sistem:

1. **Deteksi file** yang mengandung nama “nafis” atau “kimcun”.
2. **Balik nama file berbahaya** saat ditampilkan.
    - `nafis.txt` → `txt.sifan`
3. File `.txt`:
    - Jika **berbahaya** → tampil normal.
    - Jika **tidak berbahaya** → dibaca dengan **ROT13**.
4. Semua aktivitas tercatat dalam **`/var/log/it24.log`** di container.
5. **Hanya container yang memanipulasi file**. Di host tidak terjadi perubahan.

**Langkah 1: Siapkan Struktur Direktori**

```bash
mkdir -p it24_host antink_mount antink-logs
```

Masukkan file uji:

```bash
echo "file aman" > it24_host/safe_file.txt
echo "file nafis" > it24_host/nafis.txt
echo "file kimcun" > it24_host/test_kimcun.txt
```

**Langkah 2: Build Docker pada terminal 1**

docker compose down 

```bash
docker compose up --build
```

**Langkah 3: Buka 2 Terminal**

**Terminal 2**

```bash
# Cek isi folder mount
docker exec -it soal_3-antink-server-1 ls /antink_mount
```

Hasil seharusnya:

```bash
txt.sifan    txt.nucmik    safe_file.txt

```

```bash

# Tes baca isi file aman (harus di-ROT13)
docker exec -it soal_3-antink-server-1 cat /antink_mount/safe_file.txt

```

Hasil:

```

svyr nzna
```

**Langkah 4: Cek Log**

```bash
docker exec -it soal_3-antink-server-1 cat /app/antink-logs/it24.log

```

Berikut penjelasan **per baris kode** dari file `antink.c`. Program ini menggunakan **FUSE (Filesystem in Userspace)** untuk membuat filesystem virtual yang bisa mendeteksi file "berbahaya" dan menerapkan **ROT13** pada isi file teks.

---

## Penjelasan `antink.c`

```c
#define FUSE_USE_VERSION 31

```

Menentukan versi API FUSE yang digunakan (versi 3.1).

```c
#include <fuse.h>       // Header utama FUSE
#include <stdio.h>      // Fungsi I/O standar
#include <string.h>     // Fungsi string
#include <errno.h>      // Menangani kode error
#include <fcntl.h>      // Konstanta file control
#include <dirent.h>     // Membaca direktori
#include <stdlib.h>     // Fungsi umum (malloc, exit)
#include <unistd.h>     // Fungsi POSIX
#include <sys/stat.h>   // Informasi atribut file
#include <ctype.h>      // Untuk isalpha(), islower()
#include <time.h>       // Untuk mencatat waktu

```

```c
char *ROOT_DIR;
#define LOG_FILE "/app/antink-logs/it24.log"

```

`ROOT_DIR` menyimpan direktori asli (bind mount dari host).

`LOG_FILE` adalah lokasi file log di container.

---

### Fungsi Deteksi dan Utilitas

```c
int is_dangerous(const char *name) {
    return strcasestr(name, "nafis") || strcasestr(name, "kimcun");
}

```

Mengecek apakah nama file mengandung "nafis" atau "kimcun" (case-insensitive).

```c
void reverse(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

```

Membalikkan string secara in-place. Digunakan untuk membalik nama file berbahaya.

```c
void write_log(const char *msg, const char *file) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t now = time(NULL);
        char *time_str = strtok(ctime(&now), "\n");  // Hilangkan newline
        fprintf(log, "[%s] %s : %s\n", time_str, msg, file);
        fclose(log);
    }
}

```

Menulis log ke file log dengan timestamp, pesan, dan nama file.

---

### Fungsi FUSE Utama

### 1. **getattr** – Untuk mengambil atribut file

```c
static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);
    return lstat(fullpath, stbuf);
}

```

Membangun path lengkap dari ROOT_DIR dan `path`, lalu memanggil `lstat` untuk mengambil informasi file.

### 2. **readdir** – Untuk membaca isi direktori

```c
static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {

```

Membaca file/direktori dari path tersebut.

Jika file berbahaya, log ditulis dan namanya dibalik.

```c
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);
    DIR *dp = opendir(fullpath);

```

Membuka direktori asli.

```c
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        ...
        if (is_dangerous(display_name)) {
            write_log("ALERT! Anomaly detected", display_name);
            reverse(display_name);
        }
        filler(buf, display_name, NULL, 0);
    }

```

Setiap entri dicek apakah berbahaya. Jika iya, log ditulis dan nama dibalik. Nama tersebut disimpan dengan `filler()` agar tampil ke user.

---

### ROT13 – Enkripsi File Normal

```c
void rot13(char *s) {
    while (*s) {
        if (isalpha(*s)) {
            char base = islower(*s) ? 'a' : 'A';
            *s = ((*s - base + 13) % 26) + base;
        }
        s++;
    }
}

```

Enkripsi ROT13: menggantikan huruf dengan huruf ke-13 setelahnya dalam alfabet.

---

### Buka File

```c
static int antink_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", ROOT_DIR, path);

    int res = open(fullpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

```

Membuka file untuk memastikan file bisa diakses.

---

### Baca File

```c
static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {

```

Membaca isi file. Jika file bukan berbahaya **dan** berformat `.txt`, isinya diubah dengan ROT13.

```c
    FILE *f = fopen(fullpath, "r");
    ...
    size_t len = fread(buf, 1, size, f);

```

Buka file dan baca isinya.

```c
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

```

Ambil nama file dari path.

```c
    if (!is_dangerous(filename) && strstr(filename, ".txt")) {
        buf[len] = '\0';
        rot13(buf);
    }

```

Jika aman dan `.txt`, maka ROT13 diterapkan.

---

### Struktur Operasi

```c
static const struct fuse_operations operations = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
};

```

Mendaftarkan fungsi-fungsi FUSE yang akan digunakan saat filesystem aktif.

---

### Fungsi `main`

```c
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mount_point>\n", argv[0]);
        exit(1);
    }

    umask(0);
    ROOT_DIR = realpath(argv[1], NULL);
    ...
    char *fuse_argv[] = { argv[0], "-f", argv[2] };
    return fuse_main(3, fuse_argv, &operations, NULL);
}

```

Menentukan `ROOT_DIR` dari argumen pertama.

Menjalankan FUSE dengan `mount_point` dari argumen kedua.

---

## Penjelasan `docker-compose.yml`

```yaml
services:
  antink-server:

```

Mendefinisikan **satu service container** bernama `antink-server`.

---

### Build Image dari Dockerfile

```yaml
    build:
      context: .

```

Perintah `build` digunakan untuk membangun image dari `Dockerfile` yang ada di direktori ini (`.`). Artinya Docker akan mengeksekusi instruksi yang kamu tulis di `Dockerfile`.

---

### Nama Container

```yaml
    container_name: soal_3-antink-server-1

```

Memberi nama khusus untuk container, bukan default dari Docker. Nama ini konsisten dengan sistem Compose (mungkin sesuai output Compose sebelumnya).

---

### Konfigurasi Keamanan untuk FUSE

```yaml
    cap_add:
      - SYS_ADMIN

```

Menambahkan kemampuan `SYS_ADMIN` ke container. Ini **wajib** untuk menjalankan FUSE, karena mount filesystem memerlukan hak istimewa.

```yaml
    devices:
      - /dev/fuse

```

Memberikan akses ke perangkat `/dev/fuse` dari host ke container. FUSE membutuhkan akses ini untuk membuat filesystem di userspace.

```yaml
    security_opt:
      - apparmor:unconfined

```

Menonaktifkan profil keamanan AppArmor agar container tidak dibatasi. Tanpa ini, sistem operasi host bisa memblokir FUSE.

> Catatan: 3 konfigurasi di atas (cap_add, devices, security_opt) wajib untuk menjalankan FUSE di dalam container. Ini membuat container cukup "bebas" seperti lingkungan sistem nyata.
> 

---

### Mounting Volume

```yaml
    volumes:
      - ./it24_host:/source #berisi file-file asli (yang bisa mengandung nafis/kimcun)
      - ./antink_mount:/antink_mount #direktori hasil mount, tempat user berinteraksi
      - ./antink-logs:/app/antink-logs #tempat log disimpan dan bisa dipantau

```

> Volume ini digunakan agar container bisa membaca/memproses file dari host tanpa merusak file asli.
> 

---

### Command untuk Menjalankan Program

```yaml
    command: ./antink_fs /source /antink_mount

```

Menjalankan program bernama `antink_fs` (kemungkinan hasil kompilasi dari `antink.c`) dengan dua argumen:

- `/source` = direktori sumber file
- `/antink_mount` = direktori hasil mount (mount point untuk FUSE)

---

## Penjelasan `Dockerfile` Baris per Baris

```
FROM ubuntu:20.04

```

Menggunakan base image **Ubuntu 20.04** — image Linux yang stabil dan umum digunakan untuk development C dan FUSE.

---

```
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    fuse \
    gcc \
    pkg-config \
    libfuse-dev \
    nano \
    tree \
    wget

```

- `apt-get update`: Memperbarui daftar paket.
- `DEBIAN_FRONTEND=noninteractive`: Mencegah prompt interaktif selama install (agar build Docker tidak macet).
- `apt-get install -y \ ...`: Menginstal dependencies:
    - `fuse`: Library utama untuk filesystem FUSE.
    - `gcc`: Compiler untuk C.
    - `pkg-config`: Alat bantu untuk kompilasi, agar tahu flag dari library seperti `fuse`.
    - `libfuse-dev`: Library header FUSE (untuk kompilasi).
    - `nano`, `tree`: Alat bantu debugging.
    - `wget`: Untuk mengunduh file uji.

---

```
WORKDIR /app

```

Set **working directory** di dalam container ke `/app`. Semua perintah berikut dijalankan dari folder ini.

---

```
COPY antink.c .

```

Menyalin file `antink.c` dari host ke dalam container di direktori `/app`.

---

```
RUN gcc antink.c $(pkg-config fuse --cflags --libs) -o antink_fs

```

Mengompilasi `antink.c` menjadi file eksekusi `antink_fs` menggunakan `gcc` dan flag dari FUSE.

- `$(pkg-config fuse --cflags --libs)` otomatis mengambil flag kompilasi/linking untuk library FUSE.

---

```
RUN mkdir -p it24_host antink_mount antink-logs && \
    echo "ini file nafis" > it24_host/nafis.txt && \
    echo "ini file kimcun" > it24_host/test_kimcun.txt && \
    echo "ini file normal" > it24_host/safe_file.txt && \

```

Membuat direktori uji dan file teks untuk keperluan pengujian:

- `nafis.txt` → file berbahaya (harus dibalik)
- `test_kimcun.txt` → file berbahaya (harus dibalik)
- `safe_file.txt` → file normal (akan dienkripsi ROT13)

---

```
    wget "https://drive.usercontent.google.com/u/0/uc?id=1_lEz_pV3h4uippLfOLeqO1bQ5bM8a1dy&export=download" -O it24_host/nafis.jpg && \
    wget "https://drive.usercontent.google.com/u/0/uc?id=18R58eAfJ-1xkgE57WjzFgc16w2UdFlBU&export=download" -O it24_host/kimcun.jpg

```

Mengunduh file gambar untuk pengujian deteksi kata kunci di nama file. Disimpan di folder `it24_host`.
