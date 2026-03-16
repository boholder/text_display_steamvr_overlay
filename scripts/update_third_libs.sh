#!/bin/bash

INIT=$1

OPENVR_VERSION_TAG="v2.12.14"
SPDLOG_VERSION_TAG="v1.17.0"
IMGUI_VERSION_TAG="v1.92.0"
SDL_VERSION_TAG="release-3.4.2"
GLM_VERSION_TAG="1.0.3"

OPENVR_DIR="OpenVR"
SPDLOG_DIR="spdlog"
IMGUI_DIR="ImGui"
SDL_DIR="SDL"
GLM_DIR="glm"
DIRS=("$OPENVR_DIR" "$SPDLOG_DIR" "$IMGUI_DIR" "$SDL_DIR" "$GLM_DIR")

THIRD_LIB_DIR="3rdparty"

function update_spdlog() {
    git clone --depth=1 --branch "$SPDLOG_VERSION_TAG" https://github.com/gabime/spdlog.git "$SPDLOG_DIR"
    cd "$SPDLOG_DIR" || exit
    rm -rf .git
}

function update_openvr() {
    git clone --depth=1 --branch "$OPENVR_VERSION_TAG" https://github.com/ValveSoftware/openvr.git "$OPENVR_DIR"
    cd "$OPENVR_DIR" || exit
    rm -rf .git
}

function update_imgui() {
    git clone --depth=1 --branch "$IMGUI_VERSION_TAG" https://github.com/ocornut/imgui.git "$IMGUI_DIR"
    cd "$IMGUI_DIR" || exit
    rm -rf .git
}

function update_sdl() {
    git clone --depth=1 --branch "$SDL_VERSION_TAG" https://github.com/libsdl-org/SDL.git "$SDL_DIR"
    cd "$SDL_DIR" || exit
    rm -rf .git
}

function update_glm() {
    git clone --depth=1 --branch "$GLM_VERSION_TAG" https://github.com/g-truc/glm.git "$GLM_DIR"
    cd "$GLM_DIR" || exit
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
WD=$(pwd)

if [ -n "$INIT" ]; then
    for dir in "${DIRS[@]}"; do
        rm_rf_dir "$dir"
    done
fi

clone_if_not_exist "$SPDLOG_DIR" update_spdlog
clone_if_not_exist "$OPENVR_DIR" update_openvr
clone_if_not_exist "$IMGUI_DIR" update_imgui
clone_if_not_exist "$SDL_DIR" update_sdl
clone_if_not_exist "$GLM_DIR" update_glm
