# Aerostat Beam Engine

<img align="right" src="images/beamengine_third.jpg">Redis-backed highly-scale-able and cloud-fit media beam engine. Aerostat Beam Engine provides the following:

* A resilient media-aware cache of media data structured ready for processing by FFmpeg libraries that can be backed by file or object stores;
* Stateless clients providing access to the cache through the _Content Beam API_, an HTTP/S API for transporting media data, pushed and pulled, compressed or uncompressed;
* Job queues allowing multiple worker clients to carry out media transformations just-in-time, on the local systems, distributed across other nodes or via serverless compute like AWS Lambda.

The engine comes in the form of a web server application that provides access to read and write data stored in the underlying Redis cache, which may be a single Redia instance or a cluster of master/slave Redis instances. Also included are some extendible worker clients, supporting clients and the ability to trigger work based on some simple rules.

This is an implementation of the core of the [Agile Media Blueprint](https://www.streampunk.media/agile-media-blueprint).

Work in progress. For Node.js FFmpeg native bindings, please see [Aerostat Beam Coder](https://github.com/Streampunk/beamcoder).

## Installation

## Configuration

Configuration of an Aerostat Beam Engine is achieved by editing the `config.json` file.

## Content Beam API

The _content beam API_ allows FFmpeg-like media data structures to be transported over HTTP and HTTPS protocols. This allows streams of related media - a _virtual format_ or _logical cable_ - to be moved for processing, storage or presentation, either streamed in order or worked on in parallel. Assuming backing by a cache, live streams can be stored and retrieved with little delay - recorded and played back - with a mechanism to start streaming at the latest frame. Endpoints can host the content they represent as _content beams_ through a pull-based mechanism or push media to other endpoints.

All content beam API requests start with `/beams/`. The content beam API for HTTP breaks down as:

`/beams/`&langle;_content_name_&rangle;`/`&langle;_stream_name_&rangle;`/`&langle;_media_ref_&rangle;`/`&langle;_data_ref_&rangle;

* _content_name_: a reference to the source of the content and is set be default to the `url` property of the underlying format context (encoded to safe representation for use in the path part of a URL). This is a unique name for the content that can be beamed from this endpoint.
* _stream_name_: Content is subdivided into streams of _video_, _audio_, _captions_/_subtitles_ and _data_. Streams can be referenced by their index and/or type, e.g. `stream_0`, `stream_1`, ... or aliases `video`, `audio`, `subtitle`, `data` or `attachment` ... or aliases for the first audio stream `audio_0`, the second `audio_1` ... or alias `default` for the stream that FFmpeg considers to be the default stream.
* _media_ref_: The _presentation timestamp_ that uniquely represents a specific media element - _frame_ or _packet_ - or range of media elements in a stream. Most streams have a _time base_ that the presentation time stamps of each element, frame or packet, are measured. Without a _data_ref_, this refers to metadata only.
* _data_ref_: Access to data payloads associated with a single media element, usually simply `data`. For planar data used in some frame formats, `data` refers to the planes concatenated and `data_0`, `data_1` ... refer to each plane.

Content can be created, pulled and pushed, streamed, written, read and deleted using this API. A few examples follow and then the rest of this section breaks down the API in further detail.

To read a single video frame from a compressed stream at timestamp `108000` in content called `newswatch_20190312` for a stream with a time base of 90,000Hz, two GET requests are required:

    https://production.news.zbc.tv/beams/newswatch_20190312/video/108000
    https://production.news.zbc.tv/beams/newswatch_20190312/video/108000/data

The first retrieves the metadata and the second retrieves the associated payload. Here is an example of the retrieved metadata, a JSON document:

```json
{
  "type": "Packet",
  "pts": 108000,
  "dts": 108000,
  "size": 16383,
  "stream_index": 0,
  "flags": {
    "KEY": true,
    "CORRUPT": false,
    "DISCARD": false,
    "TRUSTED": false,
    "DISPOSABLE": false
  },
  "duration": 3600,
  "pos": 18430875
}
```

The second _data_ URL retrieves a payload of type `application/octet-stream` with content length `16383`.

The packet metadata contains no details as to the relative timing of the media element wrt other elements of the stream, the type of data or encoding used. To be able to decode that packet, it is necessary to find out the details of the associated stream. This can be retrieved from a GET request to one of following URLs:

    https://production.news.zbc.tv/beams/newswatch_20190312/video
    https://production.news.zbc.tv/beams/newswatch_20190312/stream_0
    https://production.news.zbc.tv/beams/newswatch_20190312/stream_0.json

The URLs are equivalent and produce the following response:

```json
{
  "type": "Stream",
  "index": 0,
  "id": 301,
  "time_base": [ 1, 9000 ],
  "start_time": 7200,
  "duration": 53666250,
  "r_frame_rate": [ 25, 1 ],
  "codecpar": {
    "type": "CodecParameters",
    "codec_type": "video",
    "codec_id": 173,
    "name": "hevc",
    "format": "yuv420p",
    "width": 1920,
    "height": 1080,
    "color_range": "tv"
  }
}
```

Time can be specified by timestamp, index count, in real time and relatively by offset, including to the _first_ and _latest_ - or _last_ - media element. Metadata can be retrieved using time ranges. Timestamp specification can be _fuzzy_ for the case where errors in timestamp digitisation, fractional framerates or stream jitter mean that timestamps do not increment by an exact, constant value. Here are some examples:

* Single data packet: `/beams/newswatch_20190312/video/packet_108000`
* Uncompressed frame: `/beams/newswatch_live/video/frame_108000`
* Fuzzy match: `/beams/newswatch_live/video/107950f` - also finds 108000
* Range of timestamps: `/beams/newswatch_live/video/108000-144000` (inclusive)
* Range of indexes: `/beams/newswatch_live/video/751st-760th` (1-based from 1st)
* Time range: `/beams/newswatch_live/video/30s-31s` - relative to time base
* To the end: `/beams/newswatch_live/video/1080th-last`
* First frame: `/beams/newswatch_live/video/first`
* Latest frame: `/beams/newswatch_live/video/latest` - redirect to most recent

Finally, these URLs may be decorated by job specifications, such as converting any referenced video frame to a JPEG:

     /beams/newswatch_live/video/frame_108000.jpeg

Another example is creating a partial MP4 file for a specified frame range for all streams:

    /beams/newswatch_live/all/30s-45s.mp4

### Listing available content



### Format - the logical cable


## Workers

### Setting up a rule

### Pre-built workers

### Writing a worker

## Status, support and further development

Although the architecture of the aerostat beam engine is such that it could be used at scale in production environments, development is not yet complete. In its current state, it is recommended that this software is used in development environments and for building prototypes. Future development will make this more appropriate for production use.

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

## License

This project is licensed under the GNU General Public License, version 3 or later. Copyright (C) 2019, Streampunk Media Ltd.

This software links to libraries from the FFmpeg project, including optional parts and optimizations covered by the GPL v2.0 or later. Your attention is drawn to the FFmpeg project's page [FFmpeg License and Legal Considerations](https://www.ffmpeg.org/legal.html).
