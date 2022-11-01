g++ dicom.cpp -ldcmdata -lofstd -ldcmimgle
./a.out /home/trent/DICOM/MOYAR_20220906_104918 | ffmpeg -y -init_hw_device qsv=hw -filter_hw_device hw \
  -f rawvideo \
  -pixel_format yuv420p10le -framerate 24000/1001 -video_size 3840x2160 \
  -i - -b:v 30000000 \
  -vf 'hwupload=extra_hw_frames=64,format=qsv' \
  -c:v hevc_qsv -profile:v main10 \
  -color_primaries 9 -color_trc 16 -colorspace 'ictcp' -color_range 1 \
  finalgreenpeak.mp4