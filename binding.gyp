{
  "targets": [{
    "target_name" : "beamcoder",
    "conditions": [
      ['OS=="win"', {
      "sources" : [ "src/beamcoder/beamcoder.cc" ],
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
        "ffmpeg/win64/include"
      ],
      "libraries": [
        "-l../ffmpeg/win64/lib/avutil"
      ],
      "copies": [
          {
            "destination": "build/Release/",
            "files": [
              "ffmpeg/win64/lib/avutil-56.dll"
            ]
          }
        ]

    }]
  ]
}]
}
