@echo off

if not exist vcpkg (
	git clone https://github.com/Microsoft/vcpkg.git
	
	cd vcpkg
	call .\bootstrap-vcpkg.bat
	cmd /K vcpkg install ffmpeg[x264,opencl] --triplet x64-windows
)