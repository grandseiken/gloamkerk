export BUILD_CONFIG="Release"
export BUILD_GENERATOR="Unix Makefiles"
export BUILD_DEFINITIONS="-m32"
git submodule update --init
spatial build --log_level=debug
