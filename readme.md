# QuickStart for compilation SDK

## Compiler introduction:<br>

If you are the first compiler SDK you need to install some thrid-party dependency libraies such as: openssl-1.1-0h/ffmpeg-4.0 and libao-1.2.0.

Once the setup is complete, you can execute the following commands for compilation.

## step1, To create a compiler directory

	mkdir ${HOME}/build_dir

## step2, To clone the Soundai sdk to ${HOME}

	git clone https://github.com/sevencheng798/MediaPlayerWrapper.git

## setp3, To execute the cmake command in the ${HOME}/build_dir directory.

	Set third-party dependency install path:
	THIRD_LIBRARY_DIR=${HOME}/work/3rd/_install

	cmake ../MediaPlayerWrapper \
        -DCMAKE_INSTALL_PREFIX=./_install \
        -DCMAKE_BUILD_TYPE=DEBUG \
        -DFFMPEG_LIB_PATH="${THIRD_LIBRARY_DIR}/ffmpeg-4.0/lib" \
        -DFFMPEG_INCLUDE_DIR="${THIRD_LIBRARY_DIR}/ffmpeg-4.0/include" \
        -DOPENSSL=ON \
        -DOPENSSL_INCLUDE_DIR="${THIRD_LIBRARY_DIR}/openssl-1.1h/include" \
        -DOPENSSL_LIB_PATH="${THIRD_LIBRARY_DIR}/openssl-1.1h/lib" \
        -DAO_INCLUDE_DIR="${THIRD_LIBRARY_DIR}/libao-1.2.0-pc/include" \
        -DAO_LIB_PATH="/usr/lib/x86_64-linux-gnu"

## step4, Start compiling.
	make && make install
	
## step5, Test the AI sample.
	First before testing, you need to copy the relevant 3rd libraries to the target device if the devices are missing them.

	Then, you need to copy MediaPlayer/AudioMediaPlayer/test/AOWrapperTest run library and execute binrary file to the target device.
	These library and execute are placed in the installation directory specified for compilation.
	such as `CMAKE_INSTALL_PREFIX=./_install`.

	when the above is ready, you can execute the following command to test:
	./AOWrapperTest -u "url"

