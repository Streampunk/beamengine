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

## API - Content items

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

## API - Media elements

## API - Ephemeral data blobs

As a means to communicate a one-time result of type `Buffer` from a worker to its caller, including a caller that initiated a request, temporary _ephemeral blobs_ may be stored in redis. These blobs expire after `config.redis.ephemeralTTL` milliseconds (defaults to 10000ms).

### storeBlob(data)

Store the given `data` blob into redis. Returns an auto-generated random `key` to use to retrieve it.

### retrieveBlob(key)

Retrieve the data blob with the given `key`.
