all:
	make rgfw_opengl2/rgfw-nuklear
	make rgfw_opengl3/rgfw-nuklear
	make rgfw_opengl4/rgfw-nuklear
	make rgfw_vulkan/rgfw-nuklear
	make rgfw_rawfb/rgfw-nuklear

rgfw_opengl2/rgfw-nuklear: rgfw_opengl2/*
	cd rgfw_opengl2 && make
rgfw_opengl3/rgfw-nuklear: rgfw_opengl3/*
	cd rgfw_opengl3 && make
rgfw_opengl4/rgfw-nuklear: rgfw_opengl4/*
	cd rgfw_opengl4 && make 
rgfw_rawfb/rgfw-nuklear: rgfw_rawfb/*
	cd rgfw_rawfb && make

debug:
	make

	./rgfw_opengl2/rgfw-nuklear
	./rgfw_opengl3/rgfw-nuklear
	./rgfw_opengl4/rgfw-nuklear
	./rgfw_rawfb/rgfw-nuklear

clean:
	cd rgfw_opengl2 && make clean
	cd rgfw_opengl3 && make clean
	cd rgfw_opengl4 && make clean
	cd rgfw_rawfb && make clean