#! /bin/bash

INIT=$1

OPENVR_VERSION_TAG="v2.12.14"
SPDLOG_VERSION_TAG="v1.17.0"

OPENVR_DIR="OpenVR"
SPDLOG_DIR="spdlog"
DIRS=("$OPENVR_DIR" "$SPDLOG_DIR")

THIRD_LIB_DIR="3rdparty"

function update_spdlog() {
    git clone --depth=1 --branch "$SPDLOG_VERSION_TAG" https://github.com/gabime/spdlog.git "$SPDLOG_DIR"
    rm -rf .git
}

function update_openvr() {
    git clone --no-checkout --depth=1 --filter=tree:0 https://github.com/ValveSoftware/openvr.git "$OPENVR_DIR"
    cd "$OPENVR_DIR" || exit
    git sparse-checkout set --no-cone /bin /headers /lib /LICENSE
    git checkout "$OPENVR_VERSION_TAG"
    rm -rf .git
}

function rm_rf_dir() {
    if [ -e "$1" ];then
      rm -rf "$1"
    fi
}

clone_if_not_exist() {
    local dir="$1"
    local func_name="$2"
    if [ -e "$dir" ]; then
      echo "[$dir] exists, skip"
    else
        "$func_name"
        cd "$WD" || exit
    fi
}

# -- main --

cd "$THIRD_LIB_DIR" || exit

if [ -n "$INIT" ]; then
    for dir in "${DIRS[@]}"; do
        rm_rf_dir "$dir"
    done
fi

WD=$(pwd)
clone_if_not_exist "$SPDLOG_DIR" update_spdlog
clone_if_not_exist "$OPENVR_DIR" update_openvr
