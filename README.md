# Aerostat Beam Engine

<img align="right" src="images/beamengine_third.jpg">Redis-backed highly-scale-able and cloud-fit media beam engine. Aerostat Beam Engine provides the following:

* A resilient media-aware cache of media data structured ready for processing by FFmpeg libraries that can be backed by file or object stores;
* Stateless clients providing access to the cache through the _Content Beam API_, an HTTP/S API for transporting media data, pushed and pulled, compressed or uncompressed;
* Job queues allowing multiple worker clients to carry out media transformations just-in-time, on local systems or via serverless compute like AWS Lambda.

The engine comes in the form of a web server application that provides access to read and write data stored in the underlying Redis cache, which may be a single Redia instance or a cluster of master/slave Redis instances. Also included are some extendible worker clients, supporting clients and the ability to trigger work based on some simple rules.

This is an implementation of the core of the [Agile Media Blueprint](https://www.streampunk.media/agile-media-blueprint).

Work in progress. For Node.js FFmpeg native bindings, please see [Aerostat Beam Coder](https://github.com/Streampunk/beamcoder).

## Installation

## Configuration

## Content Beam API

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
