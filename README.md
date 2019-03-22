# Aerostat Beam Engine

<img align="right" src="images/beamengine_third.jpg" width="50%"/>[Redis](https://redis.io/)-backed highly-scale-able and cloud-fit distributed media processing engine. A [Node.js](https://nodejs.org/en/) web application and library, _Aerostat Beam Engine_ provides the following:

* A resilient, media-aware cache of data structured ready for processing by [FFmpeg](http://ffmpeg.org/) libraries, which can be backed by file or object stores;
* Connections to stateless clients that provide access to the media-aware cache through the [_Content Beam API_](#content-beam-api), an HTTP/S API for transporting media data, pushed and pulled, compressed or uncompressed, and streamed as if live;
* Job queues allowing multiple worker clients to carry out media transformations _just-in-time_ or _just-in-case_, executing on the local system, distributed across many systems, GPU accelerated or via serverless compute services like [AWS Lambda](https://aws.amazon.com/lambda/).

The engine is a web server application that provides access to read and write data stored in the underlying redis cache, which may be a single redis instance or a cluster of master/slave redis instances. Also included a library to build customised [workers](#workers), triggered by [rules](#setting-up-a-rule), maintaining and building [content relationships](#relationships). A small library of example [pre-built workers](#pre-built workers) is provided.

Beam engine is an implementation of the core of the [Agile Media Blueprint](https://www.streampunk.media/agile-media-blueprint).

Work in progress. This README describes the _to be_ state of Beam Engine and the current implementation falls short of what is described. See the __TODO__ notes for details of implementation still to be done. For Node.js FFmpeg native bindings, please see [Aerostat Beam Coder](https://github.com/Streampunk/beamcoder).

## Installation

Before installation can take place, it is necessary to identify some connected networked resources, to include:

* system(s)/container(s)/VM(s) to run the [Node.js](https://nodejs.org/en/) Beam Engine web apps, possibly behind a load balancer such as [NGINX](https://www.nginx.com/) or [AWS Elastic Load Balancing](https://aws.amazon.com/elasticloadbalancing/).
* system(s)/container(s)/VM(s) to run [redis](https://redis.io/), either a single instance, replicated or as a cluster. Alternatively, use a redis-backed cache service such as [AWS Elasticache](https://aws.amazon.com/elasticache/) or [Azure Cache for Redis](https://azure.microsoft.com/en-gb/services/cache/).
* system(s)/container(s)/VM(s) to run workers written in Node.js, some of which may need access to a shared file system to persist cached data.
* access to serverless processing capability such as [AWS Lambda](https://aws.amazon.com/lambda/) or [Azure Functions](https://azure.microsoft.com/en-gb/services/functions/) (optional).
* systems with GPUs for workers that can do accelerated GPU processing (optional).

It is recommended that for the distributed processing of uncompressed HD video data, all network interconnects run at speeds at or over 10Gbs.

For development purposes, it is possible to run redis, beam engine and workers all on the same system.

### Node.js prerequisites

On x86_64 Windows, Mac or Linux platforms, Beam Engine uses the latest Long Term Support version of Node.js which is currently the most recent v10. Download or install Node.js from [here](https://nodejs.org/en/) or use a [system package manager](https://nodejs.org/en/download/package-manager/).

The Aerostat Beam Engine web app and workers depend on [Aerostat Beam Coder](https://github.com/Streampunk/beamcoder), a module that as a native addon requires compilation as part of its installation process. The Node.js native addon build tool is called `node-gyp`. Follow the [node-gyp installation instructions](https://github.com/nodejs/node-gyp#installation) to ensure that each Node.js system is ready to build native extensions.

Note: For MacOSX Mojave, install the following package after `xcode-select --install`:

    /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg

The Beam Coder installer downloads suitable FFmpeg `.dll`s on Windows and uses _homebrew_ to install FFmpeg `.dylib` libraries on Mac. For Linux, install FFmpeg development packages suitable for your Linux version, ensuring that the packages include the shared library `.so` files.   

### Redis

For local working, install redis on a system or systems with a substantial amount of memory. Follow the instructions from redis themselves. For testing and low criticality systems, a single Redis instance or replicating instance may be sufficient.

For mission critical applications, consider establishing a cluster. Beam engine uses [ioredis](https://github.com/luin/ioredis), a module with support for single connections and connections to members of a redis cluster.

For cloud environments, consider using Redis backed cache service, creating a connection to a virtual private network where the beam engines and workers are located. Such services include:

* [AWS ElastiCache for Redis](https://aws.amazon.com/elasticache/redis/);
* Microsoft's [Azure Cache for Redis](https://azure.microsoft.com/en-gb/services/cache/)
* Google Cloud Platform [Cloud Memorystore](https://cloud.google.com/memorystore/)

Many pre-built redis packages exist for Linux distributions, such as the [`redis-server` package for Ubuntu](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-redis-on-ubuntu-18-04). Redis is also available as a container, such as the [redis docker container](https://hub.docker.com/_/redis). For development and testing on Windows, consider installing [redis using the Windows Subsystem for Linux](https://medium.com/@RedisLabs/windows-subsystem-for-linux-wsl-10e3ca4d434e). Note that for some operations, this approach is one or two orders of magnitude slower than installing Redis on a Linux build directly and, as such, should only be used for feature testing and not performance testing.

### Development

To run a Beam Engine in development mode, clone the module from github. In the modules root folder, edit the `config.json` file to match local settings. Then run `npm install` followed by `npm run dev`. This will install dependencies and start the development web server that runs using the automated restart module [nodemon](https://github.com/remy/nodemon). To restart, type `rs`.

To see debug information from Redis connections (ioredis), the web framework (Koa 2) and from the job queues (Bull), set the debug flag as follows.

From bash:
```
$ DEBUG="*,-not_this"; npm run dev
```

From Windows Powershell:
```
PS > $env:DEBUG="*,-not_this"; npm run dev
```

To create your own workers, install beam engine in your project.

    npm install beamengine

### Production

__TODO__ - to follow. Create beam engine application.-

## Configuration

Configuration of an Aerostat Beam Engine is achieved by editing the `config.json` file. This can also be passed in on the command line as follows:

__TODO__ config on the command line following best practices

The configuration has the following properties:

| property name             | type    | description                                    |
| ------------------------- | ------- | ---------------------------------------------- |
| `redis.cluster`           | Boolean | True if connecting to a redis cluster          |
| `redis.host`              | string  | IP address of primary redis host               |
| `redis.port`              | int     | Port number of primary redis host connection   |
| `redis.db`                | int     | Redis database to be used for main cache       |
| `redis.pool`              | int     | Maximum pool size for Redis connection pool    |
| `redis.prepend`           | string  | Value to prepend to beamengine database keys   |
| `redis.packetTTL`         | int     | Time (ms) before expiry of packet data         |
| `redis.frameTTL`          | int     | Time (ms) before expiry of frame data          |
| `redis.ephemeralTTL`      | int     | Time (ms) before expiry of ephemeral data blob |
| `redis.closeTimeout`      | int     | Time (ms) before error when closing Redis pool |
| `app.port`                | int     | Local port on which to run the app server      |
| `testing.db`              | int     | Redis database to use for testing              |
| `testing.port`            | int     | Port to run test app server on                 |
| `jobs...`                 | Object  | See [workers](#workers) section                |

## Content Beam API

The _content beam API_ transports FFmpeg-like media data structures over [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol) and [HTTPS](https://en.wikipedia.org/wiki/HTTPS) protocols. This allows _content items_ in the form of streams of related media - a _virtual format_ or _logical cable_ - to be accessed for capture, processing, storage or presentation, either streamed in order or worked on in parallel. Assuming backing by a cache, live streams can be stored and retrieved with minimal delay - recorded and played back - with a [mechanism to start streaming at the latest frame](#stream_as_live). As _content beams_, API endpoints can both host the content they represent through a pull-based mechanism and/or push media to other endpoints.

All content beam API requests start with `/beams/`. The content beam API for HTTP/S (HTTP or HTTPS) breaks down as:

`/beams/`&langle;_content_name_&rangle;`/`&langle;_stream_name_&rangle;`/`&langle;_media_ref_&rangle;`/`&langle;_data_ref_&rangle;

* _content_name_: an identifier or locator for the source of the content that is used as a unique name for the content as it is beamed to or from this endpoint.
* _stream_name_: Content is subdivided into streams of _video_, _audio_, _captions_/_subtitles_ and _data_. Streams can be referenced by their index and/or type, e.g. `stream_0`, `stream_1`, ... or with media type aliases `video`, `audio`, `subtitle`, `data` or `attachment`.
* _media_ref_: The _presentation timestamp_ that uniquely represents a specific _media element_ - a _frame_ or a _packet_ - or range of media elements in a stream.  When omitting the _data_ref_, such a URL refers to JSON metadata only.
* _data_ref_: Access to data payloads associated with a single media element, usually simply `data`. For planar data used in some frame formats, `data` refers to the planes concatenated and `data_0`, `data_1` ... refer to each plane separately.

Content can be created, pulled and pushed, streamed, read, written and deleted using this API. A few introductory examples follow directly and then the rest of this section breaks down the API in further detail.

To read a single video frame from a compressed stream at timestamp `108000` in content called `newswatch_20190312` for the single video stream with a time base of 90,000Hz, two GET requests are required:

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

The second _data_ URL retrieves a payload of type `application/octet-stream` with content length `16383`. To create the packet, PUT the metadata and payload to the same URLs as the example GET requests. The metadata must exist before the payload data is posted.

The packet metadata contains no details as to the relative timing of the media element wrt other elements of the stream, the type of data payload or the encoding used. To be able to decode the payload of a packet, it is necessary to find out the details of the associated stream. This can be retrieved from a GET request to one of following URLs:

    https://production.news.zbc.tv/beams/newswatch_20190312/video
    https://production.news.zbc.tv/beams/newswatch_20190312/stream_0
    https://production.news.zbc.tv/beams/newswatch_20190312/stream_0.json

The URLs are equivalent and produce the following response (some properties removed):

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

Time can be specified by timestamp, index count, in real time and relatively by offset, including to the _first_ and _latest_ - or _last_ - media elements. Metadata can be retrieved using time ranges. Timestamp specification can be _fuzzy_ for the case where errors in timestamp digitisation, fractional framerates or stream jitter mean that timestamps do not increment by an exact, constant value. Here are some examples:

* Single data packet: `/beams/newswatch_20190312/video/packet_108000`
* Uncompressed frame: `/beams/newswatch_live/video/frame_108000`
* Fuzzy match: `/beams/newswatch_live/video/107950f` - also finds 108000
* Range of timestamps: `/beams/newswatch_live/video/108000-144000` (inclusive)
* Range of indexes: `/beams/newswatch_live/video/751st-760th` (1-based from 1st)
* Time range: `/beams/newswatch_live/video/30s-31s` - relative to time base
* To the end: `/beams/newswatch_live/video/1080th-last`
* First frame: `/beams/newswatch_live/video/first`
* Latest frame: `/beams/newswatch_live/video/latest` - redirect to most recent

Finally, these URLs may be decorated by job specifications to be carried out just-in-time by [workers](#workers), such as converting any referenced video frame to a JPEG:

     /beams/newswatch_live/video/frame_108000.jpeg

Another example of a job is creating a partial MP4 file for a specified frame range for all streams:

    /beams/newswatch_live/all/30s-45s.mp4

### Listing available content

The names of all the items of content items stored at an endpoint can be retrieved by a GET request to `/beams/`. This may be a long list and so the request supports query parameters `start` offset and `limit` to enable paging of the values. The value returned is a JSON array:

    [ 'newswatch_live', 'newswatch_20190312', 'newswatch_20190311' ]

It is assumed that an external search service, such as [Elasticsearch](https://www.elastic.co/), will be used in tandem with beam engine to provide a more competent index of available streams.

### Format - the logical cable

Content items have a _format_ that provides the technical parameters for reproducing that content. This consists of overall format description, the streams that make up the content and the technical parameters required to configure decoders. When used to access files, the streams of the beam API format correspond to the storage compartments of a virtual container. When used to access a live stream, the _format_ can be considered as if the description of a multicore _cable_, where each core wire of that cable carries the latest media elements for a particular stream. As media elements in beams are associated by URL path structure only, beams of the beam API represent _virtual formats_ and _logical cables_.

Each content item has a format that is used to create, query, update or delete that content from a beam engine. All requests take the form `/beams/`&langle;_content_name_&rangle;.

To get the details of a format associated with a content item, a JSON document, send a GET request including the content name. For example, one of:

    https://production.news.zbc.tv/beams/newswatch_20190312
    https://production.news.zbc.tv/beams/newswatch_20190312.json

This will produce a result of the form:

```json
{
  "type": "format",
  "streams": [
    {
      "type": "Stream",
      "index": 0,
      "time_base": [ 1, 90000 ],
      "codecpar": {
        "type": "CodecParameters",
        "codec_type": "video",
        "name": "hevc",
        "format": "yuv420p",
        "width": 1920,
        "height": 1080
      }
    },
    {
      "type": "Stream",
      "index": 1,
      "time_base": [ 1, 90000 ],
      "codecpar": {
        "type": "CodecParameters",
        "codec_type": "audio",
        "name": "aac",
        "format": "fltp",
        "bit_rate": 66494,
        "channel_layout": "stereo",
        "channels": 2,
        "sample_rate": 44100,
        "frame_size": 1024
      }
    }
  ],
  "url": "newswatch_20190312",
  "duration": 596291667,
  "bit_rate": 2176799,
  "metadata": {
    "title": "ZBC News Watch - evening news 12th March 2019",
    "author": "ZBC News Production"
  }
}
```

Note that the name of the content item as it appears in the URL is also stored in the `url` property. Consider using ECMAScript function [`encodeURIComponent`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent) to safely use a structured name in a path. Also note that if the `format` has in input format set (`iformat` property - not shown), the result will have type `demuxer`. Similarly, if the format has an output format set (`oformat` property - not shown) it will have type `muxer`.

To create a content item, POST a JSON format description to `/beams/`. The content item's name will be set using the `url` property if present, otherwise a [version 4 UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) will be generated. If the content item already exists, a `409 - Conflict` error response will be generated. If the content item is successfully created, a `201 - Created` response is returned with the `Location` header set to the path for the new item.

The following code shows how to create a content items from an Aerostat Beam Coder format using the [got](https://www.npmjs.com/package/got) promise-friendly HTTP request module:

```javascript
let response = await got.post(
    'https://production.news.zbc.tv/beams/',
  {
    body: wavFmt.toJSON(),
    json: true,
    headers : { 'Content-Type': 'application/json' }
  }
).catch(err => {
  if (err.statusCode === 409) {
    console.log('Got conflict: assuming OK.');
  } else {
    throw err;
  }
});
```

__TODO__ - updating a format - not yet supported

__TODO__ - deleting a format - not yet supported

### Streams - logical cable cores

Streams are the sub-components of a format, the virtual core wires of a multicore logical cable. The details of streams are provided as part of the format but can also be accessed stream-by-stream using index or stream type. Streams are located within their content item at a sub-path:

`/beams/`&langle;_content_name_&rangle;`/`&langle;_stream_name_&rangle;

The primary _stream_name_ is based on the stream index, made by appending the index to `stream_`, e.g. `stream_0`, `stream_42`. This can be shortened to just the index number, e.g. `0`, `42`.

Streams define a `time_base` for the unit of measurement for any timestamps or durations of the media elements of the stream. Streams of the same format may have a different time bases. Across the streams of a format, a media element with a timestamp calculated relative to the stream time base as having an equal time value should be presented at the same moment to the viewer. For example, for a video and audio media element:

* Video stream time base is `[1, 90000]` and the frame's presentation timestamp is `90000` then the video frame must be presented at content item time of 1s.
* Audio stream time base is `[1, 48000]` and the audio frame's presentation timestamp is `48000` then the audio samples must be played at content item time 1s, co-timed with the video frame.

Streams have codec parameters describing how data is encoded (or not), reprsented and the dimensions of each media element. This includes pixel format, sample format, resolution, colorspace etc.. The codec has a name and a type, with the type describing whether the stream consists of _video_, _audio_, _subtitles_ (_captions_), _data_ or an _attachment_. The difference between a _data_ stream and an _attachment_ stream is that a data stream contains elements of timed data associated with the video, such as interactive event triggers, whereas an _attachment_ is a single item of data for the entire stream, such as a thumbnail image.

To make it easier to make reference a stream, the codec type can be used as the name of a stream in the URL. If more than one stream of a particular type exists, a type-scoped index can be added, with for example `audio` and `audio_0` referencing the first audio stream, `audio_1` the second and so on. This is an alias ... the primary _stream_name_ is always based on the stream index.

A final alias, `default`, is a reference to the stream that FFmpeg nominates as the default for a format. This is normally the main video track. The dafault track provides a time base for making time references across all the streams of a format.

Details about a stream can be retrieved in the form of a JSON document with a GET request. The following four requests are equivalent (`.json` extension is optional):

    https://production.news.zbc.tv/beams/newswatch_2019031/stream_0
    https://production.news.zbc.tv/beams/newswatch_2019031/video.json
    https://production.news.zbc.tv/beams/newswatch_2019031/video_0
    https://production.news.zbc.tv/beams/newswatch_2019031/default

The result is the same as the stream returned as part of the parent format:

```json
{
  "type": "Stream",
  "index": 0,
  "time_base": [ 1, 90000 ],
  "codecpar": {
    "type": "CodecParameters",
    "codec_type": "video",
    "name": "hevc",
    "format": "yuv420p",
    "width": 1920,
    "height": 1080
  }
}
```

__TODO__ - add a stream to a format.

__TODO__ - updating details of a stream.

### Media elements - metadata

The sub-components of a stream of media are its _media elements_, a generic term referring to either a _packet_ of encoded data or an uncompressed _frame_ of media. Typically, a media element is either a single video frame or a sequential grouping of audio samples, whereby the size of the group is related to either the video frame timing (e.g. 1920 samples for every video frame at 25fps) or by the _blocksize_ for a given audio codec (e.g. AAC uses a blocksize of either 960 or 1024 samples). Media elements are located in time within a stream at a given _presentation timestamp_ (PTS) and the PTS is the primary identifier for a media element.

A beam engine stores data about every media element, its _metadata_, and may also store the payload of the media element. For packets, this is a sparse amount of data including the presentation timestamp, decode timestamp (e.g. for streams of H.264 video where the stored decode order and presentation order differ), duration and position in the source stream. For frames, additional details of the format, resolution, sample rate etc. are included.

Media element metadata can be read and queried with GET requests and written with PUT requests. The URL is the same as for a stream, including name aliasing, with a media reference specifying the specific media element or elements.

`/beams/`&langle;_content_name_&rangle;`/`&langle;_stream_name_&rangle;`/`&langle;_media_ref_&rangle;

GET requests can be made for either a single media element or an inclusive range of media elements. Whereas PUT requests are for a single media element only. The body of requests and responses have type `application/json`. Five kinds of request can be made with a _media_ref_:

* _direct by timestamp_: GET and PUT. Retrieve, create or update metadata about a specific packet or frame, using a direct media reference of the form `packet_`&langle;_pts_&rangle; or `frame_`&langle;_pts_&rangle;. Such direct requests bypass index lookup but require that the PTS value in the path is exact.
* _presentation timestamp_: GET only. Request contains a single presentation timestamp or range of presentation timestamps, e.g. `7200` or `7200-43200` respectively. Useful when clock sampling errors, jitter or sampling conversion cause timestamps to be non exact, fuzzy match is enabled by adding an `f` to the timestamp, e.g. `7200f` will match the closest media element to `7200`, either before or after.
* _index_: GET only. Request contains a media element that is a given count of elements through the stream in its current state, from the `first` to the `last` element. For example, `first` or `1st`, `2nd`, `3rd`, ... up to e.g. `41st`, `42nd` or `last`. Ranges can be used, such as `first-last` for all media elements or `345th-723rd`. Media element index values are 1-based.
* _real timestamps_: GET only. Request contains a real time value or range that is measured in seconds, with the beam engine converting presentation timestamps to real time based on the stream time base. For example, in a stream with a time base of 25fps, real timestamp `1.4s` resolves to frame `35`  and real timestamp range `1.2s-2.2s` refers to frames `30-55`.
* _relative timestamp_: GET only. Retrieve a media element at a given offset before or after the specified presentation timestamp. If following a live stream, this allows a request to be made for the next frame based on the current frames timestamp. For scrubbing, this allows jumping backwards and or forwards from a given point. The format is the base PTS, followed by `+`/`-`, followed by an offset value signified with `d`. For example, `7200+1d` references the frame after one with timestamp `7200`, say `10800`, and `7200-1d` is the frame before, say `3600`.

As with listing content items, requests can by _paged_ with the query parameters `offset` and `limit` to control how many values are returned and the pagination of values. GET requests return JSON arrays, even for a single value, or `404 - Not Found` if no results are found. All requests can have `.json` as an optional file extension in their paths.

Here is an example of a relative timestamp request.

    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/3600+1d

```json
[ { "type": "Packet",
    "pts": 7200,
    "dts": 7200,
    "stream_index": 1,
    "flags": {
      "KEY": true,
      "CORRUPT": false,
      "DISCARD": false,
      "TRUSTED": true,
      "DISPOSABLE": false
    },
    "duration": 3600,
    "pos": 81912,
    "buf_size": 16383
  } ]
```

Streams with codec type _attachment_ have a single media element _packet_ with a presentation timestamp of zero.

When creating or updating packets created by Aerostat Beam Coder, use the `toJSON` method. Assuming a packet called `pkt` is in the local scope, the following example shows how to create a packet using the [got](https://www.npmjs.com/package/got) promise-friendly HTTP request module:

```javascript
await got.put(
    `https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/packet_${pkt.pts}`,
  {
    body: pkt.toJSON(),
    json: true
  }
);
```

__TODO__ - check updating of media element data

__TODO__ - delete media elements - is this necessary or only when deleting the parent format?

### Payload data

The payload data for a media element is stored separately from the metadata that describes it and may have a different lifetime in the underlying cache. A situation can exist whereby it is possible to retrieve the metadata but not get access to the payload because it has expired from the cache. See details on creating relationships and workers that can mitigate against the issue.

Payload data has the same base path as the associated media element's _direct by timestamp_, then add an extension `.raw` or sub-path part `.../data`. This additional part is the _data_ref_. It is only possible to reference one media element, frame or packet, at a time:

* For packets, the path URL refers to the single [_blob_](https://en.wikipedia.org/wiki/Binary_large_object) of data contained in a packet.
* For frames that have more than one _blob_ of payload, e.g. a frame with three separate planes of data, an unqualified data reference of `/data` or `.raw` refers to all of these _blobs_ concatenated together. Reference can be made to each separate plane by adding an zero-based index, e.g. `.raw_0` or `/data_0` for the first plane, `.raw_1` or `/data_1` for the second, and so on.

In the following example of retrieving the data payload for a specific frame, the data for each plane for a format of `yuv422p` is retrieved independently, possibly concurrently:

    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/frame_7200/data_0
    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/frame_7200/data_1
    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/frame_7200/data_2

Alternatively, the data for all the planes can be retrieved in one go as follows:

    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/frame_7200/data

The value returned contains a header `Beam-Buf-Sizes`, a JSON array with the size of each separate data plane. For example:

    Beam-Buf-Sizes: [ 2073664, 1036864, 1036864 ]

Note that due to the way FFmpeg code reads buffers, the size of each data plane buffer is generally slightly larger than the minimum required to represent a plane of data.

In this example, the data for each plane can be established as follows:

```javascript
[ response.body.slice(0, 2073664),
  response.body.slice(2073664, 2073664 + 1036864),
  repsonse.body.slice(2073664 + 1036864) ]
```

An equivalent request using the `.raw` extension:

    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/frame_7200.raw

Data payloads can be retrieved with GET and created or updated with PUT. Even if a payload is associated with a MIME type, all payload data sent to or from a beam engine has content type `application/octet-stream`. For example, the payloads produced by the `png` codec could have MIME type `image/png` but beam engine uses the general type.

If the payload data is not available for whatever reason, a `404 - Not Found` status response is created. This is case even if the payload was once cached and has now longer available from the cache where, arguably, a `410 - Gone` might have been more appropriate. This is because it is possible that the payload for a given media element might be cached again in the future - it has not been permanently removed - so requesting the element again is not unreasonable behaviour for a client.

The following is an example of a [got](https://www.npmjs.com/package/got) request to retrieve payload data:

```javascript
let payloadResponse = await got(
  `https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/frame_7200/data`,
  { responseType: 'buffer', encoding: null }
);
let payload = response.body;
```

### Streaming as live

A beam can chase the progress of a growing live stream and give the impression of playing the stream live, with only a small latency of one or two frames for uncompressed data. With knowledge of the stream's time base, the process is:

1. A client requests the _latest_ frame in a stream, noting the real time (_t_) of the request, using sub-path `/latest` (or `/end`) as the media reference (_media_ref_).
2. A response with status `302 - Found` is returned with the `Location` header containing a _direct by timestamp_ link to the frame with the highest timestamp in the stream.
3. The client retrieves the metadata for the packet or frame, noting the presentation time stamp (`pts`) and duration (`duration` or `pkt_duration`).
4. Based on the metadata and media element type, the client retrieves the data payload, e.g. `/packet_10800`.
5. At time _t_ plus `duration`, request metadata and/or payload data for the next frame with timestamp `pts`&nbsp;+&nbsp;`duration`, e.g. `packet_14400`. A _relative timestamp_ could be used, e.g. `10800+1d`. Note that a `404` response might be caused by a difference in clocks between client, server and media source, so the client should be prepared to retry a few times.
6. A beam has no defined end point, so the client should assume that stream has ended when repeated requests for the next element, i.e. ten repeated requests, all generate a `404` response. A further `/latest` request can be used to check that the latest frame is not longer advancing and/or to resync with the stream.

__TODO__ - consider if beams should have decent endings!?!

The following is a request for the latest frame in the example stream:

    https://production.news.zbc.tv/beams/newswatch_2019031/stream_1/latest

Subject to at least one media element being available, an HTTP/S client that has _follow redirects_ enabled will retrieve media element metadata from this request.

With a suitably high-bandwidth network, say 10Gbps or higher, uncompressed formats can be transported as if SDI or SMPTE ST 2110 streams, trading a frame or so of latency for location independence and Internet-like scaling.

It is also possible to stream from the start using `start` as the media reference, for scenarios such as catch-up watching of an event that has already started. In this case, the redirect response is to the first media element known for the stream.

## Relationships

<img align="right" src="images/rels.png" width="50%"/>Items of content may be related to other items because they are:

* byte-for-byte _equivalent_, stored in different locations but otherwise with exactly the same encoding, format and other metadata;
* visually equivalent _renditions_, such as some source material and all of the encodings made from it, generally with the same resolution;
* _transformations_ that create a new item of content by applying a filter such as scaling or mixing, possibly with one or more inputs.

These kinds of relationships can be stored in the beam engine to allow a worker to select the most appropriate format or location from which to retrieve source data and/or deliver a result.

### Equivalent

A beam engine uses redis as a RAM cache for media data but this cannot be a permanent, medium or long term storage medium. The time-to-live from creation to cache expiry for media data payloads can be set in the [configuraion](#configuration). When data payloads expire, they are no longer available unless they have also been stored in an external equivalent content item. An equivalent content item is a byte-for-byte copy of the source data. Typically, an equivalent content item will have content name that represents a resolvable storage reference, such as a local file path or an AWS S3 bucket identifier, where a version of the cached content was persisted by a worker.

Equivalent content representations are one-to-one relationships. Although there will often be a content source, any equivalent content item should be able to be substituted by another.

Here is an example workflow for a content item being recorded via the Beam Content API.

1. A new content item is created. A file backup worker with access to a shared storage drive responds to all beam API content creation requests by creating an equivalent content item with the same format and streams. An equivalent content relationship between the source item and its copy is stored into redis.

2. Each packet that is stored into the beam engine creates a post-response backgroud job that copies the packet's metadata and payload from the cache onto the shared storage.

3. Requests for the packet's payload that take place within the time-to-live of the packet's payload in the cache are serviced from the data in the cache.

4. Requests for the packet's payload that occur after the cache expiry cause a `404` error from the beam API. This is intercepted and creates a job. A worker checks to see if any equivalent items of content exist on persistent storage. If so, the packet is retrieved from one of those locations. By policy, the packet's payload can be re-cached in redis if further requests are expected within the subsequent TTL.

As an optimisation, it may be possible to predict that if the payload of a particular packet is requested then the paylaods of the next few packets in sequence may be required, so pre-emptive read-ahead jobs can be used to pre-populate the cache for anticipated requests.

__TODO__ - description of asserting a equivalent relationship

### Rendition

As experienced from the perspective of a viewer, a content item that is a rendition of another is a visually- and/or audibly-equivalent representation. Typically, a rendition has the same resolution or shape as its source but may have different parameters such as pixel/sample format or codec, meaning that it is not an exact byte-for-byte copy. Rendition relationships between content items are one-to-one and directional, with a _source_ end and a _target_ end. A content item without a rendition source is the _original_ version.

A rendition relationship describes a many-to-many stream mapping between two related content items. For example, a source professional format video file - such as `.mxf` or `.mov` - may have a video stream and sixteen separate mono audio streams. A target rendition has a video stream and a stereo audio stream. The rendition relationship must include which of the source audio streams, say the 3rd and 4th, are used to make the left and right tracks for the target.

One-to-one transformations of a stream that are experientially equivalent may be associated by a rendition relationship, even if the result is a different resolution, number or samples or number of packets. For example:

* scaling a video stream for presentation on a lower resolution device, as this is a form of compression;
* similarly, reducing the sample rate of high-definition audio;
* compressing audio and adding a short amount of additional silence at the beginning or end to facilitate better lip sync.

_Decode rendition relationships_ have source media elements that are packets and target media elements that are frames. _Encode rendition relationships_ start with frames and end up with packets. A rendition target may be a lossless transormation of its source, such as a different packing of samples into a transport. For example, an interleaved [V210 packing](https://www.loc.gov/preservation/digital/formats/fdd/fdd000353.shtml) of an uncompressed 10-bit-per-sample video as a packet is losslessly decoded from, or encoded to, a frame with 16-bit values (only 10-bits-per-sample active) with separate data planes per component.

In a workflow, a rendition relationship can be used as follows:

1. Create a new item of content with a rendition relationship to its source. The difference between the format of the two content items provides details of the transformation required from source to target. Optionally included with the relationship are stream mappings from source to target.

2. The media element metadata and payloads of a rendition can be made in one of three ways:
   * _provided_: the payloads are sent in over the Content Beam API in the usual way.
   * _just-in-case_: payloads are made be a worker or workers from the source material as soon as possible.
   * _just-in-time_: payloads are made as requested and, where appropriate, cached for subsequent requests.

3. If a request is made for rendition media element metadata payload that does not exist, a worker can be assigned to determine whether to respond with `404 - Not Found` or whether to assign another worker to make the rendition response _just-in-time_.

__TODO__ - description of asserting a rendition relationship

### Transformation

A content item that is made by applying a [_filter_](https://github.com/Streampunk/beamcoder#filtering) to one or more other content items, causing a change of viewing experience, is a form of transformation. Transformations include cropping, scaling, mixing, graphics, shaping, retiming (e.g. slow motion), filtering (e.g. remove noise) etc.. A transformation relationship is a means to describe the correspondence between items of source material that are transformed to make items of target material.

Transformation relationships may be time-bounded and only exist for part of the target's timeline. This is a bit like a combination of the timeline in an editor and a live vision mixer:

* For timelines, recipes can be specified in advance by a form of _edit decision list_.
* For live streams, parameters such as mix level or graphics position can be updated on-the-fly.
* Or you can have a mix of the two, with recipes triggered by live triggers and/or recipes configuring potential live interaction.

At a content-item level, transformations are typically one-to-many, a target item with one or more sources. However, in some cases, a transformation may create more than one target, for example making separate key and fill versions of video with graphics overlay. At a stream level, relationships may be many-to-many. Transformation relationships tend to be specified between media elements that are frames rather than packets as they operate on uncompressed data.

As with rendition relationships, transformation relationships can be externally _provided_, made _just-in-case_ or made _just-in-time_.

__TODO__ - description of asserting a transformation relationship

## Workers

Workers run jobs, either just-in-time to create a response to an HTTP/S request or triggered to do some just-in-vase background work - e.g. housekeeping - as the result of a request. For example:

* create an image from a frame of video _on-the-fly_;
* change the sample rate of some audio as it is being played without storing the result, e.g. 44,100Hz to 48,000Hz;
* store an equivalent representation of some media into an object store or onto a filing system.

<img src="images/workers.png" width="75%"/>

Workers can run in the same application and on the same system as a beam engine. However, the design idea behind the beam engine is that work is distributed across many systems and processors. The front end is scaled by having multiple instances of beam engines with a common data source, with load balancing and caching. Redis is the data source glue in the middle, ensuring that jobs are queued and executed to order using [bull](https://www.npmjs.com/package/bull). The back end is scaled using a pool of workers that have access to redis to both consume bull job queues and access media data.

A beam engine is configured with _rules_ that determine what jobs are scheduled and when, depending on beam engine requests, classifying the work to different queues. A worker services one or more queues and is selected at runtime from a catalogue of microservices. A worker can be implemented as:

* A single-threaded Node.js process that can execute one job at a time;
* A [Node.js cluster](https://nodejs.org/docs/latest-v10.x/api/cluster.html) that can execute a number of concurrent _workers_;
* GPU-accelerated processing, facilitated by a library such as [NodenCL](https://github.com/Streampunk/nodencl);
* An invocation of an external processing function, such as an [AWS Lambda](https://aws.amazon.com/lambda/) function - see also [Aerostat Beam Lambda](https://www.npmjs.com/package/beamlambda).

### Setting up a rule

Jobs are configured in the configuration file `config.json`. This allows the content beam API routes to be overridden or supplemented by some additional work. This work can be:

* _pre-route job_: jobs executed before content beam API requests, either starting a background process or providing the response to the method;
* _post-route job_: jobs executed after the content beam API request has created a response, either starting a background process or modifying the response.

The rules that trigger jobs are listed in the `jobs` object of the configuration file, with each rule being a separate property. The value of the property is an object that describes the rule and how it is to be executed. The rule object has the following properties:

| property       | type    | description                                              |
| -------------- | ------- | -------------------------------------------------------- |
| `method`       | string or array of string | Match only HTTP methods of this type   |
| `pathPattern`  | string  | Regular expression to match against the HTTP path        |
| `statusCode`   | int or array of int | Match only responses with these status codes |
| `queue`        | string  | Name of the queue to which jobs should be added          |
| `function`     | string  | Name of the function to execute.                         |
| `postRoute`    | Boolean | Set for a _post-route job_, defaults to _pre-route job_  |

For example, here is a rule that matches all requests that end in `.jpg` or `.jpeg`:

```json
{
  "jobs": {
    "jpeg": {
      "method": "GET",
      "pathPattern": "^/beams/.*\\.(jpg|jpeg)$",
      "queue": "media_workers",
      "function": "makeJPEG",
      "postRoute": false
    }
  }
}
```

In implementation, the result of a job can indicate that it has generated a response to be returned immediately, or that the job determined that the it could not produce a response and other routes and rules should be applied. Rules are applied in the order that they appear.

This rule catches any GET request starting `beams` and ending `.jpg` or `.jpeg` and sends the details to a job queue called `media_workers`. As `postRoute` is set to `false`, this match happens before processing relating to the content beam API. The reason for naming a queue is that different kinds workers may have access to different kinds of resources. Some contrived examples:

* _media workers_: Workers that execute encoding or decoding operations that require significant amounts of CPU, probably multi-threaded, and requiring sufficient amounts of RAM.
* _file workers_: Workers that have access to a shared file system that is keeping a copy of some of the content items.
* _lambda workers_: Workers that execute functions using serverless compute resource.
* _accelerated workers_: Workers that execute part of a job on a GPU or another form of hardware-accelerated device.

The function name is used by a worker to select a function to execute from the library of functions it has available locally.

Rules for _post-route jobs_ include a status code or codes that must be matched in addition to the path pattern and method type. Here is an example of a rule that catches all data payload cache misses and attempts to retrieve the data from another source:

```json
{
  "jobs": {
    "dataMissing": {
      "method": "GET",
      "pathPattern": "^/beams/.*(packet|frame)_(\\d+)(\\.raw.*|/data.*)$",
      "statusCode": 404,
      "queue": "file_workers",
      "function": "retrieveCopy",
      "postRoute": true
    }
  }
}
```

When a _pre-route job_ has been executed by a worker, the job signals whether it has created a response to be returned directly without further processing. This could be success or an error. However, if a worker indicates that further work may be required, the request continues through the content beam API router and onto the _post-route jobs_. This means that it is possible to run more than one job per HTTP/S request.

Changing a rule requires a modification to the configuration file and a restart of the associated app server. If a number of application servers are running behind a load balancer, changing the configuration file requires restarting each server.

### Pre-built workers

__TODO__ - implement and document some pre-built workers - JPEG and MP4 in progress

### Building a worker

Workers are Node.js programs that execute on systems with access to the same redis store as the beam engines that provide them with job to do. Workers are consumers of jobs from Bull queues. Workers use the configuration information they are provided with to select a function to execute.

Functions are expected to communicate directly with redis about content items, streams and media elements, either via the content beam API or by requiring the [`redisio`](doc/redisio.js) capability provided by the beam engine. Data payloads are transported between jobs and the creator of the job via redis.

Workers are composable, in other words it is possible for one worker to put jobs onto other queues to be serviced by another worker. Care must be taken that this does not result in circular work!

A worker is provided with details of the rule defined in the configuration (`rule`), the request path (`path`), request headers (`headers`) and HTTP request method (`method`). It is then expected to carry out some work, often asynchronously, creating either a successful or errored response. The response consists of a `status`, `body` and `type` (the `Content-Type` header) to be used as the Koa context properties of the same name. For data _blobs_ with type `application/octet-stream`, the body is assumed to be a Redis key for a value to be retrieved from Redis as the body of the associated response.

Beam engine workers can access their source data or store generated results in four different ways:

1. Using the methods of the [redisio API](doc/redisio.md).
2. Using HTTP or HTTPS [content beam API](#content_beam_api) requests.
3. Directly communicating with Redis (not recommended).
4. Via URLs and a third-party store (not ideal).

Here is an example of a simple worker for a request to get a simple textual description of a stream of the form `/beams/some_content/video.txt`.

```javascript
const Queue = require('bull'); // Set Redis parameters
const consumer = new Queue('media_workers');
const { redisio } = require('beamengine');
const Boom = require('boom');

async function extractMetadata (job) {
  try {
    // Assumes path pattern "^/beams/\S+/\S+\\.txt$" has been matched
    let stream = await redisio.retrieveStream(job.data.path.slice(0, -4));
    job.data.response = true;
    switch (stream.codecpar.codec_type) {
    case 'video':
      job.data.body = `Video stream with height ${stream.codecpar.height} and width ${stream.codecpar.width}.`;
      break;
    case 'audio':
      job.data.body = `Audio stream with sample rate ${stream.codecpar.sample_rate}Hz and ${stream.codecpar.channels} channels.`;
      break;
    default:
      job.data.body = `Stream of type ${stream.codecpar.codec_type}.`;
      break;
    }
    job.data.type = 'text/plain';
    job.data.respond = true;
  } catch (err) {
    if (err.message.startsWith('Unable to retrieve')) {
      throw Boom.notFound(`Could not retrieve stream at path '${job.data.path}': ${err.message}`);
    }
    throw Boom.badImplementation(`Internal error: ${err.message}`);
  }
  return job.data;
}

consumer.process(async job => {
  if (job.data.rule.function === 'extractMetadata') return extractMetadata(job);
  throw new Error(`Unknown worker ${job.job.function} on queue "media_workers".`);
});
```

Exceptions thrown from within redisio calls are [Javascript Errors](https://nodejs.org/api/errors.html#errors_class_error). The code above translates appropriate errors to [Boom](https://www.npmjs.com/package/boom) so as to provide an informative response with an appropriate status code.

__TODO__ - consider how a response could set headers

__TODO__ - test throwing and carrying Boom errors from worker to response

__TODO__ - returning Redis keys for octet-stream values

__TODO__ - returning results stored at URLs

## Status, support and further development

Although the architecture of the aerostat beam engine is such that it could be used at scale in production environments, development is not yet complete. In its current state, it is recommended that this software is used in development environments and for building prototypes. Future development will make this more appropriate for production use.

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

## License

This project is licensed under the GNU General Public License, version 3 or later. Copyright (C) 2019, Streampunk Media Ltd.

This software links to libraries from the FFmpeg project, including optional parts and optimizations covered by the GPL v2.0 or later. Your attention is drawn to the FFmpeg project's page [FFmpeg License and Legal Considerations](https://www.ffmpeg.org/legal.html).
