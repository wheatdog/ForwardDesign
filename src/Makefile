TARGET=fd state

all: $(TARGET)

fd: imgui_impl_sdl_gl3.cpp main.cpp
	g++ `sdl2-config --cflags` -I/home/wheatdog/tool/wdb/ -I ../imgui -I ../libs/gl3w main.cpp imgui_impl_sdl_gl3.cpp ../imgui/imgui*.cpp ../libs/gl3w/GL/gl3w.c `sdl2-config --libs` -lGL -ldl -o $@

state: state.cpp
	g++ -I/home/wheatdog/tool/wdb/ state.cpp -o $@

clean:
	rm $(TARGET)
