CFLAGS = -ggdb -Wall
LFLAGS = -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -lglfw -lfreetype -lcdd
LIBCDD = lib/libcdd.a
FREETYPE2_CFLAGS = -I/usr/include/freetype2 -I/usr/include/libpng16 \
									 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 \
									 -I/usr/lib64/glib-2.0/include

simple_rs_builder: simple_rs_builder.cpp tinyexpr.o glad.o shader_s.o space.o | $(LIBCDD)
	$(CXX) $(CFLAGS) $^ $(FREETYPE2_CFLAGS) -I include -L lib -o $@ $(LFLAGS)
dontdelete: simple_rs_builder.cpp tinyexpr.o glad.o shader_s.o space.o | $(LIBCDD)
	$(CXX) -DDONTDELETE $(CFLAGS) $^ $(FREETYPE2_CFLAGS) -I include -L lib \
		-o simple_rs_builder $(LFLAGS)
space.o: space.cpp include/space.hpp include/shader_s.h
	$(CXX) $(CFLAGS) $< $(FREETYPE2_CFLAGS) -I include -L lib -c -o $@ $(LFLAGS)
glad.o: glad.c include/glad/glad.h
	$(CC) $(CFLAGS) -I include -c $< -o $@
%.o: %.c include/%.h
	$(CC) $(CFLAGS) -I include -c $< -o $@
$(LIBCDD): cddlib-*
	mkdir -p lib
	cd $^ && ./bootstrap && ./configure
	$(MAKE) -C $^
	find ./$^/lib-src/ -name 'libcdd.a' -exec cp {} lib/ \;
	find ./$^/lib-src/ -name '*.h' -exec cp {} include/ \;
clean:
	$(MAKE) -C cddlib-* clean
	rm -f glad.o space.o tinyexpr.o shader_s.o lib/libcdd.a simple_rs_builder
	cd include && rm -f cddtypes.h setoper.h cddmp_f.h cdd.h cddtypes_f.h \
		cddmp.h cdd_f.h splitmix64.h
