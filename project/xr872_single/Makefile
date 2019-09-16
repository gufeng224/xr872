
BUILD_DIR = build_dir
project_build:
	rm -fr ./${BUILD_DIR}/*
	mkdir -p ./${BUILD_DIR}
	cp -rpf ./src/* ./${BUILD_DIR}/
	make -C ${BUILD_DIR} build

project_clean:
ifeq ($(shell test -e $(BUILD_DIR) && echo $$?),0)
	make -C ${BUILD_DIR} build_clean
endif

clean:
ifeq ($(shell test -e $(BUILD_DIR) && echo $$?),0)
	make -C ${BUILD_DIR} clean
endif
