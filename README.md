# ServerPlus

## About

A lightweight, custom TCP protocol implementation. Provides E2E channel encryption with OpenSSL and extensibility via an endpoint-handler API.

### Dependencies

#### CryptPlus

https://github.com/willmroliver/CryptPlus
https://www.openssl.org/docs/

Provides Diffie-Hellman / symmetric encryption support via OpenSSL, with some support classes that offer higher-level abstractions of basic operations. 

#### LibeventPlus

https://github.com/willmroliver/LibeventPlus
https://libevent.org/

Wrapper for core functionality of the Libevent API.

#### Protobuf

https://protobuf.dev/

Protobuf offers 'reflection' capabilities on its models; we'll be using these to build in generic features to the server, so that new data-types for storage/communication can be created on-the-fly by users.