# AV1 frame splitter
Takes a raw AV1 stream (`5.2 Low overhead bitstream format`) and splits it into separate frames, `OBU_TEMPORAL_DELIMITER` to `OBU_TEMPORAL_DELIMITER`:
```sh
clang++ -std=c++17 -fcolor-diagnostics -fansi-escape-codes -O3 obusplit.cpp -o obusplit
./obusplit av1-sample.obu
```

The AV1 sample file was created with this:
```sh
ffmpeg -f lavfi -i testsrc=n=2:size=1280x720:rate=30 -pix_fmt yuv420p -t 0.333334 -f yuv4mpegpipe -pix_fmt yuv420p - | \
aomenc - --codec=av1 --obu --i420 --width=1280 --height=720 --fps=30/1 --cpu-used=5 --rt --tile-columns=3 --cq-level=35 --num-tile-groups=3 -o av1-sample.obu
```
(`ffmpeg` itself can't be configured to instruct `libaom` to produce tile groups)
