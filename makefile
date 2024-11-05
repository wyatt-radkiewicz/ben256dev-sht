PREFIX ?=/usr/local
ARCH :=$(shell uname -m)
OS :=$(shell uname -o)
ifeq ($(OS),Darwin)
	BLAKE3LIB :=libblake3.dylib
else
	BLAKE3LIB :=libblake3.so
endif

default: build-sht

# Detect architecture and build BLAKE3 if needed
build: detect-arch build-sht
	@echo "Build complete (with BLAKE3 dependency)"

# Detect architecture and build the appropriate BLAKE3 library
detect-arch:
ifeq ($(ARCH), x86_64)
	$(info building for x86-64)
	@$(MAKE) build-b3-x86
else ifeq ($(ARCH), aarch64)
	$(info building for aarch64)
	@$(MAKE) build-b3-arm
else ifeq ($(ARCH), arm64)
	$(info building for arm64)
	@$(MAKE) build-b3-arm
else
	$(info building for unknown)
	@$(MAKE) build-b3-portable
endif

# Build BLAKE3 library with portable blake3
build-b3-portable:
	@if [ ! -f $(PREFIX)/lib/$(BLAKE3LIB) ]; then \
		echo "BLAKE3 not found; building BLAKE3 with portable implementation"; \
		make b3-portable; \
	else \
		echo "Using existing BLAKE3 library"; \
	fi


b3-portable: clean-blake3
	@git clone https://github.com/BLAKE3-team/BLAKE3.git
	@cd BLAKE3/c && pwd && gcc -O3 -fPIC -shared -o $(BLAKE3LIB) \
	 blake3.c blake3_dispatch.c blake3_portable.c
	@sudo cp BLAKE3/c/$(BLAKE3LIB) $(PREFIX)/lib/
	@sudo cp BLAKE3/c/blake3.h $(PREFIX)/include/
	@sudo cp BLAKE3/c/blake3_impl.h $(PREFIX)/include/
	@if [[ $(OS) -eq 'Darwin' ]]; then \
		sudo update_dyld_shared_cache; \
	else \
		sudo ldconfig; \
	fi
	@rm -rvf BLAKE3
	@echo "BLAKE3 library built with portable implementation"

# Build BLAKE3 library for x86
build-b3-x86:
	@if [ ! -f $(PREFIX)/lib/$(BLAKE3LIB) ]; then \
		echo "BLAKE3 not found; building BLAKE3 for x86"; \
		make blake3-x86; \
	else \
		echo "Using existing BLAKE3 library"; \
	fi

blake3-x86: clean-blake3
	@git clone https://github.com/BLAKE3-team/BLAKE3.git
	@cd BLAKE3/c && pwd && gcc -O3 -fPIC -shared -o $(BLAKE3LIB) \
		blake3.c blake3_dispatch.c blake3_portable.c \
		blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S \
		blake3_avx2_x86-64_unix.S blake3_avx512_x86-64_unix.S
	@sudo cp BLAKE3/c/$(BLAKE3LIB) $(PREFIX)/lib/
	@sudo cp BLAKE3/c/blake3.h $(PREFIX)/include/
	@sudo cp BLAKE3/c/blake3_impl.h $(PREFIX)/include/
	@sudo ldconfig
	@rm -rvf BLAKE3
	@echo "BLAKE3 library built for x86 successfully"

# Build BLAKE3 library for ARM
build-b3-arm:
	@if [ ! -f $(PREFIX)/lib/$(BLAKE3LIB) ]; then \
		echo "BLAKE3 not found; building BLAKE3 for ARM"; \
		make blake3-arm; \
	else \
		echo "Using existing BLAKE3 library"; \
	fi

blake3-arm: clean-blake3
	@git clone https://github.com/BLAKE3-team/BLAKE3.git
	@cd BLAKE3/c && pwd && gcc -shared -O3 -fPIC -o $(BLAKE3LIB) -DBLAKE3_USE_NEON=1 \
		blake3.c blake3_dispatch.c blake3_portable.c blake3_neon.c
	@sudo cp BLAKE3/c/$(BLAKE3LIB) $(PREFIX)/lib/
	@sudo cp BLAKE3/c/blake3.h $(PREFIX)/include/
	@sudo cp BLAKE3/c/blake3_impl.h $(PREFIX)/include/
	@if [[ $(OS) -eq 'Darwin' ]]; then \
		sudo update_dyld_shared_cache; \
	else \
		sudo ldconfig; \
	fi
	@rm -rvf BLAKE3
	@echo "BLAKE3 library built for ARM successfully"

# Compile `sht` executable, assuming BLAKE3 is installed
build-sht: 
	@gcc sht.c -O3 -o sht -lblake3 -L$(PREFIX)/lib -I$(PREFIX)/include
	@if [[ $(OS) -eq 'Darwin' ]]; then \
		install_name_tool -add_rpath $(PREFIX)/lib sht; \
		install_name_tool -change $(BLAKE3LIB) @rpath/$(BLAKE3LIB) sht; \
	fi
	@sudo cp sht $(PREFIX)/bin/

# Debug version of 'make'
debug: 
	@gcc sht.c -g -O0 -o sht -lblake3 -L$(PREFIX)/lib -I$(PREFIX)/include
	@sudo cp sht $(PREFIX)/bin/

# Install autocompletion
autocomp:
	@echo "Attempting to install sht autocompletion..."
	@if [ -d "/etc/bash_completion.d" ]; then \
		sudo cp sht_complete.sh /etc/bash_completion.d/sht; \
	elif [ -d "$$HOME/.local/share/bash-completion/completions" ]; then \
		mkdir -p $$HOME/.local/share/bash-completion/completions && \
		cp sht_complete.sh $$HOME/.local/share/bash-completion/completions/sht; \
	fi
	@echo "You may have to \"source ~/.bashrc\" to make autocompletions accessible"

# Build manpages
PANDOC ?= pandoc
MANPAGE.md = $(PANDOC) --standalone $(PANDOCFLAGS) --to man

man/%.1: man/%.1.md
	$(MANPAGE.md) $< -o $@

man/%.3: man/%.3.md
	$(MANPAGE.md) $< -o $@

.IGNORE: manpages
manpages: man/sht.1 man/sht_parse_error.3
	@sh install_manpages.sh

.PHONY: manpages

# Full install with dependencies and autocompletion
install: build autocomp manpages
	@echo "Installation complete"

clean: clean-blake3
	rm -rvf sht
	rm -f man/*.1
	rm -f man/*.3
	@echo "Cleaned build artifacts"

clean-blake3:
	rm -rvf BLAKE3 $(BLAKE3LIB)
