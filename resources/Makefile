.PHONY: all

all: appicon_64.png appicon_256.png
appicon_64.png: appicon_16.png
	convert $< -filter point -resize 64x64 $@
appicon_256.png: appicon_16.png
	convert $< -filter point -resize 256x256 $@

all: appicon_16.ico appicon_128.ico
appicon_16.ico: appicon_16.png
	convert $< -filter point -resize 16x16 $@
appicon_128.ico: appicon_16.png
	convert $< -filter point -resize 128x128 $@

all: appicon.icns
appicon.icns: appicon_16.png appicon_256.png
	png2icns $@ $^
