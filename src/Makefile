TARGET=fd

all: $(TARGET)

fd: imgui_impl_sdl_gl3.cpp gui.cpp state.cpp
	g++ `sdl2-config --cflags` -I/home/wheatdog/tool/wdb/ -I../libs/imgui -I../libs/gl3w gui.cpp imgui*.cpp ../libs/gl3w/GL/gl3w.c `sdl2-config --libs` -lGL -ldl -o $@

clean:
	rm $(TARGET)
