all: build-macos

build-macos:
	xcodebuild -project Builds/MacOSX/MultiBandCompressor.xcodeproj -configuration Debug
