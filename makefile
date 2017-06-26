
default:
		@echo Header-only... use install

install:
		cp threadpool.h /usr/local/include/
		chmod a+r /usr/local/include/threadpool.h
