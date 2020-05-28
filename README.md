### Platforms
#### Linux
```
 $ make #build
 $ make clean #clean
 $ make run #run
```

On Linux Platform, we can command `arecord` to generate sample file.

##### Check Microphone
```
  $ arecord -l
```
##### Record
* the `d` flags means how many seconds to record, for the following sample, we record `10s`.
```
  $ arecord -d 10 /tmp/test.wav
```


#### Windows
* Currently only `VS2015 Win32 Debug` mode supported, but it's easy to support `VS2013/15/17/19 Win32/Win64 Debug/Release` since the `boost.asio` api changes little.

