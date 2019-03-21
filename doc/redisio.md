# Redis IO API

The RedisIO API provides programmatic access to manipulate beam engine media structures stored in redis, including _formats_, _streams_, _packets_ and _frames_. Redis IO uses a pool of redis connections that are recycled as they become available. Methods are asynchronous (`async`) and, as much as possible, the underlying work is done in parallel.

To use the API, include the `redisio` property of the `beamengine` module:

    const { redisio } = require('beamengine');

This will create a redis connection pool automatically with a maximum of `config.redis.pool` connections. Note that after using the redisio module, it is advisable to call asynchronous method `close`:

    await redisio.close();

Closing will return once all redis connections have safely quit, or after `config.redis.closeTimeout` milliseconds (defaults to 2000ms), whichever comes first.

Beam engine workers can access their source data or store results in four different ways:

1. Using the methods of this API.
2. Using HTTP or HTTPS content beam API requests.
3. Directly communicating with Redis (not recommended).
4. Via URLs and a third-party store (not ideal).

## API - content items

_Content items_ are described by their _format_ and the terms are used interchangably in this documentation.

### redisio.listContent([start, [limit]])

List the names of all content items from index `start` (default is `0`), returning no more than `limit` (default is `10`) items.

Returns an array of names.

### redisio.retrieveFormat(name)

Retrieve the format with the given `name`.

If a format with the given name exists, a beam coder format-type object is returned. Otherwise, an exception is thrown.

### redisio.storeFormat(format, [overwrite])

Store the given `format`, a format object that is either a beam coder [demuxer](https://github.com/Streampunk/beamcoder#demuxing) or [muxer](https://github.com/Streampunk/beamcoder#muxing). If the `format` contains a `url` property, this will be set as its name, otherwise a URI-representation of a [type 4 UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) will be generated.

Set `overwrite` to `false` to prevent overwriting an existing format. The default value is `true`.

On success, returns `[ 'OK', 'OK' ]` to indicate that both the index and the format were updated successfully.

### redisio.retrieveStream(name, stream_id)

From a format with the given `name`, retrieve details of a stream identified by `stream_id`. The stream identifier may be:

* an index number, either an integer number or prefixed with `stream_`, making e.g. `1` and `stream_1` equivalent identifiers;
* the kind of media stream - one of `video`, `audio`, `subtitle` (for captions), `data` or `attachment` - with an optional by-type index specifier e.g. `audio_0` for the first audio stream, `audio_1` for the second, and so on.
* a `default` reference - defers to FFmpeg libraries to determine the stream that is the default, normally the first video track.

Returns a beam coder stream-type object if the stream is found. Otherwise, throws an exception.

## API - media elements

Access to media elements, which are either [_packets_](https://github.com/Streampunk/beamcoder#creating-packets) or [_frames_](https://github.com/Streampunk/beamcoder#creating-frames). Some methods work with both media element metadata and payload, some just the metadata and others just the payload. Some methods a generic wrappers around both frames and packets and others a specific to type.

### retrieveMedia(name, stream_id, start, [end], [offset], [limit], [flags], [metadataOnly])

Search for and retrieve media elements matching a given query, with or without metadata payload. The search for media elements is carried out in the format with the given `name` within the stream identified by `stream_id`. [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`. The search starts at the given inclusive numerical `start` point, continues to the optional numerical `end` point (defaults to [maximum safe integer](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number/MAX_SAFE_INTEGER)), with the type of query configured by the `flags`:

* `redisio.mediaFlags.DEFAULT` - `start` and `end` are media element timestamps measured in units of stream time base
* `redisio.mediaFlags.FUZZY` - `start` point is considered to be approximate - find the closest matching value;
* `redisio.mediaFlags.INDEX` - `start` and `end` points are 1-based index values based on the sequence of media elements in the stream.
* `redisio.mediaFlags.REALTIME` - `start` and `end` points are measured in seconds, requiring timestamps to be resolved to realtime using the stream's time base
* `redisio.mediaFlags.DURATION` - `start` time is a media element timestamp measured in units of stream time base and `end` is an index offset, e.g. `1` means he next frame after `start` and `-10` is the tenth frame before `start`.

The response is an array of media elements, either packets or frames. For large responses, the array can be _paged_ by starting at the optional `offset` (defaults to `0`) and limited to `limit` values (defaults to `10`). Set the  `metadataOnly` flag to true to return media elements without payloads included, otherwise set to `false` to include payloads (default `false`). If no media elements are found or an error occurs, an exception is thrown.

### retrievePacket(nameOrKey, [stream_id], [pts])

Retrieve metadata and payload data for a specific single packet, where `nameOrKey` is either a complete redis key or the name of a format. If the name of a format, a stream identifier (`stream_id`) and exact presentation time stamp (`pts`) must be provided. [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`.

If the packet is found and retrieval was a success, the result is a beam coder _packet_ with its data payload included. Otherwise, an exception is thrown.

### retrievePacketMetadata(nameOrKey, [stream_id], [pts])

Retrieve metadata for a specific single packet, where `nameOrKey` is either a complete redis key or the name of a format. If the name of a format, a stream identifier (`stream_id`) and exact presentation time stamp (`pts`) must be provided.  [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`.

If the packet is found and retrieval was a success, the result is a beam coder _packet_ without its data payload included. Otherwise, an exception is thrown.

Property `.buf_size` is set to the size of the related data payload.

### retrievePacketData(nameOrKey, [stream_id], [pts])

Retrieve payload data for a specific single packet, where `nameOrKey` is either a complete redis key or the name of a format. If the name of a format, a stream identifier (`stream_id`) and exact presentation time stamp (`pts`) must be provided.  [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`.

If the packet is found and retrieval was a success, the result is a [Node.js `Buffer`](https://nodejs.org/api/buffer.html) containing the data payload of the packet. Otherwise, an exception is thrown.

### retrieveFrame(nameOrKey, [stream_id], [pts])

Retrieve metadata and payload data for a specific single frame, where `nameOrKey` is either a complete redis key or the name of a format. If the name of a format, a stream identifier (`stream_id`) and exact presentation time stamp (`pts`) must be provided.  [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`.

If the frame is found and retrieval was a success, the result is a beam coder _frame_ with all of its data payloads included. Otherwise, an exception is thrown.

### retrieveFrameMetadata(nameOrKey, [stream_id], [pts])

Retrieve metadata for a specific single frame, where `nameOrKey` is either a complete redis key or the name of a format. If the name of a format, a stream identifier (`stream_id`) and exact presentation time stamp (`pts`) must be provided.  [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`.

If the frame is found and retrieval was a success, the result is a beam coder _frame_ witout any of its data payloads included.  Otherwise, an exception is thrown.

Array-valued property `.buf-sizes` contains the sizes of the related data payloads.

### retrieveFrameData(nameOrKey, [stream_id], [pts], [data_index])

Retrieve payload data for a specific single frame, where `nameOrKey` is either a complete redis key or the name of a format. If the name of a format, a stream identifier (`stream_id`) and exact presentation time stamp (`pts`) must be provided.  [As described previously](#redisioretrievestreamname-stream_id), the stream identifier may be an index, media type or `default`. Where provided, the `data_index` requests data for a specific data plane by zero-based numerical index, otherwise the data for all planes is retrieved and concatenated.

If requesting data for a single plane, the result is a [Node.js `Buffer`](https://nodejs.org/api/buffer.html) containing the data payload of the single data plane in the frame. Otherwise, an exception is thrown.

If requesting data for all the planes, the result is an array containing the `data` with [Node.js `Buffer`](https://nodejs.org/api/buffer.html) elements with the data for each plane. For example:

    [ Buffer <0a 1c 7c ... >, Buffer <9f 32 00 ... >, ... ]

If the frame does not exist or one of more of the data planes requested is not available, an exception is thrown.

### storeMedia(name, element, [stream_index])

Store a single element of media including both its metadata and payload. The media `element` will be either created or replaced for format with the given `name`.

If the media element is a beam coder _packet_, the stream will be identified from its `.stream_index` property. For beam coder _frames_, either the frame has an additional `.stream_index` property or the `stream_index` must be provided as an argument. In all cases, this must be a numerical stream index, not a name or a `default`.

__TODO__ - consider allowing the use of media kind stream identifiers?

Whether a _packet_ or _frame_, the presentation timestamp of the media element will be taken from the `pts` property.

The value returned is an array indicating success or failure of the storage operation, with the first element of the array referring to whether metadata storage was successful and subsequent element relating to data storage. Possible array values are:

* `'OK-crt'` - Successful storage, new value created.
* `'OK-ovw'` - Successful storage, existing value overwritten.
* `null` - Failure to store element.

So, for example, on storing a new packet, the following result is expected:

    [ 'OK-crt', 'OK-crt' ]

If the metadata storage succeeded but data storage failed:

    [ 'OK-crt', null ]

For frames, the number of elements in the array is the number of data planes plus one. Here is the result for a frame with format `yuv420p` that has been overwritten successfully:

    [ 'OK-ovw', 'OK-ovw', 'OK-ovw', 'OK-ovw' ]

### storePacket(name, packet)

As [`storeMedia`](#storemedianame-element-stream_index) except that the `element` is a `packet` and must be a beam coder _packet_.

### storeFrame(name, frame, [stream_index])

As [`storeMedia`](#storemedianame-element-stream_index) except that the `element` is a `frame` and must be a beam coder _frame_.

## API - ephemeral data blobs

As a means to communicate a one-time result of type `Buffer` from a worker to its caller, including a caller that initiated a request, temporary _ephemeral blobs_ may be stored in redis. These blobs expire after `config.redis.ephemeralTTL` milliseconds (defaults to 10000ms).

### storeBlob(data)

Store the given `data` blob into redis. Returns an auto-generated random `key` to use to retrieve the data and starts the time-to-live clock.

### retrieveBlob(key)

Retrieve the data blob with the given `key`. Returns a `Buffer` containing the data or throws an exception if the buffer cannot be found, possibly because the value has expired in the cache.
