build-x86:
	@make clean
	@git clone https://github.com/BLAKE3-team/BLAKE3.git
	@cd BLAKE3/c/; pwd; gcc -O3 -fPIC -shared -o libblake3.so blake3.c blake3_dispatch.c blake3_portable.c \
    @blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S \
    @blake3_avx512_x86-64_unix.S
	@cp BLAKE3/c/libblake3.so .
	@gcc sht.c -o sht -I./BLAKE3/c -L./BLAKE3/c -lblake3

build-arm:
	@make clean
	@git clone https://github.com/BLAKE3-team/BLAKE3.git
	@cd BLAKE3/c/; pwd; gcc -shared -O3 -fPIC -o libblake3.so -DBLAKE3_USE_NEON=1 blake3.c blake3_dispatch.c blake3_portable.c blake3_neon.c
	@cp BLAKE3/c/libblake3.so .
	@gcc sht.c -o sht -I./BLAKE3/c -L./BLAKE3/c -lblake3
	echo "Sht built for ARM sucessfully"

clean:
	rm -rvf BLAKE3 libblake3.so sht