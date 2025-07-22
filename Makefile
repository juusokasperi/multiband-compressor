all: build-macos

build-macos:
	xcodebuild -project Builds/MacOSX/1176Compressor.xcodeproj -configuration Debug

run-host: build-macos
	open -a ~/42/Audio/JUCE-Git/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app
