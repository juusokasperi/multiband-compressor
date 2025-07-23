ifneq (,$(filter release%,$(MAKECMDGOALS)))
	CONFIG = "Release"
else
	CONFIG = "Debug"
endif

PLUGIN_HOST ?= ~/42/Audio/JUCE-Git/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app

all: build

build:
	xcodebuild -project Builds/MacOSX/1176Compressor.xcodeproj -configuration $(CONFIG)

run: build
	open -a $(PLUGIN_HOST)

clean:
	rm -rf Builds/MacOSX/build/

release: build
release-run: run-host

.PHONY: all build run clean release release-run
