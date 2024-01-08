# ServerPlus

## About

A TCP server solution & custom protocol for creating 'lobbies'. Provides E2E channel encryption with OpenSSL and extensibility via Protobuf classes.

### Dependencies

#### CryptPlus

https://github.com/willmroliver/CryptPlus
https://www.openssl.org/docs/

Provides Diffie-Hellman / symmetric encryption support via OpenSSL, with some support classes that offer high-level abstractions of basic operations. 

#### LibeventPlus

https://github.com/willmroliver/LibeventPlus
https://libevent.org/

Wrapper for core functionality of the Libevent API.

#### Protobuf

https://protobuf.dev/

