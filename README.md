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

---

## ✅ SOAL 1 – **Hexed Anomaly Visualizer** (`hexed.c`)

### 🧠 Deskripsi Soal

Shorekeeper menemukan teks anomali dalam format **hexadecimal**. Kamu diminta membuat program untuk:

* Mengonversi string hex dari file `.txt` menjadi file **image `.png`**.
* Menyimpan hasil ke folder `image/`.
* Mencatat semua konversi ke dalam `conversion.log`.

### 📁 Struktur Output

```
anomali/
├── 1.txt
├── 2.txt
├── ...
├── image/
│   ├── 1_image_2025-05-11_18:35:26.png
│   └── ...
└── conversion.log
```

---

## 🔧 Penjelasan Kode `hexed.c`

```c
#define DOWNLOAD_CMD ...
#define UNZIP_CMD ...
```

👉 Mengunduh file ZIP dan mengekstraknya ke folder `anomali`.

```c
void get_current_timestamp(...) { ... }
```

📅 Mengambil timestamp saat ini. Digunakan untuk penamaan file dan log.

```c
void log_conversion(...) { ... }
```

📝 Menulis log sukses/gagal konversi ke `conversion.log`.

```c
void hex_to_bin(...) { ... }
```

🔁 Mengubah string hex jadi binary:

* Disaring hanya karakter valid (`isxdigit`)
* Kalau panjang ganjil, ditambahi `0`.

```c
void process_file(...) { ... }
```

✨ Proses utama:

1. Buka file `.txt`.
2. Baca semua isinya.
3. Konversi hex ke binary.
4. Simpan sebagai `.png`.
5. Tulis ke log.

```c
int main() { ... }
```

🏁 Langkah-langkah:

1. Download → Unzip → Hapus zip
2. Buat folder
3. Iterasi semua `.txt` di `anomali/`
4. Konversi semua → Simpan log

---

## 🚀 Cara Menjalankan

```bash
gcc hexed.c -o hexed
./hexed
```

---

## ✅ SOAL 2 – **Baymax Reconstructor** (`baymax.c`)

### 🧠 Deskripsi Soal

Ilmuwan menemukan **14 potongan file Baymax** berformat `Baymax.jpeg.000` hingga `.013`. Kamu diminta:

* Buat virtual filesystem FUSE yang:

  * Gabungkan fragment → tampilkan sebagai `Baymax.jpeg`.
  * Bisa membaca, menyalin, dan menghapus file virtual.
  * Bisa menyimpan file baru dalam fragment ukuran 1KB.
  * Log semua aktivitas ke `activity.log`.

### 📁 Struktur

```
relic/
├── Baymax.jpeg.000
├── ...
bebas/        ← mount dir (isi Baymax.jpeg utuh)
activity.log
```

---

## 🔧 Penjelasan Kode `baymax.c`

```c
#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
...
```

📦 Include FUSE3 dan library standar.

```c
#define FRAGMENT_SIZE 1024
#define RELIC_DIR "relic"
#define virtual_file "Baymax.jpeg"
```

📁 Konfigurasi ukuran potongan (1KB), nama folder sumber, dan nama file virtual.

---

### 🔸 `fs_getattr`, `fs_readdir`, `fs_open`

```c
fs_getattr(...) // Mendefinisikan atribut file & folder
fs_readdir(...) // Mengisi isi direktori virtual
fs_open(...)    // Log saat file dibuka
```

---

### 🔸 `fs_read`

```c
fs_read(...) // Membaca fragment secara sekuensial
```

🔍 Fungsi ini membaca semua fragment dari `.000` hingga `.013` dan menyusunnya sebagai satu file utuh.

---

### 🔸 `fs_create`, `fs_write`, `fs_release`

```c
fs_create(...)  // Simpan nama file baru
fs_write(...)   // Tulis ke file temp
fs_release(...) // Setelah selesai ditulis:
                // → potong jadi fragment 1KB
                // → simpan ke folder relic
                // → log file yang ditulis
```

---

### 🔸 Logging

```c
log_activity(...) // Nge-log semua aksi (READ, WRITE, DELETE, COPY)
```

---

### 🛠 Main Function

```c
int main(...) {
    // Download ZIP berisi fragment
    // Unzip → Jalankan FUSE dengan mount point “bebas”
}
```

---

## ⚠️ NOTE PENTING

> ❗ Hingga laporan ini ditulis, **fitur copy, remove, dan fragmentasi untuk file baru belum berhasil diselesaikan** sepenuhnya. Setelah filesystem di-mount, `Baymax.jpeg` memang terlihat, namun:
>
> * ❌ `cp Baymax.jpeg /tmp/` → Tidak berhasil
> * ❌ `rm Baymax.jpeg` → Tidak menghapus pecahan
> * ❌ Membuat file baru → Belum memotong dengan benar
>
> Praktikan mengalami **stuck** dan belum menemukan solusi final untuk masalah tersebut.

---

## 🚀 Cara Menjalankan

```bash
gcc `pkg-config fuse3 --cflags --libs` baymax.c -o baymax
mkdir bebas
./baymax bebas
```

Mount akan berada di folder `bebas/`. Untuk unmount:

```bash
fusermount3 -u bebas
```

---




## SOAL 3    
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




## SOAL 4
Soal 4 adalah untuk membuat sebuah sistem file virtual yang akan menggunakan FUSE. Sistem file ini mentransformasi data otomatis pada file yang disimpan di dalam direktori tertentu, seperti enkripsi, kompresi, dan shifting.
Ada 7 bagian dari sistem file ini, atau 7 area. Yaitu:
- Starter/Beginning Area - Starter Chiho
- World’s End Area - Metropolis Chiho
- World Tree Area - Dragon Chiho
- Black Rose Area - Black Rose Chiho
- Tenkai Area - Heaven Chiho
- Youth Area - Skystreet Chiho
- Prism Area - 7sRef Chiho

Untuk semua area ada aturan dan transformasi file yang khusus untuk setiap area/direktori tersebut.
Jadi untuk terakhirnya file di direktori chiho akan terbentuk seperti:

```

├── chiho/
│   ├── starter/
│   ├── metro/
│   ├── dragon/
│   ├── blackrose/
│   ├── heaven/
│   └── youth/
│
└── fuse_dir/
	├── starter/
	├── metro/
	├── dragon/
	├── blackrose/
	├── heaven/

```

### Starter area - /starter/

Untuk semua file yang disimpan akan otomatis diberikan ekstensi `.mai`. Jadi akan muncul seperti `file.mai` di filesystem fisik. Kecuali jika menggunakan FUSE yang akan menunjukkan file nya seperti biasa.
```c

// di dalam get_real_path() dan maimai_getattr()
if (strstr(path, "/starter/") != NULL) {
    strcat(real_path, ".mai");  // tambah ekstensi .mai
}


// dalam maimai_readdir()
if (strstr(path, "/starter") != NULL) {
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char display_name[MAX_PATH_LEN];
        strcpy(display_name, de->d_name);
        
        // bagian yang menghilangkan .mai
        char* ext = strrchr(display_name, '.');
        if (ext && strcmp(ext, ".mai") == 0) {
            *ext = '\0'; // Potong ekstensi .mai
        }

        if (filler(buf, display_name, &st, 0, 0)) break;
    }
}


```
Untuk cek test case nya, gunakan:
```

* buat file
echo "Ini contoh file" > fuse_dir/starter/contoh.txt

* Cek di backend
ls -la chiho/starter/  - Harusnya ada contoh.txt.mai
cat chiho/starter/contoh.txt.mai  - Isi harus sama

* Baca melalui FUSE
cat fuse_dir/starter/contoh.txt  - harusnya tampil tanpa .mai


```

### Metropolis Chiho - /metro/

Transformasi file disini adalah shift bytes. Setiap byte diubah berdasarkan posisinya.

Penulisan (+shift): `(+shift): byte[i] = (byte[i] + i) % 256`

Pembacaan (-shift): `(-shift): byte[i] = (byte[i] - i + 256) % 256`

Contoh:
Data: `[0x41, 0x42, 0x43]` (ASCII: "ABC")

Disimpan: `[0x41+0, 0x42+1, 0x43+2]` → `[0x41, 0x43, 0x45]` ("ACE")

Dibaca: Dikembalikan ke "ABC".

```c

// fungsi transformasi
void shift_bytes(char* data, size_t size, int direction) {
    for (size_t i = 0; i < size; i++) {
        int shift = (i % 256);
        if (direction > 0) {  // enkripsi (+shift)
            data[i] = (data[i] + shift) % 256;
        } else {  // dekripsi (-shift)
            data[i] = (data[i] - shift + 256) % 256;
        }
    }
}

// dalam maimai_read() dan maimai_write()
if (strstr(path, "/metro/") != NULL) {
    shift_bytes(temp_buf, res, -1);  // dekripsi saat read

    //atau

    shift_bytes(temp_buf, size, 1);  // enkripsi saat write
}

```

Untuk cek test case nya, gunakan:
```
* buat file
echo -n "abcd" > fuse_dir/metro/test_shift.txt

* cek di backend
xxd chiho/metro/test_shift.txt 
* Harusnya: 
 a -> a (tidak shift)
 b -> c (+1)
 c -> e (+2)
 d -> g (+3)

* Baca melalui FUSE
cat fuse_dir/metro/test_shift.txt  - harusnya kembali ke "abcd"
```
### Dragon Chiho - /dragon/

File yang disimpan di direktori ini dienkripsi menggunakan ROT13 di direktori asli.
Setiap huruf digeser 13 posisi dalam alfabet.
Contoh: "HELLO" → "URYYB", "test" → "grfg".

```c

// fungsi transformasi
void rot13_transform(char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (data[i] >= 'a' && data[i] <= 'z') {
            data[i] = 'a' + ((data[i] - 'a' + 13) % 26);
        } else if (data[i] >= 'A' && data[i] <= 'Z') {
            data[i] = 'A' + ((data[i] - 'A' + 13) % 26);
        }
    }
}

// dalam maimai_read() dan maimai_write()
if (strstr(path, "/dragon/") != NULL) {
    rot13_transform(temp_buf, res); 
}

```
Untuk cek test case nya, gunakan:
```

* buat file
echo "Hello World" > fuse_dir/dragon/rot13_test.txt

* cek di backend
cat chiho/dragon/rot13_test.txt  - harusnya terenkripsi ROT13 ("Uryyb Jbeyq")

* baca lewat FUSE
cat fuse_dir/dragon/rot13_test.txt  - harusnya "Hello World"

```


### Black Rose Chiho - /blackrose/

File yang disimpan di area/direktori ini akan disimpan datanya dalam format biner murni, tanpa enkripsi atau encoding tambahan.

Untuk di gunakan:
```
* buat file
echo "Ini data biner" > fuse_dir/blackrose/test.bin
xxd fuse_dir/blackrose/test.bin

* Harusnya muncul: Ini data biner

cp pic.jpg fuse_dir/blackrose/pic.jpg
xxd chiho/blackrose/pic.jpg
(butuh file pic.jpg)
```

### Heaven Chiho - /heaven/

Untuk file yang disimpan di area ini akan di lindungi enkripsi AES-256-CBC dengan setiap file memiliki IV unik yang disimpan di file terpisah (ekstensi `.iv)`.

AES key adalah : `SEGAMaimai2025ChihoSEGAMaimai2025Chiho`

Jadi contoh struktur file nya akan terbentuk seperti: `secret.txt` (untuk data yang terenkripsi) dan `secret.txt.iv` (file IV).

```c
// dalam maimai_write() - enkripsi
if (strstr(path, "/heaven/") != NULL) {
    unsigned char iv[AES_BLOCK_SIZE];
    RAND_bytes(iv, AES_BLOCK_SIZE);  // generate IV acak

    // simpan IV ke file terpisah
    snprintf(iv_path, MAX_PATH_LEN, "%s%s", real_path, IV_FILE_EXT);
    fwrite(iv, 1, AES_BLOCK_SIZE, iv_file);

    // enkripsi
    aes_encrypt(buf, size, encrypted, iv);
    write(fd, encrypted, enc_len);
}

// dalam maimai_read() - dekripsi
if (strstr(path, "/heaven/") != NULL) {
    snprintf(iv_path, MAX_PATH_LEN, "%s%s", real_path, IV_FILE_EXT);
    fread(iv, 1, AES_BLOCK_SIZE, iv_file);

    // dekripsi
    aes_decrypt(temp_buf, res, decrypted, iv);
}
```

Untuk cek test case nya, gunakan:
```
* buat file
echo "Data rahasia" > fuse_dir/heaven/secret.txt

* cek di backend
ls -la chiho/heaven/  - harusnya ada secret.txt dan secret.txt.iv
xxd chiho/heaven/secret.txt  - harusnya terenkripsi

* baca melalui FUSE
cat fuse_dir/heaven/secret.txt  - harusnya balik ke "Data rahasia"

```
### Skystreet Chiho - /youth/

FIle yang disimpan di area ini akan dikompresi menggunakan gzip

```c
// dalam maimai_write() - kompresi
if (strstr(path, "/youth/") != NULL) {
    size_t compressed_len = compressBound(size);
    gzip_compress(buf, size, compressed, &compressed_len);
    write(fd, compressed, compressed_len);
}

// dalam maimai_read() - dekompresi
if (strstr(path, "/youth/") != NULL) {
    gzip_decompress(temp_buf, res, decompressed, &actual_decompressed);
    memcpy(buf, decompressed, actual_decompressed);
}
```

Untuk cek test case nya, gunakan:
```
- buat file
echo "Ini file yang dikompresi" > fuse_dir/youth/compressed.txt

- cek di backend
file chiho/youth/compressed.txt  - harusnya terdeteksi sebagai gzip

- baca melalui FUSE
cat fuse_dir/youth/compressed.txt  - harusnya tampil normal
```

### 7sRef Chihi - /7sref/

Dijelaskan pada soal bahwa 7sRef ini merupakan “gateway” ke semua chiho lain. Sehingga, area ini adalah area spesial yang dapat mengakses semua area lain melalui sistem penamaan khusus.
 Konsepnya adalah direktori virtual yang memetakan nama file dengan format `area_namafile` ke struktur fisik `area/nama_file`.

`[area]_[nama_file]`

Contoh:
- `/fuse_dir/7sref/starter_guide.txt` ↔ `/fuse_dir/starter/guide.txt`
- `/fuse_dir/7sref/metro_progress.dat` ↔ `/fuse_dir/metro/progress.dat`
- `/fuse_dir/7sref/heaven_scores.dat` ↔ `/fuse_dir/heaven/scores.dat`

```c
// dalam get_real_path()
if (strstr(path, "/7sref/") != NULL) {
    char* filename = strrchr(path, '/') + 1;
    char* underscore = strchr(filename, '_');
    *underscore = '\0';
    char* area = filename;
    char* real_filename = underscore + 1;

    snprintf(real_path, MAX_PATH_LEN, "%s/%s/%s", 
             mc->base_path, area, real_filename);
    *underscore = '_';  // Kembalikan underscore
}
```

Untuk cek test case nya, gunakan:
```
* akses file starter melalui gateway
echo "Ini dari gateway" > fuse_dir/7sref/starter_gateway_test.txt

* harusnya muncul di:
cat fuse_dir/starter/gateway_test.txt
ls chiho/starter/  - harusnya ada gateway_test.txt.mai

```
