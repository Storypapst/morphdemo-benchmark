.PHONY: all verify escape clean
all:
	bash build.sh
verify: all
	python3 tools/verify.py
escape: all
	python3 tools/verify.py --escape
clean:
	rm -rf build morphdemo-4k morphdemo-16k morphdemo-64k
