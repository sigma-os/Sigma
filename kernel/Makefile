.PHONY: clean lint init sigma.bin

init:
	meson . build_meson --cross-file ./build/meson_cross.ini

sigma.bin:
	cd build_meson/; ninja
	cp build_meson/sigma.bin .

clean:
	cd build_meson/; ninja clean

lint:
	cppcheck --enable=warning,performance,information,style,portability,missingInclude . -Iinclude/ -j4 --platform=unix64 --std=posix --language=c++ -q -v