CFLAGS = -ggdb -Wall
LFLAGS = -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -lglfw -lfreetype -lcdd \
				 -lcglm
LIBCDD = lib/libcdd.a
FREETYPE2_CFLAGS = -I/usr/include/freetype2 -I/usr/include/libpng16 \
									 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 \
									 -I/usr/lib64/glib-2.0/include
OBJMODULES = tinyexpr.o glad.o shader_s.o coordinate_plane.o

simple_rs_builder: simple_rs_builder.c $(OBJMODULES) | $(LIBCDD)
	$(CC) $(CFLAGS) $^ $(FREETYPE2_CFLAGS) -I include -L lib -o $@ $(LFLAGS)
coordinate_plane.o: coordinate_plane.c include/coordinate_plane.h \
                    include/shader_s.h
	$(CC) $(CFLAGS) $< $(FREETYPE2_CFLAGS) -I include -L lib -c -o $@ $(LFLAGS)
glad.o: glad.c include/glad/glad.h
	$(CC) $(CFLAGS) -I include -c $< -o $@
%.o: %.c include/%.h
	$(CC) $(CFLAGS) -I include -c $< -o $@
$(LIBCDD): cddlib-*
	mkdir -p lib
	cd $^ && ./bootstrap && ./configure
	$(MAKE) -C $^
	find ./$^/lib-src/ -name 'libcdd.a' -exec cp {} lib/ \;
clean:
	$(MAKE) -C cddlib-* clean
	rm -f *.o simple_rs_builder
