#!/bin/bash

BASE_DIR="$(dirname `realpath ${0}`)"

LIBOPENCM3_REPO="https://github.com/libopencm3/libopencm3.git"
LIBOPENCM3_PATH="$BASE_DIR/lib/libopencm3"

LIBLZG_REPO="https://github.com/mbitsnbites/liblzg.git"
LIBLZG_PATH="$BASE_DIR/lib/liblzg"

LIBSODIUM_REPO="https://github.com/jedisct1/libsodium.git"
LIBSODIUM_PATH="$BASE_DIR/lib/libsodium"

REQUIRED_COMMANDS="arm-none-eabi-gcc python"


check_error() {
    if [ "$1" -ne "0" ]; then
        cd "$BASE_DIR"
        echo
        echo "Script failed: $2"
        exit 1
    fi
}

install_libopencm() {
    [ -f "${LIBOPENCM3_PATH}/.git/HEAD" ] || git clone "$LIBOPENCM3_REPO" "$LIBOPENCM3_PATH"
    check_error $? "Error cloning libopencm3"

    cd "$LIBOPENCM3_PATH"
    git checkout 5ea4e8842a786dfc6314d7ac85ddbef4abbcde35
    FP_FLAGS="-mfloat-abi=soft" CFLAGS='-mno-unaligned-access -g -Wall -Werror -Os -mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing -fomit-frame-pointer -mthumb -mcpu=cortex-m3 -fstack-protector --param=ssp-buffer-size=4' make
    check_error $? "Error building libopencmp3"
}

install_liblzg() {
    [ -f "${LIBLZG_PATH}/.git/HEAD" ] || git clone "$LIBLZG_REPO" "$LIBLZG_PATH"
    check_error $? "Error cloning liblzg"

    cd "$LIBLZG_PATH/src"
    make
    check_error $? "Error building liblzg"
}

install_libsodium() {
    [ -f "${LIBSODIUM_PATH}/.git/HEAD" ] || git clone "$LIBSODIUM_REPO" "$LIBSODIUM_PATH"
    check_error $? "Error cloning libsodium"

    cd "$LIBSODIUM_PATH"
    ./autogen.sh
    check_error $? "Error initializing libsodium"

    CFLAGS='-DNDEBUG -mthumb -mcpu=cortex-m3 -fno-strict-overflow -mno-unaligned-access -fvisibility=hidden -Os -fomit-frame-pointer -mfix-cortex-m3-ldrd -msoft-float -fdata-sections -ffunction-sections -nostdlib -nostartfiles -ffreestanding' ./configure --host=arm-none-eabi && make all
    check_error $? "Error building libsodium"
}

for COMMAND in $REQUIRED_COMMANDS; do
	command -v "$COMMAND" >/dev/null 2>&1
	check_error $? "Missing dependency: $COMMAND"
done

python -c "import pysodium"
check_error $? "No pysodium found, run 'pip install pysodium'"

install_libopencm && echo "libopencm3: success"
install_liblzg && echo "liblzg: success"
install_libsodium && echo "libsodium: success"
