FROM ubuntu:20.04 

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    fuse \
    gcc \
    pkg-config \
    libfuse-dev \        
    nano \
    tree \
    wget

WORKDIR /app

COPY antink.c .

RUN gcc antink.c $(pkg-config fuse --cflags --libs) -o antink_fs

RUN mkdir -p it24_host antink_mount antink-logs && \
    echo "ini file nafis" > it24_host/nafis.txt && \
    echo "ini file kimcun" > it24_host/test_kimcun.txt && \
    echo "ini file normal" > it24_host/safe_file.txt && \
    wget "https://drive.usercontent.google.com/u/0/uc?id=1_lEz_pV3h4uippLfOLeqO1bQ5bM8a1dy&export=download" -O it24_host/nafis.jpg && \
    wget "https://drive.usercontent.google.com/u/0/uc?id=18R58eAfJ-1xkgE57WjzFgc16w2UdFlBU&export=download" -O it24_host/kimcun.jpg
