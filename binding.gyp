{
  "targets": [{
    "target_name" : "beamcoder",
    "conditions": [
      ['OS=="win"', {
      "sources" : [ "src/beamcoder/beamcoder.cc", "src/beamcoder/beamcoder_util.cc",
                    "src/beamcoder/governor.cc", "src/beamcoder/format.cc",
                    "src/beamcoder/decode.cc", "src/beamcoder/filter.cc",
                    "src/beamcoder/encode.cc",
                    "src/beamcoder/packet.cc", "src/beamcoder/frame.cc" ],
      "configurations": {
        "Release": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "RuntimeTypeInfo": "true"
            }
          }
        }
      },
      "include_dirs" : [
        "../ffmpeg/ffmpeg-4.1-win64-dev/include"
      ],
      "libraries": [
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/avcodec",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/avdevice",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/avfilter",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/avformat",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/avutil",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/postproc",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/swresample",
        "-l../../ffmpeg/ffmpeg-4.1-win64-dev/lib/swscale"
      ],
      "copies": [
          {
            "destination": "build/Release/",
            "files": [
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/avcodec-58.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/avdevice-58.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/avfilter-7.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/avformat-58.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/avutil-56.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/postproc-55.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/swresample-3.dll",
              "../ffmpeg/ffmpeg-4.1-win64-shared/bin/swscale-5.dll"
            ]
          }
        ]

    }]
  ]
}]
}
